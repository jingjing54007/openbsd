/*	$OpenBSD: src/regress/lib/libpthread/preemption/preemption.c,v 1.4 2003/07/31 21:48:05 deraadt Exp $	*/
/*
 * Copyright (c) 1993, 1994, 1995, 1996 by Chris Provenzano and contributors, 
 * proven@mit.edu All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Chris Provenzano,
 *	the University of California, Berkeley, and contributors.
 * 4. Neither the name of Chris Provenzano, the University, nor the names of
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CHRIS PROVENZANO AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL CHRIS PROVENZANO, THE REGENTS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */ 

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "test.h"

static void *
new_thread(void *new_buf)
{
	int i;

	printf("yielding:");
	for (i = 0; i < 10; i++) {
		printf(" %d", i);
		fflush(stdout);
		pthread_yield();
	}
	printf("\n");
	SUCCEED;
}

int
main(int argc, char *argv[])
{
	pthread_t thread;

	CHECKr(pthread_create(&thread, NULL, new_thread, NULL));

	while(1)
		;
	PANIC("while");
}
