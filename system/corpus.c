/*====================================================================

			     コーパス関連

                                             S.Kurohashi 1999. 4. 1

    $Id$
====================================================================*/
#include "knp.h"

/* Server Client extention */
extern FILE  *Infp;
extern FILE  *Outfp;
extern int   OptMode;

static char buffer[DATA_LEN];
char CorpusComment[BNST_MAX][DATA_LEN];
static DBM_FILE c_db, cc_db, op_db, cp_db;

extern char *ClauseDBname;
extern char *ClauseCDBname;
extern char *CasePredicateDBname;
extern char *OptionalCaseDBname;

typedef struct {
    int Value[BNST_MAX][BNST_MAX];
    char *Type1[BNST_MAX];
    char *Type2[BNST_MAX];
} EtcMatrix;

EtcMatrix BarrierMatrix;

#ifdef BERKELEY_DB_V2

/* データベースを開く関数 */
int db_read_open(char *filename, DBM_FILE *db) {
    DB_INFO dbinfo;

    memset(&dbinfo, 0, sizeof(dbinfo));
    dbinfo.db_cachesize = 1048576;

    if ((errno = db_open(filename, DB_HASH, DB_RDONLY, 0444, NULL, &dbinfo, db))) {
        fprintf(stderr, "db_open : %s : %s\n", filename, (char *)strerror(errno));
        exit(1);
    }
    return errno;
}

/* データベースを閉じる関数 */
void db_close(DBM_FILE db) {
    if ((errno = db->close(db, 0))) {
	fprintf(stderr, "db->close : %s\n", (char *)strerror(errno));
        exit(1);
    }
}

int dbfetch(DBM_FILE db, char *buf) {
    DBT content, key;
    int count = 0;

    /* 初期化 */
    memset(&key, 0, sizeof(DBT));
    memset(&content, 0, sizeof(DBT));

    key.data = buf;
    key.size = strlen(buf);
    errno = db->get(db, NULL, &key, &content, NULL);
    if (!errno) {
	if (content.size >= DATA_LEN) {
	    fprintf(stderr, "dbfetch: content length overflow.\n");
	    exit(1);
	}
	strncpy(buffer, content.data, content.size);
	buffer[content.size] = '\0';
	count = atoi(buffer);
    }
    return count;
}

#else

DBM_FILE db_read_open(char *filename) {
    DBM_FILE db;
    if (!(db = gdbm_open(filename, 512, GDBM_READER, 0444, 0))) {
        fprintf(stderr, "データベース (%s) が開けません。\n", filename);
        exit(1);
    }
    return db;
}

DBM_FILE db_write_open(char *filename) {
    DBM_FILE db;
    if (!(db = gdbm_open(filename, 512, GDBM_NEWDB, 0644, 0))) {
        fprintf(stderr, "データベース (%s) が開けません。\n", filename);
        exit(1);
    }
    return db;
}

void db_close(DBM_FILE db) {
    gdbm_close(db);
}

int dbfetch(DBM_FILE db, char *buf) {
    datum content, key;
    int count = 0;

    key.dptr = buf;
    key.dsize = strlen(buf);
    content = gdbm_fetch(db, key);
    if (content.dptr) {
	if (content.dsize >= DATA_LEN) {
	    fprintf(stderr, "dbfetch: content length overflow.\n");
	    exit(1);
	}
	*(content.dptr+content.dsize) = '\0';
	count = atoi(content.dptr);
    }
    return count;
}

char *dbfetch_string(DBM_FILE db, char *buf) {
    datum content, key;

    key.dptr = buf;
    key.dsize = strlen(buf);
    content = gdbm_fetch(db, key);
    if (content.dptr) {
	if (content.dsize >= DATA_LEN) {
	    fprintf(stderr, "dbfetch_string: content length overflow.\n");
	    exit(1);
	}
	*(content.dptr+content.dsize) = '\0';
	return content.dptr;
    }
    return NULL;
}

#endif

