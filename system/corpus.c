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
static DBM_FILE c_db, cc_db, op_db, cp_db, wc_db, c_temp_db;

extern char *ClauseDBname;
extern char *ClauseCDBname;
extern char *CasePredicateDBname;
extern char *OptionalCaseDBname;

DBM_FILE db_read_open(char *filename);
char *db_get(DBM_FILE db, char *buf);
void db_close(DBM_FILE db);

typedef struct {
    int Value[BNST_MAX][BNST_MAX];
    char *Type1[BNST_MAX];
    char *Type2[BNST_MAX];
} EtcMatrix;

EtcMatrix BarrierMatrix;

/* DB を search し、数値として返す関数 */
int dbfetch_num(DBM_FILE db, char *buf)
{
    int count = 0;
    char *cp;

    cp = db_get(db, buf);

    if (cp) {
	if (strlen(cp) >= DATA_LEN) {
	    fprintf(stderr, "dbfetch_num: content length overflow.\n");
	    exit(1);
	}

	strcpy(buffer, cp);
	free(cp);
	count = atoi(buffer);
    }
    return count;
}

#define CLAUSE_TEMP_DB_NAME KNP_DICT "/clause/clause-strength.gdbm"

int init_clause()
{
    int i;
    buffer[DATA_LEN-1] = '\n';
    for (i = 0; i < BNST_MAX; i++)
	CorpusComment[i][DATA_LEN-1] = '\n';

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

    /* c_temp_db = db_read_open(CLAUSE_TEMP_DB_NAME); */

    return TRUE;
}

void close_clause()
{
    db_close(c_db);
    /* db_close(c_temp_db); */
    if (!(OptInhibit & OPT_INHIBIT_C_CLAUSE))
	db_close(cc_db);
}

