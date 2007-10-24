/*	$OpenBSD: src/usr.sbin/ripd/ripe.c,v 1.6 2007/10/24 19:05:06 claudio Exp $ */

/*
 * Copyright (c) 2006 Michele Marchetto <mydecay@openbeer.it>
 * Copyright (c) 2005 Claudio Jeker <claudio@openbsd.org>
 * Copyright (c) 2004 Esben Norby <norby@openbsd.org>
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
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_types.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <pwd.h>
#include <unistd.h>
#include <event.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "ripd.h"
#include "rip.h"
#include "ripe.h"
#include "log.h"
#include "control.h"

void		 ripe_sig_handler(int, short, void *);
void		 ripe_shutdown(void);

struct ripd_conf	*oeconf = NULL;
struct imsgbuf		*ibuf_main;
struct imsgbuf		*ibuf_rde;

/* ARGSUSED */
void
ripe_sig_handler(int sig, short event, void *bula)
{
	switch (sig) {
	case SIGINT:
	case SIGTERM:
		ripe_shutdown();
		/* NOTREACHED */
	default:
		fatalx("unexpected signal");
	}
}

/* rip engine */
pid_t
ripe(struct ripd_conf *xconf, int pipe_parent2ripe[2], int pipe_ripe2rde[2],
    int pipe_parent2rde[2])
{
	struct event		 ev_sigint, ev_sigterm;
	struct sockaddr_in	 addr;
	struct iface		*iface = NULL;
	struct passwd		*pw;
	struct redistribute	*r;
	pid_t			 pid;

	switch (pid = fork()) {
	case -1:
		fatal("cannot fork");
	case 0:
		break;
	default:
		return (pid);
	}

	/* create ripd control socket outside chroot */
	if (control_init() == -1)
		fatalx("control socket setup failed");

	addr.sin_family = AF_INET;
	addr.sin_port = htons(RIP_PORT);
	addr.sin_addr.s_addr = INADDR_ANY;

	if ((xconf->rip_socket = socket(AF_INET, SOCK_DGRAM,
	    IPPROTO_UDP)) == -1)
		fatalx("if_init: cannot create socket");

	if (bind(xconf->rip_socket, (struct sockaddr *)&addr,
	    sizeof(addr)) == -1)
		fatal("error creating socket");

	/* set some defaults */
	if (if_set_opt(xconf->rip_socket) == -1)
		fatal("if_set_opt");

	if (if_set_mcast_ttl(xconf->rip_socket, IP_DEFAULT_MULTICAST_TTL) == -1)
		fatal("if_set_mcast_ttl");

	if (if_set_mcast_loop(xconf->rip_socket) == -1)
		fatal("if_set_mcast_loop");

	if (if_set_tos(xconf->rip_socket, IPTOS_PREC_INTERNETCONTROL) == -1)
		fatal("if_set_tos");

	if_set_recvbuf(xconf->rip_socket);

	oeconf = xconf;

	if ((pw = getpwnam(RIPD_USER)) == NULL)
		fatal("getpwnam");

	if (chroot(pw->pw_dir) == -1)
		fatal("chroot");
	if (chdir("/") == -1)
		fatal("chdir(\"/\")");

	setproctitle("rip engine");
	ripd_process = PROC_RIP_ENGINE;

	if (setgroups(1, &pw->pw_gid) ||
	    setresgid(pw->pw_gid, pw->pw_gid, pw->pw_gid) ||
	    setresuid(pw->pw_uid, pw->pw_uid, pw->pw_uid))
		fatal("can't drop privileges");

	event_init();
	nbr_init(NBR_HASHSIZE);

	/* setup signal handler */
	signal_set(&ev_sigint, SIGINT, ripe_sig_handler, NULL);
	signal_set(&ev_sigterm, SIGTERM, ripe_sig_handler, NULL);
	signal_add(&ev_sigint, NULL);
	signal_add(&ev_sigterm, NULL);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	/* setup pipes */
	close(pipe_parent2ripe[0]);
	close(pipe_ripe2rde[1]);
	close(pipe_parent2rde[0]);
	close(pipe_parent2rde[1]);

	if ((ibuf_rde = malloc(sizeof(struct imsgbuf))) == NULL ||
	    (ibuf_main = malloc(sizeof(struct imsgbuf))) == NULL)
		fatal(NULL);
	imsg_init(ibuf_rde, pipe_ripe2rde[0], ripe_dispatch_rde);
	imsg_init(ibuf_main, pipe_parent2ripe[1], ripe_dispatch_main);

	/* setup event handler */
	ibuf_rde->events = EV_READ;
	event_set(&ibuf_rde->ev, ibuf_rde->fd, ibuf_rde->events,
	    ibuf_rde->handler, ibuf_rde);
	event_add(&ibuf_rde->ev, NULL);

	ibuf_main->events = EV_READ;
	event_set(&ibuf_main->ev, ibuf_main->fd, ibuf_main->events,
	    ibuf_main->handler, ibuf_main);
	event_add(&ibuf_main->ev, NULL);

	event_set(&oeconf->ev, oeconf->rip_socket, EV_READ|EV_PERSIST,
	    recv_packet, oeconf);
	event_add(&oeconf->ev, NULL);

	/* remove unneeded config stuff */
	while ((r = SIMPLEQ_FIRST(&oeconf->redist_list)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&oeconf->redist_list, entry);
		free(r);
	}

	/* listen on ripd control socket */
	TAILQ_INIT(&ctl_conns);
	control_listen();

	if ((pkt_ptr = calloc(1, READ_BUF_SIZE)) == NULL)
		fatal("ripe");

	/* start interfaces */
	LIST_FOREACH(iface, &xconf->iface_list, entry) {
		if_init(xconf, iface);
		if (if_fsm(iface, IF_EVT_UP))
			log_debug("ripe: error starting interface: %s",
			    iface->name);
	}

	evtimer_set(&oeconf->report_timer, report_timer, oeconf);
	start_report_timer();

	ripe_imsg_compose_rde(IMSG_FULL_REQUEST, 0, 0, NULL, 0);

	event_dispatch();

	ripe_shutdown();
	/* NOTREACHED */
	return (0);
}