int init_clause() {
    int i;
    buffer[DATA_LEN-1] = '\n';
    for (i = 0; i < BNST_MAX; i++)
	CorpusComment[i][DATA_LEN-1] = '\n';
#ifdef BERKELEY_DB_V2
    if (!(OptInhibit & OPT_INHIBIT_C_CLAUSE)) {
	if (ClauseCDBname)
	    db_read_open(ClauseCDBname, &cc_db);
	else
	    db_read_open(CLAUSE_CDB_NAME, &cc_db);
    }
    if (ClauseDBname)
	return db_read_open(ClauseDBname, &c_db);
    else
	return db_read_open(CLAUSE_DB_NAME, &c_db);
#else
    if (!(OptInhibit & OPT_INHIBIT_C_CLAUSE)) {
	if (ClauseCDBname)
	    cc_db = db_read_open(ClauseCDBname);
	else
	    cc_db = db_read_open(CLAUSE_CDB_NAME);
    }
    if (ClauseDBname)
	c_db = db_read_open(ClauseDBname);
    else
	c_db = db_read_open(CLAUSE_DB_NAME);
    return TRUE;
#endif
}

void close_clause() {
    db_close(c_db);
    if (!(OptInhibit & OPT_INHIBIT_C_CLAUSE))
	db_close(cc_db);
}

/* 述語節間の係り受け頻度を調べる関数 */
int corpus_clause_comp(BNST_DATA *ptr1, BNST_DATA *ptr2, int para_flag) {
    char *type1, *type2, *cp, *token, *type;
    char parallel1, parallel2, touten1, touten2, touten;
    int score, offset, i;

    /* para_flag == TRUE  : 並列を考慮 (並列解析まえの呼び出しでは意味がない)
       para_flag == FALSE : 並列を無視 */

    /* 初期化 */
    parallel1 = ' ';
    parallel2 = ' ';
    touten1 = ' ';
    touten2 = ' ';

    /* 並列の設定 */
    if (para_flag == TRUE) {
	if (ptr1->para_key_type != PARA_KEY_O)
	    parallel1 = 'P';
	/* if (ptr2->para_key_type != PARA_KEY_O)
	    parallel2 = 'P'; */
    }

    /* 述語タイプの設定 */
    type1 = (char *)check_feature(ptr1->f, "ID");
    type2 = (char *)check_feature(ptr2->f, "ID");

    if (!type1) return TRUE;
    if (!type2) return FALSE;

    /* 係り先の読点を見るためのオフセット */
    if (!strcmp(type2+3, "〜（ため）"))
	offset = 1;
    else
	offset = 0;

    /* 読点の設定 */
    if ((char *)check_feature(ptr1->f, "読点"))
	touten1 = ',';
    if ((char *)check_feature((ptr2+offset)->f, "読点"))
	touten2 = ',';

    /* データベースの検索 */
    sprintf(buffer, "%s%c%c %s%c%c", 
	    type1+3, parallel1, touten1, 
	    type2+3, parallel2, touten2);
    if (buffer[DATA_LEN-1] != '\n') {
	fprintf(stderr, "corpus_clause_comp: data length overflow.\n");
	exit(1);
    }
    score = dbfetch(c_db, buffer);

    if (!(OptInhibit & OPT_INHIBIT_C_CLAUSE) && (para_flag == TRUE) && 
	parallel1 == ' ') {
	cp = dbfetch_string(cc_db, buffer);
	token = strtok(cp, "|");
	while (token) {
	    for (i = ptr1->num+1; i < ptr2->num; i++) {
		type = (char *)check_feature((bnst_data+i)->f, "ID");
		if (type) {
		    if ((char *)check_feature((bnst_data+i)->f, "読点"))
			touten = ',';
		    else
			touten = ' ';
		    sprintf(buffer, "%s %c", type+3, touten);
		    if (!strcmp(token, buffer)) {
			fprintf(Outfp, ";;;(D) %2d %2d %s\n", ptr1->num, i, buffer);
			Dpnd_matrix[ptr1->num][i] = 0;
		    }
		}
	    }
	    token = strtok(NULL, "|");
	}
    }
    /* free(cp); */

    /* デバッグ出力 */
    if (OptCheck != TRUE) {
	if (para_flag == TRUE)
	    fprintf(Outfp, ";;; %2d %2d %s%c%c %s%c%c->%d\n", ptr1->num, ptr2->num, type1+3, parallel1, touten1, 
		    type2+3, parallel2, touten2, score);
	else
	    fprintf(Outfp, ";;;(R) %s %c %s %c->%d\n", type1+3, touten1, 
		    type2+3, touten2, score);
    }


    if (score) {
	if (score == 1) {
	    if (!CorpusComment[ptr1->num][0])
		sprintf(CorpusComment[ptr1->num], "%s%c %s%c", type1+3, touten1, type2+3, touten2);
	    if (CorpusComment[ptr1->num][DATA_LEN-1] != '\n') {
		fprintf(stderr, "corpus_clause_comp: data length overflow(2).\n");
		exit(1);
	    }
	    return CORPUS_POSSIBILITY_1;
	}
	else
	    return TRUE;
    }
    else {
	/* 並列のときでスコアがないなら条件緩和して並列を無視 */
	if (parallel1 == 'P')
	    return corpus_clause_comp((BNST_DATA *)ptr1, 
				      (BNST_DATA *)ptr2, 
				      FALSE);
	else
	    return FALSE;
    }
}

