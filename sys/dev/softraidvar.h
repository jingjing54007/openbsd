/* $OpenBSD: src/sys/dev/softraidvar.h,v 1.60 2008/06/25 17:43:09 thib Exp $ */
/*
 * Copyright (c) 2006 Marco Peereboom <marco@peereboom.us>
 * Copyright (c) 2008 Chris Kuethe <ckuethe@openbsd.org>
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

#ifndef SOFTRAIDVAR_H
#define SOFTRAIDVAR_H

#define SR_UUID_MAX		4
struct sr_uuid {
	u_int32_t		sui_id[SR_UUID_MAX];
} __packed;

#define SR_META_SIZE		32	/* save space at chunk beginning */
#define SR_META_OFFSET		16	/* skip 8192 bytes at chunk beginning */
#define SR_META_VERSION		1	/* bump when sr_metadata changes */
struct sr_metadata {
	/* do not change order of ssd_magic, ssd_version & ssd_checksum */
	u_int64_t		ssd_magic;	/* magic id */
#define	SR_MAGIC		0x4d4152436372616dLLU
	u_int8_t		ssd_version;	/* meta data version */
	u_int8_t		ssd_pad1[3];
	u_int32_t		ssd_flags;	/* flags */

	/* meta-data */
	u_int32_t		ssd_checksum;	/* xor of the structure */
	u_int32_t		ssd_size;	/* sizeof(sr_metadata) */
	u_int32_t		ssd_ondisk;	/* on disk version counter */
	u_int32_t		ssd_pad2;
	struct sr_uuid		ssd_uuid;	/* unique identifier */

	/* virtual disk data */
	u_int32_t		ssd_vd_ver;	/* vd structure version */
	u_int32_t		ssd_vd_size;	/* vd structure size */
	u_int32_t		ssd_vd_volid;	/* volume id */
	u_int32_t		ssd_vd_chk;	/* vd structure xor */

	/* chunk data */
	u_int32_t		ssd_chunk_ver;	/* chunk structure version */
	u_int32_t		ssd_chunk_no;	/* number of chunks */
	u_int32_t		ssd_chunk_size;	/* chunk structure size */
	u_int32_t		ssd_chunk_id;	/* chunk identifier */
	u_int32_t		ssd_chunk_chk;	/* chunk structure xor */
	u_int32_t		ssd_pad3;

	/* optional metadata */
	u_int32_t		ssd_opt_ver;	/* optinal meta version */
	u_int32_t		ssd_opt_no;	/* nr of optional md elements */
	u_int32_t		ssd_opt_size;	/* sizeof optional metadata */
	u_int32_t		ssd_opt_chk;	/* optional metadata xor */
} __packed;

#define SR_VOL_VERSION	2	/* bump when sr_vol_meta changes */
struct sr_vol_meta {
	u_int32_t		svm_volid;	/* volume id */
	u_int32_t		svm_status; 	/* use bioc_vol status */
	u_int32_t		svm_flags;	/* flags */
#define	SR_VOL_DIRTY		0x01
	u_int32_t		svm_level;	/* raid level */
	int64_t			svm_size;	/* virt disk size in blocks */
	char			svm_devname[32];/* /dev/XXXXX */
	char			svm_vendor[8];	/* scsi vendor */
	char			svm_product[16];/* scsi product */
	char			svm_revision[4];/* scsi revision */
	u_int32_t		svm_no_chunk;	/* number of chunks */
	struct sr_uuid 		svm_uuid;	/* volume unique identifier */

	/* optional members */
	u_int32_t		svm_strip_size;	/* strip size */
} __packed;

#define SR_CHUNK_VERSION	1	/* bump when sr_chunk_meta changes */
struct sr_chunk_meta {
	u_int32_t		scm_volid;	/* vd we belong to */
	u_int32_t		scm_chunk_id;	/* chunk id */
	u_int32_t		scm_status;	/* use bio bioc_disk status */
	u_int32_t		scm_pad1;
	char			scm_devname[32];/* /dev/XXXXX */
	int64_t			scm_size;	/* size of partition in blocks*/
	int64_t			scm_coerced_size; /* coerced sz of part in blk*/
	struct sr_uuid		scm_uuid;	/* unique identifier */
} __packed;

#define SR_CRYPTO_MAXKEYBYTES	32
#define SR_CRYPTO_MAXKEYS	32
#define SR_CRYPTO_KEYBITS	512	/* AES-XTS with 2 * 256 bit keys */
#define SR_CRYPTO_KEYBYTES	(SR_CRYPTO_KEYBITS >> 3)
#define SR_CRYPTO_KDFHINTBYTES	256
#define SR_CRYPTO_CHECKBYTES	64
#define SR_CRYPTO_KEY_BLKSHIFT	30	/* 0.5TB per key */

