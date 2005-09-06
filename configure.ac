# Process this file with autoconf to produce a configure script.
AC_INIT(knp, 2.0, knp@kc.t.u-tokyo.ac.jp)
AC_PREREQ(2.53)
AM_INIT_AUTOMAKE
AC_REVISION($Revision$)
AC_CONFIG_SRCDIR([system/main.c])
AM_CONFIG_HEADER([config.h])


# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL

AC_DISABLE_SHARED
AC_PROG_LIBTOOL
AC_SYS_LARGEFILE

AC_ARG_WITH(gdbm, AC_HELP_STRING([--with-gdbm], [use GDBM library (optional)]))
AC_ARG_WITH(berkeley-db2, AC_HELP_STRING([--with-berkeley-db2], [use Berkeley DB library (2.X) (optional)]))
AC_ARG_WITH(berkeley-db, AC_HELP_STRING([--with-berkeley-db], [use Berkeley DB library (3.X, 4.X) (optional)]))
AC_ARG_WITH(boehm-gc, AC_HELP_STRING([--with-boehm-gc], [use Boehm GC library (optional)]))
AC_ARG_WITH(juman-prefix, AC_HELP_STRING([--with-juman-prefix=PREFIX], [prefix where JUMAN is installed (optional)]), 
			  juman_prefix="$withval")
AC_ARG_WITH(bgh-file, AC_HELP_STRING([--with-bgh-file=FILE], [path where Bunrui Goi Hyou (bunruidb.txt) is located (optional)]), 
			  BGH_FILE="$withval")


packagesrcdir=`cd $srcdir && pwd`

if test "$with_boehm_gc" = yes; then
    KNP_LIBS="$KNP_LIBS -lgc -ldl"
    ETC_CFLAGS="$ETC_CFLAGS -DUSE_BOEHM_GC"
    MAKE_DB_LIBS="$MAKE_DB_LIBS -lgc -ldl"
fi

AC_ARG_ENABLE(debug, AC_HELP_STRING([--enable-debug], [build a debugging version (default=no)]))

AC_MSG_CHECKING([debug version])
if test "x$enable_debug" = xyes; then
    dnl    AC_DEFINE(DEBUG)
    ETC_CFLAGS="-DDEBUG $ETC_CFLAGS"
else
    enable_debug=no
fi
AC_MSG_RESULT([$enable_debug])

KNP_DICT="$datadir/knp/dict"
KNP_RULE="$datadir/knp/rule"
RC_DEFAULT="$sysconfdir/knprc"

# Set RC_DEFAULT in config.h.
if test "x$prefix" = xNONE; then
   prefix="${ac_default_prefix}"
fi
if test "x$exec_prefix" = xNONE; then
   exec_prefix='${prefix}'
fi

eval KNP_DICT=`eval echo "$KNP_DICT"`
eval KNP_RULE=`eval echo "$KNP_RULE"`
eval RC_DEFAULT=`eval echo "$RC_DEFAULT"`

if test x"$juman_prefix" = x; then
    eval juman_prefix=`eval echo "$prefix"`
fi
AC_SUBST(juman_prefix)
JUMAN_CFLAGS="-I$juman_prefix/include"
JUMAN_LIBS="-L$juman_prefix/lib -ljuman"

old_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$JUMAN_CFLAGS $CPPFLAGS"
old_LIBS=$LIBS
LIBS="$JUMAN_LIBS $LIBS"
AC_CHECK_HEADER(juman.h, , AC_MSG_ERROR(juman.h: not found.))
AC_CHECK_LIB(juman, grammar, , AC_MSG_ERROR(libjuman: not found.))
CPPFLAGS=$old_CPPFLAGS
LIBS=$old_LIBS
ETC_CFLAGS="$JUMAN_CFLAGS $ETC_CFLAGS"


# Checks for libraries.

if ( test \( "x$with_berkeley_db2" = xyes -a "x$with_berkeley_db" = xyes \) \
    -o \( "x$with_berkeley_db2" = xyes -a "x$with_gdbm" = xyes \) \
    -o \( "x$with_berkeley_db" = xyes -a "x$with_gdbm" = xyes \) ); then
    AC_MSG_ERROR("More than two DBs are specified.")    