int init_case_pred() {
    int i, j;
    buffer[DATA_LEN-1] = '\n';
    for (i = 0; i < BNST_MAX; i++) {
	BarrierMatrix.Type1[i] = NULL;
	BarrierMatrix.Type2[i] = NULL;
	CorpusComment[i][DATA_LEN-1] = '\n';
	for (j = i+1; j < BNST_MAX; j++) {
	    BarrierMatrix.Value[i][j] = -1;
	}
    }
#ifdef BERKELEY_DB_V2
    if (CasePredicateDBname)
	return db_read_open(CasePredicateDBname, &cp_db);
    else
	return db_read_open(CASE_PRED_DB_NAME, &cp_db);
#else
    if (CasePredicateDBname)
	cp_db = db_read_open(CasePredicateDBname);
    else
	cp_db = db_read_open(CASE_PRED_DB_NAME);
    return TRUE;
#endif
}

void close_case_pred() {
    db_close(cp_db);
}

/* 格から述語への係り受け頻度を調べる関数 */
int corpus_case_predicate_check(BNST_DATA *ptr1, BNST_DATA *ptr2) {
    char *type1, *type2;
    char parallel1, touten1, touten2;
    int score, scorep, offset, para_flag = 0;

    /* 初期化 */
    parallel1 = ' ';
    touten1 = ' ';
    touten2 = ' ';

    /* 並列の設定 */
    if (ptr1->para_key_type != PARA_KEY_O)
	parallel1 = 'P';

    /* 格と述語タイプの設定 */
    type1 = (char *)check_feature(ptr1->f, "係");
    type2 = (char *)check_feature(ptr2->f, "ID");

    if (!type1) return FALSE;
    if (!type2) return FALSE;

    /* 係り先の読点を見るためのオフセット */
    if (!strcmp(type2+3, "〜（ため）"))
	offset = 1;
    else
	offset = 0;

    /* 読点の設定 */
    if ((char *)check_feature(ptr1->f, "読点"))
	touten1 = ',';
    if ((char *)check_feature((ptr2+offset)->f, "読点"))
	touten2 = ',';

    /* データベースの検索(越える例) */
    sprintf(buffer, "!%s%c%c %s %c", type1+3, parallel1, touten1, type2+3, touten2);
    if (buffer[DATA_LEN-1] != '\n') {
	fprintf(stderr, "corpus_case_predicate_check: data length overflow.\n");
	exit(1);
    }
    score = dbfetch(cp_db, buffer);

    /* 並列を考慮しないとき */
    if (!para_flag) {
	if (parallel1 == 'P')
	    sprintf(buffer, "!%s %c %s %c", type1+3, touten1, type2+3, touten2);
	else
	    sprintf(buffer, "!%sP%c %s %c", type1+3, touten1, type2+3, touten2);
	scorep = dbfetch(cp_db, buffer);
	score += scorep;
    }

    /* デバッグ出力 */
    fprintf(Outfp, ";;;(K) %s%c%c %s %c->!%d", type1+3, parallel1, touten1, type2+3, touten2, score);

    /* データベースの検索(係る例) */
    sprintf(buffer, "%s%c%c %s %c", type1+3, parallel1, touten1, type2+3, touten2);
    if (buffer[DATA_LEN-1] != '\n') {
	fprintf(stderr, "corpus_case_predicate_check: data length overflow.\n");
	exit(1);
    }
    score = dbfetch(cp_db, buffer);

    /* 並列を考慮しないとき */
    if (!para_flag) {
	if (parallel1 == 'P')
	    sprintf(buffer, "%s %c %s %c", type1+3, touten1, type2+3, touten2);
	else
	    sprintf(buffer, "%sP%c %s %c", type1+3, touten1, type2+3, touten2);
	scorep = dbfetch(cp_db, buffer);
	score += scorep;
    }

    /* デバッグ出力 */
    fprintf(Outfp, "  %d\n", score);

    if (score)
	return TRUE;
    else
	return FALSE;
}

