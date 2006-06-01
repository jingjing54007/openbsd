/*	$OpenBSD: src/usr.sbin/dvmrpd/prune.c,v 1.1 2006/06/01 14:12:20 norby Exp $ */

/*
 * Copyright (c) 2005, 2006 Esben Norby <norby@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <strings.h>

#include "igmp.h"
#include "dvmrpd.h"
#include "dvmrp.h"
#include "log.h"
#include "dvmrpe.h"

/* DVMRP prune packet handling */
int
send_prune(struct nbr *nbr, void *data, int len)
{
	struct sockaddr_in	 dst;
	struct buf		*buf;
	struct dvmrp_hdr	*dvmrp_hdr;
	int			 ret = 0;

	log_debug("send_prune: interface %s nbr %s", nbr->iface->name,
	    inet_ntoa(nbr->addr));

	if (nbr->iface->passive)
		return (0);

	if ((buf = buf_open(nbr->iface->mtu - sizeof(struct ip))) == NULL)
		fatal("send_prune");

	/* DVMRP header */
	if (gen_dvmrp_hdr(buf, nbr->iface, DVMRP_CODE_PRUNE))
		goto fail;

	dst.sin_family = AF_INET;
	dst.sin_len = sizeof(struct sockaddr_in);
	dst.sin_addr = nbr->addr;

	/* update chksum */
	dvmrp_hdr = buf_seek(buf, 0, sizeof(dvmrp_hdr));
	dvmrp_hdr->chksum = in_cksum(buf->buf, buf->wpos);

	ret = send_packet(nbr->iface, buf->buf, buf->wpos, &dst);
	buf_free(buf);
	return (ret);
fail:
	log_warn("send_prune");
	buf_free(buf);
	return (-1);
}

void
recv_prune(struct nbr *nbr, char *buf, u_int16_t len)
{
	log_debug("recv_prune: neighbor ID %s", inet_ntoa(nbr->id));

	return;
}