int
ripe_imsg_compose_parent(int type, pid_t pid, void *data, u_int16_t datalen)
{
	return (imsg_compose(ibuf_main, type, 0, pid, data, datalen));
}

int
ripe_imsg_compose_rde(int type, u_int32_t peerid, pid_t pid,
    void *data, u_int16_t datalen)
{
	return (imsg_compose(ibuf_rde, type, peerid, pid, data, datalen));
}

/* ARGSUSED */
void
ripe_dispatch_main(int fd, short event, void *bula)
{
	struct imsg	 imsg;
	struct imsgbuf	*ibuf = bula;
	struct kif	*kif;
	struct iface	*iface;
	ssize_t		 n;
	int		 link_ok, shut = 0;

	switch (event) {
	case EV_READ:
		if ((n = imsg_read(ibuf)) == -1)
			fatal("imsg_read error");
		if (n == 0)	/* connection closed */
			shut = 1;
		break;
	case EV_WRITE:
		if (msgbuf_write(&ibuf->w) == -1)
			fatal("msgbuf_write");
		imsg_event_add(ibuf);
		return;
	default:
		fatalx("unknown event");
	}

	for (;;) {
		if ((n = imsg_get(ibuf, &imsg)) == -1)
			fatal("ripe_dispatch_main: imsg_read error");
		if (n == 0)
			break;

		switch (imsg.hdr.type) {
		case IMSG_IFINFO:
			if (imsg.hdr.len - IMSG_HEADER_SIZE !=
			    sizeof(struct kif))
				fatalx("IFINFO imsg with wrong len");
			kif = imsg.data;
			link_ok = (kif->flags & IFF_UP) &&
			    (LINK_STATE_IS_UP(kif->link_state) ||
			    (kif->link_state == LINK_STATE_UNKNOWN &&
			    kif->media_type != IFT_CARP));

			LIST_FOREACH(iface, &oeconf->iface_list, entry) {
				if (kif->ifindex == iface->ifindex) {
					iface->flags = kif->flags;
					iface->linkstate = kif->link_state;

					if (link_ok) {
						if_fsm(iface, IF_EVT_UP);
						log_warnx("interface %s up",
						    iface->name);
					} else {
						if_fsm(iface, IF_EVT_DOWN);
						log_warnx("interface %s down",
						    iface->name);
					}
				}
			}
			break;
		case IMSG_CTL_IFINFO:
		case IMSG_CTL_KROUTE:
		case IMSG_CTL_KROUTE_ADDR:
		case IMSG_CTL_END:
			control_imsg_relay(&imsg);
			break;
		default:
			log_debug("ripe_dispatch_main: error handling imsg %d",
			    imsg.hdr.type);
			break;
		}
		imsg_free(&imsg);
	}
	if (!shut)
		imsg_event_add(ibuf);
	else {
		/* this pipe is dead, so remove the event handler */  
		event_del(&ibuf->ev);
		event_loopexit(NULL);
	}
}