/* 節間のバリアを調べる関数 */
int corpus_clause_barrier_check(BNST_DATA *ptr1, BNST_DATA *ptr2) {
    char *type1, *type2;
    char parallel1, touten1, touten2;
    int score, scorep, offset, para_flag = 0;
    int pos1, pos2;

    /* 文節番号 */
    pos1 = ptr1->num;
    pos2 = ptr2->num;

    /* 初期化 */
    parallel1 = ' ';
    touten1 = ' ';
    touten2 = ' ';

    /* 並列の設定 */
    if (ptr1->para_key_type != PARA_KEY_O)
	/* if (ptr1->dpnd_type == 'P') */
	parallel1 = 'P';

    /* 格と述語タイプの設定 */
    type1 = (char *)check_feature(ptr1->f, "ID");
    type2 = (char *)check_feature(ptr2->f, "ID");

    if (!type1) return FALSE;
    if (!type2) return FALSE;

    /* 係り先の読点を見るためのオフセット */
    if (!strcmp(type2+3, "〜（ため）"))
	offset = 1;
    else
	offset = 0;

    /* 読点の設定 */
    if ((char *)check_feature(ptr1->f, "読点"))
	touten1 = ',';
    if ((char *)check_feature((ptr2+offset)->f, "読点"))
	touten2 = ',';

    /* データベースの検索(越える例) */
    sprintf(buffer, "!%s %c %s %c", type1+3, touten1, type2+3, touten2);
    if (buffer[DATA_LEN-1] != '\n') {
	fprintf(stderr, "corpus_barrier_check: data length overflow.\n");
	exit(1);
    }
    score = dbfetch(c_db, buffer);

    /* 並列を考慮しないとき */
    if (!para_flag) {
	if (parallel1 == 'P')
	    sprintf(buffer, "!%s %c %s %c", type1+3, touten1, type2+3, touten2);
	else
	    sprintf(buffer, "!%sP%c %s %c", type1+3, touten1, type2+3, touten2);
	scorep = dbfetch(c_db, buffer);
	score += scorep;
    }

    /* 越えたことがあるとバリアではない */
    if (score) {
	/* デバッグ出力 */
	fprintf(Outfp, ";;;(C) %2d %2d %s %c %s %c->!%d\n", pos1, pos2, type1+3, touten1, type2+3, touten2, score);
	return FALSE;
    }
    else
	/* デバッグ出力 */
	fprintf(Outfp, ";;;(C) %2d %2d %s %c %s %c->!%d", pos1, pos2, type1+3, touten1, type2+3, touten2, score);

    /* データベースの検索(係る例) */
    sprintf(buffer, "%s%c%c %s %c", type1+3, parallel1, touten1, type2+3, touten2);
    if (buffer[DATA_LEN-1] != '\n') {
	fprintf(stderr, "corpus_barrier_check: data length overflow.\n");
	exit(1);
    }
    score = dbfetch(c_db, buffer);

    /* 並列を考慮しないとき */
    if (!para_flag) {
	if (parallel1 == 'P')
	    sprintf(buffer, "%s %c %s %c", type1+3, touten1, type2+3, touten2);
	else
	    sprintf(buffer, "%sP%c %s %c", type1+3, touten1, type2+3, touten2);
	scorep = dbfetch(c_db, buffer);
	score += scorep;
    }

    /* デバッグ出力 */
    fprintf(Outfp, "  %d\n", score);

    /* あとは係ったことがあればよい */
    if (score)
	return TRUE;
    else
	return FALSE;
}

