/*	$OpenBSD: src/sbin/isakmpd/ipsec.c,v 1.23 2000/01/26 15:23:32 niklas Exp $	*/
/*	$EOM: ipsec.c,v 1.115 1999/12/20 10:12:17 ho Exp $	*/

/*
 * Copyright (c) 1998, 1999 Niklas Hallqvist.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Ericsson Radio Systems.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This code was written under funding by Ericsson Radio Systems.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

#include "sysdep.h"

#include "attribute.h"
#include "conf.h"
#include "constants.h"
#include "crypto.h"
#include "dh.h"
#include "doi.h"
#include "exchange.h"
#include "hash.h"
#include "ike_aggressive.h"
#include "ike_auth.h"
#include "ike_main_mode.h"
#include "ike_quick_mode.h"
#include "ipsec.h"
#include "ipsec_doi.h"
#include "isakmp.h"
#include "log.h"
#include "math_group.h"
#include "message.h"
#include "prf.h"
#include "sa.h"
#include "timer.h"
#include "transport.h"
#include "util.h"

/* The replay window size used for all IPSec protocols if not overridden.  */
#define DEFAULT_REPLAY_WINDOW 16

struct ipsec_decode_arg {
  struct message *msg;
  struct sa *sa;
  struct proto *proto;
};

/* These variables hold the contacted peers ADT state.  */
struct contact {
  struct sockaddr *addr;
  socklen_t len;
} *contacts = 0;
int contact_cnt = 0, contact_limit = 0;

static int addr_cmp (const void *, const void *);
static int ipsec_add_contact (struct message *msg);
static int ipsec_contacted (struct message *msg);
static int ipsec_debug_attribute (u_int16_t, u_int8_t *, u_int16_t, void *);
static void ipsec_delete_spi (struct sa *, struct proto *, int);
static u_int16_t *ipsec_exchange_script (u_int8_t);
static void ipsec_finalize_exchange (struct message *);
static void ipsec_free_exchange_data (void *);
static void ipsec_free_proto_data (void *);
static void ipsec_free_sa_data (void *);
static struct keystate *ipsec_get_keystate (struct message *);
static u_int8_t *ipsec_get_spi (size_t *, u_int8_t, struct message *);
static int ipsec_handle_leftover_payload (struct message *, u_int8_t,
					  struct payload *);
static int ipsec_informational_post_hook (struct message *);
static int ipsec_informational_pre_hook (struct message *);
static int ipsec_initiator (struct message *);
static void ipsec_proto_init (struct proto *, char *);
static int ipsec_responder (struct message *);
static void ipsec_setup_situation (u_int8_t *);
static void ipsec_set_network (u_int8_t *, u_int8_t *, struct ipsec_sa *);
static size_t ipsec_situation_size (void);
static u_int8_t ipsec_spi_size (u_int8_t);
static int ipsec_validate_attribute (u_int16_t, u_int8_t *, u_int16_t, void *);
static int ipsec_validate_exchange (u_int8_t);
static int ipsec_validate_id_information (u_int8_t, u_int8_t *, u_int8_t *,
					  size_t, struct exchange *);
static int ipsec_validate_key_information (u_int8_t *, size_t);
static int ipsec_validate_notification (u_int16_t);
static int ipsec_validate_proto (u_int8_t);
static int ipsec_validate_situation (u_int8_t *, size_t *);
static int ipsec_validate_transform_id (u_int8_t, u_int8_t);

static struct doi ipsec_doi = {
  { 0 }, IPSEC_DOI_IPSEC,
  sizeof (struct ipsec_exch), sizeof (struct ipsec_sa),
  sizeof (struct ipsec_proto),
  ipsec_debug_attribute,
  ipsec_delete_spi,
  ipsec_exchange_script,
  ipsec_finalize_exchange,
  ipsec_free_exchange_data,
  ipsec_free_proto_data,
  ipsec_free_sa_data,
  ipsec_get_keystate,
  ipsec_get_spi,
  ipsec_handle_leftover_payload,
  ipsec_informational_post_hook,
  ipsec_informational_pre_hook,
  ipsec_is_attribute_incompatible,
  ipsec_proto_init,
  ipsec_setup_situation,
  ipsec_situation_size,
  ipsec_spi_size,
  ipsec_validate_attribute,
  ipsec_validate_exchange,
  ipsec_validate_id_information,
  ipsec_validate_key_information,
  ipsec_validate_notification,
  ipsec_validate_proto,
  ipsec_validate_situation,
  ipsec_validate_transform_id,
  ipsec_initiator,
  ipsec_responder
};

int16_t script_quick_mode[] = {
  ISAKMP_PAYLOAD_HASH,		/* Initiator -> responder.  */
  ISAKMP_PAYLOAD_SA,
  ISAKMP_PAYLOAD_NONCE,
  EXCHANGE_SCRIPT_SWITCH,
  ISAKMP_PAYLOAD_HASH,		/* Responder -> initiator.  */
  ISAKMP_PAYLOAD_SA,
  ISAKMP_PAYLOAD_NONCE,
  EXCHANGE_SCRIPT_SWITCH,
  ISAKMP_PAYLOAD_HASH,		/* Initiator -> responder.  */
  EXCHANGE_SCRIPT_END
};

int16_t script_new_group_mode[] = {
  ISAKMP_PAYLOAD_HASH,		/* Initiator -> responder.  */
  ISAKMP_PAYLOAD_SA,
  EXCHANGE_SCRIPT_SWITCH,
  ISAKMP_PAYLOAD_HASH,		/* Responder -> initiator.  */
  ISAKMP_PAYLOAD_SA,
  EXCHANGE_SCRIPT_END
};

struct dst_spi_proto_arg {
  in_addr_t dst;
  u_int32_t spi;
  u_int8_t proto;
};

/*
 * Check if SA matches what we are asking for through V_ARG.  It has to
 * be a finished phase 2 SA.
 */
static int
ipsec_sa_check (struct sa *sa, void *v_arg)
{
  struct dst_spi_proto_arg *arg = v_arg;
  struct proto *proto;
  struct sockaddr *dst, *src;
  int dstlen, srclen;
  int incoming;

  if (sa->phase != 2 || !(sa->flags & SA_FLAG_READY))
    return 0;

  sa->transport->vtbl->get_dst (sa->transport, &dst, &dstlen);
  if (((struct sockaddr_in *)dst)->sin_addr.s_addr == arg->dst)
    incoming = 0;
  else
    {
      sa->transport->vtbl->get_src (sa->transport, &src, &srclen);
      if (((struct sockaddr_in *)src)->sin_addr.s_addr == arg->dst)
	incoming = 1;
      else
	return 0;
    }

  for (proto = TAILQ_FIRST (&sa->protos); proto;
       proto = TAILQ_NEXT (proto, link))
    if (proto->proto == arg->proto
	&& memcmp (proto->spi[incoming], &arg->spi, sizeof arg->spi) == 0)
      return 1;
  return 0;
}

/* Find an SA with a "name" of DST, SPI & PROTO.  */
struct sa *
ipsec_sa_lookup (in_addr_t dst, u_int32_t spi, u_int8_t proto)
{
  struct dst_spi_proto_arg arg = { dst, spi, proto };

  return sa_find (ipsec_sa_check, &arg);
}

/*
 * Check if SA matches the flow of another SA in V_ARG.  It has to
 * be a finished non-replaced phase 2 SA.
 * XXX At some point other selectors will matter here too.
 */
