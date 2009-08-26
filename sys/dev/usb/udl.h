/*	$OpenBSD: src/sys/dev/usb/udl.h,v 1.5 2009/08/26 12:23:39 mglocker Exp $ */

/*
 * Copyright (c) 2009 Marcus Glocker <mglocker@openbsd.org>
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

/*
 * Bulk command xfer structure.
 */
#define UDL_CMD_MAX_XFER_SIZE	1048576
#define UDL_CMD_MAX_DATA_SIZE	512
#define UDL_CMD_MAX_PIXEL_COUNT	(UDL_CMD_MAX_DATA_SIZE / 2)
#define UDL_CMD_WRITE_HEAD_SIZE	6
#define UDL_CMD_COPY_HEAD_SIZE	9

struct udl_cmd_xfer {
	struct udl_softc	*sc;
	usbd_xfer_handle	 xfer;
	uint8_t			 busy;
	uint8_t			*buf;
};

struct udl_cmd_buf {
	uint16_t		 compblock;
	uint32_t		 off;
	uint8_t			*buf;
};

/*
 * Our per device structure.
 */
struct udl_softc {
	struct device		 sc_dev;
	usbd_device_handle	 sc_udev;
	usbd_interface_handle	 sc_iface;
	usbd_pipe_handle	 sc_tx_pipeh;

	/* wsdisplay glue */
	struct device		*sc_wsdisplay;
	struct rasops_info	 sc_ri;
	uint8_t			 sc_nscreens;

#define UDL_CMD_XFER_COUNT	 8
	int			 sc_cmd_xfer_cnt;
	struct udl_cmd_xfer	 sc_cmd_xfer[UDL_CMD_XFER_COUNT];
	struct udl_cmd_buf	 sc_cmd_buf;
	uint8_t			*sc_huffman;
	size_t			 sc_huffman_size;
	uint16_t		 sc_width;
	uint16_t		 sc_height;
	uint8_t			 sc_depth;
	uint8_t			 sc_cursor_on;

	/*
	 * We use function pointers to the framebuffer manipulation
	 * functions so we can easily differ between compressed and
	 * none-compressed mode.
	 */
	void			 (*udl_fb_off_write)
				     (struct udl_softc *, uint16_t, uint32_t,
				     uint16_t);
	void			 (*udl_fb_line_write)
				     (struct udl_softc *, uint16_t, uint32_t,
				     uint32_t, uint32_t);
	void			 (*udl_fb_block_write)
				     (struct udl_softc *, uint16_t, uint32_t,
				     uint32_t, uint32_t, uint32_t);
	void			 (*udl_fb_buf_write)
				     (struct udl_softc *, uint8_t *, uint32_t,
				     uint32_t, uint16_t);
	void			 (*udl_fb_off_copy)
				     (struct udl_softc *, uint32_t, uint32_t,
				     uint16_t);
	void			 (*udl_fb_line_copy)
				     (struct udl_softc *, uint32_t, uint32_t,
				     uint32_t, uint32_t, uint32_t);
	void			 (*udl_fb_block_copy)
				     (struct udl_softc *, uint32_t, uint32_t,
				     uint32_t, uint32_t, uint32_t, uint32_t);
};

/*
 * Chip commands.
 */
#define UDL_CTRL_CMD_READ_EDID		0x02
#define UDL_CTRL_CMD_WRITE_1		0x03
#define UDL_CTRL_CMD_READ_1		0x04
#define UDL_CTRL_CMD_POLL		0x06
#define UDL_CTRL_CMD_SET_KEY		0x12

#define UDL_BULK_SOC			0xaf	/* start of command token */

#define UDL_BULK_CMD_REG_WRITE_1	0x20	/* write 1 byte to register */
#define UDL_BULK_CMD_EOC		0xa0	/* end of command stack */
#define UDL_BULK_CMD_DECOMP		0xe0	/* send decompression table */

#define UDL_BULK_CMD_FB_BASE		0x60
#define UDL_BULK_CMD_FB_WORD		0x08
#define UDL_BULK_CMD_FB_COMP		0x10
#define UDL_BULK_CMD_FB_WRITE		(UDL_BULK_CMD_FB_BASE | 0x00)
#define UDL_BULK_CMD_FB_COPY		(UDL_BULK_CMD_FB_BASE | 0x02)

/*
 * Chip registers.
 */
#define UDL_REG_ADDR_START16		0x20
#define UDL_REG_ADDR_STRIDE16		0x23
#define UDL_REG_ADDR_START8		0x26
#define UDL_REG_ADDR_STRIDE8		0x29

#define UDL_REG_SCREEN			0x1f
 #define UDL_REG_SCREEN_ON		0x00
 #define UDL_REG_SCREEN_OFF		0x01
#define UDL_REG_SYNC			0xff

/*
 * Register values for screen resolution initialization.
 */
uint8_t udl_reg_vals_640[] = {
	0x00, 0x99, 0x30, 0x26, 0x94, 0x60, 0xa9, 0xce, 0x60, 0x07, 0xb3, 0x0f,
	0x79, 0xff, 0xff, 0x02, 0x80, 0x83, 0xbc, 0xff, 0xfc, 0xff, 0xff, 0x01,
	0xe0, 0x01, 0x02, 0xab, 0x13
};
uint8_t udl_reg_vals_800[] = {
	0x00, 0x20, 0x3c, 0x7a, 0xc9, 0x93, 0x60, 0xc8, 0xc7, 0x70, 0x53, 0xff,
	0xff, 0x21, 0x27, 0x03, 0x20, 0x91, 0x8f, 0xff, 0xff, 0xff, 0xf2, 0x02,
	0x58, 0x01, 0x02, 0x40, 0x1f
};
uint8_t udl_reg_vals_1024[] = {
	0x00, 0x36, 0x18, 0xd5, 0x10, 0x60, 0xa9, 0x7b, 0x33, 0xa1, 0x2b, 0x27,
	0x32, 0xff, 0xff, 0x04, 0x00, 0xd9, 0x9a, 0xff, 0xca, 0xff, 0xff, 0x03,
	0x00, 0x04, 0x03, 0xc8, 0x32
};
uint8_t udl_reg_vals_1280[] = {
	0x00, 0x98, 0xf8, 0x0d, 0x57, 0x2a, 0x55, 0x4d, 0x54, 0xca, 0x0d, 0xff,
	0xff, 0x94, 0x43, 0x05, 0x00, 0x9a, 0xa8, 0xff, 0xff, 0xff, 0xf9, 0x04,
	0x00, 0x04, 0x02, 0x60, 0x54
};

