/*
 * Copyright (c) 1995-2000 Kungliga Tekniska H�gskolan
 * (Royal Institute of Technology, Stockholm, Sweden).
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * The interface for the file-cache.
 */

/* $KTH: fcache.h,v 1.72.2.2 2001/10/02 16:13:07 jimmy Exp $ */

#ifndef _FCACHE_H_
#define _FCACHE_H_

#include <xfs/xfs_message.h>
#include <fcntl.h>
#include <cred.h>
#include <heap.h>

/*
 * For each entry in the filecache we save the rights of NACCESS users.
 * The value should be the same as MAXRIGHTS from xfs_message.h
 * If it isn't you can get some very strange behavior from xfs, so don't
 * even try. XXX
 */ 

#define NACCESS MAXRIGHTS

typedef struct {
     xfs_pag_t cred;
     u_long access;
} AccessEntry;

enum Access { ANONE   = 0x0,
              AREAD   = 0x01,
	      AWRITE  = 0x02,
	      AINSERT = 0x04,
	      ALIST   = 0x08,
	      ADELETE = 0x10,
	      ALOCK   = 0x20,
	      AADMIN  = 0x40 };

typedef struct {
    Bool valid;
    xfs_cache_handle xfs_handle;
} fcache_cache_handle;

typedef struct {
    struct Lock lock;		/* locking information for this entry */
    VenusFid fid;		/* The fid of the file for this entry */
    unsigned refcount;		/* reference count */
    u_int32_t host;		/* the source of this entry */
    size_t length;		/* length of our cached copy */
    AFSFetchStatus status;	/* Removed unused stuff later */
    AFSCallBack callback;	/* Callback to the AFS-server */
    AFSVolSync volsync;		/* Sync info for ro-volumes */
    AccessEntry acccache[NACCESS]; /* cache for the access rights */
    u_int32_t anonaccess;	/* the access mask for system:anyuser */
    unsigned index;		/* this is V%u */
    fcache_cache_handle handle;	/* handle */
    struct {
	unsigned usedp : 1;	/* Is this entry used? */
	unsigned attrp : 1;	/* Are the attributes in status valid? */
	unsigned datap : 1;	/* Is the cache-file valid? */
	unsigned attrusedp : 1;	/* Attr is used in the kernel */
	unsigned datausedp : 1;	/* Data is used in the kernel */
	unsigned extradirp : 1;	/* Has this directory been "converted"? */
	unsigned mountp : 1;	/* Is this an AFS mount point? */
	unsigned kernelp : 1;	/* Does this entry exist in the kernel? */
	unsigned sentenced : 1;	/* This entry should die */
	unsigned silly : 1;	/* Instead of silly-rename */
	unsigned fake_mp : 1;	/* a `fake' mount point */
	unsigned vol_root : 1;	/* root of a volume */
    } flags;
    u_int tokens;		/* read/write tokens for the kernel */
    VenusFid parent;
    Listitem *lru_le;		/* lru */
    heap_ptr invalid_ptr;	/* pointer into the heap */
    VolCacheEntry *volume;	/* pointer to the volume entry */
    Bool priority;		/* is the file worth keeping */
    int hits;			/* number of lookups */
    int cleanergen;		/* generation cleaner */
} FCacheEntry;

/*
 * The fileservers to ask for a particular volume.
 */

struct fs_server_context {
    ConnCacheEntry *conns[NMAXNSERVERS];
    int i;			/* current number being probed */
    int num_conns;		/* number in `conns' */
};

typedef struct fs_server_context fs_server_context;

/*
 * This is magic cookie for the dump of the fcache.
 * It's supposed not to be able to be confused with an old-style
 * dump (with no header)
 */

#define FCACHE_MAGIC_COOKIE	0xff1201ff

/*
 * current version number of the dump file
 */

#define FCACHE_VERSION		0x2

/*
 * How far the cleaner will go went cleaning things up.
 */

extern Bool fprioritylevel;

void
fcache_init (u_long alowvnodes,
	     u_long ahighvnodes,
	     u_long alowbytes,
	     u_long ahighbytes,
	     Bool recover);

int
fcache_reinit(u_long alowvnodes,
	      u_long ahighvnodes,
	      u_long alowbytes,
	      u_long ahighbytes);

void
fcache_purge_volume (VenusFid fid);

void
fcache_purge_host (u_long host);

void
fcache_purge_cred (xfs_pag_t cred, int32_t cell);

void
fcache_stale_entry (VenusFid fid, AFSCallBack callback);

int
fcache_file_name (FCacheEntry *entry, char *s, size_t len);

int
fcache_dir_name (FCacheEntry *entry, char *s, size_t len);

int
fcache_extra_file_name (FCacheEntry *entry, char *s, size_t len);

int
fcache_create_file (FCacheEntry *entry);

int
fcache_open_file (FCacheEntry *entry, int flag);

int
fcache_open_extra_dir (FCacheEntry *entry, int flag, mode_t mode);