static int
ipsec_sa_check_flow (struct sa *sa, void *v_arg)
{
  struct sa *sa2 = v_arg;
  struct ipsec_sa *isa = sa->data, *isa2 = sa2->data;

  if (sa == sa2 || sa->phase != 2
      || (sa->flags & (SA_FLAG_READY | SA_FLAG_REPLACED)) != SA_FLAG_READY)
    return 0;

  return isa->src_net == isa2->src_net && isa->src_mask == isa2->src_mask
    && isa->dst_net == isa2->dst_net && isa->dst_mask == isa2->dst_mask;
}

/*
 * Do IPSec DOI specific finalizations task for the exchange where MSG was
 * the final message.
 */
static void
ipsec_finalize_exchange (struct message *msg)
{
  struct sa *isakmp_sa;
  struct ipsec_sa *isa;
  struct exchange *exchange = msg->exchange;
  struct ipsec_exch *ie = exchange->data;
  struct sa *sa = 0, *old_sa;
  struct proto *proto, *last_proto = 0;

  switch (exchange->phase)
    {
    case 1:
      switch (exchange->type)
	{
	case ISAKMP_EXCH_ID_PROT:
	case ISAKMP_EXCH_AGGRESSIVE:
	  isakmp_sa = msg->isakmp_sa;
	  isa = isakmp_sa->data;
	  isa->hash = ie->hash->type;
	  isa->prf_type = ie->prf_type;
	  isa->skeyid_len = ie->skeyid_len;
	  isa->skeyid_d = ie->skeyid_d;
	  isa->skeyid_a = ie->skeyid_a;
	  /* Prevents early free of SKEYID_*.  */
	  ie->skeyid_a = ie->skeyid_d = 0;

	  /* If a lifetime was negotiated setup the expiration timers.  */
	  if (isakmp_sa->seconds)
	    sa_setup_expirations (isakmp_sa);
	  break;
	}
      break;

    case 2:
      switch (exchange->type)
	{
	case IKE_EXCH_QUICK_MODE:
	  /*
	   * Tell the application(s) about the SPIs and key material.
	   */
	  for (sa = TAILQ_FIRST (&exchange->sa_list); sa;
	       sa = TAILQ_NEXT (sa, next))
	    {
	      for (proto = TAILQ_FIRST (&sa->protos), last_proto = 0; proto;
		   proto = TAILQ_NEXT (proto, link))
		{
		  if (sysdep_ipsec_set_spi (sa, proto, 0)
		      || (last_proto
			  && sysdep_ipsec_group_spis (sa, last_proto, proto,
						      0))
		      || sysdep_ipsec_set_spi (sa, proto, 1)
		      || (last_proto
			  && sysdep_ipsec_group_spis (sa, last_proto, proto,
						      1)))
		    /* XXX Tear down this exchange.  */
		    return;
		  last_proto = proto;
		}

	      isa = sa->data;

	      if (exchange->initiator)
		/* Initiator is source, responder is destination.  */
		ipsec_set_network (ie->id_ci, ie->id_cr, isa);
	      else
		/* Responder is source, initiator is destination.  */
		ipsec_set_network (ie->id_cr, ie->id_ci, isa);

	      log_debug (LOG_EXCHANGE, 50,
			 "ipsec_finalize_exchange: src %x %x dst %x %x",
			 ntohl (isa->src_net), ntohl (isa->src_mask),
			 ntohl (isa->dst_net), ntohl (isa->dst_mask));

	      if (sysdep_ipsec_enable_sa (sa))
		/* XXX Tear down this exchange.  */
		return;

	      /* Mark elder SAs with the same flow information as replaced.  */
	      while ((old_sa = sa_find (ipsec_sa_check_flow, sa)) != 0)
		sa_mark_replaced (old_sa);
	    }
	  break;
	}
    }
}

/* Set the client addresses in ISA from SRC_ID and DST_ID.  */
static void
ipsec_set_network (u_int8_t *src_id, u_int8_t *dst_id, struct ipsec_sa *isa)
{
  int id;

  /* Set source address. */
  id = GET_ISAKMP_ID_TYPE (src_id);
  switch (id)
    {
    case IPSEC_ID_IPV4_ADDR:
      memcpy (&isa->src_net, src_id + ISAKMP_ID_DATA_OFF, sizeof isa->src_net);
      isa->src_mask = htonl (0xffffffff);
      break;
    case IPSEC_ID_IPV4_ADDR_SUBNET:
      memcpy (&isa->src_net, src_id + ISAKMP_ID_DATA_OFF, sizeof isa->src_net);
      memcpy (&isa->src_mask,
	      src_id + ISAKMP_ID_DATA_OFF + sizeof isa->src_net,
	      sizeof isa->src_mask);
      break;
  }

  /* Set destination address.  */
  id = GET_ISAKMP_ID_TYPE (dst_id);
  switch (id)
    {
    case IPSEC_ID_IPV4_ADDR:
      memcpy (&isa->dst_net, dst_id + ISAKMP_ID_DATA_OFF, sizeof isa->dst_net);
      isa->dst_mask = htonl (0xffffffff);
      break;
    case IPSEC_ID_IPV4_ADDR_SUBNET:
      memcpy (&isa->dst_net, dst_id + ISAKMP_ID_DATA_OFF, sizeof isa->dst_net);
      memcpy (&isa->dst_mask,
	      dst_id + ISAKMP_ID_DATA_OFF + sizeof isa->dst_net,
	      sizeof isa->dst_mask);
      break;
    }
}

/* Free the DOI-specific exchange data pointed to by VIE.  */
static void
ipsec_free_exchange_data (void *vie)
{
  struct ipsec_exch *ie = vie;

  if (ie->sa_i_b)
    free (ie->sa_i_b);
  if (ie->id_ci)
    free (ie->id_ci);
  if (ie->id_cr)
    free (ie->id_cr);
  if (ie->g_xi)
    free (ie->g_xi);
  if (ie->g_xr)
    free (ie->g_xr);
  if (ie->g_xy)
    free (ie->g_xy);
  if (ie->skeyid)
    free (ie->skeyid);
  if (ie->skeyid_d)
    free (ie->skeyid_d);
  if (ie->skeyid_a)
    free (ie->skeyid_a);
  if (ie->skeyid_e)
    free (ie->skeyid_e);
  if (ie->hash_i)
    free (ie->hash_i);
  if (ie->hash_r)
    free (ie->hash_r);
  if (ie->group)
    group_free (ie->group);
}

/* Free the DOI-specific SA data pointed to by VISA.  */
static void
ipsec_free_sa_data (void *visa)
{
  struct ipsec_sa *isa = visa;

  if (isa->skeyid_a)
    free (isa->skeyid_a);
  if (isa->skeyid_d)
    free (isa->skeyid_d);
}

/* Free the DOI-specific protocol data of an SA pointed to by VIPROTO.  */
static void
ipsec_free_proto_data (void *viproto)
{
  struct ipsec_proto *iproto = viproto;
  int i;

  for (i = 0; i < 2; i++)
    if (iproto->keymat[i])
      free (iproto->keymat[i]);
}

/* Return exchange script based on TYPE.  */
static u_int16_t *
ipsec_exchange_script (u_int8_t type)
{
  switch (type)
    {
    case IKE_EXCH_QUICK_MODE:
      return script_quick_mode;
    case IKE_EXCH_NEW_GROUP_MODE:
      return script_new_group_mode;
    }
  return 0;
}