/*
 * Encryption.
 */
uint8_t udl_null_key_1[] = {
	0x57, 0xcd, 0xdc, 0xa7, 0x1c, 0x88, 0x5e, 0x15, 0x60, 0xfe, 0xc6, 0x97,
	0x16, 0x3d, 0x47, 0xf2
};

/*
 * Compression.
 */
struct udl_huffman {
	uint32_t	bit_count;
	uint32_t	bit_pattern;
};
#define UDL_HUFFMAN_RECORD_SIZE		sizeof(struct udl_huffman)
#define UDL_HUFFMAN_RECORDS		(65536 + 1)
#define UDL_HUFFMAN_BASE		(((UDL_HUFFMAN_RECORDS - 1) / 2) * \
					    UDL_HUFFMAN_RECORD_SIZE)

#define UDL_CB_TOTAL_SIZE		512
#define UDL_CB_TAIL_SIZE		4
#define UDL_CB_BODY_SIZE		(UDL_CB_TOTAL_SIZE - UDL_CB_TAIL_SIZE)
#define UDL_CB_RESTART1_SIZE		(UDL_CB_BODY_SIZE - 4)
#define UDL_CB_RESTART2_SIZE		(UDL_CB_BODY_SIZE - 9)

uint8_t udl_decomp_table[] = {
	0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x01, 0x60, 0x01, 0x00, 0x00, 0x00, 0x61,
	0x00, 0x00, 0x00, 0x01, 0x23, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x01, 0x60, 0x05, 0x00, 0x00, 0x00, 0x61,
	0x00, 0x00, 0x00, 0x01, 0x67, 0x00, 0x01, 0x00, 0x01,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0x00, 0x00, 0x00, 0x01, 0x89, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x61, 0xab, 0x00, 0x00, 0x00, 0x61,
	0x00, 0x00, 0x00, 0x01, 0xcd, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x01, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x01, 0x60, 0x0f, 0x00, 0x00, 0x00, 0x61,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x02,
	0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x02,
	0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x01, 0x01, 0x22, 0x00, 0x00, 0x01, 0x02,
	0x00, 0x00, 0x00, 0x02, 0x33, 0x00, 0x01, 0x00, 0x02,
	0x00, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x02,
	0x00, 0x00, 0x00, 0x02, 0x44, 0x00, 0x02, 0x00, 0x02,
	0x00, 0x00, 0x00, 0x02, 0x44, 0x00, 0x02, 0x00, 0x02,
	0x00, 0x00, 0x01, 0x00, 0x05, 0x00, 0x00, 0x00, 0x02,
	0x00, 0x00, 0x00, 0x02, 0x67, 0x00, 0x00, 0x00, 0x02,
	0x00, 0x00, 0x01, 0x00, 0x05, 0x00, 0x00, 0x00, 0x02,
	0x00, 0x00, 0x00, 0x02, 0x67, 0x00, 0x00, 0x00, 0x02,
	0x00, 0x00, 0x00, 0x02, 0x5b, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x01, 0x01, 0xc8, 0x00, 0x00, 0x01, 0x02,
	0x00, 0x00, 0x00, 0x02, 0x5b, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x02, 0x5b, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x02, 0x9a, 0x00, 0x00, 0x00, 0x02,
	0x00, 0x00, 0x00, 0x02, 0x33, 0x00, 0x01, 0x00, 0x02,
	0x00, 0x00, 0x01, 0x01, 0x2b, 0x00, 0x00, 0x01, 0x02,
	0x00, 0x00, 0x00, 0x02, 0xcc, 0x00, 0x02, 0x00, 0x02,
	0x00, 0x00, 0x00, 0x02, 0xdd, 0x00, 0x04, 0x00, 0x02,
	0x00, 0x00, 0x00, 0x02, 0xef, 0x00, 0x00, 0x00, 0x02,
	0x00, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0x00, 0x03,
	0x00, 0x00, 0x00, 0x03, 0x23, 0x00, 0x00, 0x00, 0x03,
	0x00, 0x00, 0x01, 0x01, 0xc4, 0x00, 0x00, 0x01, 0x03,
	0x00, 0x29, 0x01, 0x00, 0x05, 0x00, 0x00, 0x00, 0x03,
	0x00, 0xb7, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00, 0x03,
	0x00, 0x00, 0x00, 0x03, 0x78, 0x00, 0x00, 0x00, 0x03,
	0x00, 0x00, 0x00, 0x03, 0x99, 0x00, 0x04, 0x00, 0x03,
	0x00, 0x00, 0x00, 0x03, 0xaa, 0x00, 0x08, 0x00, 0x03,
	0x00, 0x00, 0x00, 0x03, 0xbc, 0x00, 0x00, 0x00, 0x03,
	0x00, 0x00, 0x00, 0x03, 0xde, 0x00, 0x00, 0x00, 0x03,
	0x00, 0x00, 0x00, 0x03, 0xf0, 0x00, 0x00, 0x00, 0x04,
	0x00, 0x00, 0x00, 0x04, 0x12, 0x00, 0x00, 0x00, 0x04,
	0x00, 0x00, 0x00, 0x04, 0x34, 0x00, 0x00, 0x00, 0x04,
	0x00, 0x00, 0x00, 0x04, 0x56, 0x00, 0x00, 0x00, 0x04,
	0x00, 0x00, 0x00, 0x04, 0x78, 0x00, 0x00, 0x00, 0x04,
	0x00, 0x00, 0x00, 0x04, 0x9a, 0x00, 0x00, 0x00, 0x04,
	0x00, 0xd7, 0x01, 0x00, 0x00, 0x00, 0x09, 0x01, 0x00,
	0x00, 0x00, 0x01, 0x01, 0x22, 0x00, 0x00, 0x02, 0x01,
	0x00, 0x00, 0x02, 0x04, 0xbb, 0x00, 0x00, 0x03, 0x04,
	0x00, 0x00, 0x00, 0x04, 0xcc, 0x00, 0x08, 0x00, 0x04,
	0x00, 0x00, 0x00, 0x04, 0xdd, 0x00, 0x10, 0x00, 0x04,
	0x00, 0x20, 0x01, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x04,
	0x00, 0x00, 0x00, 0x04, 0xf0, 0x00, 0x00, 0x00, 0x05,
	0xff, 0xe0, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x05,
	0x00, 0x00, 0x00, 0x05, 0x23, 0x00, 0x00, 0x00, 0x05,
	0x08, 0x00, 0x01, 0x00, 0x00, 0x08, 0x20, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x05, 0x45, 0x00, 0x00, 0x00, 0x05,
	0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x21, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x05, 0x67, 0x00, 0x00, 0x00, 0x05,
	0xf8, 0x00, 0x01, 0x00, 0x00, 0xf7, 0xe0, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x05, 0x89, 0x00, 0x00, 0x00, 0x05,
	0xff, 0xff, 0x01, 0x00, 0x00, 0xff, 0xdf, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x05, 0xab, 0x00, 0x00, 0x00, 0x05,
	0x00, 0x00, 0x01, 0x01, 0xcc, 0x00, 0x00, 0x02, 0x01,
	0x00, 0x00, 0x02, 0x05, 0xcc, 0x00, 0x00, 0x03, 0x05,
	0x00, 0x12, 0x01, 0x00, 0x00, 0x00, 0x32, 0x01, 0x00,
	0x00, 0xce, 0x01, 0x00, 0x00, 0x00, 0xae, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x05, 0xde, 0x00, 0x00, 0x00, 0x05,
	0x00, 0x00, 0x00, 0x05, 0xff, 0x00, 0x10, 0x00, 0x05,
	0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x20, 0x00, 0x06,
	0x08, 0x41, 0x01, 0x00, 0x00, 0x08, 0x21, 0x01, 0x00,
	0x08, 0x61, 0x00, 0x06, 0x12, 0x00, 0x00, 0x00, 0x06,
	0x00, 0x00, 0x00, 0x06, 0x34, 0x00, 0x00, 0x00, 0x06,
	0xf7, 0xbf, 0x01, 0x00, 0x00, 0xf7, 0xdf, 0x01, 0x00,
	0xf7, 0x9f, 0x00, 0x06, 0x56, 0x00, 0x00, 0x00, 0x06,
	0x00, 0x00, 0x00, 0x06, 0x78, 0x00, 0x00, 0x00, 0x06,
	0x10, 0x61, 0x00, 0x06, 0x9a, 0x00, 0x00, 0x00, 0x06,
	0x00, 0x41, 0x00, 0x06, 0x9b, 0x00, 0x00, 0x00, 0x06,
	0x08, 0x62, 0x00, 0x06, 0x9c, 0x00, 0x00, 0x00, 0x06,
	0x08, 0x40, 0x00, 0x06, 0x9d, 0x00, 0x00, 0x00, 0x06,
	0xef, 0x9f, 0x00, 0x06, 0xef, 0x00, 0x00, 0x00, 0x06,
	0xff, 0xbf, 0x00, 0x06, 0xe0, 0x00, 0x00, 0x00, 0x07,
	0xf7, 0x9e, 0x00, 0x06, 0xe1, 0x00, 0x00, 0x00, 0x07,
	0xf7, 0xc0, 0x00, 0x06, 0xe2, 0x00, 0x00, 0x00, 0x07,
	0x00, 0x00, 0x00, 0x07, 0x34, 0x00, 0x00, 0x00, 0x07,
	0x00, 0x00, 0x01, 0x01, 0x22, 0x00, 0x00, 0x03, 0x01,
	0x00, 0x00, 0x04, 0x07, 0x55, 0x00, 0x00, 0x06, 0x07,
	0x00, 0x00, 0x00, 0x07, 0x66, 0x00, 0x20, 0x00, 0x07,
	0x00, 0x00, 0x00, 0x07, 0x77, 0x00, 0x40, 0x00, 0x07,
	0x00, 0x00, 0x00, 0x07, 0x88, 0x00, 0x20, 0x00, 0x07,
	0x08, 0x01, 0x01, 0x00, 0x09, 0x10, 0x02, 0x00, 0x07,
	0xef, 0xfe, 0x00, 0x07, 0xab, 0x00, 0x00, 0x00, 0x07,
	0x00, 0x00, 0x00, 0x07, 0xcf, 0xff, 0xff, 0xff, 0x7f,
	0x00, 0x00, 0x00, 0x07, 0xdd, 0xff, 0xe0, 0x00, 0x07,
	0xf7, 0xff, 0x01, 0x00, 0x0e, 0xef, 0xfe, 0x00, 0x07,
	0x10, 0x02, 0x00, 0x07, 0xf0, 0x00, 0x00, 0x00, 0x08,
	0x00, 0x00, 0x00, 0x08, 0x1f, 0xff, 0xff, 0xff, 0x7f,
	0x00, 0x00, 0x01, 0x00, 0x02, 0x08, 0x41, 0x00, 0x08,
	0x10, 0x41, 0x00, 0x06, 0x93, 0x00, 0x00, 0x00, 0x08,
	0x00, 0x00, 0x00, 0x08, 0x45, 0x00, 0x00, 0x00, 0x08,
	0x08, 0x42, 0x00, 0x06, 0x96, 0x00, 0x00, 0x00, 0x08,
	0x00, 0x00, 0x00, 0x08, 0x78, 0x00, 0x00, 0x00, 0x08,
	0x00, 0x00, 0x01, 0x00, 0x09, 0xf7, 0xbf, 0x00, 0x08,
	0xef, 0xbf, 0x00, 0x06, 0xea, 0x00, 0x00, 0x00, 0x08,
	0x00, 0x00, 0x00, 0x08, 0xbc, 0x00, 0x00, 0x00, 0x08,
	0xf7, 0xbe, 0x00, 0x06, 0xed, 0x00, 0x00, 0x00, 0x08,
	0x00, 0x00, 0x00, 0x08, 0xef, 0x00, 0x00, 0x00, 0x08,
	0x00, 0x00, 0x01, 0x01, 0xcc, 0x00, 0x00, 0x03, 0x01,
	0x00, 0x00, 0x04, 0x09, 0x00, 0x00, 0x00, 0x06, 0x09,
	0x00, 0x00, 0x00, 0x09, 0x12, 0x00, 0x00, 0x00, 0x09,
	0x00, 0x00, 0x00, 0x09, 0x33, 0x00, 0x40, 0x00, 0x09,
	0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x80, 0x04, 0x00,
	0x00, 0x00, 0x01, 0x00, 0x04, 0x08, 0x41, 0x00, 0x09,
	0x00, 0x00, 0x00, 0x09, 0x55, 0x08, 0x01, 0x00, 0x09,
	0x00, 0x00, 0x00, 0x09, 0x66, 0xf7, 0xff, 0x00, 0x09,
	0x10, 0x42, 0x00, 0x06, 0x99, 0x10, 0x62, 0x00, 0x06,
	0xf8, 0x1f, 0x00, 0x06, 0x99, 0x00, 0x40, 0x00, 0x06,
	0x00, 0x00, 0x01, 0x00, 0x07, 0xf7, 0xbf, 0x00, 0x09,
	0x00, 0x00, 0x00, 0x09, 0x88, 0xf7, 0xff, 0x00, 0x09,
	0x00, 0x00, 0x00, 0x09, 0x99, 0x08, 0x01, 0x00, 0x09,
	0xef, 0xbe, 0x00, 0x06, 0xee, 0xef, 0x9e, 0x00, 0x06,
	0x07, 0xe1, 0x00, 0x06, 0xee, 0xff, 0xc0, 0x00, 0x06,
	0x00, 0x00, 0x01, 0x00, 0x0a, 0x08, 0x41, 0x00, 0x09,
	0xf8, 0x20, 0x00, 0x06, 0x99, 0x07, 0xff, 0x00, 0x06,
	0x08, 0x1f, 0x00, 0x06, 0x9b, 0x00, 0x00, 0x00, 0x09,
	0x00, 0x00, 0x00, 0x09, 0xcd, 0x00, 0x00, 0x00, 0x09,
	0x00, 0x1f, 0x00, 0x06, 0x99, 0xf8, 0x01, 0x00, 0x06,
	0xf8, 0x21, 0x00, 0x06, 0x9e, 0x00, 0x00, 0x00, 0x09,
	0x00, 0x00, 0x00, 0x09, 0xf0, 0x00, 0x00, 0x00, 0x0a,
	0x00, 0x00, 0x01, 0x00, 0x01, 0xf7, 0xbf, 0x00, 0x0a,
	0x07, 0xe0, 0x00, 0x06, 0xee, 0xf8, 0x01, 0x00, 0x06,
	0xf7, 0xe1, 0x00, 0x06, 0xe2, 0x00, 0x00, 0x00, 0x0a,
	0x00, 0x00, 0x00, 0x0a, 0x34, 0x00, 0x00, 0x00, 0x0a,
	0xff, 0xe1, 0x00, 0x06, 0xee, 0x07, 0xff, 0x00, 0x06,
	0x07, 0xdf, 0x00, 0x06, 0xe5, 0x00, 0x00, 0x00, 0x0a,
	0x00, 0x00, 0x00, 0x0a, 0x67, 0x00, 0x00, 0x00, 0x0a,
	0x00, 0x00, 0x00, 0x0a, 0x89, 0x00, 0x00, 0x00, 0x0a,
	0x00, 0x00, 0x01, 0x01, 0x22, 0x00, 0x00, 0x05, 0x01,
	0x00, 0x00, 0x08, 0x0a, 0xaa, 0x00, 0x00, 0x0c, 0x0a,
	0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x80, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x0a, 0xbb, 0x08, 0x41, 0x00, 0x0a,
	0x00, 0x00, 0x00, 0x0a, 0xcd, 0x10, 0x02, 0x00, 0x0a,
	0x00, 0x00, 0x00, 0x0a, 0xce, 0xef, 0xfe, 0x00, 0x0a,
	0x00, 0x00, 0x00, 0x0a, 0xff, 0xf7, 0xbf, 0x00, 0x0a,
	0x00, 0x00, 0x00, 0x0b, 0x01, 0xef, 0xfe, 0x00, 0x0b,
	0x00, 0x00, 0x00, 0x0b, 0x02, 0x10, 0x02, 0x00, 0x0b,
	0x00, 0x00, 0x01, 0x00, 0x03, 0x08, 0x41, 0x00, 0x0b,
	0x10, 0x21, 0x00, 0x06, 0x94, 0x00, 0x00, 0x00, 0x0b,
	0xef, 0xff, 0x00, 0x06, 0x95, 0x00, 0x00, 0x00, 0x0b,
	0x00, 0x00, 0x00, 0x0b, 0x66, 0x00, 0x20, 0x00, 0x0b,
	0x08, 0x22, 0x00, 0x06, 0x97, 0x00, 0x00, 0x00, 0x0b,
	0xf7, 0xfe, 0x00, 0x06, 0x98, 0x00, 0x00, 0x00, 0x0b,
	0x00, 0x00, 0x00, 0x0b, 0x99, 0x00, 0x20, 0x00, 0x0b,
	0x00, 0x00, 0x01, 0x00, 0x0a, 0xf7, 0xbf, 0x00, 0x0b,
	0xef, 0xdf, 0x00, 0x06, 0xeb, 0x00, 0x00, 0x00, 0x0b,
	0x10, 0x01, 0x00, 0x06, 0xec, 0x00, 0x00, 0x00, 0x0b,
	0x00, 0x00, 0x00, 0x0b, 0xdd, 0xff, 0xe0, 0x00, 0x0b,
	0xf7, 0xde, 0x00, 0x06, 0xee, 0x00, 0x00, 0x00, 0x0b,
	0x08, 0x02, 0x00, 0x06, 0xef, 0x00, 0x00, 0x00, 0x0b,
	0x00, 0x00, 0x00, 0x0c, 0x00, 0xff, 0xe0, 0x00, 0x0c,
	0x00, 0x00, 0x01, 0x01, 0xcc, 0x00, 0x00, 0x05, 0x01,
	0x00, 0x00, 0x08, 0x0c, 0x11, 0x00, 0x00, 0x0c, 0x0c,
	0x00, 0x00, 0x00, 0x0c, 0x23, 0x00, 0x00, 0x00, 0x0c,
	0x00, 0x00, 0x01, 0x00, 0x04, 0x10, 0x82, 0x00, 0x0c,
	0x00, 0x00, 0x00, 0x0c, 0x56, 0x00, 0x00, 0x00, 0x0c,
	0x00, 0x00, 0x00, 0x0c, 0x77, 0x10, 0x02, 0x00, 0x0c,
	0x00, 0x00, 0x00, 0x0c, 0x88, 0xef, 0xfe, 0x00, 0x0c,
	0x00, 0x00, 0x01, 0x00, 0x09, 0xef, 0x7e, 0x00, 0x0c,
	0x00, 0x00, 0x00, 0x0c, 0xab, 0x00, 0x00, 0x00, 0x0c,
	0x00, 0x00, 0x00, 0x0c, 0xcc, 0xef, 0xfe, 0x00, 0x0c,
	0x00, 0x00, 0x00, 0x0c, 0xdd, 0x10, 0x02, 0x00, 0x0c,
	0x00, 0x00, 0x01, 0x00, 0x0e, 0x08, 0x41, 0x00, 0x0c,
	0x10, 0x01, 0x00, 0x06, 0x99, 0xf0, 0x1f, 0x00, 0x06,
	0x10, 0x00, 0x00, 0x0a, 0xcc, 0xf0, 0x00, 0x00, 0x0a,
	0x00, 0x00, 0x00, 0x0c, 0xf0, 0x08, 0x41, 0x00, 0x0d,
	0x08, 0x02, 0x00, 0x06, 0x99, 0xf8, 0x1e, 0x00, 0x06,
	0x00, 0x02, 0x00, 0x0a, 0xcc, 0xff, 0xfe, 0x00, 0x0a,
	0x00, 0x00, 0x00, 0x0d, 0x12, 0x08, 0x41, 0x00, 0x0d,
	0x00, 0x00, 0x01, 0x00, 0x03, 0xf7, 0xbf, 0x00, 0x0d,
	0xef, 0xff, 0x00, 0x06, 0xee, 0x0f, 0xe1, 0x00, 0x06,
	0xf0, 0x00, 0x00, 0x0b, 0x00, 0x10, 0x00, 0x00, 0x0b,
	0x00, 0x00, 0x00, 0x0d, 0x45, 0xf7, 0xbf, 0x00, 0x0d,
	0xf7, 0xfe, 0x00, 0x06, 0xee, 0x07, 0xe2, 0x00, 0x06,
	0xff, 0xfe, 0x00, 0x0b, 0x00, 0x00, 0x02, 0x00, 0x0b,
	0x00, 0x00, 0x00, 0x0d, 0x67, 0xf7, 0xbf, 0x00, 0x0d,
	0x00, 0x00, 0x00, 0x0d, 0x89, 0x00, 0x00, 0x00, 0x0d,
	0x00, 0x00, 0x01, 0x01, 0x22, 0x00, 0x00, 0x09, 0x01,
	0x00, 0x00, 0x10, 0x0d, 0xaa, 0x00, 0x00, 0x18, 0x0d,
	0x00, 0x00, 0x00, 0x0d, 0xbb, 0x10, 0x82, 0x00, 0x0d,
	0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x20, 0x01, 0x00,
	0x08, 0x41, 0x00, 0x09, 0x44, 0x08, 0x61, 0x00, 0x09,
	0x00, 0x00, 0x00, 0x0a, 0xcc, 0x20, 0x04, 0x00, 0x0d,
	0x00, 0x00, 0x00, 0x0a, 0xcd, 0xdf, 0xfc, 0x00, 0x0d,
	0x00, 0x00, 0x00, 0x0d, 0xee, 0xef, 0x7e, 0x00, 0x0d,
	0x00, 0x00, 0x01, 0x00, 0x00, 0xff, 0xe0, 0x01, 0x00,
	0xf7, 0xbf, 0x00, 0x09, 0x77, 0xf7, 0x9f, 0x00, 0x09,
	0x00, 0x00, 0x00, 0x0b, 0x0f, 0xdf, 0xfc, 0x00, 0x0d,
	0x00, 0x00, 0x00, 0x0b, 0x00, 0x20, 0x04, 0x00, 0x0e,
	0x00, 0x00, 0x01, 0x00, 0x01, 0x08, 0x41, 0x00, 0x0e,
	0x00, 0x00, 0x00, 0x0e, 0x23, 0x00, 0x00, 0x00, 0x0e,
	0x00, 0x00, 0x00, 0x0e, 0x44, 0x08, 0x41, 0x00, 0x0e,
	0x00, 0x00, 0x00, 0x0e, 0x56, 0x00, 0x00, 0x00, 0x0e,
	0x00, 0x00, 0x00, 0x0e, 0x77, 0x08, 0x41, 0x00, 0x0e,
	0x00, 0x00, 0x01, 0x00, 0x08, 0xf7, 0xbf, 0x00, 0x0e,
	0x00, 0x00, 0x00, 0x0e, 0x9a, 0x00, 0x00, 0x00, 0x0e,
	0x00, 0x00, 0x00, 0x0e, 0xbb, 0xf7, 0xbf, 0x00, 0x0e,
	0x00, 0x00, 0x00, 0x0e, 0xcd, 0x00, 0x00, 0x00, 0x0e,
	0x00, 0x00, 0x00, 0x0e, 0xee, 0xf7, 0xbf, 0x00, 0x0e,
	0x00, 0x00, 0x01, 0x01, 0xcc, 0x00, 0x00, 0x09, 0x01,
	0x00, 0x00, 0x10, 0x0e, 0xff, 0x00, 0x00, 0x18, 0x0e,
	0x00, 0x00, 0x00, 0x0f, 0x01, 0x00, 0x00, 0x00, 0x0f,
	0x00, 0x00, 0x01, 0x00, 0x02, 0x21, 0x04, 0x00, 0x0f,
	0x00, 0x00, 0x00, 0x0a, 0xc3, 0x00, 0x00, 0x00, 0x0f,
	0x00, 0x00, 0x00, 0x0a, 0xc4, 0x00, 0x00, 0x00, 0x0f,
	0x00, 0x00, 0x01, 0x00, 0x05, 0xde, 0xfc, 0x00, 0x0f,
	0x00, 0x00, 0x00, 0x0b, 0x06, 0x00, 0x00, 0x00, 0x0f,
	0x00, 0x00, 0x00, 0x0b, 0x07, 0x00, 0x00, 0x00, 0x0f,
	0x00, 0x00, 0x01, 0x00, 0x08, 0x08, 0x41, 0x00, 0x0f,
	0x08, 0x00, 0x00, 0x0f, 0x9a, 0x0f, 0xff, 0x00, 0x0f,
	0xf8, 0x00, 0x00, 0x0f, 0xbc, 0xf0, 0x01, 0x00, 0x0f,
	0x00, 0x00, 0x00, 0x0c, 0xfd, 0x10, 0x82, 0x00, 0x0f,
	0x00, 0x01, 0x00, 0x0f, 0xec, 0xf8, 0x02, 0x00, 0x0f,
	0xff, 0xff, 0x00, 0x0f, 0xfa, 0x07, 0xfe, 0x00, 0x0f,
	0x00, 0x00, 0x00, 0x0d, 0x10, 0x10, 0x82, 0x00, 0x10,
	0x00, 0x00, 0x01, 0x00, 0x01, 0xf7, 0xbf, 0x00, 0x10,
	0xf8, 0x00, 0x00, 0x0f, 0xb2, 0xf0, 0x01, 0x00, 0x10,
	0x08, 0x00, 0x00, 0x0f, 0x93, 0x0f, 0xff, 0x00, 0x10,
	0x00, 0x00, 0x00, 0x0d, 0x44, 0xef, 0x7e, 0x00, 0x10,
	0xff, 0xff, 0x00, 0x0f, 0xf3, 0x07, 0xfe, 0x00, 0x10,
	0x00, 0x01, 0x00, 0x0f, 0xe2, 0xf8, 0x02, 0x00, 0x10,
	0x00, 0x00, 0x00, 0x0d, 0x65, 0xef, 0x7e, 0x00, 0x10,
	0x00, 0x00, 0x00, 0x10, 0x67, 0x00, 0x00, 0x00, 0x10,
	0x00, 0x00, 0x01, 0x01, 0x22, 0x00, 0x00, 0x11, 0x01,
	0x00, 0x00, 0x20, 0x10, 0x88, 0x00, 0x00, 0x30, 0x10,
	0x00, 0x00, 0x01, 0x00, 0x09, 0x00, 0x00, 0x00, 0x10,
	0x20, 0x04, 0x00, 0x0a, 0xcc, 0x40, 0x08, 0x00, 0x0a,
	0xdf, 0xfc, 0x00, 0x0a, 0xcc, 0xbf, 0xf8, 0x00, 0x0a,
	0x00, 0x00, 0x01, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x10,
	0xdf, 0xfc, 0x00, 0x0b, 0x00, 0xbf, 0xf8, 0x00, 0x0b,
	0x20, 0x04, 0x00, 0x0b, 0x00, 0x40, 0x08, 0x00, 0x0b,
	0x00, 0x00, 0x01, 0x00, 0x0b, 0x08, 0x41, 0x00, 0x10,
	0x00, 0x00, 0x00, 0x10, 0xcc, 0x08, 0x01, 0x00, 0x10,
	0x00, 0x00, 0x00, 0x10, 0xdd, 0x08, 0x00, 0x00, 0x10,
	0x00, 0x00, 0x00, 0x10, 0xee, 0xf7, 0xff, 0x00, 0x10,
	0x00, 0x00, 0x00, 0x10, 0xff, 0x00, 0x01, 0x00, 0x10,
	0x00, 0x00, 0x00, 0x11, 0x00, 0x10, 0x82, 0x00, 0x11,
	0x00, 0x00, 0x00, 0x11, 0x11, 0x08, 0x01, 0x00, 0x11,
	0x00, 0x00, 0x00, 0x11, 0x22, 0xf7, 0xff, 0x00, 0x11,
	0x00, 0x00, 0x00, 0x11, 0x33, 0x10, 0x82, 0x00, 0x11,
	0x00, 0x00, 0x01, 0x00, 0x04, 0xf7, 0xbf, 0x00, 0x11,
	0x00, 0x00, 0x00, 0x11, 0x55, 0xf8, 0x00, 0x00, 0x11,
	0x00, 0x00, 0x00, 0x11, 0x66, 0xff, 0xff, 0x00, 0x11,
	0x00, 0x00, 0x00, 0x11, 0x77, 0xef, 0x7e, 0x00, 0x11,
	0x00, 0x00, 0x00, 0x11, 0x88, 0xef, 0x7e, 0x00, 0x11,
	0x00, 0x00, 0x01, 0x01, 0xcc, 0x00, 0x00, 0x11, 0x01,
	0x00, 0x00, 0x20, 0x11, 0x99, 0x00, 0x00, 0x30, 0x11,
	0x00, 0x00, 0x00, 0x11, 0xab, 0x00, 0x00, 0x00, 0x11,
	0x21, 0x04, 0x01, 0x00, 0x00, 0x42, 0x08, 0x01, 0x00,
	0xde, 0xfc, 0x01, 0x00, 0x00, 0xbd, 0xf8, 0x01, 0x00,
	0x00, 0x00, 0x01, 0x00, 0x0c, 0x08, 0x41, 0x00, 0x11,
	0x00, 0x00, 0x00, 0x11, 0xde, 0x10, 0x02, 0x00, 0x11,
	0x00, 0x00, 0x00, 0x11, 0xf0, 0x10, 0x00, 0x00, 0x12,
	0x00, 0x00, 0x00, 0x12, 0x12, 0xef, 0xfe, 0x00, 0x12,
	0x00, 0x00, 0x00, 0x12, 0x13, 0x00, 0x02, 0x00, 0x12,
	0x00, 0x00, 0x00, 0x0c, 0xf4, 0x21, 0x04, 0x00, 0x12,
	0x00, 0x00, 0x00, 0x12, 0x56, 0x10, 0x02, 0x00, 0x12,
	0x00, 0x00, 0x00, 0x11, 0xf7, 0xef, 0xfe, 0x00, 0x12,
	0x00, 0x00, 0x00, 0x0d, 0x18, 0x21, 0x04, 0x00, 0x12,
	0x00, 0x00, 0x01, 0x00, 0x09, 0xf7, 0xbf, 0x00, 0x12,
	0x00, 0x00, 0x00, 0x12, 0x5a, 0xf0, 0x00, 0x00, 0x12,
	0x00, 0x00, 0x00, 0x11, 0xdb, 0xff, 0xfe, 0x00, 0x12,
	0x00, 0x00, 0x00, 0x0d, 0x4c, 0xde, 0xfc, 0x00, 0x12,
	0x00, 0x00, 0x00, 0x0d, 0x6d, 0xde, 0xfc, 0x00, 0x12,
	0x00, 0x00, 0x00, 0x12, 0xef, 0x00, 0x00, 0x00, 0x12,
	0x00, 0x00, 0x01, 0x01, 0x22, 0x00, 0x00, 0x21, 0x01,
	0x00, 0x00, 0x40, 0x13, 0x00, 0x00, 0x00, 0x60, 0x13,
	0x00, 0x00, 0x01, 0x00, 0x01, 0x08, 0x41, 0x00, 0x13,
	0x00, 0x00, 0x00, 0x13, 0x22, 0x08, 0x00, 0x00, 0x13,
	0x00, 0x00, 0x00, 0x13, 0x33, 0x10, 0x02, 0x00, 0x13,
	0x00, 0x00, 0x00, 0x13, 0x44, 0xff, 0xff, 0x00, 0x13,
	0x00, 0x00, 0x00, 0x13, 0x55, 0x10, 0x00, 0x00, 0x13,
	0x00, 0x00, 0x00, 0x13, 0x66, 0xf8, 0x00, 0x00, 0x13,
	0x00, 0x00, 0x00, 0x13, 0x77, 0xef, 0xfe, 0x00, 0x13,
	0x00, 0x00, 0x00, 0x13, 0x88, 0x00, 0x02, 0x00, 0x13,
	0x00, 0x00, 0x00, 0x13, 0x99, 0x21, 0x04, 0x00, 0x13,
	0x00, 0x00, 0x00, 0x13, 0xaa, 0x00, 0x01, 0x00, 0x13,
	0x00, 0x00, 0x00, 0x13, 0xbb, 0x10, 0x02, 0x00, 0x13,
	0x00, 0x00, 0x00, 0x13, 0xcc, 0xef, 0xfe, 0x00, 0x13,
	0x00, 0x00, 0x00, 0x13, 0xdd, 0x21, 0x04, 0x00, 0x13,
	0x00, 0x00, 0x01, 0x00, 0x0e, 0xf7, 0xbf, 0x00, 0x13,
	0x00, 0x00, 0x00, 0x13, 0xff, 0xf0, 0x00, 0x00, 0x13,
	0x00, 0x00, 0x00, 0x14, 0x00, 0xff, 0xfe, 0x00, 0x14,
	0x00, 0x00, 0x00, 0x14, 0x11, 0xde, 0xfc, 0x00, 0x14,
	0x00, 0x00, 0x00, 0x14, 0x22, 0xde, 0xfc, 0x00, 0x14,
	0x00, 0x00, 0x01, 0x01, 0xcc, 0x00, 0x00, 0x21, 0x01,
	0x00, 0x00, 0x40, 0x14, 0x33, 0x00, 0x00, 0x60, 0x14,
	0x00, 0x00, 0x01, 0x01, 0x22, 0x00, 0x00, 0x41, 0x01,
	0x00, 0x00, 0x01, 0x00, 0x04, 0x08, 0x41, 0x00, 0x14,
	0x00, 0x00, 0x01, 0x00, 0x05, 0x10, 0x00, 0x00, 0x14,
	0x00, 0x00, 0x00, 0x11, 0xd6, 0x20, 0x04, 0x00, 0x14,
	0x00, 0x00, 0x01, 0x00, 0x07, 0xff, 0xfe, 0x00, 0x14,
	0x00, 0x00, 0x00, 0x11, 0xf8, 0x20, 0x00, 0x00, 0x14,
	0x00, 0x00, 0x01, 0x00, 0x09, 0xf0, 0x00, 0x00, 0x14,
	0x00, 0x00, 0x00, 0x12, 0x1a, 0xdf, 0xfc, 0x00, 0x14,
	0x00, 0x00, 0x00, 0x12, 0x1b, 0x00, 0x04, 0x00, 0x14,
	0x00, 0x00, 0x00, 0x0c, 0xfc, 0x42, 0x08, 0x00, 0x14,
	0x00, 0x00, 0x01, 0x00, 0x0d, 0x00, 0x02, 0x00, 0x14,
	0x00, 0x00, 0x00, 0x12, 0x5e, 0x20, 0x04, 0x00, 0x14,
	0x00, 0x00, 0x00, 0x11, 0xff, 0xdf, 0xfc, 0x00, 0x14,
	0x00, 0x00, 0x00, 0x0d, 0x10, 0x42, 0x08, 0x00, 0x15,
	0x00, 0x00, 0x01, 0x00, 0x01, 0xf7, 0xbf, 0x00, 0x15,
	0x00, 0x00, 0x00, 0x12, 0x52, 0xe0, 0x00, 0x00, 0x15,
	0x00, 0x00, 0x00, 0x11, 0xd3, 0xff, 0xfc, 0x00, 0x15,
	0x00, 0x00, 0x00, 0x0d, 0x44, 0xbd, 0xf8, 0x00, 0x15,
	0x00, 0x00, 0x00, 0x0d, 0x65, 0xbd, 0xf8, 0x00, 0x15,
	0x00, 0x00, 0x01, 0x01, 0xcc, 0x00, 0x00, 0x41, 0x01,
	0x00, 0x00, 0x01, 0x00, 0x06, 0x08, 0x41, 0x00, 0x15,
	0x00, 0x00, 0x00, 0x15, 0x77, 0x10, 0x00, 0x00, 0x15,
	0x00, 0x00, 0x00, 0x11, 0xd8, 0x00, 0x00, 0x00, 0x15,
	0x00, 0x00, 0x00, 0x15, 0x99, 0xff, 0xfe, 0x00, 0x15,
	0x00, 0x00, 0x00, 0x11, 0xfa, 0x00, 0x00, 0x00, 0x15,
	0x00, 0x00, 0x00, 0x15, 0xbb, 0xf0, 0x00, 0x00, 0x15,
	0x00, 0x00, 0x00, 0x12, 0x1c, 0x00, 0x00, 0x00, 0x15,
	0x00, 0x00, 0x00, 0x12, 0x1d, 0x00, 0x00, 0x00, 0x15,
	0x00, 0x00, 0x00, 0x0c, 0xfe, 0x00, 0x00, 0x00, 0x15,
	0x00, 0x00, 0x00, 0x15, 0xff, 0x00, 0x02, 0x00, 0x15,
	0x00, 0x00, 0x00, 0x12, 0x50, 0x00, 0x00, 0x00, 0x16,
	0x00, 0x00, 0x00, 0x11, 0xf1, 0x00, 0x00, 0x00, 0x16,
	0x00, 0x00, 0x00, 0x0d, 0x12, 0x00, 0x00, 0x00, 0x16,
	0x00, 0x00, 0x01, 0x00, 0x03, 0xf7, 0xbf, 0x00, 0x16,
	0x00, 0x00, 0x00, 0x12, 0x54, 0x00, 0x00, 0x00, 0x16,
	0x00, 0x00, 0x00, 0x11, 0xd5, 0x00, 0x00, 0x00, 0x16,
	0x00, 0x00, 0x00, 0x0d, 0x46, 0x00, 0x00, 0x00, 0x16,
	0x00, 0x00, 0x00, 0x0d, 0x67, 0x00, 0x00, 0x00, 0x16,
	0x00, 0x00, 0x01, 0x00, 0x08, 0x08, 0x41, 0x00, 0x16,
	0x00, 0x00, 0x01, 0x00, 0x09, 0x20, 0x00, 0x00, 0x16,
	0x20, 0x04, 0x00, 0x11, 0xdd, 0x40, 0x08, 0x00, 0x11,
	0x00, 0x00, 0x01, 0x00, 0x0a, 0xff, 0xfc, 0x00, 0x16,
	0x20, 0x00, 0x00, 0x11, 0xff, 0x40, 0x00, 0x00, 0x11,
	0x00, 0x00, 0x01, 0x00, 0x0b, 0xe0, 0x00, 0x00, 0x16,
	0xdf, 0xfc, 0x00, 0x12, 0x11, 0xbf, 0xf8, 0x00, 0x12,
	0x00, 0x04, 0x00, 0x12, 0x11, 0x00, 0x08, 0x00, 0x12,
	0x42, 0x08, 0x00, 0x0c, 0xff, 0x83, 0xf0, 0x00, 0x0c,
	0x00, 0x00, 0x01, 0x00, 0x0c, 0x00, 0x04, 0x00, 0x16,
	0x20, 0x04, 0x00, 0x12, 0x55, 0x40, 0x08, 0x00, 0x12,
	0xdf, 0xfc, 0x00, 0x11, 0xff, 0xbf, 0xf8, 0x00, 0x11,
	0x42, 0x08, 0x00, 0x0d, 0x11, 0x83, 0xf1, 0x00, 0x0d,
	0x00, 0x00, 0x01, 0x00, 0x0d, 0xf7, 0xbf, 0x00, 0x16,
	0xe0, 0x00, 0x00, 0x12, 0x55, 0xc0, 0x00, 0x00, 0x12,
	0xff, 0xfc, 0x00, 0x11, 0xdd, 0xff, 0xf8, 0x00, 0x11,
	0xbd, 0xf8, 0x00, 0x0d, 0x44, 0x7b, 0xf0, 0x00, 0x0d,
	0xbd, 0xf8, 0x00, 0x0d, 0x66, 0x74, 0x0f, 0x00, 0x0d,
	0x00, 0x00, 0x01, 0x00, 0x0e, 0x08, 0x41, 0x00, 0x16,
	0x00, 0x00, 0x01, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x16,
	0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17,
	0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x17,
	0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x17,
	0x00, 0x00, 0x01, 0x00, 0x03, 0xf7, 0xbf, 0x00, 0x17,
	0x00, 0x00, 0x01, 0x00, 0x04, 0x08, 0x41, 0x00, 0x17,
	0x20, 0x00, 0x01, 0x00, 0x00, 0x40, 0x00, 0x01, 0x00,
	0xff, 0xfc, 0x01, 0x00, 0x00, 0xff, 0xf8, 0x01, 0x00,
	0xe0, 0x00, 0x01, 0x00, 0x00, 0xc0, 0x00, 0x01, 0x00,
	0x00, 0x04, 0x01, 0x00, 0x00, 0x00, 0x08, 0x01, 0x00,
	0x00, 0x00, 0x01, 0x00, 0x05, 0xf7, 0xbf, 0x00, 0x17,
	0x00, 0x00, 0x01, 0x00, 0x06, 0x08, 0x41, 0x00, 0x17,
	0x00, 0x00, 0x01, 0x00, 0x07, 0xf7, 0xbf, 0x00, 0x17,
	0x00, 0x00, 0x01, 0x00, 0x00, 0x08, 0x41, 0x01, 0x00,
	0x00, 0x00, 0x01, 0x00, 0x00, 0xf7, 0xbf, 0x01, 0x00,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f,
	0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f
};
