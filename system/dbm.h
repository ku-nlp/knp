/*
  Filename : dbm.h 
  Purpose  : ndbm から gdbm への移植を簡単にするためのヘッダ
  Author   : Mitsunobu SHIMADA<shimada@pine.kuee.kyoto-u.ac.jp>
  Date     : Jan 31 1997
*/
/* $Id$ */

#define DBM_CON_MAX 8192

#define DBM_APPEND	1
#define DBM_OVERRIDE	2
#define DBM_AND		3
#define DBM_OR		4

#ifdef GDBM

/*  for GDBM  */

#include <gdbm.h>

typedef GDBM_FILE DBM_FILE;
#define DBM_BLOCK_SIZE 1024

/*  functions  */

#define DBM_open( name, rw, mode ) \
  gdbm_open( name, DBM_BLOCK_SIZE, rw, mode, NULL )
#define DBM_close( dbf ) \
  gdbm_close( dbf )
#define DBM_fetch( dbf, key ) \
  gdbm_fetch( dbf, key )
#define DBM_store( dbf, key, content, flag ) \
  gdbm_store( dbf, key, content, flag )
#define DBM_delete( dbf, key ) \
  gdbm_delete( dbf, key )
#define DBM_firstkey( dbf ) \
  gdbm_fetch( dbf )
#define DBM_nextkey( dbf, key ) \
  gdbm_fetch( dbf, key )

/*  DBM_store flags  */

#define DBM_INSERT GDBM_INSERT
#define DBM_REPLACE GDBM_REPLACE

#else

#ifdef NDBM

/*  for NDBM  */

#include <ndbm.h>

typedef DBM* DBM_FILE;

/*  functions  */

#define DBM_open( name, rw, mode ) \
  dbm_open( name, rw, mode )
#define DBM_close( dbf ) \
  dbm_close( dbf )
#define DBM_fetch( dbf, key ) \
  dbm_fetch( dbf, key )
#define DBM_store( dbf, key, content, flag ) \
  dbm_store( dbf, key, content, flag )
#define DBM_delete( dbf, key ) \
  dbm_delete( dbf, key )
#define DBM_firstkey( dbf ) \
  dbm_fetch( dbf )
#define DBM_nextkey( dbf, key ) \
  dbm_fetch( dbf, key )

#else

#include <fcntl.h>
#include <db.h>

typedef DB *DBM_FILE;

/*  functions  */

#define DBM_open(name, rw, mode) \
    db_read_open(name)
#define DBM_close(dbf) \
    db_close(dbf)

#endif
#endif

/*  end of dbm.h  */
