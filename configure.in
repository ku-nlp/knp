dnl Process this file with autoconf to produce a configure script.
AC_INIT(system/knp.h)
PACKAGE=knp
VERSION=2.1

AM_INIT_AUTOMAKE($PACKAGE, $VERSION, no-define)
AM_CONFIG_HEADER(config.h)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_MAKE_SET

AC_ARG_WITH(gdbm, [  --with-gdbm             use GDBM library [default=yes]])
AC_ARG_WITH(berkeley-db, [  --with-berkeley-db      use Berkeley DB library [default=no]])
AC_ARG_WITH(included-juman, [  --with-included-juman   use the JUMAN library included here])

AC_ARG_ENABLE(juman-lib-dir, [  --enable-juman-lib-dir  use specified JUMAN library DIR [default=../juman/lib]], 
JUMAN_LIB_DIR=$enable_juman_lib_dir, JUMAN_LIB_DIR="")

AC_ARG_ENABLE(rcpath, [  --enable-rcpath         use specified rcpath], 
RCPATH=$enable_rcpath, RCPATH="")

SUBDIRS="system rule docs"

if test "$with_included_juman" = yes; then
    SUBDIRS="juman $SUBDIRS"
    JUMAN_LIB_DIR="../juman"
    AC_MSG_CHECKING("rcpath")
    if test -z "$RCPATH"; then
	if test -f juman/jumanrc.sample; then
	    RCPATH="`pwd`/juman/jumanrc.sample"
	elif test -f /share/tool/juman/jumanrc.sample; then
	    RCPATH="/share/tool/juman/jumanrc.sample"
	else
	    AC_MSG_ERROR("jumanrc.sample is not found.")
	fi
    else
	if test ! -f "$RCPATH"; then
	    if test -f "$RCPATH/jumanrc.sample"; then
		RCPATH="$RCPATH/jumanrc.sample"
	    else
		AC_MSG_ERROR("jumanrc.sample is not found.")
	    fi
	fi
    fi
    AC_MSG_RESULT("$RCPATH")
    RCPATH_CFLAGS="-DRC_DEFAULT=\"\\\"$RCPATH\"\\\""
else
    AC_MSG_CHECKING("juman library DIR")
    if test -z "$JUMAN_LIB_DIR"; then
	if test -d /share/tool/juman/lib; then
	    JUMAN_LIB_DIR="/share/tool/juman/lib"
	elif test -d ../juman/lib; then
	    JUMAN_LIB_DIR="`pwd`/../juman/lib"
	else
	    AC_MSG_ERROR("JUMAN is not found.")
	fi
    else
	if test ! -d $JUMAN_LIB_DIR; then
	    AC_MSG_ERROR("JUMAN is not found.")
	fi
    fi
    AC_MSG_RESULT("$JUMAN_LIB_DIR")
    RCPATH_CFLAGS=""
fi

JUMAN_CFLAGS="-I$JUMAN_LIB_DIR"
JUMAN_LDFLAGS="-L$JUMAN_LIB_DIR"

AC_ARG_ENABLE(debug, 
	[  --disable-debug         build a non-debugging version [default=no]])

AC_ARG_ENABLE(small, 
	[  --enable-small          make a small version [default=no]])

AC_MSG_CHECKING("debug version")
if test ! "x$enable_debug" = "xyes"; then
    dnl    AC_DEFINE(DEBUG)
    ETC_CFLAGS="-DDEBUG $ETC_CFLAGS"
fi
AC_MSG_RESULT("$enable_debug")

if test "x$enable_small" = "xyes"; then
    dnl    AC_DEFINE(SMALL)
    ETC_CFLAGS="-DSMALL $ETC_CFLAGS"
fi

AC_PREFIX_DEFAULT(`pwd`)
KNP_DIR=@prefix@
KNP_SYS="$KNP_DIR/system"
KNP_DICT="$KNP_DIR/dict"
KNP_RULE="$KNP_DIR/rule"

LDFLAGS="$JUMAN_LDFLAGS -L/usr/local/lib $LDFLAGS"

dnl Checks for libraries.

if ( test "x$with_berkeley_db" != xyes -a \( "x$with_gdbm" = x -o "x$with_gdbm" = xyes \) ); then
    AC_CHECK_LIB(gdbm, gdbm_open, DB_LIBS="-lgdbm $DB_LIBS" ETC_CFLAGS="-DGDBM $ETC_CFLAGS", 
	         AC_MSG_ERROR(not found.))
fi

if ( test "x$with_berkeley_db" = xyes ); then
    AC_CHECK_LIB(db, db_open, DB_LIBS="-ldb $DB_LIBS", 
	         AC_MSG_ERROR(not found.))
fi

AC_CHECK_LIB(m, sqrt, ETC_LIBS="$ETC_LIBS -lm")
AC_CHECK_LIB(nsl, gethostbyname, KNP_LIBS="-lnsl $KNP_LIBS")
AC_CHECK_LIB(socket, socket, KNP_LIBS="-lsocket $KNP_LIBS")

KNP_LIBS="$KNP_LIBS $DB_LIBS $ETC_LIBS"
MAKE_DB_LIBS="$MAKE_DB_LIBS $DB_LIBS"

AC_SUBST(KNP_LIBS)
AC_SUBST(MAKE_DB_LIBS)

CFLAGS="-I/usr/local/include $JUMAN_CFLAGS -DKNP_SYS=\"\\\"$KNP_SYS\"\\\" -DKNP_DICT=\"\\\"$KNP_DICT\"\\\" -DKNP_RULE=\"\\\"$KNP_RULE\"\\\" $RCPATH_CFLAGS $ETC_CFLAGS $CFLAGS"

dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h limits.h malloc.h sys/file.h unistd.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_STRUCT_TM

AC_SUBST(SUBDIRS)

AC_OUTPUT([
Makefile
system/Makefile
juman/Makefile
rule/Makefile
docs/Makefile
])
