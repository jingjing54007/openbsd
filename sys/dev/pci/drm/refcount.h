/*	$OpenBSD: src/sys/dev/pci/drm/refcount.h,v 1.1 2013/08/12 04:11:53 jsg Exp $	*/
/*-
 * Copyright (c) 2005 John Baldwin <jhb@FreeBSD.org>
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
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef __SYS_REFCOUNT_H__
#define __SYS_REFCOUNT_H__

#include <sys/limits.h>
#include <machine/atomic.h>

#ifdef _KERNEL
#include <sys/systm.h>
#else
#define	KASSERT(exp, msg)	/* */
#endif

static __inline void
refcount_init(volatile u_int *count, u_int value)
{
	*count = value;
}

static __inline void
refcount_acquire(volatile u_int *count)
{
	/* check if refcount overflowed */
	KASSERT(*count < UINT_MAX);
	atomic_inc(count);
}

static __inline int
refcount_release(volatile u_int *count)
{
	u_int old;

	/* XXX: Should this have a rel membar? */
	old = atomic_fetchsub_int(count, 1);

	/* check for negative refcount */
	KASSERT(old > 0);
	return (old == 1);
}

#endif	/* ! __SYS_REFCOUNT_H__ */