/* ARGSUSED */
void
ripe_dispatch_rde(int fd, short event, void *bula)
{
	struct rip_route	*rr;
	struct imsg		 imsg;
	struct imsgbuf		*ibuf = bula;
	struct iface		*iface;
	struct nbr		*nbr;
	ssize_t			 n;
	int			 shut = 0;

	switch (event) {
	case EV_READ:
		if ((n = imsg_read(ibuf)) == -1)
			fatal("imsg_read error");
		if (n == 0)	/* connection closed */
			shut = 1;
		break;
	case EV_WRITE:
		if (msgbuf_write(&ibuf->w) == -1)
			fatal("msgbuf_write");
		imsg_event_add(ibuf);
		return;
	default:
		fatalx("unknown event");
	}

	for (;;) {
		if ((n = imsg_get(ibuf, &imsg)) == -1)
			fatal("ripe_dispatch_rde: imsg_read error");
		if (n == 0)
			break;

		switch (imsg.hdr.type) {
		case IMSG_REQUEST_ADD:
			if (imsg.hdr.len - IMSG_HEADER_SIZE != sizeof(*rr))
				fatalx("invalid size of RDE request");

			if ((rr = malloc(sizeof(*rr))) == NULL)
				fatal("ripe_dispatch_rde");

			memcpy(rr, imsg.data, sizeof(*rr));

			if (imsg.hdr.peerid != 0) {
				if ((nbr = nbr_find_peerid(imsg.hdr.peerid)) ==
				    NULL) {
					log_debug("unknown neighbor id %u",
					    imsg.hdr.peerid);
					break;
				}
				add_entry(&nbr->rq_list, rr);
				break;
			}

			LIST_FOREACH(iface, &oeconf->iface_list, entry) {
				add_entry(&iface->rq_list, rr);
			}
			break;
		case IMSG_SEND_REQUEST:
			if (imsg.hdr.peerid != 0) {
				if ((nbr = nbr_find_peerid(imsg.hdr.peerid)) ==
				    NULL) {
					log_debug("unknown neighbor id %u",
					    imsg.hdr.peerid);
					break;
				}
				send_request(&nbr->rq_list, NULL, nbr);
				break;
			}

			LIST_FOREACH(iface, &oeconf->iface_list, entry) {
				send_request(&iface->rq_list, iface, NULL);
			}
			break;
		case IMSG_RESPONSE_ADD:
			if (imsg.hdr.len - IMSG_HEADER_SIZE != sizeof(*rr))
				fatalx("invalid size of RDE request");

			if ((rr = malloc(sizeof(*rr))) == NULL)
				fatal("ripe_dispatch_rde");

			memcpy(rr, imsg.data, sizeof(*rr));

			if (imsg.hdr.peerid == 0) {
				LIST_FOREACH(iface, &oeconf->iface_list,
				    entry) {
					if ((iface->addr.s_addr &
					    iface->mask.s_addr) !=
					    rr->address.s_addr ||
					    iface->mask.s_addr !=
					    rr->mask.s_addr)
						add_entry(&iface->rp_list,
						    rr);
				}
				break;
			}

			if ((nbr = nbr_find_peerid(imsg.hdr.peerid)) == NULL) {
				log_debug("unknown neighbor id %u",
				    imsg.hdr.peerid);
				break;
			}
			iface = nbr->iface;
			if ((iface->addr.s_addr & iface->mask.s_addr) !=
			    rr->address.s_addr ||
			    iface->mask.s_addr != rr->mask.s_addr)
				add_entry(&nbr->rp_list, rr);
			break;
		case IMSG_SEND_RESPONSE:
			if (imsg.hdr.peerid == 0) {
				LIST_FOREACH(iface, &oeconf->iface_list,
				    entry) {
					send_response(&iface->rp_list,
					    iface, NULL);
				}
				break;
			}

			if ((nbr = nbr_find_peerid(imsg.hdr.peerid)) == NULL) {
				log_debug("unknown neighbor id %u",
				    imsg.hdr.peerid);
				break;
			}
			send_response(&nbr->rp_list, NULL, nbr);
			nbr_fsm(nbr, NBR_EVT_RESPONSE_SENT);
			break;
		case IMSG_SEND_TRIGGERED_UPDATE:
			if (imsg.hdr.len - IMSG_HEADER_SIZE != sizeof(struct
			    rip_route))
				fatalx("invalid size of RDE request");

			rr = imsg.data;

			LIST_FOREACH(iface, &oeconf->iface_list,
			    entry) {
				if (rr->ifindex != iface->ifindex)
					send_triggered_update(iface, rr);
			}
			break;
		case IMSG_CTL_END:
		case IMSG_CTL_SHOW_RIB:
			control_imsg_relay(&imsg);
			break;
		default:
			log_debug("ripe_dispatch_rde: error handling imsg %d",
			    imsg.hdr.type);
			break;
		}
		imsg_free(&imsg);
	}
	if (!shut)
		imsg_event_add(ibuf);
	else {
		/* this pipe is dead, so remove the event handler */  
		event_del(&ibuf->ev);
		event_loopexit(NULL);
	}
}

