dnl Process this file with autoconf to produce a configure script.
AC_INIT(system/knp.h)
PACKAGE=knp
VERSION=2.0

AM_INIT_AUTOMAKE($PACKAGE, $VERSION, no-define)
AM_CONFIG_HEADER(config.h)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_MAKE_SET

AC_ARG_WITH(gdbm, [  --with-gdbm             use GDBM library [default=yes]])
AC_ARG_WITH(berkeley-db, [  --with-berkeley-db      use Berkeley DB library (2.X) [default=no]])
AC_ARG_WITH(included-juman, [  --with-included-juman   use the JUMAN library included here])
AC_ARG_WITH(integrate-juman, [  --with-integrate-juman  integrate JUMAN into KNP], 
ETC_CFLAGS="-DINTEGRATE_JUMAN $ETC_CFLAGS")

AC_ARG_WITH(jumanlib-dir, [  --with-jumanlib-dir     use specified JUMAN library DIR], 
JUMAN_LIB_DIR=$with_juman_lib_dir, JUMAN_LIB_DIR="")

AC_ARG_WITH(rcpath, [  --with-rcpath           use specified rcpath], 
RCPATH=$with_rcpath, RCPATH="")

SUBDIRS="system rule docs perl dict"

packagesrcdir=`cd $srcdir && pwd`

if test "$with_included_juman" = yes; then
    SUBDIRS="juman $SUBDIRS"
    JUMAN_LIB_DIR="../juman"
    AC_MSG_CHECKING("rcpath")
    if test -z "$RCPATH"; then
	if test -f juman/jumanrc.sample; then
	    RCPATH="$packagesrcdir/juman/jumanrc.sample"
	elif test -f /share/tool/juman/jumanrc.sample; then
	    RCPATH="/share/tool/juman/jumanrc.sample"
	else
	    AC_MSG_ERROR("jumanrc.sample is not found.")
	fi
    fi
    AC_MSG_RESULT("$RCPATH")
    RCPATH_CFLAGS="-DRC_DEFAULT=\"\\\"$RCPATH\"\\\""
else
    AC_MSG_CHECKING("JUMAN library DIR")
    if test -z "$JUMAN_LIB_DIR"; then
	if test -d /share/tool/juman/lib; then
	    JUMAN_LIB_DIR="/share/tool/juman/lib"
	elif test -d ../juman/lib; then
	    JUMAN_LIB_DIR="$packagesrcdir/../juman/lib"
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

JUMAN_CFLAGS="-I$JUMAN_LIB_DIR $RCPATH_CFLAGS"
JUMAN_LDFLAGS="-L$JUMAN_LIB_DIR"

AC_SUBST(JUMAN_CFLAGS)

AC_ARG_ENABLE(debug, 
	[  --disable-debug         build a non-debugging version [default=no]])

AC_MSG_CHECKING("debug version")
if test ! "x$enable_debug" = "xno"; then
    dnl    AC_DEFINE(DEBUG)
    ETC_CFLAGS="-DDEBUG $ETC_CFLAGS"
    enable_debug=yes
fi
AC_MSG_RESULT("$enable_debug")

AC_PREFIX_DEFAULT("$packagesrcdir")
KNP_DIR=$packagesrcdir
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
    AC_SEARCH_LIBS(db_open, db2 db, , 
	         AC_MSG_ERROR(not found.))
fi

AC_CHECK_LIB(m, sqrt, ETC_LIBS="$ETC_LIBS -lm")
AC_CHECK_LIB(nsl, gethostbyname, KNP_LIBS="-lnsl $KNP_LIBS")
AC_CHECK_LIB(socket, socket, KNP_LIBS="-lsocket $KNP_LIBS")

KNP_LIBS="$KNP_LIBS $DB_LIBS $ETC_LIBS"
MAKE_DB_LIBS="$MAKE_DB_LIBS $DB_LIBS"

AC_SUBST(KNP_LIBS)
AC_SUBST(MAKE_DB_LIBS)

KNP_CFLAGS="-I/usr/local/include $JUMAN_CFLAGS -DKNP_SYS=\"\\\"$KNP_SYS\"\\\" -DKNP_DICT=\"\\\"$KNP_DICT\"\\\" -DKNP_RULE=\"\\\"$KNP_RULE\"\\\" $ETC_CFLAGS $KNP_CFLAGS"

AC_SUBST(KNP_CFLAGS)

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
perl/Makefile
dict/Makefile
dict/src/Makefile
])