/* 格から述語に対するバリアを調べる関数 */
int corpus_barrier_check(BNST_DATA *ptr1, BNST_DATA *ptr2) {
    char *type1, *type2;
    char parallel1, touten1, touten2;
    int score, scorep, offset, para_flag = 0;
    int pos1, pos2;

    /* 文節番号 */
    pos1 = ptr1->num;
    pos2 = ptr2->num;

    /* 初期化 */
    parallel1 = ' ';
    touten1 = ' ';
    touten2 = ' ';

    /* 並列の設定 */
    if (ptr1->para_key_type != PARA_KEY_O)
	/* if (ptr1->dpnd_type == 'P') */
	parallel1 = 'P';

    /* 格と述語タイプの設定 */
    type1 = (char *)check_feature(ptr1->f, "係");
    type2 = (char *)check_feature(ptr2->f, "ID");

    if (!type1) return FALSE;
    if (!type2) return FALSE;

    /* 係り先の読点を見るためのオフセット */
    if (!strcmp(type2+3, "〜（ため）"))
	offset = 1;
    else
	offset = 0;

    /* 読点の設定 */
    if ((char *)check_feature(ptr1->f, "読点"))
	touten1 = ',';
    if ((char *)check_feature((ptr2+offset)->f, "読点"))
	touten2 = ',';

    /* データベースの検索(越える例) */
    sprintf(buffer, "!%s %c %s %c", type1+3, touten1, type2+3, touten2);
    if (buffer[DATA_LEN-1] != '\n') {
	fprintf(stderr, "corpus_barrier_check: data length overflow.\n");
	exit(1);
    }
    score = dbfetch(cp_db, buffer);

    /* 並列を考慮しないとき */
    if (!para_flag) {
	if (parallel1 == 'P')
	    sprintf(buffer, "!%s %c %s %c", type1+3, touten1, type2+3, touten2);
	else
	    sprintf(buffer, "!%sP%c %s %c", type1+3, touten1, type2+3, touten2);
	scorep = dbfetch(cp_db, buffer);
	score += scorep;
    }

    /* 越えたことがあるとバリアではない */
    if (score) {
	/* デバッグ出力 */
	fprintf(Outfp, ";;;(B) %2d %2d %s %c %s %c->!%d\n", pos1, pos2, type1+3, touten1, type2+3, touten2, score);
	return FALSE;
    }
    else
	/* デバッグ出力 */
	fprintf(Outfp, ";;;(B) %2d %2d %s %c %s %c->!%d", pos1, pos2, type1+3, touten1, type2+3, touten2, score);

    /* データベースの検索(係る例) */
    sprintf(buffer, "%s%c%c %s %c", type1+3, parallel1, touten1, type2+3, touten2);
    if (buffer[DATA_LEN-1] != '\n') {
	fprintf(stderr, "corpus_barrier_check: data length overflow.\n");
	exit(1);
    }
    score = dbfetch(cp_db, buffer);

    /* 並列を考慮しないとき */
    if (!para_flag) {
	if (parallel1 == 'P')
	    sprintf(buffer, "%s %c %s %c", type1+3, touten1, type2+3, touten2);
	else
	    sprintf(buffer, "%sP%c %s %c", type1+3, touten1, type2+3, touten2);
	scorep = dbfetch(cp_db, buffer);
	score += scorep;
    }

    /* 出力するために記録しておく */
    BarrierMatrix.Value[pos1][pos2] = score;
    if (BarrierMatrix.Type1[pos1] == NULL)
	BarrierMatrix.Type1[pos1] = strdup(type1+3);
    if (BarrierMatrix.Type2[pos2] == NULL)
	BarrierMatrix.Type2[pos2] = strdup(type2+3);

    /* デバッグ出力 */
    fprintf(Outfp, "  %d\n", score);

    /* あとは係ったことがあればよい */
    if (score)
	return TRUE;
    else
	return FALSE;
}