elif ( test "x$with_berkeley_db2" = xyes ); then
    AC_SEARCH_LIBS(db_open, db2 db, , 
		   AC_MSG_ERROR(not found.))
elif ( test "x$with_berkeley_db" = xyes ); then
    AC_CHECK_LIB(db-4.2, db_create, DB_LIBS="-ldb-4.2 $DB_LIBS" ETC_CFLAGS="-DDB41 -DDB3 $ETC_CFLAGS", 
		 AC_SEARCH_LIBS(db_create, db-3 db-4 db, ETC_CFLAGS="-DDB3 $ETC_CFLAGS", 
				AC_MSG_ERROR(not found.)))
elif ( test "x$with_gdbm" = xyes ); then
    AC_CHECK_LIB(gdbm, gdbm_open, DB_LIBS="-lgdbm $DB_LIBS" ETC_CFLAGS="-DGDBM $ETC_CFLAGS", 
	         AC_MSG_ERROR(not found.))
else
    ETC_CFLAGS="-DINTERNAL_HASH $ETC_CFLAGS"    
fi

AC_CHECK_LIB(m, sqrt, ETC_LIBS="$ETC_LIBS -lm")
AC_CHECK_LIB(nsl, gethostbyname, KNP_LIBS="-lnsl $KNP_LIBS")
AC_CHECK_LIB(socket, socket, KNP_LIBS="-lsocket $KNP_LIBS")


KNP_LIBS="$KNP_LIBS $JUMAN_LIBS $DB_LIBS $ETC_LIBS"
MAKE_DB_LIBS="$MAKE_DB_LIBS $DB_LIBS"

MAKE_DB_COMMAND="$packagesrcdir/system/make_db"
CF_COMMAND="$packagesrcdir/system/ipal"

AC_MSG_CHECKING([Bunrui Goi Hyou (BGH)])
if test x"$BGH_FILE" != x; then
    bgh_exist=yes
else
    if test -f "$packagesrcdir/dict/scode/bgh/bgh.dat"; then
	bgh_exist=yes
    elif test -f /share/dict/bgh/monitor/sakuin; then
	bgh_exist=yes
	BGH_FILE=/share/dict/bgh/monitor/sakuin
    else
	bgh_exist=no
	BGH_FILE=
    fi
fi
AC_MSG_RESULT([$bgh_exist])
AM_CONDITIONAL(BGH_EXIST, test "$bgh_exist" = yes)

AC_SUBST(KNP_LIBS)
AC_SUBST(MAKE_DB_LIBS)
AC_SUBST(MAKE_DB_COMMAND)
AC_SUBST(CF_COMMAND)
AC_SUBST(BGH_FILE)

KNP_CFLAGS="-DKNP_DICT=\"\\\"$KNP_DICT\"\\\" -DKNP_RULE=\"\\\"$KNP_RULE\"\\\" -DKNP_RC_DEFAULT=\"\\\"$RC_DEFAULT\"\\\" $ETC_CFLAGS $KNP_CFLAGS"

AC_SUBST(KNP_CFLAGS)

# LDFLAGS="$JUMAN_LDFLAGS $LDFLAGS"


# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(stdlib.h string.h fcntl.h signal.h setjmp.h math.h \
    sys/types.h sys/file.h sys/stat.h unistd.h errno.h time.h ctype.h \
    sys/socket.h netinet/in.h netdb.h pwd.h grp.h windows.h)


# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_UID_T
AC_TYPE_SIZE_T
AC_STRUCT_TM


# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([alarm gethostbyname memset pow socket sqrt strcasecmp strchr strdup strerror strncasecmp strstr])


AC_CONFIG_FILES([
Makefile
system/Makefile
rule/Makefile
doc/Makefile
dict/Makefile
dict/src/Makefile
dict/scode/Makefile
dict/scode/bgh/Makefile
dict/gcf/Makefile
])
AC_OUTPUT