/* 述語節間の係り受け頻度を調べる関数 */
int corpus_clause_comp(BNST_DATA *ptr1, BNST_DATA *ptr2, int para_flag)
{
    char *type1, *type2, *cp, *token, *type, *level1, *level2, *sparse;
    char parallel1, parallel2, touten1, touten2, touten;
    int score, offset, i, score2;

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

    /* level の設定(表示に用いるだけ) */
    level1 = (char *)check_feature(ptr1->f, "レベル");
    level2 = (char *)check_feature(ptr2->f, "レベル");

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
    score = dbfetch_num(c_db, buffer);

    if (!(OptInhibit & OPT_INHIBIT_C_CLAUSE) && (para_flag == TRUE) && 
	parallel1 == ' ') {
	cp = db_get(cc_db, buffer);
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

    sprintf(buffer, "!%s%c%c %s%c%c", 
	    type1+3, parallel1, touten1, 
	    type2+3, parallel2, touten2);
    if (buffer[DATA_LEN-1] != '\n') {
	fprintf(stderr, "corpus_clause_comp: data length overflow.\n");
	exit(1);
    }
    score2 = dbfetch_num(c_db, buffer);

    /* デバッグ出力 */
    if (para_flag == TRUE) {
	if (level1 && level2) {
	    fprintf(Outfp, ";;; %2d %2d %s%c%c(%s) %s%c%c(%s) -> %d (%d)\n", 
		    ptr1->num, ptr2->num, type1+3, parallel1, touten1, level1+7, 
		    type2+3, parallel2, touten2, level2+7, score, score2);
	}
	else {
	    fprintf(Outfp, ";;; %2d %2d %s%c%c %s%c%c -> %d (%d)\n", 
		    ptr1->num, ptr2->num, type1+3, parallel1, touten1, 
		    type2+3, parallel2, touten2, score, score2);
	}
    }
    else {
	fprintf(Outfp, ";;;(R) %s %c %s %c->%d (%d)\n", type1+3, touten1, 
		type2+3, touten2, score, score2);
    }

    if (score) {
	if (score == 1) {
	    /*
	    if (!CorpusComment[ptr1->num][0])
		sprintf(CorpusComment[ptr1->num], "%s%c %s%c", type1+3, touten1, type2+3, touten2);
	    if (CorpusComment[ptr1->num][DATA_LEN-1] != '\n') {
		fprintf(stderr, "corpus_clause_comp: data length overflow(2).\n");
		exit(1);
	    }
	    */
	    return CORPUS_POSSIBILITY_1;
	}
	else
	    return TRUE;
    }
    else {
	/* 並列のときでスコアがないなら条件緩和して並列を無視
	if (parallel1 == 'P')
	    return corpus_clause_comp((BNST_DATA *)ptr1, 
				      (BNST_DATA *)ptr2, 
				      FALSE);
	else */

	if (!score2) {
	    if (sparse = (char *)check_feature(ptr1->f, "SPARSE"))
		sprintf(buffer, "%s:%d%c", sparse, ptr2->num, parallel1);
	    else
		sprintf(buffer, "SPARSE:%d%c", ptr2->num, parallel1);
	    assign_cfeature(&(ptr1->f), buffer);
	}
	return FALSE;
    }
}

int init_case_pred()
{
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

    if (CasePredicateDBname)
	cp_db = db_read_open(CasePredicateDBname);
    else
	cp_db = db_read_open(CASE_PRED_DB_NAME);
    return TRUE;
}

void close_case_pred()
{
    db_close(cp_db);
}

/* 格から述語への係り受け頻度を調べる関数 */
int corpus_case_predicate_check(BNST_DATA *ptr1, BNST_DATA *ptr2)
{
    char *type1, *type2, *level2;
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

    /* level の設定(表示に用いるだけ) */
    level2 = (char *)check_feature(ptr2->f, "レベル");

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
    score = dbfetch_num(cp_db, buffer);

    /* 並列を考慮しないとき */
    if (!para_flag) {
	if (parallel1 == 'P')
	    sprintf(buffer, "!%s %c %s %c", type1+3, touten1, type2+3, touten2);
	else
	    sprintf(buffer, "!%sP%c %s %c", type1+3, touten1, type2+3, touten2);
	scorep = dbfetch_num(cp_db, buffer);
	score += scorep;
    }

#ifdef DEBUGMORE
    fprintf(Outfp, ";;;(K) %2d %2d %s%c%c %s %c->!%d", ptr1->num, ptr2->num, type1+3, parallel1, touten1, type2+3, touten2, score);
#endif

    /* データベースの検索(係る例) */
    sprintf(buffer, "%s%c%c %s %c", type1+3, parallel1, touten1, type2+3, touten2);
    if (buffer[DATA_LEN-1] != '\n') {
	fprintf(stderr, "corpus_case_predicate_check: data length overflow.\n");
	exit(1);
    }
    score = dbfetch_num(cp_db, buffer);

    /* 並列を考慮しないとき */
    if (!para_flag) {
	if (parallel1 == 'P')
	    sprintf(buffer, "%s %c %s %c", type1+3, touten1, type2+3, touten2);
	else
	    sprintf(buffer, "%sP%c %s %c", type1+3, touten1, type2+3, touten2);
	scorep = dbfetch_num(cp_db, buffer);
	score += scorep;
    }

#ifdef DEBUGMORE
    fprintf(Outfp, "  %d\n", score);
#endif

    if (score)
	return TRUE;
    else
	return FALSE;
}

/* 節間のバリアを調べる関数 */
int corpus_clause_barrier_check(BNST_DATA *ptr1, BNST_DATA *ptr2)
{
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
    score = dbfetch_num(c_db, buffer);

    /* 並列を考慮しないとき */
    if (!para_flag) {
	if (parallel1 == 'P')
	    sprintf(buffer, "!%s %c %s %c", type1+3, touten1, type2+3, touten2);
	else
	    sprintf(buffer, "!%sP%c %s %c", type1+3, touten1, type2+3, touten2);
	scorep = dbfetch_num(c_db, buffer);
	score += scorep;
    }

    /* 越えたことがあるとバリアではない */
    if (score) {
#ifdef DEBUG
	/* デバッグ出力 */
	fprintf(Outfp, ";;; (C壁) %2d %2d %s %c %s %c-> !%d\n", pos1, pos2, type1+3, touten1, type2+3, touten2, score);
#endif
	return FALSE;
    }
#ifdef DEBUG
    else
	/* デバッグ出力 */
	fprintf(Outfp, ";;; (C壁) %2d %2d %s %c %s %c-> !%d", pos1, pos2, type1+3, touten1, type2+3, touten2, score);
#endif

    /* データベースの検索(係る例) */
    sprintf(buffer, "%s%c%c %s %c", type1+3, parallel1, touten1, type2+3, touten2);
    if (buffer[DATA_LEN-1] != '\n') {
	fprintf(stderr, "corpus_barrier_check: data length overflow.\n");
	exit(1);
    }
    score = dbfetch_num(c_db, buffer);

    /* 並列を考慮しないとき */
    if (!para_flag) {
	if (parallel1 == 'P')
	    sprintf(buffer, "%s %c %s %c", type1+3, touten1, type2+3, touten2);
	else
	    sprintf(buffer, "%sP%c %s %c", type1+3, touten1, type2+3, touten2);
	scorep = dbfetch_num(c_db, buffer);
	score += scorep;
    }

#ifdef DEBUG
    /* デバッグ出力 */
    fprintf(Outfp, "  %d\n", score);
#endif

    /* あとは係ったことがあればよい */
    if (score)
	return TRUE;
    else
	return FALSE;
}

/* 格から述語に対するバリアを調べる関数 */
int corpus_barrier_check(BNST_DATA *ptr1, BNST_DATA *ptr2)
{
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
    score = dbfetch_num(cp_db, buffer);

    /* 並列を考慮しないとき */
    if (!para_flag) {
	if (parallel1 == 'P')
	    sprintf(buffer, "!%s %c %s %c", type1+3, touten1, type2+3, touten2);
	else
	    sprintf(buffer, "!%sP%c %s %c", type1+3, touten1, type2+3, touten2);
	scorep = dbfetch_num(cp_db, buffer);
	score += scorep;
    }

    /* 越えたことがあるとバリアではない */
    if (score) {
	/* デバッグ出力
	fprintf(Outfp, ";;;(B) %2d %2d %s %c %s %c->!%d\n", pos1, pos2, type1+3, touten1, type2+3, touten2, score); */
	return FALSE;
    }
    /* else
	デバッグ出力
	fprintf(Outfp, ";;;(B) %2d %2d %s %c %s %c->!%d", pos1, pos2, type1+3, touten1, type2+3, touten2, score); */

    /* データベースの検索(係る例) */
    sprintf(buffer, "%s%c%c %s %c", type1+3, parallel1, touten1, type2+3, touten2);
    if (buffer[DATA_LEN-1] != '\n') {
	fprintf(stderr, "corpus_barrier_check: data length overflow.\n");
	exit(1);
    }
    score = dbfetch_num(cp_db, buffer);

    /* 並列を考慮しないとき */
    if (!para_flag) {
	if (parallel1 == 'P')
	    sprintf(buffer, "%s %c %s %c", type1+3, touten1, type2+3, touten2);
	else
	    sprintf(buffer, "%sP%c %s %c", type1+3, touten1, type2+3, touten2);
	scorep = dbfetch_num(cp_db, buffer);
	score += scorep;
    }

    /* 出力するために記録しておく */
    BarrierMatrix.Value[pos1][pos2] = score;
    if (BarrierMatrix.Type1[pos1] == NULL)
	BarrierMatrix.Type1[pos1] = strdup(type1+3);
    if (BarrierMatrix.Type2[pos2] == NULL)
	BarrierMatrix.Type2[pos2] = strdup(type2+3);

    /* デバッグ出力
    fprintf(Outfp, "  %d\n", score); */

    /* あとは係ったことがあればよい */
    if (score)
	return TRUE;
    else
	return FALSE;
}

/* 行列表示する関数 */
void print_barrier(int bnum)
{
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
    int i;
    buffer[DATA_LEN-1] = '\n';
    for (i = 0; i < BNST_MAX; i++)
	CorpusComment[i][DATA_LEN-1] = '\n';

    if (OptionalCaseDBname)
	op_db = db_read_open(OptionalCaseDBname);
    else
	op_db = db_read_open(OP_DB_NAME);
    wc_db = db_read_open(WC_DB_NAME);
    return TRUE;
}

void close_optional_case()
{
    db_close(op_db);
}

/* 事例の係り受け頻度を返す関数 (この中でスコア計算してしまう) */
float CorpusExampleDependencyFrequency(BNST_DATA *ptr1, char *case1, BNST_DATA *ptr2) {
    int i, k, score, flag, pos1, pos2, count;
    char *cp1, *cp2;
    float maxscore = 0, tempscore;

    /* 文節番号 */
    pos1 = ptr1->num;
    pos2 = ptr2->num;

    /* 文節 feature のチェック */
    if (check_feature_for_optional_case(ptr1->f) == TRUE)
	return 0;

    /* Memory 確保 */
    cp1 = (char *)malloc_data(strlen(ptr1->Jiritu_Go), "optional case");
    cp1[0] = '\0';
    cp2 = (char *)malloc_data(strlen(ptr2->Jiritu_Go), "optional case");
    cp2[0] = '\0';

    /* 受け側自立語 */
    cp2[0] = '\0';
    flag = 1;

    /* 受け側自立語列の作成 */
    for (k = 0; k < ptr2->jiritu_num; k++) {
	/* 副詞的名詞, 形式名詞を含んでいたらやめる */
	if (check_Morph_for_optional_case(ptr2->jiritu_ptr+k) == TRUE) {
	    flag = 0;
	    break;
	}
	strcat(cp2, (ptr2->jiritu_ptr+k)->Goi);
    }

    if (!flag)
	return 0;

    /* 「なる」, 「ない」, 「する」, 「ある」 は除く */
    if (check_JiritsuGo_for_optional_case(cp2) == TRUE)
	return 0;

    count = dbfetch_num(wc_db, cp2);

    /* 係り側自立語列を順に短くしていく */
    for (i = 0; i < ptr1->jiritu_num; i++) {
	cp1[0] = '\0';
	flag = 1;

	/* 係り側自立語列の作成 */
	for (k = i; k < ptr1->jiritu_num; k++) {
	    /* 副詞的名詞, 形式名詞を含んでいたらやめる */
	    if (check_Morph_for_optional_case(ptr1->jiritu_ptr+k) == TRUE) {
		flag = 0;
		break;
	    }
	    strcat(cp1, (ptr1->jiritu_ptr+k)->Goi);
	}

	if (!flag)
	    continue;

	/* 「なる」, 「ない」, 「する」, 「ある」 は除く */
	if (check_JiritsuGo_for_optional_case(cp1) == TRUE)
	    continue;

	/* DB を検索するためのキーを作成 */
	sprintf(buffer, "%s:%s %s", cp1, case1, cp2);
	if (buffer[DATA_LEN-1] != '\n') {
	    fprintf(stderr, "CorpusExampleDependencyFrequency: data length overflow.\n");
	    exit(1);
	}

	/* DB 検索 */
	score = dbfetch_num(op_db, buffer);
#ifdef DEBUGMORE
	fprintf(Outfp, ";;;(O) %d %d %s:%s %s(%d) ->%d\n", pos1, pos2, cp1, case1, cp2, count, score);
#endif DEBUGMORE

	if (score) {
	    /* tempscore = (float)score/count; */
	    tempscore = (float)score;
	    if (maxscore < tempscore) {
		maxscore = tempscore;
	    }
	}
    }

    free(cp1);
    free(cp2);
    return maxscore;
}

/* 係り先リストが与えられたときに、指定された係り先のスコアを計算する関数 */
int CorpusExampleDependencyCalculation(BNST_DATA *ptr1, char *case1, int h, CHECK_DATA *list)
{
    int i;
    float score, currentscore = -1, totalscore = 0;

    if (list->num < 0)
	return 0;

    /* 候補すべてのスコアを計算する */
    for (i = 0; i < list->num; i++) {
	score = CorpusExampleDependencyFrequency(ptr1, case1, bnst_data+list->pos[i]);
	totalscore += score;

	/* 今調べている Head であるとき */
	if (list->pos[i] == h)
	    currentscore = score;
    }

    if (currentscore < 0) {
	fprintf(stderr, "A contradiction occured.\n");
	exit(1);
    }

    if (totalscore)
	return (int)((currentscore/totalscore)*10);
    else
	return 0;
}

/* 任意格からの係り受け頻度を調べる関数 */
int corpus_optional_case_comp(BNST_DATA *ptr1, char *case1, BNST_DATA *ptr2)
{
    int i, j, k, score, flag, pos1, pos2, firstscore = 0;
    char *cp1, *cp2;

    /* 文節番号 */
    pos1 = ptr1->num;
    pos2 = ptr2->num;

    /* 文節 feature のチェック */
    if (check_feature_for_optional_case(ptr1->f) == TRUE)
	return 0;

    /* Memory 確保 */
    cp1 = (char *)malloc_data(strlen(ptr1->Jiritu_Go), "optional case");
    cp1[0] = '\0';
    cp2 = (char *)malloc_data(strlen(ptr2->Jiritu_Go), "optional case");
    cp2[0] = '\0';

    /* 係り側自立語列を順に短くしていく */
    for (i = 0; i < ptr1->jiritu_num; i++) {
	cp1[0] = '\0';
	flag = 1;

	/* 係り側自立語列の作成 */
	for (k = i; k < ptr1->jiritu_num; k++) {
	    /* 副詞的名詞, 形式名詞を含んでいたらやめる */
	    if (check_Morph_for_optional_case(ptr1->jiritu_ptr+k) == TRUE) {
		flag = 0;
		break;
	    }
	    strcat(cp1, (ptr1->jiritu_ptr+k)->Goi);
	}

	if (!flag)
	    continue;

	/* 「なる」, 「ない」, 「する」, 「ある」 は除く */
	if (check_JiritsuGo_for_optional_case(cp1) == TRUE)
	    continue;

	/* 受け側自立語列を順に短くしていく */
	for (j = 0; j < ptr2->jiritu_num; j++) {
	    cp2[0] = '\0';
	    flag = 1;

	    /* 受け側自立語列の作成 */
	    for (k = j; k < ptr2->jiritu_num; k++) {
		/* 副詞的名詞, 形式名詞を含んでいたらやめる */
		if (check_Morph_for_optional_case(ptr2->jiritu_ptr+k) == TRUE) {
		    flag = 0;
		    break;
		}
		strcat(cp2, (ptr2->jiritu_ptr+k)->Goi);
	    }

	    if (!flag)
		continue;

	    /* 「なる」, 「ない」, 「する」, 「ある」 は除く */
	    if (check_JiritsuGo_for_optional_case(cp2) == TRUE)
		continue;

	    /* DB を検索するためのキーを作成 */
	    sprintf(buffer, "%s:%s %s", cp1, case1, cp2);
	    if (buffer[DATA_LEN-1] != '\n') {
		fprintf(stderr, "corpus_optional_case_comp: data length overflow.\n");
		exit(1);
	    }

	    /* DB 検索 */
	    score = dbfetch_num(op_db, buffer);
	    fprintf(Outfp, ";;;(O) %d %d %s:%s %s ->%d\n", pos1, pos2, cp1, case1, cp2, score);

	    if (score && !firstscore) {
		if (!CorpusComment[ptr1->num][0]) {
		    sprintf(CorpusComment[ptr1->num], "%s:%s %s %d", cp1, case1, cp2, score);
		    if (CorpusComment[ptr1->num][DATA_LEN-1] != '\n') {
			fprintf(stderr, "corpus_optional_case_comp: data length overflow.\n");
			exit(1);
		    }
		}

		/* full match */
		if (!i && !j)
		    firstscore = 2;
		/* part match */
		else
		    firstscore = 1;
	    }
	}
    }

    free(cp1);
    free(cp2);
    return firstscore;
}

/* 扱うべき任意格であれば真を返す関数 */
int check_optional_case(char *scase)
{

    /* オプションで与えられた格 */
    if (OptOptionalCase) {
	if (str_eq(scase, OptOptionalCase))
	    return TRUE;
	else
	    return FALSE;
    }
    else {
	/* デ, カラ, マデ, ト格かな */
	if (str_eq(scase, "カラ格") || 
	    str_eq(scase, "ガ格") || 
	    str_eq(scase, "デ格") || 
	    str_eq(scase, "ト格") || 
	    str_eq(scase, "ニ格") || 
	    str_eq(scase, "ヘ格") || 
	    str_eq(scase, "マデ格") || 
	    str_eq(scase, "ヨリ格"))
	    return TRUE;
	else
	    return FALSE;
    }
}

/* 自立語 : 文節 feature の制限 */
int check_feature_for_optional_case(FEATURE *f)
{
    if ((char *)check_feature(f, "指示詞") || 
	(char *)check_feature(f, "相対名詞"))
	return TRUE;
    return FALSE;
}

/* 自立語 : 自立語の制限 */
int check_JiritsuGo_for_optional_case(char *cp)
{
     if (!strcmp(cp, "なる") || 
	 !strcmp(cp, "ない") || 
	 !strcmp(cp, "する") ||
	 !strcmp(cp, "ある")) {
	return TRUE;
    }
    return FALSE;
}

/* 自立語 : 形態素の制限 */
int check_Morph_for_optional_case(MRPH_DATA *m)
{
    /* 副詞的名詞 */
    if (m->Hinshi == 6 && m->Bunrui == 9)
	return TRUE;
    /* 形式名詞 */
    else if (m->Hinshi == 6 && m->Bunrui == 8)
	return TRUE;
    return FALSE;
}

/* 事例情報を用いた構文木の選択するか否か */
void optional_case_evaluation()
{
    int i;
    int appropriate = 0;

    /* 普通の Best 解と、事例を用いた場合の Best 解が同じならば return */
    if (Op_Best_mgr.ID < 0 || Best_mgr.ID == Op_Best_mgr.ID)
	return;

    /* 学習時でなければ */
    if (!OptLearn) {
	for (i = 0;i < Bnst_num; i++) {
	    /* 事例を用いた文節 */
	    if (Op_Best_mgr.dpnd.op[i].flag && Best_mgr.dpnd.head[i] != Op_Best_mgr.dpnd.head[i]) {
		/* 読点があれば */
		if (check_feature(Op_Best_mgr.dpnd.f[i], "読点")) {
		    appropriate++;
		}
		/* 読点がなければ */
		else {
		    /* 事例を用いたときのほうが近いとき */
		    if (Op_Best_mgr.dpnd.head[i] < Best_mgr.dpnd.head[i])
			appropriate++;
		    /* 事例を用いたときのほうが遠いとき */
		    else
			appropriate--;
		}
	    }
	}

	/* 事例を用いたときのスコアが大きく、
	   かつ appropriate が 0 以上だったら適用 */
	/* 
	   if (Op_Best_mgr.score > Best_mgr.score && appropriate >= 0) {
	   */
	if (Op_Best_mgr.score > Best_mgr.score) {
	    Best_mgr = Op_Best_mgr;
	    return;
	}
    }
}

/*==================================================================*/
    int subordinate_level_check_special(char *cp, BNST_DATA *ptr2)
/*==================================================================*/
{
    char *level1, *level2;

    level1 = cp;
    level2 = (char *)check_feature(ptr2->f, "レベル");

    /* 連体は FALSE */
    if (check_feature(ptr2->f, "係:連体") || check_feature(ptr2->f, "係:連格"))
	return FALSE;

    if (level1 == NULL) return TRUE;		/* 何もなし --> 何でもOK */
    else if (level2 == NULL) return FALSE;	/* 何でも× --> 何もなし */
    else if (levelcmp(level1, level2 + strlen("レベル:")) <= 0)
	return TRUE;				/* ptr1 <= ptr2 ならOK */
    else return FALSE;
}

/* 実験 (強弱関係を一次元リストにした場合)*/
int temp_corpus_clause_comp(BNST_DATA *ptr1, BNST_DATA *ptr2, int para_flag)
{
    char *type1, *type2, *cp, *token, *type, *level1, *level2;
    char parallel1, parallel2, touten1, touten2, touten;
    int score1, score2, score3, offset, i;

    /* para_flag == TRUE  : 並列を考慮 (並列解析まえの呼び出しでは意味がない)
       para_flag == FALSE : 並列を無視 */

    /* 初期化 */
    parallel1 = ' ';
    parallel2 = ' ';
    touten1 = ' ';
    touten2 = ' ';

    /* 並列の設定
    if (para_flag == TRUE) {
	if (ptr1->para_key_type != PARA_KEY_O)
	    parallel1 = 'P';
    }
    */

    /* 述語タイプの設定 */
    type1 = (char *)check_feature(ptr1->f, "ID");
    type2 = (char *)check_feature(ptr2->f, "ID");

    /* level の設定(表示に用いるだけ) */
    level1 = (char *)check_feature(ptr1->f, "レベル");
    level2 = (char *)check_feature(ptr2->f, "レベル");

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

    /* 係り側 */
    sprintf(buffer, "%s%c%c", type1+3, parallel1, touten1);
    if (buffer[DATA_LEN-1] != '\n') {
	fprintf(stderr, "corpus_clause_comp: data length overflow.\n");
	exit(1);
    }

    /*  fprintf(stdout, ";;; K★ %s ", buffer); */
    score1 = dbfetch_num(c_temp_db, buffer);
    /*  fprintf(stdout, "%d\n", score1); */

    /* 受け側 */
    sprintf(buffer, "%s%c%c", type2+3, parallel2, touten2);
    if (buffer[DATA_LEN-1] != '\n') {
	fprintf(stderr, "corpus_clause_comp: data length overflow.\n");
	exit(1);
    }

    /*  fprintf(stdout, ";;; U★ %s ", buffer); */
    score2 = dbfetch_num(c_temp_db, buffer);
    /*  fprintf(stdout, "%d\n", score2); */

    /*
    fprintf(stderr, ";;; ★ %d %d\n", score1, score2);
    */

    /* 前より後の方が強ければ TRUE */
    /* if (!score2 || score1 <= score2) */
    if (!score1 || score1 <= score2)
	return TRUE;
    else
	return FALSE;
}

void CheckChildCaseFrame() {
    int i, j;
    TOTAL_MGR *tm = &Best_mgr;

    for (i = Bnst_num-1; i > 0; i--) {
	if (!check_feature((bnst_data+i)->f, "用言"))
	    continue;
	for (j = 0; j < i; j++) {
	    if (tm->dpnd.head[j] == i) {
		assign_cfeature(&((bnst_data+i)->f), "子○");
		break;
		/* check_feature((bnst_data+j)->f, "係") */
	    }
	}
    }
}
