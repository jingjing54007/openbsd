/*	$OpenBSD: src/lib/libm/arch/sh/e_sqrtf.c,v 1.2 2014/04/18 15:09:52 guenther Exp $	*/

/*
 * Written by Martynas Venckus.  Public domain
 */

#include <sys/types.h>
#include <math.h>

#define	FPSCR_PR	(1 << 19)

float
sqrtf(float f)
{
	register_t fpscr, nfpscr;

	__asm__ volatile ("sts fpscr, %0" : "=r" (fpscr));

	/* Set floating-point mode to single-precision. */
	nfpscr = fpscr & ~FPSCR_PR;

	__asm__ volatile ("lds %0, fpscr" : : "r" (nfpscr));
	__asm__ volatile ("fsqrt %0" : "+f" (f));

	/* Restore fp status/control register. */
	__asm__ volatile ("lds %0, fpscr" : : "r" (fpscr));

	return (f);
}