/* Initialize this DOI, requires doi_init to already have been called.  */
void
ipsec_init ()
{
  doi_register (&ipsec_doi);
}

/* Given a message MSG, return a suitable IV (or rather keystate).  */
static struct keystate *
ipsec_get_keystate (struct message *msg)
{
  struct keystate *ks;
  struct hash *hash;

  /* If we have already have an IV, use it.  */
  if (msg->exchange && msg->exchange->keystate)
    {
      ks = malloc (sizeof *ks);
      if (!ks)
	{
	  log_error ("ipsec_get_keystate: malloc (%d) failed", sizeof *ks);
	  return 0;
	}
      memcpy (ks, msg->exchange->keystate, sizeof *ks);
      return ks;
    }

  /*
   * For phase 2 when no SA yet is setup we need to hash the IV used by
   * the ISAKMP SA concatenated with the message ID, and use that as an
   * IV for further cryptographic operations.
   */
  ks = crypto_clone_keystate (msg->isakmp_sa->keystate);
  if (!ks)
    return 0;

  hash = hash_get (((struct ipsec_sa *)msg->isakmp_sa->data)->hash);
  hash->Init (hash->ctx);
  log_debug_buf (LOG_CRYPTO, 80, "ipsec_get_keystate: final phase 1 IV",
		 ks->riv, ks->xf->blocksize);
  hash->Update (hash->ctx, ks->riv, ks->xf->blocksize);
  log_debug_buf (LOG_CRYPTO, 80, "ipsec_get_keystate: message ID",
		 ((u_int8_t *)msg->iov[0].iov_base)
		 + ISAKMP_HDR_MESSAGE_ID_OFF,
		 ISAKMP_HDR_MESSAGE_ID_LEN);
  hash->Update (hash->ctx,
		((u_int8_t *)msg->iov[0].iov_base) + ISAKMP_HDR_MESSAGE_ID_OFF,
		ISAKMP_HDR_MESSAGE_ID_LEN);
  hash->Final (hash->digest, hash->ctx);
  crypto_init_iv (ks, hash->digest, ks->xf->blocksize);
  log_debug_buf (LOG_CRYPTO, 80, "ipsec_get_keystate: phase 2 IV",
		 hash->digest, ks->xf->blocksize);
  return ks;
}

static void
ipsec_setup_situation (u_int8_t *buf)
{
  SET_IPSEC_SIT_SIT (buf + ISAKMP_SA_SIT_OFF, IPSEC_SIT_IDENTITY_ONLY);
}

static size_t
ipsec_situation_size (void)
{
  return IPSEC_SIT_SIT_LEN;
}

static u_int8_t
ipsec_spi_size (u_int8_t proto)
{
  return IPSEC_SPI_SIZE;
}

static int
ipsec_validate_attribute (u_int16_t type, u_int8_t *value, u_int16_t len,
			  void *vmsg)
{
  struct message *msg = vmsg;

  if ((msg->exchange->phase == 1
       && (type < IKE_ATTR_ENCRYPTION_ALGORITHM
	   || type > IKE_ATTR_GROUP_ORDER))
      || (msg->exchange->phase == 2
	  && (type < IPSEC_ATTR_SA_LIFE_TYPE
	      || type > IPSEC_ATTR_COMPRESS_PRIVATE_ALGORITHM)))
    return -1;
  return 0;
}

static int
ipsec_validate_exchange (u_int8_t exch)
{
  return exch != IKE_EXCH_QUICK_MODE && exch != IKE_EXCH_NEW_GROUP_MODE;
}

static int
ipsec_validate_id_information (u_int8_t type, u_int8_t *extra, u_int8_t *buf,
			       size_t sz, struct exchange *exchange)
{
  u_int8_t proto = GET_IPSEC_ID_PROTO (extra);
  u_int16_t port = GET_IPSEC_ID_PORT (extra);

  log_debug (LOG_MESSAGE, 0, 
	     "ipsec_validate_id_information: proto %d port %d type %d",
	     proto, port, type);
  if (type < IPSEC_ID_IPV4_ADDR || type > IPSEC_ID_KEY_ID)
    return -1;

  switch (type)
    {
    case IPSEC_ID_IPV4_ADDR:
      log_debug_buf (LOG_MESSAGE, 40, "ipsec_validate_id_information: IPv4",
		     buf, 4);
      break;

    case IPSEC_ID_IPV4_ADDR_SUBNET:
      log_debug_buf (LOG_MESSAGE, 40,
		     "ipsec_validate_id_information: IPv4 network/netmask",
		     buf, 8);
      break;

    default:
      break;
    }

  if (exchange->phase == 1
      && (proto != IPPROTO_UDP || port != UDP_DEFAULT_PORT)
      && (proto != 0 || port != 0))
    {
/* XXX SSH's ISAKMP tester fails this test (proto 17 - port 0).  */
#ifdef notyet
      return -1;
#else
      log_print ("ipsec_validate_id_information: "
		 "dubious ID information accepted");
#endif
    }

  /* XXX More checks?  */

  return 0;
}

static int
ipsec_validate_key_information (u_int8_t *buf, size_t sz)
{
  /* XXX Not implemented yet.  */
  return 0;
}

static int
ipsec_validate_notification (u_int16_t type)
{
  return type < IPSEC_NOTIFY_RESPONDER_LIFETIME
    || type > IPSEC_NOTIFY_INITIAL_CONTACT ? -1 : 0;
}

static int
ipsec_validate_proto (u_int8_t proto)
{
  return proto < IPSEC_PROTO_IPSEC_AH || proto > IPSEC_PROTO_IPCOMP ? -1 : 0;
}

static int
ipsec_validate_situation (u_int8_t *buf, size_t *sz)
{
  int sit = GET_IPSEC_SIT_SIT (buf);
  int off;

  if (sit & (IPSEC_SIT_SECRECY | IPSEC_SIT_INTEGRITY))
    {
      /*
       * XXX All the roundups below, round up to 32 bit boundaries given
       * that the situation field is aligned.  This is not necessarily so,
       * but I interpret the drafts as this is like this they want it.
       */
      off = ROUNDUP_32 (GET_IPSEC_SIT_SECRECY_LENGTH (buf));
      off += ROUNDUP_32 (GET_IPSEC_SIT_SECRECY_CAT_LENGTH (buf + off));
      off += ROUNDUP_32 (GET_IPSEC_SIT_INTEGRITY_LENGTH (buf + off));
      off += ROUNDUP_32 (GET_IPSEC_SIT_INTEGRITY_CAT_LENGTH (buf + off));
      *sz = off + IPSEC_SIT_SZ;
    }
  else
    *sz = IPSEC_SIT_SIT_LEN;

  /* Currently only "identity only" situations are supported.  */
#ifdef notdef
  return
    sit & ~(IPSEC_SIT_IDENTITY_ONLY | IPSEC_SIT_SECRECY | IPSEC_SIT_INTEGRITY);
#else
   return sit & ~IPSEC_SIT_IDENTITY_ONLY;
#endif
    return 1;
  return 0;
}

