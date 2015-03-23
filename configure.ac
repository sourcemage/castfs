AC_INIT(castfs, 0.6.2)
AM_INIT_AUTOMAKE
AC_PROG_CC

AM_MAINTAINER_MODE

AC_ARG_ENABLE(debug,
AC_HELP_STRING([--enable-debug],[Enable debug flags]),
	[ CFLAGS="$CFLAGS -Wall -g" ],
	[])

AC_ARG_WITH(pkgconfigdir,
AC_HELP_STRING([--with-pkgconfigdir=DIR],[pkgconfig file in DIR @<:@LIBDIR/pkgconfig@:>@]),
	[pkgconfigdir=$withval],
	[pkgconfigdir='${libdir}/pkgconfig'])
AC_SUBST(pkgconfigdir)

CPPFLAGS="${CPPFLAGS} `pkg-config --cflags fuse` -D_REENTRANT -DFUSE_USE_VERSION=25"

AC_CHECK_HEADERS(fuse.h,, AC_MSG_ERROR([fuse 2.4 or newer is required to build castfs]))

AC_CHECK_LIB(fuse, fuse_main,, AC_MSG_ERROR([fuse 2.4 or newer is required to build castfs]))

FUSE_PREFIX=`pkg-config --variable=exec_prefix fuse `
AC_SUBST(FUSE_PREFIX,PREFIX,DATAROOTDIR)

AC_ARG_ENABLE(static-fuse,
AC_HELP_STRING([--enable-static-fuse], [Enable building castfs against fuse statically]),
[static_fuse=${enableval}], [static_fuse=no])

if test "x$static_fuse" = "xyes"
then
  LIBS=`pkg-config --libs fuse | sed 's/-lfuse/$(FUSE_PREFIX)\/lib\/libfuse.a -lrt/'`
fi

AC_CHECK_FUNCS([setxattr])

AC_CONFIG_FILES([
Makefile
include/Makefile
src/Makefile
test/Makefile
debian/Makefile
include/config.h
])
AC_OUTPUT