/*====================================================================

			       Database

                                               S.Kurohashi 1999. 4. 1

    $Id$
====================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef GDBM

#include <gdbm.h>
typedef GDBM_FILE DBM_FILE;

/* DB open for reading */
DBM_FILE db_read_open(char *filename)
{
    DBM_FILE db;

    if (!(db = gdbm_open(filename, 1024, GDBM_READER, 0444, 0))) {
        fprintf(stderr, "db_read_open: %s: %s\n", filename, (char *)strerror(errno));
        exit(1);
    }
    return db;
}

/* DB open for creating */
DBM_FILE db_write_open(char *filename)
{
    DBM_FILE db;

    if (!(db = gdbm_open(filename, 1024, GDBM_NEWDB, 0644, 0))) {
        fprintf(stderr, "db_write_open: %s: %s\n", filename, (char *)strerror(errno));
        exit(1);
    }
    return db;
}

/* DB close */
void db_close(DBM_FILE db)
{
    gdbm_close(db);
}

/* DB list */
void db_list(DBM_FILE db)
{
    datum content, key, nextkey;

    key = gdbm_firstkey(db);
    while (key.dptr) {
	content = gdbm_fetch(db, key);
	*(key.dptr+key.dsize) = '\0';
	*(content.dptr+content.dsize) = '\0';
	fprintf(stdout, "%s %s\n", key.dptr, content.dptr);
	free(content.dptr);
	nextkey = gdbm_nextkey(db, key);
	free(key.dptr);
	key = nextkey;
    }
}

/* DB get */
char *db_get(DBM_FILE db, char *buf)
{
    datum content, key;

    key.dptr = buf;
    key.dsize = strlen(buf);
    content = gdbm_fetch(db, key);
    if (content.dptr) {
	*(content.dptr+content.dsize) = '\0';
	return content.dptr;
    }
    return NULL;
}

#else

#include <fcntl.h>
#include <db.h>
typedef DB *DBM_FILE;

/* DB open for reading */
DBM_FILE db_read_open(char *filename)
{
    DB_INFO dbinfo;
    DBM_FILE db;

    memset(&dbinfo, 0, sizeof(dbinfo));
    dbinfo.db_cachesize = 1048576;

    if ((errno = db_open(filename, DB_HASH, DB_RDONLY, 0444, NULL, &dbinfo, &db))) {
        fprintf(stderr, "db_read_open: %s: %s\n", filename, (char *)strerror(errno));
        exit(1);
    }
    return db;
}

/* DB open for creating */
DBM_FILE db_write_open(char *filename)
{
    DB_INFO dbinfo;
    DBM_FILE db;

    memset(&dbinfo, 0, sizeof(dbinfo));
    dbinfo.db_cachesize = 1048576;

    if ((errno = db_open(filename, DB_HASH, DB_CREATE | DB_TRUNCATE, 0644, NULL, &dbinfo, &db))) {
        fprintf(stderr, "db_read_open: %s: %s\n", filename, (char *)strerror(errno));
        exit(1);
    }
    return db;
}

/* DB close */
void db_close(DBM_FILE db)
{
    if ((errno = db->close(db, 0))) {
	fprintf(stderr, "db->close : %s\n", (char *)strerror(errno));
        exit(1);
    }
}

/* DB list */
void db_list(DBM_FILE db)
{
    DBC *dbc;
    DBT content, key;

    if ((errno = db->cursor(db, NULL, &dbc, 0))) {
        fprintf(stderr, "db_list : %s\n", strerror(errno));
	exit(1);
    }

    memset(&key, 0, sizeof(key));
    memset(&content, 0, sizeof(content));

    while ((errno = dbc->c_get(dbc, &key, &content, DB_NEXT)) == 0) {	
	fprintf(stdout, "%.*s %.*s\n", (int)key.size, (char *)key.data, 
		(int)content.size, (char *)content.data);
    }
    dbc->c_close(dbc);
}

/* DB get */
char *db_get(DBM_FILE db, char *buf)
{
    DBT content, key;

    memset(&key, 0, sizeof(DBT));
    memset(&content, 0, sizeof(DBT));

    key.data = buf;
    key.size = strlen(key.data);
    errno = db->get(db, NULL, &key, &content, 0);

    if (errno == DB_NOTFOUND)
	return NULL;
    else if (errno) {
        fprintf(stderr, "db_get: %s\n", (char *)strerror(errno));
        exit(1);
    }
    *((char *)content.data+content.size) = '\0';
    return content.data;
}

#endif