static int
ipsec_validate_transform_id (u_int8_t proto, u_int8_t transform_id)
{
  switch (proto)
    {
      /*
       * As no unexpected protocols can occur, we just tie the default case
       * to the first case, in orer to silence a GCC warning.
       */
    default:
    case ISAKMP_PROTO_ISAKMP:
      return transform_id != IPSEC_TRANSFORM_KEY_IKE;
    case IPSEC_PROTO_IPSEC_AH:
      return
	transform_id < IPSEC_AH_MD5 || transform_id > IPSEC_AH_DES ? -1 : 0;
    case IPSEC_PROTO_IPSEC_ESP:
      return transform_id < IPSEC_ESP_DES_IV64
	|| transform_id > IPSEC_ESP_NULL ? -1 : 0;
    case IPSEC_PROTO_IPCOMP:
      return transform_id < IPSEC_IPCOMP_OUI
	|| transform_id > IPSEC_IPCOMP_V42BIS ? -1 : 0;
    }
}

static int
ipsec_initiator (struct message *msg)
{
  struct exchange *exchange = msg->exchange;
  int (**script) (struct message *msg) = 0;

  /* Check that the SA is coherent with the IKE rules.  */
  if ((exchange->phase == 1 && exchange->type != ISAKMP_EXCH_ID_PROT
       && exchange->type != ISAKMP_EXCH_AGGRESSIVE
       && exchange->type != ISAKMP_EXCH_INFO)
      || (exchange->phase == 2 && exchange->type != IKE_EXCH_QUICK_MODE
	  && exchange->type != ISAKMP_EXCH_INFO))
    {
      log_print ("ipsec_initiator: unsupported exchange type %d in phase %d",
		 exchange->type, exchange->phase);
      return -1;
    }
    
  switch (exchange->type)
    {
    case ISAKMP_EXCH_BASE:
      break;
    case ISAKMP_EXCH_ID_PROT:
      script = ike_main_mode_initiator;
      break;
    case ISAKMP_EXCH_AUTH_ONLY:
      log_print ("ipsec_initiator: unuspported exchange type %d",
		 exchange->type);
      return -1;
    case ISAKMP_EXCH_AGGRESSIVE:
      script = ike_aggressive_initiator;
      break;
    case ISAKMP_EXCH_INFO:
      return message_send_info (msg);
    case IKE_EXCH_QUICK_MODE:
      script = ike_quick_mode_initiator;
      break;
    case IKE_EXCH_NEW_GROUP_MODE:
      break;
    }

  /* Run the script code for this step.  */
  if (script)
    return script[exchange->step] (msg);

  return 0;
}

static int
ipsec_responder (struct message *msg)
{
  struct exchange *exchange = msg->exchange;
  int (**script) (struct message *msg) = 0;
  struct payload *p;

  /* Check that a new exchange is coherent with the IKE rules.  */
  if (exchange->step == 0
      && ((exchange->phase == 1 && exchange->type != ISAKMP_EXCH_ID_PROT
	   && exchange->type != ISAKMP_EXCH_AGGRESSIVE
	   && exchange->type != ISAKMP_EXCH_INFO)
	  || (exchange->phase == 2 && exchange->type == ISAKMP_EXCH_ID_PROT)))
    {
      message_drop (msg, ISAKMP_NOTIFY_UNSUPPORTED_EXCHANGE_TYPE, 0, 1, 0);
      return -1;
    }
    
  log_debug (LOG_MISC, 30,
	     "ipsec_responder: phase %d exchange %d step %d", exchange->phase,
	     exchange->type, exchange->step);
  switch (exchange->type)
    {
    case ISAKMP_EXCH_BASE:
    case ISAKMP_EXCH_AUTH_ONLY:
      message_drop (msg, ISAKMP_NOTIFY_UNSUPPORTED_EXCHANGE_TYPE, 0, 1, 0);
      return -1;

    case ISAKMP_EXCH_ID_PROT:
      script = ike_main_mode_responder;
      break;

    case ISAKMP_EXCH_AGGRESSIVE:
      script = ike_aggressive_responder;
      break;

    case ISAKMP_EXCH_INFO:
      for (p = TAILQ_FIRST (&msg->payload[ISAKMP_PAYLOAD_NOTIFY]); p;
	   p = TAILQ_NEXT (p, link))
	{
	  log_debug (LOG_EXCHANGE, 10,
		     "ipsec_responder: got NOTIFY of type %s",
		     constant_lookup (isakmp_notify_cst,
				      GET_ISAKMP_NOTIFY_MSG_TYPE (p->p)));
	  p->flags |= PL_MARK;
	}

      /*
       * If any DELETEs are in here, let the logic of leftover payloads deal
       * with them.
       */

      return 0;

    case IKE_EXCH_QUICK_MODE:
      script = ike_quick_mode_responder;
      break;

    case IKE_EXCH_NEW_GROUP_MODE:
      /* XXX Not implemented yet.  */
      break;
    }

  /* Run the script code for this step.  */
  if (script)
    return script[exchange->step] (msg);

  /*
   * XXX So far we don't accept any proposals for exchanges we don't support.
   */
  if (TAILQ_FIRST (&msg->payload[ISAKMP_PAYLOAD_SA]))
    {
      message_drop (msg, ISAKMP_NOTIFY_NO_PROPOSAL_CHOSEN, 0, 1, 0);
      return -1;
    }
  return 0;
}

static enum hashes from_ike_hash (u_int16_t hash)
{
  switch (hash)
    {
    case IKE_HASH_MD5:
      return HASH_MD5;
    case IKE_HASH_SHA:
      return HASH_SHA1;
    }
  return -1;
}

static enum transform from_ike_crypto (u_int16_t crypto)
{
  /* Coincidentally this is the null operation :-)  */
  return crypto;
}

/*
 * Find out whether the attribute of type TYPE with a LEN length value
 * pointed to by VALUE is incompatible with what we can handle.
 * VMSG is a pointer to the current message.
 */
int
ipsec_is_attribute_incompatible (u_int16_t type, u_int8_t *value,
				 u_int16_t len, void *vmsg)
{
  struct message *msg = vmsg;

  if (msg->exchange->phase == 1)
    {
      switch (type)
	{
	case IKE_ATTR_ENCRYPTION_ALGORITHM:
	  return !crypto_get (from_ike_crypto (decode_16 (value)));
	case IKE_ATTR_HASH_ALGORITHM:
	  return !hash_get (from_ike_hash (decode_16 (value)));
	case IKE_ATTR_AUTHENTICATION_METHOD:
	  return !ike_auth_get (decode_16 (value));
	case IKE_ATTR_GROUP_DESCRIPTION:
	  return decode_16 (value) < IKE_GROUP_DESC_MODP_768
	    || decode_16 (value) > IKE_GROUP_DESC_MODP_1536;
	case IKE_ATTR_GROUP_TYPE:
	  return 1;
	case IKE_ATTR_GROUP_PRIME:
	  return 1;
	case IKE_ATTR_GROUP_GENERATOR_1:
	  return 1;
	case IKE_ATTR_GROUP_GENERATOR_2:
	  return 1;
	case IKE_ATTR_GROUP_CURVE_A:
	  return 1;
	case IKE_ATTR_GROUP_CURVE_B:
	  return 1;
	case IKE_ATTR_LIFE_TYPE:
	  return decode_16 (value) < IKE_DURATION_SECONDS
	    || decode_16 (value) > IKE_DURATION_KILOBYTES;
	case IKE_ATTR_LIFE_DURATION:
	  return 0;
	case IKE_ATTR_PRF:
	  return 1;
	case IKE_ATTR_KEY_LENGTH:
	  /*
	   * Our crypto routines only allows key-lengths which are multiples
	   * of an octet.
	   */
	  return decode_16 (value) % 8 != 0; 
	case IKE_ATTR_FIELD_SIZE:
	  return 1;
	case IKE_ATTR_GROUP_ORDER:
	  return 1;
	}
    }
  else
    {
      switch (type)
	{
	case IPSEC_ATTR_SA_LIFE_TYPE:
	  return decode_16 (value) < IPSEC_DURATION_SECONDS
	    || decode_16 (value) > IPSEC_DURATION_KILOBYTES;
	case IPSEC_ATTR_SA_LIFE_DURATION:
	  return 0;
	case IPSEC_ATTR_GROUP_DESCRIPTION:
	  return decode_16 (value) < IKE_GROUP_DESC_MODP_768
	    || decode_16 (value) > IKE_GROUP_DESC_MODP_1536;
	case IPSEC_ATTR_ENCAPSULATION_MODE:
	  return decode_16 (value) < IPSEC_ENCAP_TUNNEL
	    || decode_16 (value) > IPSEC_ENCAP_TRANSPORT;
	case IPSEC_ATTR_AUTHENTICATION_ALGORITHM:
	  return decode_16 (value) < IPSEC_AUTH_HMAC_MD5
	    || decode_16 (value) > IPSEC_AUTH_KPDK;
	case IPSEC_ATTR_KEY_LENGTH:
	  /* XXX Blowfish needs '0'. Others appear to disregard this attr?  */
	  return 0;
	case IPSEC_ATTR_KEY_ROUNDS:
	  return 1;
	case IPSEC_ATTR_COMPRESS_DICTIONARY_SIZE:
	  return 1;
	case IPSEC_ATTR_COMPRESS_PRIVATE_ALGORITHM:
	  return 1;
	}
    }
  /* XXX Silence gcc.  */
  return 1;
}

