# Process this file with autoconf to produce a configure script.
AC_INIT(knp, m4_esyscmd([tr -d '\n' < VERSION]), nl-resource@nlp.ist.i.kyoto-u.ac.jp)
AC_PREREQ(2.53)
AC_REVISION($Revision$)
AC_CONFIG_SRCDIR([system/main.c])
AC_CONFIG_SUBDIRS([CRF++-0.58])
AM_INIT_AUTOMAKE(${PACKAGE_NAME}, ${PACKAGE_VERSION})
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
AC_ARG_WITH(tokyo-cabinet, AC_HELP_STRING([--with-tokyo-cabinet], [use Tokyo Cabinet library]))
AC_ARG_WITH(svm, AC_HELP_STRING([--with-svm], [use TinySVM library (optional)]))
AC_ARG_WITH(svm-light, AC_HELP_STRING([--with-svm-light], [use SVM_light library (optional)]))
AC_ARG_WITH(boehm-gc, AC_HELP_STRING([--with-boehm-gc], [use Boehm GC library (optional)]))
AC_ARG_WITH(juman-prefix, AC_HELP_STRING([--with-juman-prefix=PREFIX], [prefix where JUMAN is installed (optional)]), 
			  juman_prefix="$withval")
AC_ARG_WITH(included-juman, AC_HELP_STRING([--with-included-juman], [use included JUMAN (optional)]))
AC_ARG_WITH(bgh-file, AC_HELP_STRING([--with-bgh-file=FILE], [path where Bunrui Goi Hyou (bunruidb.txt) is located (optional)]), 
			  BGH_FILE="$withval")


SUBDIRS="system rule doc dict"

packagesrcdir=`cd $srcdir && pwd`

if test "$with_svm" = yes; then
    SUBDIRS="svm $SUBDIRS"
    SVM_CFLAGS="-I$packagesrcdir/svm -DUSE_SVM"
    KNP_LIBS="$KNP_LIBS $packagesrcdir/svm/libsvm.la -ltinysvm -lstdc++"
    LIBSVM_VERSION_INFO="0:0:1"
    AC_SUBST(LIBSVM_VERSION_INFO)

    AC_MSG_CHECKING([SVM Model File])
    if test -f "$packagesrcdir/svm/anaphora-ga.model"; then
	svm_model_exist=yes
    else
	svm_model_exist=no
    fi
    AC_MSG_RESULT([$svm_model_exist])
elif test "$with_svm_light" = yes; then
    SUBDIRS="svm_light $SUBDIRS"
    SVM_CFLAGS="-I$packagesrcdir/svm_light -DUSE_SVM"
    KNP_LIBS="$KNP_LIBS $packagesrcdir/svm_light/libsvm.la -lsvmlight"
    LIBSVM_VERSION_INFO="0:0:1"
    AC_SUBST(LIBSVM_VERSION_INFO)
fi
AM_CONDITIONAL(HAVE_TINYSVM, test "x$with_svm" = xyes)
AM_CONDITIONAL(HAVE_SVMLIGHT, test "x$with_svm_light" = xyes)
AM_CONDITIONAL(SVM_MODEL_EXIST, test "x$svm_model_exist" = xyes)

SUBDIRS="crf $SUBDIRS"
CRF_CFLAGS="-I$packagesrcdir/CRF++-0.58 -I$packagesrcdir/crf -DUSE_CRF"
KNP_LIBS="$KNP_LIBS $packagesrcdir/crf/libcrf.la $packagesrcdir/CRF++-0.58/.libs/libcrfpp.a"
LIBCRF_VERSION_INFO="0:0:1"
AC_SUBST(LIBCRF_VERSION_INFO)
crf_model_exist=yes
AM_CONDITIONAL(CRF_MODEL_EXIST, test "x$crf_model_exist" = xyes)

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

use_included_juman=no
if test x"$juman_prefix" = x; then
   eval juman_prefix=`eval echo "$prefix"`
   if test ! -f "$prefix/include/juman.h" -o "$with_included_juman" = yes; then
      AC_CONFIG_SUBDIRS([juman])
      JUMAN_CFLAGS="$JUMAN_CFLAGS -I$packagesrcdir/juman/lib"
      JUMAN_LIBS="$JUMAN_LIBS $packagesrcdir/juman/lib/.libs/libjuman.a"
      use_included_juman=yes
   fi
fi
AC_SUBST(juman_prefix)
if test x"$use_included_juman" = xno; then
   JUMAN_CFLAGS="$JUMAN_CFLAGS -I$juman_prefix/include"
   JUMAN_LIBS="$JUMAN_LIBS -L$juman_prefix/lib -ljuman"
fi

old_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$JUMAN_CFLAGS $CPPFLAGS"
old_LIBS=$LIBS
LIBS="$JUMAN_LIBS $LIBS"
AC_CHECK_HEADER(juman.h, , AC_MSG_ERROR(juman.h: not found.))
# AC_CHECK_LIB(juman, grammar, , AC_MSG_ERROR(libjuman: not found.))
CPPFLAGS=$old_CPPFLAGS
LIBS=$old_LIBS
ETC_CFLAGS="$JUMAN_CFLAGS $ETC_CFLAGS"