struct sr_crypto_genkdf {
	u_int32_t	len;
	u_int32_t	type;
#define SR_CRYPTOKDFT_INVALID	(0)
#define SR_CRYPTOKDFT_PBKDF2	(1<<0)
};

struct sr_crypto_kdf_pbkdf2 {
	u_int32_t	len;
	u_int32_t	type;
	u_int32_t	rounds;
	u_int8_t	salt[128];
};

/*
 * Check that HMAC-SHA1_k(decrypted scm_key) == sch_mac, where
 * k = SHA1(masking key)
 */
struct sr_crypto_chk_hmac_sha1 {
	u_int8_t	sch_mac[20];
};

struct sr_crypto_kdfinfo {
	u_int32_t	len;
	u_int32_t	flags;
#define SR_CRYPTOKDF_INVALID	(0)
#define SR_CRYPTOKDF_KEY	(1<<0)
#define SR_CRYPTOKDF_HINT	(1<<1)
	u_int8_t	maskkey[SR_CRYPTO_MAXKEYBYTES];
	union {
		struct sr_crypto_genkdf		generic;
		struct sr_crypto_kdf_pbkdf2	pbkdf2;
	}		_kdfhint;
#define genkdf		_kdfhint.generic
#define pbkdf2		_kdfhint.pbkdf2
};

struct sr_crypto_metadata {
	u_int32_t		scm_alg;
#define SR_CRYPTOA_AES_XTS_128	1
#define SR_CRYPTOA_AES_XTS_256	2
	u_int32_t		scm_flags;
#define SR_CRYPTOF_INVALID	(0)
#define SR_CRYPTOF_KEY		(1<<0)
#define SR_CRYPTOF_KDFHINT	(1<<1)
	u_int32_t		scm_mask_alg;
#define SR_CRYPTOM_AES_ECB_256	1
	u_int8_t		scm_reserved[64];

	u_int8_t		scm_key[SR_CRYPTO_MAXKEYS][SR_CRYPTO_KEYBYTES];
	u_int8_t		scm_kdfhint[SR_CRYPTO_KDFHINTBYTES];

	u_int32_t		scm_check_alg;
#define SR_CRYPTOC_HMAC_SHA1		1
	union {
		struct sr_crypto_chk_hmac_sha1	chk_hmac_sha1;
		u_int8_t			chk_reserved2[64];
	}			_scm_chk;
#define	chk_hmac_sha1	_scm_chk.chk_hmac_sha1
};

#define SR_OPT_VERSION		1	/* bump when sr_opt_meta changes */
struct sr_opt_meta {
	u_int32_t		som_type;
#define SR_OPT_INVALID		0x00
#define SR_OPT_CRYPTO		0x01
	u_int32_t		som_pad;
	union {
		struct sr_crypto_metadata smm_crypto;
	}			som_meta;
};

#ifdef _KERNEL

#include <dev/biovar.h>

#include <sys/buf.h>
#include <sys/queue.h>
#include <sys/rwlock.h>

#include <scsi/scsi_all.h>
#include <scsi/scsi_disk.h>
#include <scsi/scsiconf.h>

#define DEVNAME(_s)     ((_s)->sc_dev.dv_xname)

/* #define SR_DEBUG */
#ifdef SR_DEBUG
extern u_int32_t		sr_debug;
#define DPRINTF(x...)		do { if (sr_debug) printf(x); } while(0)
#define DNPRINTF(n,x...)	do { if (sr_debug & n) printf(x); } while(0)
#define	SR_D_CMD		0x0001
#define	SR_D_INTR		0x0002
#define	SR_D_MISC		0x0004
#define	SR_D_IOCTL		0x0008
#define	SR_D_CCB		0x0010
#define	SR_D_WU			0x0020
#define	SR_D_META		0x0040
#define	SR_D_DIS		0x0080
#define	SR_D_STATE		0x0100
#else
#define DPRINTF(x...)
#define DNPRINTF(n,x...)
#endif

#define	SR_MAXFER		MAXPHYS
#define	SR_MAX_LD		1
#define	SR_MAX_CMDS		16
#define	SR_MAX_STATES		7

/* forward define to prevent dependency goo */
struct sr_softc;

struct sr_ccb {
	struct buf		ccb_buf;	/* MUST BE FIRST!! */

	struct sr_workunit	*ccb_wu;
	struct sr_discipline	*ccb_dis;

	int			ccb_target;
	int			ccb_state;
#define SR_CCB_FREE		0
#define SR_CCB_INPROGRESS	1
#define SR_CCB_OK		2
#define SR_CCB_FAILED		3

	void			*ccb_opaque; /* discipline usable pointer */

	TAILQ_ENTRY(sr_ccb)	ccb_link;
} __packed;

TAILQ_HEAD(sr_ccb_list, sr_ccb);