/*
 * Log the attribute of TYPE with a LEN length value pointed to by VALUE
 * in human-readable form.  VMSG is a pointer to the current message.
 */
int
ipsec_debug_attribute (u_int16_t type, u_int8_t *value, u_int16_t len,
		       void *vmsg)
{
  struct message *msg = vmsg;
  char val[20];

  /* XXX Transient solution.  */
  if (len == 2)
    sprintf (val, "%d", decode_16 (value));
  else if (len == 4)
    sprintf (val, "%d", decode_32 (value));
  else
    sprintf (val, "unrepresentable");

  log_debug (LOG_MESSAGE, 50, "Attribute %s value %s",
	     constant_name (msg->exchange->phase == 1
			    ? ike_attr_cst : ipsec_attr_cst, type),
	     val);
  return 0;
}

/*
 * Decode the attribute of type TYPE with a LEN length value pointed to by
 * VALUE.  VIDA is a pointer to a context structure where we can find the
 * current message, SA and protocol.
 */
int
ipsec_decode_attribute (u_int16_t type, u_int8_t *value, u_int16_t len,
			void *vida)
{
  struct ipsec_decode_arg *ida = vida;
  struct message *msg = ida->msg;
  struct sa *sa = ida->sa;
  struct ipsec_sa *isa = sa->data;
  struct proto *proto = ida->proto;
  struct ipsec_proto *iproto = proto->data;
  struct exchange *exchange = msg->exchange;
  struct ipsec_exch *ie = exchange->data;
  static int lifetype = 0;

  if (exchange->phase == 1)
    {
      switch (type)
	{
	case IKE_ATTR_ENCRYPTION_ALGORITHM:
	  /* XXX Errors possible?  */
	  exchange->crypto = crypto_get (from_ike_crypto (decode_16 (value)));
	  break;
	case IKE_ATTR_HASH_ALGORITHM:
	  /* XXX Errors possible?  */
	  ie->hash = hash_get (from_ike_hash (decode_16 (value)));
	  break;
	case IKE_ATTR_AUTHENTICATION_METHOD:
	  /* XXX Errors possible?  */
	  ie->ike_auth = ike_auth_get (decode_16 (value));
	  break;
	case IKE_ATTR_GROUP_DESCRIPTION:
	  isa->group_desc = decode_16 (value);
	  break;
	case IKE_ATTR_GROUP_TYPE:
	  break;
	case IKE_ATTR_GROUP_PRIME:
	  break;
	case IKE_ATTR_GROUP_GENERATOR_1:
	  break;
	case IKE_ATTR_GROUP_GENERATOR_2:
	  break;
	case IKE_ATTR_GROUP_CURVE_A:
	  break;
	case IKE_ATTR_GROUP_CURVE_B:
	  break;
	case IKE_ATTR_LIFE_TYPE:
	  lifetype = decode_16 (value);
	  return 0;
	case IKE_ATTR_LIFE_DURATION:
	  switch (lifetype)
	    {
	    case IKE_DURATION_SECONDS:
	      switch (len)
		{
		case 2:
		  sa->seconds = decode_16 (value);
		  break;
		case 4:
		  sa->seconds = decode_32 (value);
		  break;
		default:
		  /* XXX Log.  */
		}
	      break;
	    case IKE_DURATION_KILOBYTES:
	      switch (len)
		{
		case 2:
		  sa->kilobytes = decode_16 (value);
		  break;
		case 4:
		  sa->kilobytes = decode_32 (value);
		  break;
		default:
		  /* XXX Log.  */
		}
	      break;
	    default:
	      /* XXX Log!  */
	    }
	  break;
	case IKE_ATTR_PRF:
	  break;
	case IKE_ATTR_KEY_LENGTH:
	  exchange->key_length = decode_16 (value) / 8;
	  break;
	case IKE_ATTR_FIELD_SIZE:
	  break;
	case IKE_ATTR_GROUP_ORDER:
	  break;
	}
    }
  else
    {
      switch (type)
	{
	case IPSEC_ATTR_SA_LIFE_TYPE:
	  lifetype = decode_16 (value);
	  return 0;
	case IPSEC_ATTR_SA_LIFE_DURATION:
	  switch (lifetype)
	    {
	    case IPSEC_DURATION_SECONDS:
	      switch (len)
		{
		case 2:
		  sa->seconds = decode_16 (value);
		  break;
		case 4:
		  sa->seconds = decode_32 (value);
		  break;
		default:
		  /* XXX Log.  */
		}
	      break;
	    case IPSEC_DURATION_KILOBYTES:
	      switch (len)
		{
		case 2:
		  sa->kilobytes = decode_16 (value);
		  break;
		case 4:
		  sa->kilobytes = decode_32 (value);
		  break;
		default:
		  /* XXX Log.  */
		}
	      break;
	    default:
	      /* XXX Log!  */
	    }
	  break;
	case IPSEC_ATTR_GROUP_DESCRIPTION:
	  isa->group_desc = decode_16 (value);
	  break;
	case IPSEC_ATTR_ENCAPSULATION_MODE:
	  /* XXX Multiple protocols must have same encapsulation mode, no?  */
	  iproto->encap_mode = decode_16 (value);
	  break;
	case IPSEC_ATTR_AUTHENTICATION_ALGORITHM:
	  iproto->auth = decode_16 (value);
	  break;
	case IPSEC_ATTR_KEY_LENGTH:
	  iproto->keylen = decode_16 (value);
	  break;
	case IPSEC_ATTR_KEY_ROUNDS:
	  iproto->keyrounds = decode_16 (value);
	  break;
	case IPSEC_ATTR_COMPRESS_DICTIONARY_SIZE:
	  break;
	case IPSEC_ATTR_COMPRESS_PRIVATE_ALGORITHM:
	  break;
	}
    }
  lifetype = 0;
  return 0;
}

