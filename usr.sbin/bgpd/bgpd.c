/*	$OpenBSD: src/usr.sbin/bgpd/bgpd.c,v 1.11 2003/12/21 23:26:37 henning Exp $ */

/*
 * Copyright (c) 2003 Henning Brauer <henning@openbsd.org>
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
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mrt.h"
#include "bgpd.h"

void	sighdlr(int);
void	usage(void);
int	main(int, char *[]);
int	reconfigure(char *, struct bgpd_config *, struct mrt_config *);
int	dispatch_imsg(struct imsgbuf *, int, struct mrt_config *);

int			mrtfd = -1;
volatile sig_atomic_t	mrtdump = 0;
volatile sig_atomic_t	quit = 0;
volatile sig_atomic_t	reconfig = 0;
struct imsgbuf		ibuf_se;
struct imsgbuf		ibuf_rde;

void
sighdlr(int sig)
{
	switch (sig) {
	case SIGTERM:
	case SIGKILL:
	case SIGINT:
	case SIGCHLD:
		quit = 1;
		break;
	case SIGHUP:
		reconfig = 1;
		break;
	case SIGALRM:
		mrtdump = 1;
		break;
	case SIGUSR1:
		mrtdump = 2;
		break;
	}
}

void
usage(void)
{
	extern char *__progname;

	fprintf(stderr, "usage: %s [-dv] ", __progname);
	fprintf(stderr, "[-D macro=value] [-f file]\n");
	exit(1);
}

#define POLL_MAX		8
#define PFD_PIPE_SESSION	0
#define PFD_PIPE_ROUTE		1
#define PFD_MRT_START		2

int
main(int argc, char *argv[])
{
	struct bgpd_config	 conf;
	struct mrt_config	 mrtconf;
	struct mrtdump_config	*mconf, *(mrt[POLL_MAX]);
	struct pollfd		 pfd[POLL_MAX];
	pid_t			 io_pid = 0, rde_pid = 0;
	char			*conffile;
	int			 debug = 0;
	int			 ch, i, j, n, nfds;
	int			 pipe_m2s[2];
	int			 pipe_m2r[2];
	int			 pipe_s2r[2];

	conffile = CONFFILE;
	bgpd_process = PROC_MAIN;

	log_init(1);		/* log to stderr until daemonized */

	if (geteuid())
		fatal("need root privileges", 0);

	bzero(&conf, sizeof(conf));
	bzero(&mrtconf, sizeof(mrtconf));
	LIST_INIT(&mrtconf);

	while ((ch = getopt(argc, argv, "dD:f:v")) != -1) {
		switch (ch) {
		case 'd':
			debug = 1;
			break;
		case 'D':
			if (cmdline_symset(optarg) < 0)
				logit(LOG_CRIT,
				    "could not parse macro definition %s",
				    optarg);
			break;
		case 'f':
			conffile = optarg;
			break;
		case 'v':
			if (conf.opts & BGPD_OPT_VERBOSE)
				conf.opts |= BGPD_OPT_VERBOSE2;
			conf.opts |= BGPD_OPT_VERBOSE;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}

	if (parse_config(conffile, &conf, &mrtconf))
		fatal("config file has errors", 0);

	signal(SIGTERM, sighdlr);
	signal(SIGKILL, sighdlr);
	signal(SIGINT, sighdlr);
	signal(SIGCHLD, sighdlr);
	signal(SIGHUP, sighdlr);
	signal(SIGALRM, sighdlr);
	signal(SIGUSR1, sighdlr);

	log_init(debug);

	if (!debug)
		daemon(1, 0);

	logit(LOG_INFO, "startup");

	if (pipe(pipe_m2s) == -1)
		fatal("pipe", errno);
	if (fcntl(pipe_m2s[0], F_SETFL, O_NONBLOCK) == -1 ||
	    fcntl(pipe_m2s[1], F_SETFL, O_NONBLOCK) == -1)
		fatal("fcntl", errno);
	if (pipe(pipe_m2r) == -1)
		fatal("pipe", errno);
	if (fcntl(pipe_m2r[0], F_SETFL, O_NONBLOCK) == -1 ||
	    fcntl(pipe_m2r[1], F_SETFL, O_NONBLOCK) == -1)
		fatal("fcntl", errno);
	if (pipe(pipe_s2r) == -1)
		fatal("pipe", errno);
	if (fcntl(pipe_s2r[0], F_SETFL, O_NONBLOCK) == -1 ||
	    fcntl(pipe_s2r[1], F_SETFL, O_NONBLOCK) == -1)
		fatal("fcntl", errno);

	if ((rde_pid = rde_main(&conf, pipe_m2r, pipe_s2r)) < 0)
		fatal("could not start route decision engine", 0);

	if ((io_pid = session_main(&conf, pipe_m2s, pipe_s2r)) < 0)
		fatal("could not start session engine", 0);

	setproctitle("parent");

	close(pipe_m2s[1]);
	close(pipe_m2r[1]);
	close(pipe_s2r[0]);
	close(pipe_s2r[1]);

	imsg_init(&ibuf_se, pipe_m2s[0]);
	imsg_init(&ibuf_rde, pipe_m2r[0]);

	while (quit == 0) {
		pfd[PFD_PIPE_SESSION].fd = ibuf_se.sock;
		pfd[PFD_PIPE_SESSION].events = POLLIN;
		if (ibuf_se.w.queued)
			pfd[PFD_PIPE_SESSION].events |= POLLOUT;
		pfd[PFD_PIPE_ROUTE].fd = ibuf_rde.sock;
		pfd[PFD_PIPE_ROUTE].events = POLLIN;
		if (ibuf_rde.w.queued)
			pfd[PFD_PIPE_ROUTE].events |= POLLOUT;
		i = PFD_MRT_START;
		LIST_FOREACH(mconf, &mrtconf, list)
			if (mconf->msgbuf.queued > 0) {
				pfd[i].fd = mconf->msgbuf.sock;
				pfd[i].events |= POLLOUT;
				mrt[i++] = mconf;
			}

		if ((nfds = poll(pfd, 2, INFTIM)) == -1)
			if (errno != EINTR)
				fatal("poll error", errno);

		if (nfds > 0 && (pfd[PFD_PIPE_SESSION].revents & POLLOUT))
			if ((n = msgbuf_write(&ibuf_se.w)) == -1)
				fatal("pipe write error", errno);

		if (nfds > 0 && (pfd[PFD_PIPE_ROUTE].revents & POLLOUT))
			if ((n = msgbuf_write(&ibuf_rde.w)) == -1)
				fatal("pipe write error", errno);

		if (nfds > 0 && pfd[PFD_PIPE_SESSION].revents & POLLIN) {
			nfds--;
			dispatch_imsg(&ibuf_se, PFD_PIPE_SESSION, &mrtconf);
		}

		if (nfds > 0 && pfd[PFD_PIPE_ROUTE].revents & POLLIN) {
			nfds--;
			dispatch_imsg(&ibuf_rde, PFD_PIPE_ROUTE, &mrtconf);
		}

		for (j =  PFD_MRT_START; j < i && nfds > 0 ; j++) {
			if (pfd[j].revents & POLLOUT) {
				if ((n = msgbuf_write(&mrt[i]->msgbuf)) == -1)
					fatal("pipe write error", errno);
			}
		}

		if (reconfig) {
			logit(LOG_CRIT, "rereading config");
			reconfigure(conffile, &conf, &mrtconf);
			LIST_FOREACH(mconf, &mrtconf, list)
				mrt_state(mconf, IMSG_NONE, &ibuf_rde);
			reconfig = 0;
		}

		if (mrtdump == 1) {
			mrt_alrm(&mrtconf, &ibuf_rde);
			mrtdump = 0;
		} else if (mrtdump == 2) {
			mrt_usr1(&mrtconf, &ibuf_rde);
			mrtdump = 0;
		}
	}

	signal(SIGCHLD, SIG_IGN);

	if (io_pid)
		kill(io_pid, SIGTERM);

	if (rde_pid)
		kill(rde_pid, SIGTERM);

	do {
		i = waitpid(-1, NULL, WNOHANG);
	} while (i > 0 || (i == -1 && errno == EINTR));

	logit(LOG_CRIT, "Terminating");
	return (0);
}