/* 行列表示する関数 */
void print_barrier(int bnum) {
    int i, j;

    fprintf(Outfp, ";;;(B)   ");
    for (i = 0; i < bnum; i++)
	fprintf(Outfp, " %2d", i);
    fprintf(Outfp, "\n");
    for (i = 0; i < bnum-1; i++) {
	fprintf(Outfp, ";;;(B) %2d   ", i);
	for (j = 0; j < i; j++) {
	    fprintf(Outfp, "   ");
	}
	for (j = i+1; j < bnum; j++) {
	    /* fprintf(Outfp, " %2c", BarrierMatrix.Value[i][j] ? 'o' : '-'); */
	    if (BarrierMatrix.Value[i][j] > 99)
		fprintf(Outfp, "  *");
	    else if (BarrierMatrix.Value[i][j] == -1)
		fprintf(Outfp, "  -");
	    else
		fprintf(Outfp, " %2d", BarrierMatrix.Value[i][j]);
	    
	}
	fprintf(Outfp, "\n");
    }

    for (i = 0; i < bnum; i++)
	fprintf(Outfp, ";;;(B) %2d %10s %20s\n", i, BarrierMatrix.Type1[i] ? BarrierMatrix.Type1[i] : " ", BarrierMatrix.Type2[i] ? BarrierMatrix.Type2[i] : " ");

    /* 初期化 */
    for (i = 0; i < bnum; i++) {
	if (BarrierMatrix.Type1[i]) {
	    free(BarrierMatrix.Type1[i]);
	    BarrierMatrix.Type1[i] = NULL;
	}
	if (BarrierMatrix.Type2[i]) {
	    free(BarrierMatrix.Type2[i]);
	    BarrierMatrix.Type2[i] = NULL;
	}
	for (j = i+1; j < bnum; j++)
	    BarrierMatrix.Value[i][j] = -1;
    }
}

int init_optional_case() {
    buffer[DATA_LEN-1] = '\n';
#ifdef BERKELEY_DB_V2
    if (OptionalCaseDBname)
	return db_read_open(OptionalCaseDBname, &op_db);
    else
	return db_read_open(OP_DB_NAME, &op_db);
#else
    if (OptionalCaseDBname)
	op_db = db_read_open(OptionalCaseDBname);
    else
	op_db = db_read_open(OP_DB_NAME);
    return TRUE;
#endif
}

void close_optional_case() {
    db_close(op_db);
}