void
ripe_shutdown(void)
{
	struct iface	*iface;

	LIST_FOREACH(iface, &oeconf->iface_list, entry) {
		if (if_fsm(iface, IF_EVT_DOWN)) {
			log_debug("error stopping interface %s",
			    iface->name);
		}
	}
	while ((iface = LIST_FIRST(&oeconf->iface_list)) != NULL) {
		LIST_REMOVE(iface, entry);
		if_del(iface);
	}

	close(oeconf->rip_socket);

	/* clean up */
	msgbuf_write(&ibuf_rde->w);
	msgbuf_clear(&ibuf_rde->w);
	free(ibuf_rde);
	msgbuf_write(&ibuf_main->w);
	msgbuf_clear(&ibuf_main->w);
	free(ibuf_main);
	free(oeconf);
	free(pkt_ptr);

	log_info("rip engine exiting");
	_exit(0);
}

void
ripe_iface_ctl(struct ctl_conn *c, unsigned int idx)
{
	struct iface		*iface;
	struct ctl_iface	*ictl;

	LIST_FOREACH(iface, &oeconf->iface_list, entry) {
		if (idx == 0 || idx == iface->ifindex) {
			ictl = if_to_ctl(iface);
			imsg_compose(&c->ibuf, IMSG_CTL_SHOW_IFACE,
			    0, 0, ictl, sizeof(struct ctl_iface));
		}
	}
}

void
ripe_nbr_ctl(struct ctl_conn *c)
{
	struct iface	*iface;
	struct nbr	*nbr;
	struct ctl_nbr	*nctl;

	LIST_FOREACH(iface, &oeconf->iface_list, entry)
		LIST_FOREACH(nbr, &iface->nbr_list, entry) {
				nctl = nbr_to_ctl(nbr);
				imsg_compose(&c->ibuf,
				    IMSG_CTL_SHOW_NBR, 0, 0, nctl,
				    sizeof(struct ctl_nbr));
		}

	imsg_compose(&c->ibuf, IMSG_CTL_END, 0, 0, NULL, 0);
}
