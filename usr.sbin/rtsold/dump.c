/*	$OpenBSD: src/usr.sbin/rtsold/dump.c,v 1.15 2013/04/21 19:46:31 deraadt Exp $	*/
/*	$KAME: dump.c,v 1.10 2002/05/31 10:10:03 itojun Exp $	*/

/*
 * Copyright (C) 1999 WIDE Project.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/icmp6.h>

#include <syslog.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "rtsold.h"

static FILE *fp;

extern struct ifinfo *iflist;

static void dump_interface_status(void);
static char *sec2str(time_t);
char *ifstatstr[] = {"IDLE", "DELAY", "PROBE", "DOWN", "TENTATIVE"};

static void
dump_interface_status(void)
{
	struct ifinfo *ifinfo;
	struct timeval now;

	gettimeofday(&now, NULL);

	for (ifinfo = iflist; ifinfo; ifinfo = ifinfo->next) {
		fprintf(fp, "Interface %s\n", ifinfo->ifname);
		fprintf(fp, "  probe interval: ");
		if (ifinfo->probeinterval) {
			fprintf(fp, "%d\n", ifinfo->probeinterval);
			fprintf(fp, "  probe timer: %d\n", ifinfo->probetimer);
		} else {
			fprintf(fp, "infinity\n");
			fprintf(fp, "  no probe timer\n");
		}
		fprintf(fp, "  interface status: %s\n",
		    ifinfo->active > 0 ? "active" : "inactive");
#ifndef SMALL
		fprintf(fp, "  other config: %s\n",
		    ifinfo->otherconfig ? "on" : "off");
#endif
		fprintf(fp, "  rtsold status: %s\n", ifstatstr[ifinfo->state]);
		fprintf(fp, "  carrier detection: %s\n",
		    ifinfo->mediareqok ? "available" : "unavailable");
		fprintf(fp, "  probes: %d, dadcount = %d\n",
		    ifinfo->probes, ifinfo->dadcount);
		if (ifinfo->stoptimer)
			fprintf(fp, "  no timer\n");
		else {
			fprintf(fp, "  timer: interval=%lld:%ld, expire=%s\n",
			    (long long)ifinfo->timer.tv_sec,
			    ifinfo->timer.tv_usec,
			    (ifinfo->expire.tv_sec < now.tv_sec) ? "expired"
			    : sec2str(ifinfo->expire.tv_sec - now.tv_sec));
		}
		fprintf(fp, "  number of valid RAs: %d\n", ifinfo->racnt);
	}
}

void
rtsold_dump_file(char *dumpfile)
{
	if ((fp = fopen(dumpfile, "w")) == NULL) {
		warnmsg(LOG_WARNING, __func__, "open a dump file(%s): %s",
		    dumpfile, strerror(errno));
		return;
	}
	dump_interface_status();
	fclose(fp);
}

static char *
sec2str(time_t total)
{
	static char result[256];
	time_t days;
	int hours, mins, secs;
	int first = 1;
	char *p = result;
	char *ep = &result[sizeof(result)];
	int n;

	days = total / 3600 / 24;
	hours = (total / 3600) % 24;
	mins = (total / 60) % 60;
	secs = total % 60;

	if (days) {
		first = 0;
		n = snprintf(p, ep - p, "%lldd", (long long)days);
		if (n < 0 || n >= ep - p)
			return "?";
		p += n;
	}
	if (!first || hours) {
		first = 0;
		n = snprintf(p, ep - p, "%dh", hours);
		if (n < 0 || n >= ep - p)
			return "?";
		p += n;
	}
	if (!first || mins) {
		first = 0;
		n = snprintf(p, ep - p, "%dm", mins);
		if (n < 0 || n >= ep - p)
			return "?";
		p += n;
	}
	snprintf(p, ep - p, "%ds", secs);
	return(result);
}
