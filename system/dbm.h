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

#define DB_open(name, rw, mode) \
  gdbm_open( name, DBM_BLOCK_SIZE, rw, mode, NULL )
#define DB_close(dbf) \
  gdbm_close(dbf)

/*  DBM_store flags  */

#define DBM_INSERT GDBM_INSERT
#define DBM_REPLACE GDBM_REPLACE

#else

#ifdef INTERNAL_HASH

#include "hash.h"

typedef HASH_FILE *DBM_FILE;

/*  functions  */

#define DB_open(name, rw, mode) \
    hash_read_open(name)
#define DB_close(dbf) \
    hash_close(dbf)
#define	db_read_open(name) \
    hash_read_open(name)
#define	db_write_open(name) \
    hash_write_open(name)
#define	db_get(dbf, buf) \
    hash_fetch(dbf, buf)

#else

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <db.h>

typedef DB *DBM_FILE;

/*  functions  */

#define DB_open(name, rw, mode) \
    db_read_open(name)
#define DB_close(dbf) \
    db_close(dbf)

#endif
#endif