struct sr_workunit {
	struct scsi_xfer	*swu_xs;
	struct sr_discipline	*swu_dis;

	int			swu_state;
#define SR_WU_FREE		0
#define SR_WU_INPROGRESS	1
#define SR_WU_OK		2
#define SR_WU_FAILED		3
#define SR_WU_PARTIALLYFAILED	4
#define SR_WU_DEFERRED		5
#define SR_WU_PENDING		6
#define SR_WU_RESTART		7
#define SR_WU_REQUEUE		8

	int			swu_fake;	/* faked wu */
	/* workunit io range */
	daddr64_t		swu_blk_start;
	daddr64_t		swu_blk_end;

	/* in flight totals */
	u_int32_t		swu_ios_complete;
	u_int32_t		swu_ios_failed;
	u_int32_t		swu_ios_succeeded;

	/* number of ios that makes up the whole work unit */
	u_int32_t		swu_io_count;

	/* colliding wu */
	struct sr_workunit	*swu_collider;

	/* all ios that make up this workunit */
	struct sr_ccb_list	swu_ccb;

	TAILQ_ENTRY(sr_workunit) swu_link;
};

TAILQ_HEAD(sr_wu_list, sr_workunit);

/* RAID 0 */
#define SR_RAID0_NOWU		16
struct sr_raid0 {
	int32_t			sr0_strip_bits;
};

/* RAID 1 */
#define SR_RAID1_NOWU		16
struct sr_raid1 {
	u_int32_t		sr1_counter;
};

/* CRYPTO */
#define SR_CRYPTO_NOWU		16
struct sr_crypto {
	struct sr_crypto_metadata scr_meta;

	u_int8_t		scr_key[SR_CRYPTO_MAXKEYS][SR_CRYPTO_KEYBYTES];
	u_int8_t		scr_maskkey[SR_CRYPTO_MAXKEYBYTES];
	u_int64_t		scr_sid[SR_CRYPTO_MAXKEYS];
};

struct sr_metadata_list {
	struct sr_metadata	*sml_metadata;
	dev_t			sml_mm;
	int			sml_used;

	SLIST_ENTRY(sr_metadata_list) sml_link;
};

SLIST_HEAD(sr_metadata_list_head, sr_metadata_list);

struct sr_chunk {
	struct sr_chunk_meta	src_meta;	/* chunk meta data */

	/* runtime data */
	dev_t			src_dev_mm;	/* major/minor */

	/* helper members before metadata makes it onto the chunk  */
	int			src_meta_ondisk;/* set when meta is on disk */
	char			src_devname[32];
	int64_t			src_size;	/* in blocks */

	SLIST_ENTRY(sr_chunk)	src_link;
};

SLIST_HEAD(sr_chunk_head, sr_chunk);

struct sr_volume {
	struct sr_vol_meta	sv_meta;	/* meta data */

	/* runtime data */
	struct sr_chunk_head	sv_chunk_list;	/* linked list of all chunks */
	struct sr_chunk		**sv_chunks;	/* array to same chunks */

	/* sensors */
	struct ksensor		sv_sensor;
	struct ksensordev	sv_sensordev;
	int			sv_sensor_valid;
};

struct sr_discipline {
	struct sr_softc		*sd_sc;		/* link back to sr softc */
	u_int8_t		sd_type;	/* type of discipline */
#define	SR_MD_RAID0		0
#define	SR_MD_RAID1		1
#define	SR_MD_RAID5		2
#define	SR_MD_CACHE		3
#define	SR_MD_CRYPTO		4
	char			sd_name[10];	/* human readable dis name */
	u_int8_t		sd_scsibus;	/* scsibus discipline uses */
	struct scsi_link	sd_link;	/* link to midlayer */

	union {
	    struct sr_raid0	mdd_raid0;
	    struct sr_raid1	mdd_raid1;
	    struct sr_crypto	mdd_crypto;
	}			sd_dis_specific;/* dis specific members */
#define mds			sd_dis_specific

	/* discipline metadata */
	struct sr_metadata	*sd_meta;	/* in memory copy of metadata */
	u_int32_t		sd_meta_flags;

	int			sd_sync;
	int			sd_must_flush;

	int			sd_deleted;

	struct device		*sd_scsibus_dev;
	void			(*sd_shutdownhook)(void *);

	/* discipline volume */
	struct sr_volume	sd_vol;		/* volume associated */

	/* discipline resources */
	struct sr_ccb		*sd_ccb;
	struct sr_ccb_list	sd_ccb_freeq;
	u_int32_t		sd_max_ccb_per_wu;

	struct sr_workunit	*sd_wu;		/* all workunits */
	u_int32_t		sd_max_wu;

	struct sr_wu_list	sd_wu_freeq;	/* free wu queue */
	struct sr_wu_list	sd_wu_pendq;	/* pending wu queue */
	struct sr_wu_list	sd_wu_defq;	/* deferred wu queue */

