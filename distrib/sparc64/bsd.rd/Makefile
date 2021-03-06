#	$OpenBSD: src/distrib/sparc64/bsd.rd/Makefile,v 1.27 2013/10/18 14:04:05 deraadt Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
IMAGE=		ramdisk${REV}.fs
CBIN?=		instbin
LISTS?=		${.CURDIR}/list
UTILS?=		${TOP}/../miniroot

MOUNT_POINT=	/mnt

VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CRDEV=	/dev/r${VND}c
PID!=		echo $$$$
REALIMAGE!=	echo /var/tmp/image.${PID}

STRIP?=		strip
GZIPCMD?=	gzip
GZIPFLAGS?=	-9fn
INSTALLBOOT?=	/usr/mdec/installboot -v
MTREE=		${UTILS}/mtree.conf

RAMDISK=	RAMDISK
BOOT?=		/usr/mdec/boot
BOOTXX?=	/usr/mdec/bootxx

CLEANFILES+=	bsd.rd_unz rdsetroot

#.include "${TOP}/common/Makefile.inc"

all: ${CBIN} bsd.rd

DISKTYPE=       rdroot
NBLKS=          5744

# old format, minfree, opt, b/i  trks, sects, cpg
NEWFSARGS=	-t ffs -m 0 -o space
NEWFSARGS_RD=	-i 4096 -c ${NBLKS}
NEWFSARGS_FD=	-i 524288 -c 2880

bsd.rd:	bsd.rd_unz
	objcopy -Sg -R .comment bsd.rd_unz bsd.strip
	${STRIP} bsd.strip
	${GZIPCMD} -c ${GZIPFLAGS} bsd.strip > bsd.rd

bsd.rd_unz: bsd ${IMAGE} rdsetroot
	cp bsd bsd.rd_unz
	${.OBJDIR}/rdsetroot bsd.rd_unz ${IMAGE}

bsd:
	cd ${.CURDIR}/../../../sys/arch/sparc64/conf && config ${RAMDISK}
	cd ${.CURDIR}/../../../sys/arch/sparc64/compile/${RAMDISK} && \
	    ${MAKE} clean && COPTS=-Os exec ${MAKE}
	cp ${.CURDIR}/../../../sys/arch/sparc64/compile/${RAMDISK}/bsd bsd

${IMAGE}:	do_prep do_mount do_files do_umount
	mv -f ${REALIMAGE} ${IMAGE}

rdsetroot:	${TOP}/../common/elfrdsetroot.c
	${HOSTCC} ${HOSTCFLAGS} -o rdsetroot \
	    ${TOP}/../common/elfrdsetroot.c ${TOP}/../common/elf32.c \
	    ${TOP}/../common/elf64.c

do_prep:
	dd if=/dev/zero of=${REALIMAGE} count=${NBLKS}
	vnconfig -v -c ${VND} ${REALIMAGE}
	disklabel -w ${VND} ${DISKTYPE}
	
do_mount:
	newfs ${NEWFSARGS} ${NEWFSARGS_RD} ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}

do_umount:
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND}

unconfig:
	-umount ${MOUNT_POINT}
	-vnconfig -u ${VND}

rd_setup: ${CBIN}
	dd if=/dev/zero of=${REALIMAGE} bs=512 count=${NBLKS}
	vnconfig -v -c ${VND} ${REALIMAGE}
	disklabel -w ${VND} ${DISKTYPE}
	newfs ${NEWFSARGS} ${VND_RDEV}
	fsck ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}

rd_teardown:
	@df -i ${MOUNT_POINT}
	-umount ${MOUNT_POINT}
	-vnconfig -u ${VND}
	cp ${REALIMAGE} ${IMAGE}
	rm ${REALIMAGE}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND}
	-/bin/rm -f ${IMAGE}

.PRECIOUS:	${IMAGE}

.ifdef RELEASEDIR
install:
.ifndef NOBSDRD
	cp bsd.rd ${RELEASEDIR}
.endif
.endif

${CBIN}.conf: ${LISTS}
	awk -f ${UTILS}/makeconf.awk CBIN=${CBIN} ${LISTS} > ${CBIN}.conf

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CBIN}.conf
	crunchgen -E -D ${BSDSRCDIR} -L ${DESTDIR}/usr/lib \
	-c ${CBIN}.c -e ${CBIN} -m ${CBIN}.mk ${CBIN}.conf

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	${MAKE} -f ${CBIN}.mk SRCLIBDIR=${.CURDIR}/../../../lib all

do_files:
	mtree -def ${MTREE} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    REV=${REV} TARGDIR=${MOUNT_POINT} UTILS=${UTILS} \
	    RELEASEDIR=${RELEASEDIR} sh ${UTILS}/runlist.sh ${LISTS}
	rm ${MOUNT_POINT}/${CBIN}

clean cleandir:
	/bin/rm -f *.core ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}*.cache \
	    lib*.a lib*.olist ${CBIN}.map \
	    *.o *.lo *.c bsd bsd.rd bsd.gz bsd.strip floppy*.fs rdsetroot

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
