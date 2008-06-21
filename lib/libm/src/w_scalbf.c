/* w_scalbf.c -- float version of w_scalb.c.
 * Conversion to float by Ian Lance Taylor, Cygnus Support, ian@cygnus.com.
 */

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice 
 * is preserved.
 * ====================================================
 */

#if defined(LIBM_SCCS) && !defined(lint)
static char rcsid[] = "$NetBSD: w_scalbf.c,v 1.3 1995/05/10 20:49:50 jtc Exp $";
#endif

/*
 * wrapper scalbf(float x, float fn) is provide for
 * passing various standard test suite. One 
 * should use scalbn() instead.
 */

#include "math.h"
#include "math_private.h"

#include <errno.h>

float
#ifdef _SCALB_INT
scalbf(float x, int fn)		/* wrapper scalbf */
#else
scalbf(float x, float fn)	/* wrapper scalbf */
#endif
{
#ifdef _IEEE_LIBM
	return __ieee754_scalbf(x,fn);
#else
	float z;
	z = __ieee754_scalbf(x,fn);
	if(_LIB_VERSION == _IEEE_) return z;
	if(!(finitef(z)||isnanf(z))&&finitef(x)) {
	    /* scalbf overflow */
	    return (float)__kernel_standard((double)x,(double)fn,132);
	}
	if(z==(float)0.0&&z!=x) {
	    /* scalbf underflow */
	    return (float)__kernel_standard((double)x,(double)fn,133);
	} 
#ifndef _SCALB_INT
	if(!finitef(fn)) errno = ERANGE;
#endif
	return z;
#endif 
}
