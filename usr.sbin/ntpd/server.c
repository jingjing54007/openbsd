/*	$OpenBSD: src/usr.sbin/ntpd/server.c,v 1.4 2004/07/04 18:07:15 henning Exp $ */

/*
 * Copyright (c) 2003, 2004 Henning Brauer <henning@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <ifaddrs.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ntpd.h"
#include "ntp.h"

int
setup_listeners(struct servent *se, struct ntpd_conf *conf, u_int *cnt)
{
	char			 ntopbuf[INET6_ADDRSTRLEN];
	struct listen_addr	*la;
	struct ifaddrs		*ifap;
	struct sockaddr		*sap;
	u_int			 new_cnt = 0;

	if (TAILQ_EMPTY(&conf->listen_addrs)) {
		if (getifaddrs(&ifap) == -1)
			fatal("getifaddrs");

		for (; ifap != NULL; ifap = ifap->ifa_next) {
			sap = ifap->ifa_addr;

			if (sap->sa_family != AF_INET &&
			    sap->sa_family != AF_INET6)
				continue;

			if ((la = calloc(1, sizeof(struct listen_addr))) ==
			    NULL)
				fatal("setup_listeners calloc");

			memcpy(&la->sa, sap, SA_LEN(sap));
			TAILQ_INSERT_TAIL(&conf->listen_addrs, la, entry);
		}

		freeifaddrs(ifap);
	}

	TAILQ_FOREACH(la, &conf->listen_addrs, entry) {
		sap = (struct sockaddr *)&la->sa;
		new_cnt++;

		switch (la->sa.ss_family) {
		case AF_INET:
			if (((struct sockaddr_in *)sap)->sin_port == 0)
				((struct sockaddr_in *)sap)->sin_port =
				    se->s_port;
			inet_ntop(AF_INET,
			    &((struct sockaddr_in *)sap)->sin_addr,
			    ntopbuf, sizeof(ntopbuf));
			break;
		case AF_INET6:
			if (((struct sockaddr_in6 *)sap)->sin6_port == 0)
				((struct sockaddr_in6 *)sap)->sin6_port =
				    se->s_port;
			inet_ntop(AF_INET6,
			    &((struct sockaddr_in6 *)sap)->sin6_addr,
			    ntopbuf, sizeof(ntopbuf));
			break;
		default:
			fatalx("king bula sez: af borked");
		}

		log_debug("adding listener on %s", ntopbuf);

		if ((la->fd = socket(la->sa.ss_family, SOCK_DGRAM, 0)) == -1)
			fatal("socket");

		if (bind(la->fd, (struct sockaddr *)&la->sa, la->sa.ss_len) ==
		    -1)
			fatal("bind");
	}

	*cnt = new_cnt;

	return (0);
}

int
ntp_reply(int fd, struct sockaddr *sa, struct ntp_msg *query, int auth)
{
	ssize_t			 len;
	struct l_fixedpt	 t;
	struct ntp_msg		 reply;

	if (auth)
		len = NTP_MSGSIZE;
	else
		len = NTP_MSGSIZE_NOAUTH;

	bzero(&reply, sizeof(reply));
	reply.status = 0 | (query->status & VERSIONMASK);
	if ((query->status & MODEMASK) == MODE_CLIENT)
		reply.status |= MODE_SERVER;
	else
		reply.status |= MODE_SYM_PAS;

	reply.stratum =	2;
	reply.ppoll = query->ppoll;
	reply.precision = 0;			/* XXX */
	reply.refid = htonl(t.fraction);	/* XXX */
	get_ts(&t);
	reply.reftime.int_part = htonl(t.int_part);	/* XXX */
	reply.reftime.fraction = htonl(t.fraction);	/* XXX */
	reply.rectime.int_part = htonl(t.int_part);
	reply.rectime.fraction = htonl(t.fraction);
	reply.xmttime.int_part = htonl(t.int_part);
	reply.xmttime.fraction = htonl(t.fraction);
	reply.orgtime.int_part = query->xmttime.int_part;
	reply.orgtime.fraction = query->xmttime.fraction;

	return (ntp_sendmsg(fd, sa, &reply, len, auth));
}
