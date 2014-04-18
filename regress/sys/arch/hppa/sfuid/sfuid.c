/*	$OpenBSD: src/regress/sys/arch/hppa/sfuid/sfuid.c,v 1.2 2014/04/18 14:38:21 guenther Exp $	*/

/*
 * Written by Michael Shalayeff, 2004. Public Domain.
 */

#include <sys/param.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <err.h>

#define	sfuid(i,r)	__asm volatile(	\
    "spop1,%1,0	%0" : "=r" (r) : "i" (i))

volatile int sfu;

void
sigill(int sig, siginfo_t *sip, void *scp)
{
	char buf[1024];

	snprintf(buf, sizeof buf, "sfuid(%d) not decoded\n", sfu);
	write(STDOUT_FILENO, buf, strlen(buf));
        _exit(1);
}

int
main(int argc, char *argv[])
{
	struct sigaction sa;
	int rv;

	sa.sa_sigaction = &sigill;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGILL, &sa, NULL);

#define	test_sfuid(i,rv)	\
	rv = -1, sfu = i;	\
	sfuid(i, rv);		\
	if (rv != 0)		\
		errx(1, "sfuid(%d) returned %d", i, rv);

	sfuid(0, rv);
	sfuid(1, rv);
	sfuid(2, rv);
	sfuid(3, rv);
	sfuid(4, rv);
	sfuid(5, rv);
	sfuid(6, rv);
	sfuid(7, rv);

	exit(0);
}
