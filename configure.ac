# Process this file with autoconf to produce a configure script.
AC_INIT(knp, 3.0, knp@kc.t.u-tokyo.ac.jp)
AC_PREREQ(2.53)
AM_INIT_AUTOMAKE
AC_REVISION($Revision$)
AC_CONFIG_SRCDIR([system/main.c])
AM_CONFIG_HEADER([config.h])


# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB
# AC_PROG_MAKE_SET

AC_ARG_WITH(gdbm, AC_HELP_STRING([--with-gdbm], [use GDBM library (default=no)]))
AC_ARG_WITH(berkeley-db2, AC_HELP_STRING([--with-berkeley-db2], [use Berkeley DB library (2.X) (default=no)]))
AC_ARG_WITH(berkeley-db, AC_HELP_STRING([--with-berkeley-db], [use Berkeley DB library (3.X, 4.X) (default=no)]))
AC_ARG_WITH(internal-hash, AC_HELP_STRING([--with-internal-hash], [use internal hash DB (default=yes)]))
AC_ARG_WITH(svm, AC_HELP_STRING([--with-svm], [use TinySVM library (default=no)]))
AC_ARG_WITH(juman-prefix, AC_HELP_STRING([--with-juman-prefix], [prefix where JUMAN is installed (optional)]), 
			  juman_prefix="$withval")
AC_ARG_WITH(bgh-file, AC_HELP_STRING([--with-bgh-file], [path where Bunrui Goi Hyou (sakuin) is located (optional)]), 
			  BGH_FILE="$withval")


SUBDIRS="system rule docs perl dict"

packagesrcdir=`cd $srcdir && pwd`

if test "$with_svm" = yes; then
    SUBDIRS="svm $SUBDIRS"
    SVM_CFLAGS="-I$packagesrcdir/svm -DUSE_SVM"
    SVM_LDFLAGS="-L$packagesrcdir/svm"
    KNP_LIBS="$KNP_LIBS -lsvm -ltinysvm"
fi

if test x"$juman_prefix" != x; then
   JUMAN_CFLAGS="-I$juman_prefix/include"
   JUMAN_LDFLAGS="-L$juman_prefix/lib"
fi

AC_ARG_ENABLE(debug, AC_HELP_STRING([--enable-debug], [build a debugging version (default=no)]))

AC_MSG_CHECKING("debug version")
if test "x$enable_debug" = "xyes"; then
    dnl    AC_DEFINE(DEBUG)
    ETC_CFLAGS="-DDEBUG $ETC_CFLAGS"
else
    enable_debug=no
fi
AC_MSG_RESULT("$enable_debug")


KNP_DIR=$packagesrcdir
KNP_SYS="$KNP_DIR/system"
KNP_DICT="$KNP_DIR/dict"
KNP_RULE="$KNP_DIR/rule"
RCPATH="$packagesrcdir/knprc.sample"


# Checks for libraries.

if ( test \( "x$with_berkeley_db2" = xyes -a "x$with_berkeley_db" = xyes \) \
    -o \( "x$with_berkeley_db2" = xyes -a "x$with_gdbm" = xyes \) \
    -o \( "x$with_berkeley_db" = xyes -a "x$with_gdbm" = xyes \) ); then
    AC_MSG_ERROR("More than two DBs are specified.")    
elif ( test "x$with_berkeley_db2" = xyes ); then
    AC_SEARCH_LIBS(db_open, db2 db, , 
	         AC_MSG_ERROR(not found.))
elif ( test "x$with_berkeley_db" = xyes ); then
    AC_SEARCH_LIBS(db_create, db-3 db-4 db, ETC_CFLAGS="-DDB3 $ETC_CFLAGS", 
	         AC_MSG_ERROR(not found.))
elif ( test "x$with_gdbm" = xyes ); then
    AC_CHECK_LIB(gdbm, gdbm_open, DB_LIBS="-lgdbm $DB_LIBS" ETC_CFLAGS="-DGDBM $ETC_CFLAGS", 
	         AC_MSG_ERROR(not found.))
else
    ETC_CFLAGS="-DINTERNAL_HASH $ETC_CFLAGS"    
fi

AC_CHECK_LIB(m, sqrt, ETC_LIBS="$ETC_LIBS -lm")
AC_CHECK_LIB(nsl, gethostbyname, KNP_LIBS="-lnsl $KNP_LIBS")
AC_CHECK_LIB(socket, socket, KNP_LIBS="-lsocket $KNP_LIBS")

KNP_LIBS="$KNP_LIBS $DB_LIBS $ETC_LIBS"
MAKE_DB_LIBS="$MAKE_DB_LIBS $DB_LIBS"

MAKE_DB_COMMAND="$packagesrcdir/system/make_db"
CF_COMMAND="$packagesrcdir/system/ipal"

if test x"$BGH_FILE" = x; then
    BGH_FILE=/share/dict/bgh/monitor/sakuin
fi

AC_SUBST(KNP_LIBS)
AC_SUBST(MAKE_DB_LIBS)
AC_SUBST(MAKE_DB_COMMAND)
AC_SUBST(CF_COMMAND)
AC_SUBST(BGH_FILE)

KNP_CFLAGS="$SVM_CFLAGS $JUMAN_CFLAGS -DKNP_SYS=\"\\\"$KNP_SYS\"\\\" -DKNP_DICT=\"\\\"$KNP_DICT\"\\\" -DKNP_RULE=\"\\\"$KNP_RULE\"\\\" -DKNP_RC_DEFAULT=\"\\\"$RCPATH\"\\\" $ETC_CFLAGS $KNP_CFLAGS"

AC_SUBST(KNP_CFLAGS)

LDFLAGS="$SVM_LDFLAGS $JUMAN_LDFLAGS $LDFLAGS"


# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(stdlib.h string.h fcntl.h signal.h setjmp.h math.h \
    sys/types.h sys/file.h sys/stat.h unistd.h errno.h time.h ctype.h getopt.h \
    sys/socket.h netinet/in.h netdb.h windows.h)


# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_STRUCT_TM


# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([alarm getcwd gethostbyname memset pow socket sqrt strcasecmp strchr strdup strerror strncasecmp strstr])


AC_SUBST(SUBDIRS)

AC_CONFIG_FILES([
Makefile
system/Makefile
svm/Makefile
rule/Makefile
docs/Makefile
perl/Makefile
dict/Makefile
dict/src/Makefile
dict/scode/Makefile
dict/scode/bgh/Makefile
dict/scode/ntt/Makefile
dict/ebcf/Makefile
dict/gcf/Makefile
])
AC_OUTPUT
