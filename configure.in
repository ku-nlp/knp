dnl Process this file with autoconf to produce a configure script.
AC_INIT(system/knp.h)
PACKAGE=knp
VERSION=2.1

AM_INIT_AUTOMAKE($PACKAGE, $VERSION, no-define)
AM_CONFIG_HEADER(config.h)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET

AC_ARG_WITH(gdbm, [ --with-gdbm              use GDBM library [default=yes]])
AC_ARG_WITH(berkeley-db, [ --with-berkeley-db       use Berkeley DB library [default=no]])

AC_ARG_ENABLE(juman-lib-dir, [ --enable-juman-lib-dir   use specified juman library DIR [default=../juman/lib]], 
JUMAN_LIB_DIR=$enable_juman_lib_dir, JUMAN_LIB_DIR="")

AC_ARG_ENABLE(rcpath, [ --enable-rcpath          use specified rcpath [default=../juman/jumanrc.sample]], 
RCPATH=$enable_rcpath, RCPATH="")

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

AC_MSG_CHECKING("rcpath")
if test -z "$RCPATH"; then
    if test -f /share/tool/juman/jumanrc.sample; then
	RCPATH="/share/tool/juman/jumanrc.sample"
    elif test -f ../juman/jumanrc.sample; then
	RCPATH="`pwd`/../juman/jumanrc.sample"
    else
	AC_MSG_ERROR("jumanrc.sample is not found.")
    fi
else
    if test ! -f $RCPATH; then
	if test -f "$RCPATH/jumanrc.sample"; then
	    RCPATH="$RCPATH/jumanrc.sample"
	else
	    AC_MSG_ERROR("jumanrc.sample is not found.")
	fi
    fi
fi
AC_MSG_RESULT("$RCPATH")

AC_ARG_ENABLE(debug, 
	[ --disable-debug          build a non-debugging version [default=no]])

AC_ARG_ENABLE(small, 
	[ --enable-small           make a small version [default=no]])

ETC_LIBS=""
ETC_CFLAGS=""

AC_MSG_CHECKING("non-debug version")
if test ! x$enable_debug = "xyes"; then
    dnl    AC_DEFINE(DEBUG)
    ETC_CFLAGS="-DDEBUG $ETC_CFLAGS"
fi
AC_MSG_RESULT($ts_cv_debug)

if test "x$enable_small" = "xyes"; then
    dnl    AC_DEFINE(SMALL)
    ETC_CFLAGS="-DSMALL $ETC_CFLAGS"
fi

AC_PREFIX_DEFAULT(`pwd`)
KNP_DIR=@prefix@
KNP_SYS="$KNP_DIR/system"
KNP_DICT="$KNP_DIR/dict"
KNP_RULE="$KNP_DIR/rule"

LDFLAGS="-L/usr/local/lib -R/usr/local/lib $LDFLAGS"

dnl Checks for libraries.

if ( test "x$with_berkeley_db" != xyes -a \( "x$with_gdbm" = x -o "x$with_gdbm" = xyes \) ); then
    AC_CHECK_LIB(gdbm, gdbm_open, ETC_LIBS="-lgdbm $ETC_LIBS" ETC_CFLAGS="-DGDBM $ETC_CFLAGS", 
	         AC_MSG_ERROR(not found.))
fi

if ( test "x$with_berkeley_db" = xyes ); then
    AC_CHECK_LIB(db, db_open, ETC_LIBS="-ldb $ETC_LIBS", 
	         AC_MSG_ERROR(not found.))
fi

AC_CHECK_LIB(m, sqrt, ETC_LIBS="$ETC_LIBS -lm")
AC_CHECK_LIB(nsl, gethostbyname, ETC_LIBS="-lnsl $ETC_LIBS")
AC_CHECK_LIB(socket, socket, ETC_LIBS="-lsocket $ETC_LIBS")

CFLAGS="-I/usr/local/include -I$JUMAN_LIB_DIR -DKNP_SYS=\"\\\"$KNP_SYS\"\\\" -DKNP_DICT=\"\\\"$KNP_DICT\"\\\" -DKNP_RULE=\"\\\"$KNP_RULE\"\\\" -DRC_DEFAULT=\"\\\"$RCPATH\"\\\" $ETC_CFLAGS $CFLAGS"

JUMAN_LIBS="$JUMAN_LIB_DIR/iotool.o $JUMAN_LIB_DIR/lisp.o $JUMAN_LIB_DIR/grammar.o $JUMAN_LIB_DIR/katuyou.o $JUMAN_LIB_DIR/connect.o $JUMAN_LIB_DIR/getid.o"
LIBS="$JUMAN_LIBS $ETC_LIBS $LIBS"

dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h limits.h malloc.h sys/file.h unistd.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_STRUCT_TM

dnl Checks for library functions.

AC_OUTPUT([
Makefile
system/Makefile
])