/*
 * Walk over the attributes of the transform payload found in BUF, and
 * fill out the fields of the SA attached to MSG.  Also mark the SA as
 * processed.
 */
void
ipsec_decode_transform (struct message *msg, struct sa *sa,
			struct proto *proto, u_int8_t *buf)
{
  struct ipsec_exch *ie = msg->exchange->data;
  struct ipsec_decode_arg ida;

  log_debug (LOG_MISC, 20, "ipsec_decode_transform: transform %d chosen",
	     GET_ISAKMP_TRANSFORM_NO (buf));

  ida.msg = msg;
  ida.sa = sa;
  ida.proto = proto;

  /* The default IKE lifetime is 8 hours.  */
  if (sa->phase == 1)
    sa->seconds = 28800;

  /* Extract the attributes and stuff them into the SA.  */
  attribute_map (buf + ISAKMP_TRANSFORM_SA_ATTRS_OFF,
		 GET_ISAKMP_GEN_LENGTH (buf) - ISAKMP_TRANSFORM_SA_ATTRS_OFF,
		 ipsec_decode_attribute, &ida);

  /*
   * If no pseudo-random function was negotiated, it's HMAC.
   * XXX As PRF_HMAC currently is zero, this is a no-op.
   */
  if (!ie->prf_type)
    ie->prf_type = PRF_HMAC;
}

/*
 * Delete the IPSec SA represented by the INCOMING direction in protocol PROTO
 * of the IKE security association SA.
 */
static void
ipsec_delete_spi (struct sa *sa, struct proto *proto, int incoming)
{
  if (sa->phase == 1)
    return;
  /* XXX Error handling?  Is it interesting?  */
  sysdep_ipsec_delete_spi (sa, proto, incoming);
}

/*
 * Store BUF into the g^x entry of the exchange that message MSG belongs to.
 * PEER is non-zero when the value is our peer's, and zero when it is ours.
 */
static int
ipsec_g_x (struct message *msg, int peer, u_int8_t *buf)
{
  struct exchange *exchange = msg->exchange;
  struct ipsec_exch *ie = exchange->data;
  u_int8_t **g_x;
  int initiator = exchange->initiator ^ peer;
  char header[32];

  g_x = initiator ? &ie->g_xi : &ie->g_xr;
  *g_x = malloc (ie->g_x_len);
  if (!*g_x)
    {
      log_error ("ipsec_g_x: malloc (%d) failed", ie->g_x_len);
      return -1;
    }
  memcpy (*g_x, buf, ie->g_x_len);
  snprintf (header, 32, "ipsec_g_x: g^x%c", initiator ? 'i' : 'r');
  log_debug_buf (LOG_MISC, 80, header, *g_x, ie->g_x_len);
  return 0;
}

/* Generate our DH value.  */
int
ipsec_gen_g_x (struct message *msg)
{
  struct exchange *exchange = msg->exchange;
  struct ipsec_exch *ie = exchange->data;
  u_int8_t *buf;

  buf = malloc (ISAKMP_KE_SZ + ie->g_x_len);
  if (!buf)
    {
      log_error ("ipsec_gen_g_x: malloc (%d) failed",
		 ISAKMP_KE_SZ + ie->g_x_len);
      return -1;
    }

  if (message_add_payload (msg, ISAKMP_PAYLOAD_KEY_EXCH, buf,
			   ISAKMP_KE_SZ + ie->g_x_len, 1))
    {
      free (buf);
      return -1;
    }

  if (dh_create_exchange (ie->group, buf + ISAKMP_KE_DATA_OFF))
    {
      log_print ("ipsec_gen_g_x: dh_create_exchange failed");
      free (buf);
      return -1;
    }
  return ipsec_g_x (msg, 0, buf + ISAKMP_KE_DATA_OFF);
}

/* Save the peer's DH value.  */
int
ipsec_save_g_x (struct message *msg)
{
  struct exchange *exchange = msg->exchange;
  struct ipsec_exch *ie = exchange->data;
  struct payload *kep;

  kep = TAILQ_FIRST (&msg->payload[ISAKMP_PAYLOAD_KEY_EXCH]);
  kep->flags |= PL_MARK;
  ie->g_x_len = GET_ISAKMP_GEN_LENGTH (kep->p) - ISAKMP_KE_DATA_OFF;

  /* Check that the given length matches the group's expectancy.  */
  if (ie->g_x_len != dh_getlen (ie->group))
    {
      /* XXX Is this a good notify type?  */
      message_drop (msg, ISAKMP_NOTIFY_PAYLOAD_MALFORMED, 0, 1, 0);
      return -1;
    }

  return ipsec_g_x (msg, 1, kep->p + ISAKMP_KE_DATA_OFF);
}

/*
 * Get a SPI for PROTO and the transport MSG passed over.  Store the
 * size where SZ points.  NB!  A zero return is OK if *SZ is zero.
 */
static u_int8_t *
ipsec_get_spi (size_t *sz, u_int8_t proto, struct message *msg)
{
  struct sockaddr *dst, *src;
  int dstlen, srclen;
  struct transport *transport = msg->transport;

  if (msg->exchange->phase == 1)
    {
      *sz = 0;
      return 0;
    }
  else
    {
      /* We are the destination in the SA we want a SPI for.  */
      transport->vtbl->get_src (transport, &dst, &dstlen);
      /* The peer is the source.  */
      transport->vtbl->get_dst (transport, &src, &srclen);
      return sysdep_ipsec_get_spi (sz, proto, src, srclen, dst, dstlen);
    }
}

/*
 * We have gotten a payload PAYLOAD of type TYPE, which did not get handled
 * by the logic of the exchange MSG takes part in.  Now is the time to deal
 * with such a payload if we know how to, if we don't, return -1, otherwise
 * 0.
 */
int
ipsec_handle_leftover_payload (struct message *msg, u_int8_t type,
			       struct payload *payload)
{
  struct sockaddr *dst;
  socklen_t dstlen;
  struct sa *sa;

  /* So far, the only thing we handle is an INITIAL-CONTACT NOTIFY.  */
  switch (type)
    {
    case ISAKMP_PAYLOAD_NOTIFY:
      switch (GET_ISAKMP_NOTIFY_MSG_TYPE (payload->p))
	{
	case IPSEC_NOTIFY_INITIAL_CONTACT:
	  /*
	   * Find out who is sending this and then delete every SA that is
	   * ready.  Exchanges will timeout themselves and then the
	   * non-ready SAs will disappear too.
	   */
	  msg->transport->vtbl->get_dst (msg->transport, &dst, &dstlen);
	  while ((sa = sa_lookup_by_peer (dst, dstlen)) != 0)
	    {
	      log_debug (LOG_SA, 30,
			 "ipsec_handle_leftover_payload: "
			 "INITIAL-CONTACT made us delete SA %p",
			 sa);
	      sa_delete (sa, 0);
	    }

	  payload->flags |= PL_MARK;
	  return 0;
	  break;
	}
    }
  return -1;
}

/* Return the encryption keylength in octets of the ESP protocol PROTO.  */
int
ipsec_esp_enckeylength (struct proto *proto)
{
  struct ipsec_proto *iproto = proto->data;

  /* Compute the keylength to use.  */
  switch (proto->id)
    {
    case IPSEC_ESP_DES:
    case IPSEC_ESP_DES_IV32:
    case IPSEC_ESP_DES_IV64:
      return 8;
    case IPSEC_ESP_3DES:
      return 24;
    case IPSEC_ESP_CAST:
      if (!iproto->keylen)
	return 16;
      /* Fallthrough */
    default:
      return iproto->keylen / 8;
    }
}