int
fcache_fhget (char *filename, fcache_cache_handle *handle);

int
write_data (FCacheEntry *entry, AFSStoreStatus *status, CredCacheEntry *ce);

int
truncate_file (FCacheEntry *entry, off_t size, CredCacheEntry *ce);

int
write_attr (FCacheEntry *entry, const AFSStoreStatus *status,
	    CredCacheEntry *ce);

int
create_file (FCacheEntry *dir_entry,
	     const char *name, AFSStoreStatus *store_attr,
	     VenusFid *child_fid, AFSFetchStatus *fetch_attr,
	     CredCacheEntry *ce);

int
create_directory (FCacheEntry *dir_entry,
		  const char *name, AFSStoreStatus *store_attr,
		  VenusFid *child_fid, AFSFetchStatus *fetch_attr,
		  CredCacheEntry *ce);

int
create_symlink (FCacheEntry *dir_entry,
		const char *name, AFSStoreStatus *store_attr,
		VenusFid *child_fid, AFSFetchStatus *fetch_attr,
		const char *contents,
		CredCacheEntry *ce);

int
create_link (FCacheEntry *dir_entry,
	     const char *name,
	     FCacheEntry *existing_entry,
	     CredCacheEntry *ce);

int
remove_file (FCacheEntry *dire, const char *name, CredCacheEntry *ce);

int
remove_directory (FCacheEntry *dire, const char *name, CredCacheEntry *ce);

int
rename_file (FCacheEntry *old_dir,
	     const char *old_name,
	     FCacheEntry *new_dir,
	     const char *new_name,
	     CredCacheEntry *ce);

int
getroot (VenusFid *res, CredCacheEntry *ce);

int
fcache_get (FCacheEntry **res, VenusFid fid, CredCacheEntry *ce);

void
fcache_release (FCacheEntry *e);

int
fcache_find (FCacheEntry **res, VenusFid fid);

int
fcache_get_data (FCacheEntry **res, VenusFid *fid, CredCacheEntry **ce);

int
fcache_verify_attr (FCacheEntry *entry, FCacheEntry *parent_entry,
		    const char *prefered_name, CredCacheEntry* ce);

int
fcache_verify_data (FCacheEntry *e, CredCacheEntry *ce);

int
followmountpoint (VenusFid *fid, const VenusFid *parent, FCacheEntry *parent_e,
		  CredCacheEntry **ce);

void
fcache_status (void);

int
fcache_store_state (void);

int
getacl(VenusFid fid, CredCacheEntry *ce,
       AFSOpaque *opaque);

int
setacl(VenusFid fid, CredCacheEntry *ce,
       AFSOpaque *opaque, FCacheEntry **ret);

int
getvolstat(VenusFid fid, CredCacheEntry *ce,
	   AFSFetchVolumeStatus *volstat,
	   char *volumename,
	   char *offlinemsg,
	   char *motd);

int
setvolstat(VenusFid fid, CredCacheEntry *ce,
	   AFSStoreVolumeStatus *volstat,
	   char *volumename,
	   char *offlinemsg,
	   char *motd);

u_long
fcache_highbytes(void);

u_long
fcache_usedbytes(void);

u_long
fcache_lowbytes(void);

u_long
fcache_highvnodes(void);

u_long
fcache_usedvnodes(void);

u_long
fcache_lowvnodes(void);

int
fcache_need_bytes(u_long needed);

Bool
fcache_need_nodes (void);

int
fcache_giveup_all_callbacks (void);

int
fcache_reobtain_callbacks (void);

/* XXX - this shouldn't be public, but getrights in inter.c needs it */
int
read_attr (FCacheEntry *, CredCacheEntry *);

Bool
findaccess (xfs_pag_t cred, AccessEntry *ae, AccessEntry **pos);

void
fcache_unused(FCacheEntry *entry);

void
fcache_update_length (FCacheEntry *entry, size_t len);

ConnCacheEntry *
find_first_fs (FCacheEntry *e,
	       CredCacheEntry *ce,
	       fs_server_context *context);

ConnCacheEntry *
find_next_fs (fs_server_context *context,
	      ConnCacheEntry *prev_conn,
	      int mark_as_dead);

void
free_fs_server_context (fs_server_context *context);

void
recon_hashtabadd(FCacheEntry *entry);
 
void
recon_hashtabdel(FCacheEntry *entry);

int
fcache_get_fbuf (FCacheEntry *centry, int *fd, fbuf *fbuf,
		 int open_flags, int fbuf_flags);

u_long
fcache_calculate_usage (void);

const VenusFid *
fcache_realfid (const FCacheEntry *entry);

void
fcache_mark_as_mountpoint (FCacheEntry *entry);

int
collectstats_hostpart(u_int32_t *host, u_int32_t *part, int *n);

int
collectstats_getentry(u_int32_t host, u_int32_t part, u_int32_t type,
		      u_int32_t items_slot, u_int32_t *count,
		      int64_t *items_total, int64_t *total_time);

#endif /* _FCACHE_H_ */
