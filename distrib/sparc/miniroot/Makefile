#	$OpenBSD: src/distrib/sparc/miniroot/Makefile,v 1.1 2012/09/10 22:21:03 deraadt Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

TARGET=miniroot${REV}.fs

.ifndef DESTDIR
all ${TARGET}:
	@echo setenv DESTDIR before making a ramdisk!
	@false
.else

all: ${TARGET}

${TARGET}: vn_up install_files installboot showit vn_down

vn_up: blank_filesystem
	vnconfig vnd0 ${TARGET}
	disklabel -w vnd0 fakeramdisk
	newfs -m 0 /dev/rvnd0a
	mount /dev/vnd0a /mnt

showit:
	df -ki /mnt

vn_down:
	-umount /mnt
	-vnconfig -u vnd0

install_files: bsd.rd ofwboot

bsd.rd:
	install -c -m 555 -o root -g wheel \
	    ${.OBJDIR}/../../ramdisk/bsd.rd /mnt/bsd

ofwboot:
	install -c -m 555 -o root -g wheel \
	    ${DESTDIR}/usr/mdec/boot /mnt/boot

installboot:
	/usr/mdec/binstall -v ffs /mnt

blank_filesystem:
	dd if=/dev/zero of=${TARGET} bs=512 count=12288

.endif

unconfig:
	-umount -f /mnt
	-vnconfig -u /dev/vnd0a

.ifdef RELEASEDIR
install:
	cp ${TARGET} ${RELEASEDIR}
.endif

clean:
	rm -f ${TARGET}

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.subdir.mk>