/* Return the authentication keylength in octets of the ESP protocol PROTO.  */
int
ipsec_esp_authkeylength (struct proto *proto)
{
  struct ipsec_proto *iproto = proto->data;

  switch (iproto->auth)
    {
    case IPSEC_AUTH_HMAC_MD5:
      return 16;
    case IPSEC_AUTH_HMAC_SHA:
      return 20;
    default:
      return 0;
    }
}

/* Return the authentication keylength in octets of the AH protocol PROTO.  */
int
ipsec_ah_keylength (struct proto *proto)
{
  switch (proto->id)
    {
    case IPSEC_AH_MD5:
      return 16;
    case IPSEC_AH_SHA:
      return 20;
    default:
      return -1;
    }
}

/* Return the total keymaterial length of the protocol PROTO.  */
int
ipsec_keymat_length (struct proto *proto)
{
  switch (proto->proto)
    {
    case IPSEC_PROTO_IPSEC_ESP:
      return ipsec_esp_enckeylength (proto) + ipsec_esp_authkeylength (proto);
    case IPSEC_PROTO_IPSEC_AH:
      return ipsec_ah_keylength (proto);
    default:
      return -1;
    }
}

/*
 * Out of a named section SECTION in the configuration file find out
 * the network address and mask as well as the ID type.  Put the info
 * in the areas pointed to by ADDR, MASK and ID respectively.  Return
 * 0 on success and -1 on failure.
 */
int
ipsec_get_id (char *section, int *id, struct in_addr *addr,
	      struct in_addr *mask)
{
  char *type, *address, *netmask;

  type = conf_get_str (section, "ID-type");
  if (!type)
    {
      log_print ("ipsec_get_id: section %s has no \"ID-type\" tag", section);
      return -1;
    }

  *id = constant_value (ipsec_id_cst, type);
  switch (*id)
    {
    case IPSEC_ID_IPV4_ADDR:
      address = conf_get_str (section, "Address");
      if (!address)
	{
	  log_print ("ipsec_get_id: section %s has no \"Address\" tag",
		     section);
	  return -1;
	}

      if (!inet_aton (address, addr))
	{
	  log_print ("ipsec_get_id: invalid address %s in section %s", section,
		     address);
	  return -1;
	}
      break;

#ifdef notyet
    case IPSEC_ID_FQDN:
      return -1;

    case IPSEC_ID_USER_FQDN:
      return -1;
#endif

    case IPSEC_ID_IPV4_ADDR_SUBNET:
      address = conf_get_str (section, "Network");
      if (!address)
	{
	  log_print ("ipsec_get_id: section %s has no \"Network\" tag",
		     section);
	  return -1;
	}

      if (!inet_aton (address, addr))
	{
	  log_print ("ipsec_get_id: invalid section %s network %s", section,
		     address);
	  return -1;
	}

      netmask = conf_get_str (section, "Netmask");
      if (!netmask)
	{
	  log_print ("ipsec_get_id: section %s has no \"Netmask\" tag",
		     section);
	  return -1;
	}

      if (!inet_aton (netmask, mask))
	{
	  log_print ("ipsec_id_build: invalid section %s network %s", section,
		     netmask);
	  return -1;
	}
      break;

#ifdef notyet
    case IPSEC_ID_IPV6_ADDR:
      return -1;

    case IPSEC_ID_IPV6_ADDR_SUBNET:
      return -1;

    case IPSEC_ID_IPV4_RANGE:
      return -1;

    case IPSEC_ID_IPV6_RANGE:
      return -1;

    case IPSEC_ID_DER_ASN1_DN:
      return -1;

    case IPSEC_ID_DER_ASN1_GN:
      return -1;

    case IPSEC_ID_KEY_ID:
      return -1;
#endif

    default:
      log_print ("ipsec_get_id: unknown ID type \"%s\" in section %s", type,
		 section);
      return -1;
    }

  return 0;
}

/*
 * Out of a named section SECTION in the configuration file build an
 * ISAKMP ID payload.  Ths payload size should be stashed in SZ.
 * The caller is responsible for freeing the payload.
 */
u_int8_t *
ipsec_build_id (char *section, size_t *sz)
{
  struct in_addr addr, mask;
  u_int8_t *p;
  int id;

  if (ipsec_get_id (section, &id, &addr, &mask))
    return 0;

  *sz = ISAKMP_ID_SZ;
  switch (id)
    {
    case IPSEC_ID_IPV4_ADDR:
      *sz += sizeof addr;
      break;
    case IPSEC_ID_IPV4_ADDR_SUBNET:
      *sz += sizeof addr + sizeof mask;
      break;
    }

  p = malloc (*sz);
  if (!p)
    {
      log_print ("ipsec_build_id: malloc(%d) failed", *sz);
      return 0;
    }

  SET_ISAKMP_ID_TYPE (p, id);
  SET_ISAKMP_ID_DOI_DATA (p, "\000\000\000");
  
  switch (id)
    {
    case IPSEC_ID_IPV4_ADDR:
      encode_32 (p + ISAKMP_ID_DATA_OFF, ntohl (addr.s_addr));
      break;
    case IPSEC_ID_IPV4_ADDR_SUBNET:
      encode_32 (p + ISAKMP_ID_DATA_OFF, ntohl (addr.s_addr));
      encode_32 (p + ISAKMP_ID_DATA_OFF + 4, ntohl (mask.s_addr));
      break;
    }

  return p;
}

/*
 * IPSec-specific PROTO initializations.  SECTION is only set if we are the
 * initiator thus only usable there.
 * XXX I want to fix this later.
 */
void
ipsec_proto_init (struct proto *proto, char *section)
{
  struct ipsec_proto *iproto = proto->data;

  if (proto->sa->phase == 2 && section)
    iproto->replay_window
      = conf_get_num (section, "ReplayWindow", DEFAULT_REPLAY_WINDOW);
}

/*
 * Add a notification payload of type INITIAL CONTACT to MSG if this is
 * the first contact we have made to our peer.
 */
int
ipsec_initial_contact (struct message *msg)
{
  u_int8_t *buf;

  if (ipsec_contacted (msg))
    return 0;

  buf = malloc (ISAKMP_NOTIFY_SZ + ISAKMP_HDR_COOKIES_LEN);
  if (!buf)
    {
      log_error ("ike_phase_1_initial_contact: malloc (%d) failed",
		 ISAKMP_NOTIFY_SZ + ISAKMP_HDR_COOKIES_LEN);
      return -1;
    }
  SET_ISAKMP_NOTIFY_DOI (buf, IPSEC_DOI_IPSEC);
  SET_ISAKMP_NOTIFY_PROTO (buf, ISAKMP_PROTO_ISAKMP);
  SET_ISAKMP_NOTIFY_SPI_SZ (buf, ISAKMP_HDR_COOKIES_LEN);
  SET_ISAKMP_NOTIFY_MSG_TYPE (buf, IPSEC_NOTIFY_INITIAL_CONTACT);
  memcpy (buf + ISAKMP_NOTIFY_SPI_OFF, msg->isakmp_sa->cookies,
	  ISAKMP_HDR_COOKIES_LEN);
  if (message_add_payload (msg, ISAKMP_PAYLOAD_NOTIFY, buf,
			   ISAKMP_NOTIFY_SZ + ISAKMP_HDR_COOKIES_LEN, 1))
    {
      free (buf);
      return -1;
    }

  return ipsec_add_contact (msg);
}

