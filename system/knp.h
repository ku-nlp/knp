/*====================================================================

				knp.h

                                             S.Kurohashi 1999. 4. 1

    $Id$
====================================================================*/

#ifdef USE_SVM
#include <svm.h>
#endif

#ifdef USE_CRF
#include <crf.h>
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef HAVE_SETJMP_H
#include <setjmp.h>
#endif

#ifdef HAVE_MATH_H
#include <math.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef _WIN32
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif
#endif

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#ifdef HAVE_GRP_H
#include <grp.h>
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifdef HAVE_IO_H
#include <io.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_ZLIB_H
#include <zlib.h>
#endif

#include <juman.h>

#include "path.h"
#include "const.h"
#include "extern.h"

#include "distsim_for_knp.h"

#ifdef USE_BOEHM_GC
#include <gc.h>
#define	malloc	GC_malloc
#define	calloc	GC_calloc
#define	realloc	GC_realloc
#define	free
#endif

#ifdef _WIN32
#ifndef strcasecmp
#define strcasecmp stricmp
#endif
#ifndef strncasecmp
#define strncasecmp strnicmp
#endif

#ifndef PACKAGE_NAME
#define PACKAGE_NAME "knp"
#endif

#ifndef VERSION
#define VERSION "5.0"
#endif

#ifndef CF_VERSION
#define CF_VERSION "CF1.1"
#endif
#endif /* _WIN32 */

/*====================================================================
				 END
====================================================================*/
