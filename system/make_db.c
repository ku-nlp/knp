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
#define DBM_KEY_MAX 	2048
#endif
#ifndef DBM_CON_MAX
#define DBM_CON_MAX	6553600
#endif

extern DBM_FILE db_write_open(char *filename);
extern int db_put(DBM_FILE db, char *buf, char *value, char *Separator, int mode);
extern void DB_close(DBM_FILE db);
int OptEncoding;

int content_process(char *content, char **pre_content, int *pre_content_size, 
		    int Type, char *Separator)
{
    int content_len = strlen(content);

    if (Type == DBM_OVERRIDE) {
	if (*pre_content_size == 0) {
	    *pre_content_size = DBM_CON_MAX;
	    *pre_content = (char *)malloc(*pre_content_size);
	}

	while (content_len >= *pre_content_size) {
	    *pre_content = (char *)realloc(*pre_content, *pre_content_size *= 2);
	}
	strcpy(*pre_content, content);
    }
    else if (Type == DBM_APPEND) {
	content_len += strlen(*pre_content);
	if (Separator) {
	    content_len += strlen(Separator);
	}
	while (content_len >= *pre_content_size) {
	    *pre_content = (char *)realloc(*pre_content, *pre_content_size *= 2);
	}
	if (Separator) {
	    strcat(*pre_content, Separator);
	}
	strcat(*pre_content, content);
    }
    else if (Type == DBM_OR) {
	int i;

	for (i = 0; i < strlen(*pre_content); i++) {
	    if (*(content + i) == '1') {
		*(*pre_content + i) = '1';
	    }
	}
    }
    else if (Type == DBM_AND) {
	int i;

	for (i = 0; i < strlen(*pre_content); i++) {
	    if (*(content + i) == '0') {
		*(*pre_content + i) = '0';
	    }
	}
    }
}

int main(int argc, char *argv[])
{
    DBM_FILE db;
    int Type, num, pre_content_size = 0;
    char *Separator = NULL, *cp;
    char key[DBM_KEY_MAX];
    char pre_key[DBM_KEY_MAX], *pre_content;

    char *buffer = (char *)malloc(DBM_CON_MAX);
    char *content = (char *)malloc(DBM_CON_MAX);

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
    } else if (argc == 3 && !strcmp(argv[2], "-z")) {
	Type = DBM_Z;
    } else {
	fprintf(stderr,"usage: %s database_name [-append string|-and|-or|-z]\n",argv[0]);
	exit(1);
    }

    /* データベース作成 */
    db = db_write_open(argv[1]);
    fprintf(stderr, "Create Database <%s>.\n", argv[1]);

    pre_key[0] = '\0';
	
    buffer[DBM_CON_MAX - 1] = '\n';
    num = 0;
    while (fgets(buffer, DBM_CON_MAX, stdin) != NULL) {
	/* 行の長さチェック */
	if (buffer[DBM_CON_MAX - 1] != '\n') {
	    fprintf(stderr, "Line %d is larger than %d bytes.\n", num, DBM_CON_MAX);
	    free(buffer);
	    exit(1);
	}

	/* キーの長さチェック */
	if (cp = strchr(buffer, ' ')) {
	    if (cp - buffer >= DBM_KEY_MAX) {
		fprintf(stderr, "Key is too long (in %s).\n", buffer);
	    free(buffer);
		exit(1);
	    }
	}
	else {
	    /* スペースがないとき (スペースの前に\0を含む場合もひっかかる) */
	    fprintf(stderr, "Line %d is strange.\n", num);
	    if ((num++ % 100000) == 0) fputc('*', stderr);
	    continue;
	}

	/* keyとcontentに分離 */
	sscanf(buffer, "%s %[^\n]", key, content);
	if ((num++ % 100000) == 0) fputc('*', stderr);

	/* 直前のkeyと同じなら連結して保存 */
	if (!strcmp(pre_key, key)) {
	    content_process(content, &pre_content, &pre_content_size, Type, Separator);
	}
	else {
	    /* 書き込み */
	    if (pre_key[0]) {
		db_put(db, pre_key, pre_content, Separator, Type);
	    }

	    strcpy(pre_key, key);
	    content_process(content, &pre_content, &pre_content_size, DBM_OVERRIDE, Separator);
	}
    }

    if (pre_key[0]) {
	db_put(db, pre_key, pre_content, Separator, Type);
    }

    if (pre_content_size > 0) {
	free(pre_content);
    }
    if (Separator) {
	free(Separator);
    }

    fputc('\n', stderr);

    DB_close(db);
    free(buffer);
    free(content);
    return 0;
}
