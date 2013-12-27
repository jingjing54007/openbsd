/*	$OpenBSD: src/usr.sbin/installboot/installboot.c,v 1.1 2013/12/27 13:52:40 jsing Exp $	*/

/*
 * Copyright (c) 2012, 2013 Joel Sing <jsing@openbsd.org>
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

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <util.h>

#include "installboot.h"

int	nowrite;
int	stages;
int	verbose;

char	*stage1;
char	*stage2;

static __dead void
usage(void)
{
	extern char *__progname;

	fprintf(stderr, "usage: %s [-nv] disk [stage1%s]\n",
	    __progname, (stages >= 2) ? " [stage2]" : "");

	exit(1);
}

int
main(int argc, char **argv)
{
	char *dev, *realdev;
	int devfd;
	char opt;

	md_init();

	while ((opt = getopt(argc, argv, "nv")) != -1) {
		switch (opt) {
		case 'n':
			nowrite = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 1 || argc > stages + 1)
		usage();

	dev = argv[0];
	if (argc > 1)
		stage1 = argv[1];
	if (argc > 2)
		stage2 = argv[2];

	if ((devfd = opendev(dev, (nowrite ? O_RDONLY : O_RDWR), OPENDEV_PART,
	    &realdev)) < 0)
		err(1, "open: %s", realdev);

        if (verbose) {
		fprintf(stderr, "%s bootstrap on %s\n",
		    (nowrite ? "would install" : "installing"), realdev);
		if (stage1)
			fprintf(stderr, "using first-stage %s", stage1);
		if (stage2)
			fprintf(stderr, ", second-stage %s", stage2);
		fprintf(stderr, "\n");
	} 

	md_loadboot();

#ifdef SOFTRAID
	sr_installboot(devfd, dev);
#else
	md_installboot(devfd, realdev);
#endif

	return 0;
}
