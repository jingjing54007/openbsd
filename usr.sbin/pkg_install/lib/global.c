/*	$OpenBSD: src/usr.sbin/pkg_install/lib/Attic/global.c,v 1.5 2003/07/04 17:31:19 avsm Exp $	*/

#ifndef lint
static const char rcsid[] = "$OpenBSD: src/usr.sbin/pkg_install/lib/Attic/global.c,v 1.5 2003/07/04 17:31:19 avsm Exp $";
#endif

/*
 * FreeBSD install - a package for the installation and maintainance
 * of non-core utilities.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * Jordan K. Hubbard

 * 18 July 1993
 *
 * Semi-convenient place to stick some needed globals.
 *
 */

#include "lib.h"

/* These are global for all utils */
Boolean	Verbose		= FALSE;
Boolean	Fake		= FALSE;
Boolean	Force		= FALSE;