int
reconfigure(char *conffile, struct bgpd_config *conf, struct mrt_config *mrtc)
{
	struct peer		*p;

	if (parse_config(conffile, conf, mrtc)) {
		logit(LOG_CRIT, "config file %s has errors, not reloading",
		    conffile);
		return (-1);
	}
	imsg_compose(&ibuf_se, IMSG_RECONF_CONF, 0,
	    conf, sizeof(struct bgpd_config));
	imsg_compose(&ibuf_rde, IMSG_RECONF_CONF, 0,
	    conf, sizeof(struct bgpd_config));
	for (p = conf->peers; p != NULL; p = p->next) {
		imsg_compose(&ibuf_se, IMSG_RECONF_PEER, p->conf.id,
		    &p->conf, sizeof(struct peer_config));
		imsg_compose(&ibuf_rde, IMSG_RECONF_PEER, p->conf.id,
		    &p->conf, sizeof(struct peer_config));
	}
	imsg_compose(&ibuf_se, IMSG_RECONF_DONE, 0, NULL, 0);
	imsg_compose(&ibuf_rde, IMSG_RECONF_DONE, 0, NULL, 0);

	return (0);
}

/*
 * XXX currently messages are only buffered for mrt files.
 */
int
dispatch_imsg(struct imsgbuf *ibuf, int idx, struct mrt_config *conf)
{
	struct imsg		 imsg;
	struct buf		*wbuf;
	struct mrtdump_config	*m;
	ssize_t			 len;
	int			 n;

	if (get_imsg(ibuf, &imsg) > 0) {
		switch (imsg.hdr.type) {
		case IMSG_MRT_MSG:
		case IMSG_MRT_END:
			LIST_FOREACH(m, conf, list) {
				if (m->id != imsg.hdr.peerid)
					continue;
				if (mrt_state(m, imsg.hdr.type, ibuf) == 0)
					break;
				if (m->msgbuf.sock == -1)
					break;
				len = imsg.hdr.len - IMSG_HEADER_SIZE;
				wbuf = buf_open(len);
				if (wbuf == NULL)
					fatal("buf_open error", 0);
				if (buf_add(wbuf, imsg.data, len) == -1)
					fatal("buf_add error", 0);
				if ((n = buf_close(&m->msgbuf, wbuf)) == -1)
					fatal("buf_close error", 0);
				break;
			}
			break;
		default:
			break;
		}
		imsg_free(&imsg);
	}
	return (0);
}

