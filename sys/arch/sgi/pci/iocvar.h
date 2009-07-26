/*	$OpenBSD: src/sys/arch/sgi/pci/iocvar.h,v 1.3 2009/07/26 19:58:51 miod Exp $	*/

/*
 * Copyright (c) 2008 Miodrag Vallat.
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

struct ioc_attach_args {
	const char		*iaa_name;

	bus_space_tag_t		 iaa_memt;
	bus_space_handle_t	 iaa_memh;
	bus_dma_tag_t		 iaa_dmat;

	bus_addr_t		 iaa_base;
	int			 iaa_dev;

	uint8_t			 iaa_enaddr[6];
};

void   *ioc_intr_establish(void *, u_long, int, int (*)(void *),
	    void *, char *);
