#	$OpenBSD: src/regress/libexec/ld.so/dlsym/test3/prog1/Makefile,v 1.1.1.1 2005/09/19 03:34:18 kurt Exp $

PROG=		prog1
SRCS=		main.c
LDADD+=		-laa
LDFLAGS+=	-Wl,-E
LDFLAGS+=	-Wl,-rpath,$(AA_OBJDIR)
LDFLAGS+=	-Wl,-rpath,$(BB_OBJDIR)
LDFLAGS+=	-Wl,-rpath,$(DD_OBJDIR)
LDFLAGS+=	-L$(AA_OBJDIR)

.include <bsd.regress.mk>
