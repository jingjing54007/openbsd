/*	$OpenBSD: src/regress/lib/libm/toint/toint.c,v 1.3 2003/07/31 21:48:04 deraadt Exp $	*/

/*	Written by Michael Shalayeff, 2003, Public domain.	*/

#include <stdio.h>
#include <signal.h>
#include <unistd.h>

static void
sigfpe(int sig, siginfo_t *si, void *v)
{
	char buf[132];

	if (si) {
		snprintf(buf, sizeof(buf), "sigfpe: addr=%p, code=%d\n",
		    si->si_addr, si->si_code);
		write(1, buf, strlen(buf));
	}
	_exit(1);
}

static int
toint(double d)
{
	return (int)d;
}

int
main(int argc, char *argv[])
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = sigfpe;
	sa.sa_flags = SA_SIGINFO;
	sigaction(SIGFPE, &sa, NULL);

	if (toint(8.6) != 8)
		exit(1);

	exit(0);
}
