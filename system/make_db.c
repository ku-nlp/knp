/*====================================================================

 	データベース作成，登録
						S.Kurohashi 92/11/05

	Usage: make_db database_name [-append string|-and|-or]

	-append  同一キーがあった場合 append (default)
		 (stringは区切り文字列 defaultはなし)
	-and     同一キーがあった場合 merge  (データは0-1ベクトル)
	-or      同一キーがあった場合 merge  (データは0-1ベクトル)

====================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dbm.h"

#ifndef DBM_KEY_MAX
#define DBM_KEY_MAX 	1024
#endif
#ifndef DBM_CON_MAX
#define DBM_CON_MAX 	8192
#endif

extern DBM_FILE db_write_open(char *filename);
extern int db_put(DBM_FILE db, char *buf, char *value, char *Separator, int mode);
extern void db_close(DBM_FILE db);

int main(int argc, char *argv[])
{
    DBM_FILE db;
    int Type, num;
    char *Separator = NULL;
    char key[DBM_KEY_MAX], content[DBM_CON_MAX];
    char buffer[DBM_CON_MAX];

    if (argc == 2) {
	Type = DBM_APPEND;
    } else if (argc == 3 && !strcmp(argv[2], "-append")) {
	Type = DBM_APPEND;
    } else if (argc == 4 && !strcmp(argv[2], "-append")) {
	Type = DBM_APPEND;
	Separator = strdup(argv[3]);
    } else if (argc == 3 && !strcmp(argv[2], "-and")) {
	Type = DBM_AND;
    } else if (argc == 3 && !strcmp(argv[2], "-or")) {
	Type = DBM_OR;
    } else {
	fprintf(stderr,"usage: %s database_name [-append string|-and|-or]\n",argv[0]);
	exit(1);
    }

    /* データベース作成 */
    db = db_write_open(argv[1]);
    fprintf(stderr, "Create Database <%s>.\n", argv[1]);

    num = 0;
    while (fgets(buffer, DBM_CON_MAX, stdin) != NULL) {

	sscanf(buffer, "%s %[^\n]", key, content);

	if ((num++ % 1000) == 0) fputc('*', stderr);

	db_put(db, key, content, Separator, Type);
    }
    fputc('\n', stderr);

    db_close(db);
    return 0;
}