/* 任意格からの係り受け頻度を調べる関数 */
int corpus_optional_case_comp(BNST_DATA *ptr1, char *case1, BNST_DATA *ptr2) {
    int i, j, k, score, flag, pos1, pos2;
    char *cp1, *cp2;

    /* 文節番号 */
    pos1 = ptr1->num;
    pos2 = ptr2->num;

    /* 文節 feature のチェック */
    if (check_feature_for_optional_case(ptr1->f) == TRUE)
	return FALSE;

    /* Memory 確保 */
    cp1 = (char *)malloc_data(strlen(ptr1->Jiritu_Go), "optional case");
    cp1[0] = '\0';
    cp2 = (char *)malloc_data(strlen(ptr2->Jiritu_Go), "optional case");
    cp2[0] = '\0';

    for (i = 0; i < ptr1->jiritu_num; i++) {
	cp1[0] = '\0';
	flag = 1;
	for (k = i; k < ptr1->jiritu_num; k++) {
	    if (check_Morph_for_optional_case(ptr1->jiritu_ptr+k) == TRUE) {
		flag = 0;
		break;
	    }
	    strcat(cp1, ptr1->jiritu_ptr+k);
	}

	if (!flag)
	    continue;

	if (check_JiritsuGo_for_optional_case(cp1) == TRUE)
	    continue;

	for (j = 0; j < ptr2->jiritu_num; j++) {
	    cp2[0] = '\0';
	    flag = 1;
	    for (k = j; k < ptr2->jiritu_num; k++) {
		if (check_Morph_for_optional_case(ptr2->jiritu_ptr+k) == TRUE) {
		    flag = 0;
		    break;
		}
		strcat(cp2, ptr2->jiritu_ptr+k);
	    }

	    if (!flag)
		continue;

	    if (check_JiritsuGo_for_optional_case(cp2) == TRUE)
		continue;

	    sprintf(buffer, "%s:%s %s", cp1, case1, cp2);
	    if (buffer[DATA_LEN-1] != '\n') {
		fprintf(stderr, "corpus_optional_case_comp: data length overflow.\n");
		exit(1);
	    }

	    score = dbfetch(op_db, buffer);

	    if (score) {
		fprintf(Outfp, ";;;(O) %d %d %s:%s %s ->%d\n", pos1, pos2, cp1, case1, cp2, score);
		free(cp1);
		free(cp2);
		/* full match */
		if (!i && !j)
		    return 1;
		/* part match */
		else
		    return 0.5;
	    }
	    else
		fprintf(Outfp, ";;;(O) %d %d %s:%s %s ->%d\n", pos1, pos2, cp1, case1, cp2, score);
	}
    }

    free(cp1);
    free(cp2);
    return 0;

    /* データベースの検索
    sprintf(buffer, "%s:%s %s", ptr1->Jiritu_Go, 
	    case1, ptr2->Jiritu_Go);
    if (buffer[DATA_LEN-1] != '\n') {
	fprintf(stderr, "corpus_optional_case_comp: data length overflow.\n");
	exit(1);
    }
    score = dbfetch(op_db, buffer);

    fprintf(Outfp, ";;;(O) %s:%s %s ->%d\n", ptr1->Jiritu_Go, case1, 
	    ptr2->Jiritu_Go, score);

    if (score)
	return TRUE;
    else
	return FALSE; */
}

/* 扱うべき任意格であれば真を返す関数 */
int check_optional_case(int scase) {

    /* デ, カラ, マデ, ト格かな */
    if (scase == case2num("デ格") || 
	scase == case2num("カラ格") || 
	scase == case2num("マデ格") || 
	scase == case2num("ト格"))
	return TRUE;
    else
	return FALSE;
}

/* 自立語 : 文節 feature の制限 */
int check_feature_for_optional_case(FEATURE *f) {
    if ((char *)check_feature(f, "指示詞") || 
	(char *)check_feature(f, "相対名詞"))
	return TRUE;
    return FALSE;
}

/* 自立語 : 自立語の制限 */
int check_JiritsuGo_for_optional_case(char *cp) {
     if (!strcmp(cp, "なる") || 
	 !strcmp(cp, "ない") || 
	 !strcmp(cp, "する")) {
	return TRUE;
    }
    return FALSE;
}

/* 自立語 : 形態素の制限 */
int check_Morph_for_optional_case(MRPH_DATA *m) {
    /* 副詞的名詞 */
    if (m->Hinshi == 6 && m->Bunrui == 9)
	return TRUE;
    /* 形式名詞 */
    else if (m->Hinshi == 6 && m->Bunrui == 8)
	return TRUE;
    return FALSE;
/*     int i; */
/*     for (i = 0; i < b->jiritu_num; i++) { */
/* 	if ((b->jiritu_ptr+i)->Hinshi == 6 && (b->jiritu_ptr+i)->Bunrui == 9) { */
/* 	    return TRUE; */
/* 	} */
/*     } */
/*     return FALSE; */
}