/*
 * Compare the two contacts pointed to by A and B.  Return negative if
 * *A < *B, 0 if they are equal, and positive if *A is the largest of them.
 */
static int
addr_cmp (const void *a, const void *b)
{
  const struct contact *x = a, *y = b;
  int minlen = MIN (x->len, y->len);
  int rv = memcmp (x->addr, y->addr, minlen);

  return rv ? rv : (x->len - y->len);
}

/*
 * Add the peer that MSG is bound to as an address we don't want to send
 * INITIAL CONTACT too from now on.  Do not call this function with a
 * specific address duplicate times. We want fast lookup, speed of insertion
 * is unimportant, if this is to scale.
 */
static int
ipsec_add_contact (struct message *msg)
{
  struct contact *new_contacts;
  struct sockaddr *dst, *addr;
  socklen_t dstlen;
  int cnt;

  if (contact_cnt == contact_limit)
    {
      cnt = contact_limit ? 2 * contact_limit : 64;
      new_contacts = realloc (contacts, cnt * sizeof contacts[0]);
      if (!new_contacts)
	{
	  log_error ("ipsec_add_contact: realloc (%p, %d) failed", contacts,
		     cnt * sizeof contacts[0]);
	  return -1;
	}
      contact_limit = cnt;
      contacts = new_contacts;
    }
  msg->transport->vtbl->get_dst (msg->transport, &dst, &dstlen);
  addr = malloc (dstlen);
  if (!addr)
    {
      log_error ("ipsec_add_contact: malloc (%d) failed", dstlen);
      return -1;
    }
  memcpy (addr, dst, dstlen);
  contacts[contact_cnt].addr = addr;
  contacts[contact_cnt++].len = dstlen;

  /*
   * XXX There are better algorithms for already mostly-sorted data like
   * this, but only qsort is standard.  I will someday do this inline.
   */
  qsort (contacts, contact_cnt, sizeof *contacts, addr_cmp);
  return 0;
}

/* Return true if the recipient of MSG has already been contacted.  */
static int
ipsec_contacted (struct message *msg)
{
  struct contact contact;

  msg->transport->vtbl->get_dst (msg->transport, &contact.addr, &contact.len);
  return contacts
    ? (bsearch (&contact, contacts, contact_cnt, sizeof *contacts, addr_cmp)
       != 0)
    : 0;
}

/* Add a HASH for to MSG.  */
u_int8_t *
ipsec_add_hash_payload (struct message *msg, size_t hashsize)
{
  u_int8_t *buf;

  buf = malloc (ISAKMP_HASH_SZ + hashsize);
  if (!buf)
    {
      log_error ("ipsec_add_hash_payload: malloc (%d) failed",
		 ISAKMP_HASH_SZ + hashsize);
      return 0;
    }

  if (message_add_payload (msg, ISAKMP_PAYLOAD_HASH, buf,
			   ISAKMP_HASH_SZ + hashsize, 1))
    {
      free (buf);
      return 0;
    }

  return buf;
}

/* Fill in the HASH payload of MSG.  */
int
ipsec_fill_in_hash (struct message *msg)
{
  struct exchange *exchange = msg->exchange;
  struct sa *isakmp_sa = msg->isakmp_sa;
  struct ipsec_sa *isa = isakmp_sa->data;
  struct hash *hash = hash_get (isa->hash);
  size_t hashsize = hash->hashsize;
  struct prf *prf;
  struct payload *payload;
  u_int8_t *buf;
  int i;
  char header[80];

  /* If no SKEYID_a, we need not do anything.  */
  if (!isa->skeyid_a)
    return 0;

  payload = TAILQ_FIRST (&msg->payload[ISAKMP_PAYLOAD_HASH]);
  if (!payload)
    {
      log_print ("ipsec_fill_in_hash: no HASH payload found");
      return -1;
    }
  buf = payload->p;

  /* Allocate the prf and start calculating our HASH(1).  */
  log_debug_buf (LOG_MISC, 90, "ipsec_fill_in_hash: SKEYID_a", isa->skeyid_a,
		 isa->skeyid_len);
  prf = prf_alloc (isa->prf_type, hash->type, isa->skeyid_a, isa->skeyid_len);
  if (!prf)
    return -1;

  prf->Init (prf->prfctx);
  log_debug_buf (LOG_MISC, 90, "ipsec_fill_in_hash: message_id",
		 exchange->message_id, ISAKMP_HDR_MESSAGE_ID_LEN);
  prf->Update (prf->prfctx, exchange->message_id, ISAKMP_HDR_MESSAGE_ID_LEN);

  /* Loop over all payloads after HASH(1).  */
  for (i = 2; i < msg->iovlen; i++)
    {
      /* XXX Misleading payload type printouts.  */
      snprintf (header, 80, "ipsec_fill_in_hash: payload %d after HASH(1)",
		i - 1);
      log_debug_buf (LOG_MISC, 90, header, msg->iov[i].iov_base,
		     msg->iov[i].iov_len);
      prf->Update (prf->prfctx, msg->iov[i].iov_base, msg->iov[i].iov_len);
    }
  prf->Final (buf + ISAKMP_HASH_DATA_OFF, prf->prfctx);
  prf_free (prf);
  log_debug_buf (LOG_MISC, 80, "ipsec_fill_in_hash: HASH(1)",
		 buf + ISAKMP_HASH_DATA_OFF, hashsize);

  return 0;
}

/* Add a HASH payload to MSG, if we have an ISAKMP SA we're protected by.  */
static int
ipsec_informational_pre_hook (struct message *msg)
{
  struct sa *isakmp_sa = msg->isakmp_sa;
  struct ipsec_sa *isa;
  struct hash *hash;

  if (!isakmp_sa)
    return 0;
  isa = isakmp_sa->data;
  hash = hash_get (isa->hash);
  return ipsec_add_hash_payload (msg, hash->hashsize) == 0;
}

/*
 * Fill in the HASH payload in MSG, if we have an ISAKMP SA we're protected by.
 */
static int
ipsec_informational_post_hook (struct message *msg)
{
  if (!msg->isakmp_sa)
    return 0;
  return ipsec_fill_in_hash (msg);
}

ssize_t
ipsec_id_size (char *section, u_int8_t *id)
{
  char *type, *data;

  type = conf_get_str (section, "ID-type");
  if (!type)
    {
      log_print ("ipsec_id_size: section %s has no \"ID-type\" tag", section);
      return -1;
    }

  *id = constant_value (ipsec_id_cst, type);
  switch (*id)
    {
    case IPSEC_ID_IPV4_ADDR:
      return sizeof (in_addr_t);
    case IPSEC_ID_IPV4_ADDR_SUBNET:
      return 2 * sizeof (in_addr_t);
    case IPSEC_ID_FQDN:
    case IPSEC_ID_USER_FQDN:
      data = conf_get_str (section, "Name");
      if (!data)
	{
	  log_print ("ipsec_id_size: section %s has no \"Name\" tag", section);
	  return -1;
	}
      return strlen (data);
    }
  log_print ("ipsec_id_size: unrecognized ID-type %d (%s)", *id, type);
  return -1;
}
