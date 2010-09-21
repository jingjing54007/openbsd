/*	$OpenBSD: src/sys/arch/octeon/dev/Attic/comreg.h,v 1.2 2010/09/21 06:13:21 syuu Exp $	*/
/*	$NetBSD: ns16550reg.h,v 1.4 1994/10/27 04:18:43 cgd Exp $	*/

/*-
 * Copyright (c) 1991 The Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ns16550.h	7.1 (Berkeley) 5/9/91
 */

/*
 * OCTEON UART registers
 */

#define	com_rxdata	0x00	/* data register (R) */
#define	com_txdata	0x40	/* data register (W) */
#define	com_dlbl	0x80	/* divisor latch low (W) */
#define	com_dlbh	0x88	/* divisor latch high (W) */
#define	com_ier		0x08	/* interrupt enable (W) */
#define	com_iir		0x10	/* interrupt identification (R) */
#define	com_fifo	0x50	/* FIFO control (W) */
#define com_fctl	0x408	/* extended FIFO control (W) */
#define com_efr		0x408	/* extended features register (W) */
#define	com_lctl	0x18	/* line control register (R/W) */
#define	com_cfcr	0x18	/* line control register (R/W) */
#define	com_mcr		0x20	/* modem control register (R/W) */
#define	com_lsr		0x28	/* line status register (R/W) */
#define	com_msr		0x30	/* modem status register (R/W) */
#define com_scratch	0x38	/* scratch register (R/W) */