# Checks for libraries.

if ( test \( "x$with_berkeley_db2" = xyes -a "x$with_berkeley_db" = xyes \) \
    -o \( "x$with_berkeley_db2" = xyes -a "x$with_cdb" = xyes \) \
    -o \( "x$with_berkeley_db" = xyes -a "x$with_cdb" = xyes \) \
    -o \( "x$with_berkeley_gdbm" = xyes -a "x$with_cdb" = xyes \) \
    -o \( "x$with_berkeley_db2" = xyes -a "x$with_gdbm" = xyes \) \
    -o \( "x$with_berkeley_db" = xyes -a "x$with_gdbm" = xyes \) ); then
    AC_MSG_ERROR("More than two DBs are specified.")    
elif ( test "x$with_berkeley_db2" = xyes ); then
    AC_SEARCH_LIBS(db_open, db2 db, , 
		   AC_MSG_ERROR(not found.))
elif ( test "x$with_berkeley_db" = xyes ); then
    AC_CHECK_LIB(db-4.3, db_create, DB_LIBS="-ldb-4.3 $DB_LIBS" ETC_CFLAGS="-DDB41 -DDB3 $ETC_CFLAGS", 
		 AC_SEARCH_LIBS(db_create, db-3 db-4 db, ETC_CFLAGS="-DDB3 $ETC_CFLAGS", 
				AC_MSG_ERROR(not found.)))
elif ( test "x$with_gdbm" = xyes ); then
    AC_CHECK_LIB(gdbm, gdbm_open, DB_LIBS="-lgdbm $DB_LIBS" ETC_CFLAGS="-DGDBM $ETC_CFLAGS", 
	         AC_MSG_ERROR(not found.))
elif ( test "x$with_intdb" = xyes ); then
    ETC_CFLAGS="-DINTERNAL_HASH $ETC_CFLAGS"
elif ( test "x$with_tokyo_cabinet" = xyes ); then
    AC_CHECK_LIB(tokyocabinet, tchdbnew, DB_LIBS="-ltokyocabinet $DB_LIBS" ETC_CFLAGS="-DTOKYO_CABINET $ETC_CFLAGS", 
	         AC_MSG_ERROR(not found.))
else
    DB_LIBS="-L$packagesrcdir/cdb -lcdb $DB_LIBS"
    ETC_CFLAGS="-DCDB -I$packagesrcdir/cdb $ETC_CFLAGS"
fi

AC_CHECK_LIB(m, sqrt, ETC_LIBS="$ETC_LIBS -lm")
AC_CHECK_LIB(nsl, gethostbyname, KNP_LIBS="-lnsl $KNP_LIBS")
AC_CHECK_LIB(socket, socket, KNP_LIBS="-lsocket $KNP_LIBS")
AC_CHECK_LIB(z, deflate, DB_LIBS="$DB_LIBS -lz")

ETC_LIBS="$ETC_LIBS $packagesrcdir/distsim/libdistsim.la -lstdc++"
KNP_LIBS="$KNP_LIBS $JUMAN_LIBS $ETC_LIBS $DB_LIBS"
MAKE_DB_LIBS="$MAKE_DB_LIBS $DB_LIBS"

MAKE_DB_COMMAND="$packagesrcdir/system/make_db"
CF_COMMAND="$packagesrcdir/system/ipal"

AC_MSG_CHECKING([Bunrui Goi Hyou (BGH)])
if test x"$BGH_FILE" != x; then
    bgh_exist=yes
else
    if test -f "$packagesrcdir/dict/scode/bgh/bgh.orig"; then
	bgh_exist=yes
    else
	bgh_exist=no
	BGH_FILE=
    fi
fi
AC_MSG_RESULT([$bgh_exist])

AC_MSG_CHECKING([Case Frame])
if test -f "$packagesrcdir/dict/ebcf/cf.knpdict"; then
    cf_exist=yes
elif test -f "$packagesrcdir/dict/ebcf/cf.knpdict.xz"; then
    cf_exist=yes
elif test -f "$packagesrcdir/dict/ebcf/cf.dat" -a -f "$packagesrcdir/dict/ebcf/cf.db" ; then
    cf_exist=yes
else
    AC_MSG_ERROR("$packagesrcdir/dict/ebcf/cf.knpdict{.xz}: not found")
    cf_exist=no
fi
AC_MSG_RESULT([$cf_exist])

AC_MSG_CHECKING([Case Frame (noun)])
if test -f "$packagesrcdir/dict/ebcf/noun.knpdict"; then
    cf_noun_exist=yes
elif test -f "$packagesrcdir/dict/ebcf/noun.knpdict.xz"; then
    cf_noun_exist=yes
elif test -f "$packagesrcdir/dict/ebcf/noun.dat" -a -f "$packagesrcdir/dict/ebcf/noun.db" ; then
    cf_noun_exist=yes
else
    AC_MSG_ERROR("$packagesrcdir/dict/ebcf/noun.knpdict{.xz}: not found")
    cf_noun_exist=no
fi
AC_MSG_RESULT([$cf_noun_exist])

