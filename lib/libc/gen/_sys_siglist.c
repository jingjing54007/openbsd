/*	$OpenBSD: src/lib/libc/gen/_sys_siglist.c,v 1.4 2012/12/05 23:19:59 deraadt Exp $ */
/*
 * Written by J.T. Conklin, December 12, 1994
 * Public domain.
 */

#include <sys/types.h>

#ifdef __indr_reference
__indr_reference(_sys_siglist, sys_siglist);
__indr_reference(_sys_siglist, __sys_siglist); /* Backwards compat with v.12 */
#else

#undef _sys_siglist
#define	_sys_siglist	sys_siglist
#include "siglist.c"

#undef _sys_siglist
#define	_sys_siglist	__sys_siglist
#include "siglist.c"

#endif
