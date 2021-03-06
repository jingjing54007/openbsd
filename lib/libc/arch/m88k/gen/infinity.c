/*	$OpenBSD: src/lib/libc/arch/m88k/gen/infinity.c,v 1.4 2005/08/07 16:40:14 espie Exp $	*/

/* infinity.c */

#include <math.h>

/* bytes for +Infinity on m88k */
char __infinity[] __attribute__((__aligned__(sizeof(double)))) =
    { 0x7f, 0xf0, 0, 0, 0, 0, 0, 0 };