AC_MSG_CHECKING([Probabilities for case analysis])
if test -f "$packagesrcdir/dict/ebcf/case.prob"; then
    prob_exist=yes
elif test -f "$packagesrcdir/dict/ebcf/case.prob.xz"; then
    prob_exist=yes
elif test -f "$packagesrcdir/dict/ebcf/case.db"; then
    prob_exist=yes
else
    AC_MSG_ERROR("$packagesrcdir/dict/ebcf/case.prob{.xz}: not found")
    prob_exist=no
fi
AC_MSG_RESULT([$prob_exist])

AC_MSG_CHECKING([Additional probabilities for case analysis])
if test -f "$packagesrcdir/dict/ebcf/renyou.prob"; then
    additional_prob_exist=yes
else
#    AC_MSG_ERROR("$packagesrcdir/dict/ebcf/renyou.prob: not found")
    additional_prob_exist=no
fi
AC_MSG_RESULT([$additional_prob_exist])

AC_MSG_CHECKING([noun-verb MI DB])
if test -f "$packagesrcdir/dict/auto/nv_mi.dat"; then
    nv_mi_exist=yes
else
#    AC_MSG_ERROR("$packagesrcdir/dict/auto/nv_mi.dat: not found")
    nv_mi_exist=no
fi
AC_MSG_RESULT([$nv_mi_exist])

AC_MSG_CHECKING([distributional similarity MI DB])
if test -f "$packagesrcdir/dict/distsim/mi.dat"; then
    distsim_mi_exist=yes
elif test -f "$packagesrcdir/dict/distsim/mi.dat.xz"; then
    distsim_mi_exist=yes
elif test -f "$packagesrcdir/dict/distsim/mi.db"; then
    distsim_mi_exist=yes
else
    AC_MSG_ERROR("$packagesrcdir/dict/distsim/mi.dat{.xz}: not found")
    distsim_mi_exist=no
fi
AC_MSG_RESULT([$distsim_mi_exist])

AC_MSG_CHECKING([NE CRF model])
if test -f "$packagesrcdir/dict/crf.model"; then
    ne_crf_model_exist=yes
else
    AC_MSG_ERROR("$packagesrcdir/dict/crf.model: not found")
    ne_crf_model_exist=no
fi
AC_MSG_RESULT([$ne_crf_model_exist])

AM_CONDITIONAL(BGH_EXIST, test "$bgh_exist" = yes)
AM_CONDITIONAL(CF_EXIST, test "$cf_exist" = yes)
AM_CONDITIONAL(CF_NOUN_EXIST, test "$cf_noun_exist" = yes)
AM_CONDITIONAL(PROB_EXIST, test "$prob_exist" = yes)
AM_CONDITIONAL(ADDITIONAL_PROB_EXIST, test "$additional_prob_exist" = yes)
AM_CONDITIONAL(NV_MI_EXIST, test "$nv_mi_exist" = yes)
AM_CONDITIONAL(DISTSIM_MI_EXIST, test "$distsim_mi_exist" = yes)
AM_CONDITIONAL(NE_CRF_MODEL_EXIST, test "$ne_crf_model_exist" = yes)
AM_CONDITIONAL(USE_INCLUDED_JUMAN, test "$use_included_juman" = yes)

AC_SUBST(KNP_LIBS)
AC_SUBST(MAKE_DB_LIBS)
AC_SUBST(MAKE_DB_COMMAND)
AC_SUBST(CF_COMMAND)
AC_SUBST(BGH_FILE)

ETC_CFLAGS="$ETC_CFLAGS -I$packagesrcdir/distsim"
KNP_CFLAGS="$SVM_CFLAGS $CRF_CFLAGS -DKNP_DICT=\"\\\"$KNP_DICT\"\\\" -DKNP_RULE=\"\\\"$KNP_RULE\"\\\" -DKNP_RC_DEFAULT=\"\\\"$RC_DEFAULT\"\\\" $ETC_CFLAGS $KNP_CFLAGS"

AC_SUBST(KNP_CFLAGS)

# LDFLAGS="$JUMAN_LDFLAGS $LDFLAGS"


# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(stdio.h stdlib.h string.h fcntl.h signal.h setjmp.h math.h \
    sys/types.h sys/file.h sys/stat.h unistd.h errno.h time.h ctype.h \
    sys/socket.h netinet/in.h netdb.h pwd.h grp.h windows.h winsock2.h limits.h zlib.h)
# link ws2_32 on Windows
AC_CHECK_HEADER(winsock2.h, KNP_LIBS="-lws2_32 $KNP_LIBS")


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


AC_SUBST(SUBDIRS)

AC_CONFIG_FILES([
Makefile
system/Makefile
svm/Makefile
svm_light/Makefile
crf/Makefile
distsim/Makefile
rule/Makefile
chinese_rule/Makefile
doc/Makefile
dict/Makefile
dict/src/Makefile
dict/scode/Makefile
dict/scode/bgh/Makefile
dict/ebcf/Makefile
dict/gcf/Makefile
dict/auto/Makefile
dict/synonym/Makefile
dict/distsim/Makefile
])
AC_OUTPUT