	/* discipline stats */
	int			sd_wu_pending;
	u_int64_t		sd_wu_collisions;

	/* discipline functions */
	int			(*sd_alloc_resources)(struct sr_discipline *);
	int			(*sd_assemble_volume)(void *);
	int			(*sd_bringup_volume)(void *);
	int			(*sd_shutdown_volume)(void *);
	int			(*sd_free_resources)(struct sr_discipline *);
	int			(*sd_quiesce_io)(struct sr_discipline *);
	void			(*sd_set_chunk_state)(struct sr_discipline *,
				    int, int);
	void			(*sd_set_vol_state)(struct sr_discipline *);

	/* SCSI emulation */
	struct scsi_sense_data	sd_scsi_sense;
	int			(*sd_scsi_rw)(struct sr_workunit *);
	int			(*sd_scsi_sync)(struct sr_workunit *);
	int			(*sd_scsi_tur)(struct sr_workunit *);
	int			(*sd_scsi_start_stop)(struct sr_workunit *);
	int			(*sd_scsi_inquiry)(struct sr_workunit *);
	int			(*sd_scsi_read_cap)(struct sr_workunit *);
	int			(*sd_scsi_req_sense)(struct sr_workunit *);
};

struct sr_softc {
	struct device		sc_dev;

	int			(*sc_ioctl)(struct device *, u_long, caddr_t);

	struct rwlock		sc_lock;

	int			sc_sensors_running;
	/*
	 * during scsibus attach this is the discipline that is in use
	 * this variable is protected by sc_lock and splhigh
	 */
	struct sr_discipline	*sc_attach_dis;

	/*
	 * XXX expensive, alternative would be nice but has to be cheap
	 * since the scsibus lookup happens on each IO
	 */
#define SR_MAXSCSIBUS		256
	struct sr_discipline	*sc_dis[SR_MAXSCSIBUS]; /* scsibus is u_int8_t */
};

struct pool;
extern struct pool	sr_uiopl;
extern struct pool	sr_iovpl;

/* work units & ccbs */
int			sr_alloc_ccb(struct sr_discipline *);
void			sr_free_ccb(struct sr_discipline *);
struct sr_ccb		*sr_get_ccb(struct sr_discipline *);
void			sr_put_ccb(struct sr_ccb *);
int			sr_alloc_wu(struct sr_discipline *);
void			sr_free_wu(struct sr_discipline *);
struct sr_workunit	*sr_get_wu(struct sr_discipline *);
void			sr_put_wu(struct sr_workunit *);

/* misc functions */
int32_t			sr_validate_stripsize(u_int32_t);
void			sr_save_metadata_callback(void *, void *);
int			sr_validate_io(struct sr_workunit *, daddr64_t *,
			    char *);
int			sr_check_io_collision(struct sr_workunit *);

/* discipline functions */
int			sr_raid_inquiry(struct sr_workunit *);
int			sr_raid_read_cap(struct sr_workunit *);
int			sr_raid_tur(struct sr_workunit *);
int			sr_raid_request_sense( struct sr_workunit *);
int			sr_raid_start_stop(struct sr_workunit *);
int			sr_raid_sync(struct sr_workunit *);
void			sr_raid_startwu(struct sr_workunit *);

/* raid 0 */
int			sr_raid0_alloc_resources(struct sr_discipline *);
int			sr_raid0_free_resources(struct sr_discipline *);
int			sr_raid0_rw(struct sr_workunit *);
void			sr_raid0_intr(struct buf *);
void			sr_raid0_set_chunk_state(struct sr_discipline *,
			    int, int);
void			sr_raid0_set_vol_state(struct sr_discipline *);

/* raid 1 */
int			sr_raid1_alloc_resources(struct sr_discipline *);
int			sr_raid1_free_resources(struct sr_discipline *);
int			sr_raid1_rw(struct sr_workunit *);
void			sr_raid1_intr(struct buf *);
void			sr_raid1_recreate_wu(struct sr_workunit *);
void			sr_raid1_set_chunk_state(struct sr_discipline *,
			    int, int);
void			sr_raid1_set_vol_state(struct sr_discipline *);

/* crypto discipline */
int			sr_crypto_alloc_resources(struct sr_discipline *);
int			sr_crypto_free_resources(struct sr_discipline *);
int			sr_crypto_rw(struct sr_workunit *);
int			sr_crypto_get_kdf(struct bioc_createraid *,
			    struct sr_discipline *);
int			sr_crypto_create_keys(struct sr_discipline *);

#ifdef SR_DEBUG
void			sr_dump_mem(u_int8_t *, int);
#endif

#endif /* _KERNEL */

#endif /* SOFTRAIDVAR_H */
