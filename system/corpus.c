/*====================================================================

			     コーパス関連

                                             S.Kurohashi 1999. 4. 1

    $Id$
====================================================================*/
#include "knp.h"

static char buffer[DATA_LEN];
char CorpusComment[BNST_MAX][DATA_LEN];
static DBM_FILE c_db, cc_db, op_db, op_sm_db, cp_db, c_temp_db;

extern char *ClauseDBname;
extern char *ClauseCDBname;
extern char *CasePredicateDBname;
extern char *OptionalCaseDBname;

DBM_FILE db_read_open(char *filename);
char *db_get(DBM_FILE db, char *buf);
void DB_close(DBM_FILE db);

typedef struct {
    int Value[BNST_MAX][BNST_MAX];
    char *Type1[BNST_MAX];
    char *Type2[BNST_MAX];
} EtcMatrix;

EtcMatrix BarrierMatrix;

typedef struct {
    char *sm;
    int frequency;
} SMwithFrequency;

#define SM_ALLOCATION_STEP 100

typedef struct {
    char *relation;
    SMwithFrequency *list;
    int num;
    int maxnum;
} _SMCaseFrame;

#define CASE_ALLOCATION_STEP 10

typedef struct {
    _SMCaseFrame *frame;
    int num;
    int maxnum;
    char *predicate;
} SMCaseFrame;

SMCaseFrame smcf;

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
    DB_close(c_db);
    /* db_close(c_temp_db); */
    if (!(OptInhibit & OPT_INHIBIT_C_CLAUSE))
	DB_close(cc_db);
}

/* 述語節間の係り受け頻度を調べる関数 */
int corpus_clause_comp(BNST_DATA *ptr1, BNST_DATA *ptr2, int para_flag)
{
    char *type1, *type2, *cp, *token, *type, *level1, *level2, *sparse;
    char parallel1, parallel2, touten1, touten2, touten;
    int score, offset, i, score2;
    BNST_DATA *bnst_data;

    bnst_data = ptr1-ptr1->num;

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

    /* 頻度があるか、補完されているとき */
    if (score > 0 || score == -2) {
	return TRUE;
    }
    else {
	/* 並列のときでスコアがないなら条件緩和して並列を無視
	if (parallel1 == 'P')
	    return corpus_clause_comp((BNST_DATA *)ptr1, 
				      (BNST_DATA *)ptr2, 
				      FALSE);
	else */

	if (!score && !score2) {
	    if ((sparse = (char *)check_feature(ptr1->f, "SPARSE")))
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
    DB_close(cp_db);
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

    /* データベースの検索 (係る例) */
    sprintf(buffer, "%s%c%c %s %c", type1+3, parallel1, touten1, type2+3, touten2);
    if (buffer[DATA_LEN-1] != '\n') {
	fprintf(stderr, "corpus_barrier_check: data length overflow.\n");
	exit(1);
    }
    score = dbfetch_num(c_db, buffer);

    /* 補完されたときと、精錬により削除されたときは壁としないことにする */

    if (score >= 0) {
	/* 並列を考慮しないとき */
	if (!para_flag) {
	    if (parallel1 == 'P')
		sprintf(buffer, "%s %c %s %c", type1+3, touten1, type2+3, touten2);
	    else
		sprintf(buffer, "%sP%c %s %c", type1+3, touten1, type2+3, touten2);
	    scorep = dbfetch_num(c_db, buffer);
	    score += scorep;
	}
    }

#ifdef DEBUG
    /* デバッグ出力 */
    fprintf(Outfp, "  %d\n", score);
#endif

    /* あとは係ったことがあればよい */
    if (score > 0)
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
    /* wc_db = db_read_open(WC_DB_NAME); */
    op_sm_db = db_read_open(OP_SM_DB_NAME);

    smcf.maxnum = 0;
    smcf.frame = NULL;

    return TRUE;
}

void close_optional_case()
{
    DB_close(op_db);
    DB_close(op_sm_db);
}

/* 任意格からの係り受け頻度を調べる関数 */
int corpus_optional_case_comp(SENTENCE_DATA *sp, BNST_DATA *ptr1, char *case1, BNST_DATA *ptr2, CORPUS_DATA *corpus)
{
    int i, j, k, score, flag, pos1, pos2, firstscore = 0, special = 0;
    int fukugojiflag = 0;
    char *cp1 = NULL, *cp2 = NULL, *cp;

    /* 文節番号 */
    pos1 = ptr1->num;
    pos2 = ptr2->num;

    /* 文節 feature のチェック */
    if (check_feature_for_optional_case(ptr1->f) == TRUE)
	return 0;

    /* 受け側が受動態の場合 */
    if (check_feature(ptr2->f, "〜れる") || check_feature(ptr2->f, "〜られる"))
	return 0;

    /* 使役の場合 */
    if (check_feature(ptr2->f, "〜せる") || check_feature(ptr2->f, "〜させる"))
	return 0;

    /* 補文 */
    if (check_feature(ptr1->f, "ID:〜と（引用）")) {
	cp1 = strdup("C:補文");
	special = 1;
    }
    /* 時間 */
    else if (check_feature(ptr1->f, "時間")) {
	cp1 = strdup("C:時間");
	special = 1;
    }
    /* 複合辞 */
    else if (check_feature(ptr1->f, "複合辞") && ptr1->num > 0) {
	cp = ptr1->Jiritu_Go;

	/* ひとつ前の文節の自立語をみる */
	pos1 = ptr1->num - 1;
	ptr1 = sp->bnst_data + pos1;

	/* 係の関係としては、前の付属語+自分の自立語 */
	case1 = (char *)malloc_data(strlen((ptr1->fuzoku_ptr + ptr1->fuzoku_num - 1)->Goi) + 
				    strlen(cp) + 1, "optional_case");
	sprintf(case1, "%s%s", (ptr1->fuzoku_ptr + ptr1->fuzoku_num - 1)->Goi, cp);
	fukugojiflag = 1;
    }

    /* Memory 確保 */
    if (!cp1) {
	cp1 = (char *)malloc_data(strlen(ptr1->Jiritu_Go)+1, "optional case");
	cp1[0] = '\0';
    }
    if (!cp2) {
	cp2 = (char *)malloc_data(strlen(ptr2->Jiritu_Go)+1, "optional case");
	cp2[0] = '\0';
    }

    /* 係り側自立語列を順に短くしていく */
    for (i = 0; i < ptr1->jiritu_num; i++) {
	if (!special) {
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
	}

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

	    sprintf(buffer, "Unsupervised:%s:%s %s -> %d", cp1, case1, cp2, score);
	    if (buffer[DATA_LEN-1] != '\n') {
		fprintf(stderr, "corpus_optional_case_comp: data length overflow.\n");
		exit(1);
	    }
#ifdef DEBUGMORE
	    fprintf(Outfp, ";;;(O) %d %d %s:%s %s ->%d\n", pos1, pos2, cp1, case1, cp2, score);
#endif

	    if (score && !firstscore) {
		if (!CorpusComment[ptr1->num][0]) {
		    sprintf(CorpusComment[ptr1->num], "%s:%s %s %d", cp1, case1, cp2, score);
		    if (CorpusComment[ptr1->num][DATA_LEN-1] != '\n') {
			fprintf(stderr, "corpus_optional_case_comp: data length overflow.\n");
			exit(1);
		    }
		}

		if (corpus)
		    corpus->data = strdup(buffer);

		/* full match */
		if (!i && !j)
		    firstscore = 2;
		/* part match */
		else
		    firstscore = 1;
	    }
	}
	if (special)
	    break;
    }

    free(cp1);
    free(cp2);

    if (fukugojiflag)
	free(case1);

    return firstscore*5;
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

/* 事例情報を用いた構文木の選択するか否か */
void optional_case_evaluation(SENTENCE_DATA *sp)
{
    int i;
    int appropriate = 0;

    /* 普通の Best 解と、事例を用いた場合の Best 解が同じならば return */
    if (Op_Best_mgr.ID < 0 || sp->Best_mgr->ID == Op_Best_mgr.ID)
	return;

    /* 学習時でなければ */
    if (!OptLearn) {
	for (i = 0;i < sp->Bnst_num; i++) {
	    /* 事例を用いた文節 */
	    if (Op_Best_mgr.dpnd.op[i].flag && sp->Best_mgr->dpnd.head[i] != Op_Best_mgr.dpnd.head[i]) {

		/* 係り先が異なり、事例を用いていればデータ出力 */
		if (Op_Best_mgr.dpnd.op[i].data)
		    assign_cfeature(&(Op_Best_mgr.dpnd.f[i]), Op_Best_mgr.dpnd.op[i].data);

		/* 読点があれば */
		if (check_feature(Op_Best_mgr.dpnd.f[i], "読点")) {
		    appropriate++;
		}
		/* 読点がなければ */
		else {
		    /* 事例を用いたときのほうが近いとき */
		    if (Op_Best_mgr.dpnd.head[i] < sp->Best_mgr->dpnd.head[i])
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
	   if (Op_Best_mgr.score > sp->Best_mgr->score && appropriate >= 0) {
	   */
	if (Op_Best_mgr.score > sp->Best_mgr->score) {
	    *(sp->Best_mgr) = Op_Best_mgr;
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
    char *type1, *type2, *level1, *level2;
    char parallel1, parallel2, touten1, touten2;
    int score1, score2, offset;

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

void CheckChildCaseFrame(SENTENCE_DATA *sp) {
    int i, j;
    TOTAL_MGR *tm = sp->Best_mgr;

    for (i = sp->Bnst_num-1; i > 0; i--) {
	if (!check_feature((sp->bnst_data+i)->f, "用言"))
	    continue;
	for (j = 0; j < i; j++) {
	    if (tm->dpnd.head[j] == i) {
		assign_cfeature(&((sp->bnst_data+i)->f), "子○");
		break;
	    }
	}
    }
}

/* これより下 Unsupervised 関連 (未整理) */

void _make_sm_frame(char *buf, char *delimiter, _SMCaseFrame* cf) {
    char *token;

    token = strtok_r(buf, delimiter, &buf);
    while (token) {
	if (cf->num >= cf->maxnum) {
	    cf->maxnum += SM_ALLOCATION_STEP;
	    cf->list = (SMwithFrequency *)realloc(cf->list, sizeof(SMwithFrequency)*cf->maxnum);
	}
	(cf->list+cf->num)->sm = (char *)malloc_data(SM_CODE_SIZE+1, "_make_sm_frame");
	strncpy((cf->list+cf->num)->sm, token, SM_CODE_SIZE);
	*((cf->list+cf->num)->sm+SM_CODE_SIZE) = '\0';
	(cf->list+cf->num)->frequency = atoi(token+SM_CODE_SIZE+1);
	/* printf("====> <%s>\n", token); */
	token = strtok_r(NULL, delimiter, &buf);
	cf->num++;
    }
}

void make_sm_frame(char *buf, char *delimiter, SMCaseFrame* cf) {
    char *token, *cp;

    token = strtok_r(buf, delimiter, &buf);
    while (token) {
	cp = strchr(token, ':');
	if (cp) {
	    *cp = '\0';
	    if (cf->num >= cf->maxnum) {
		cf->maxnum += CASE_ALLOCATION_STEP;
		cf->frame = (_SMCaseFrame *)realloc(cf->frame, sizeof(_SMCaseFrame)*cf->maxnum);
	    }
	    (cf->frame+cf->num)->relation = strdup(token);	/* 係関係の設定 */
	    (cf->frame+cf->num)->list = NULL;
	    (cf->frame+cf->num)->num = 0;
	    (cf->frame+cf->num)->maxnum = 0;
	    token = cp+1;
	}
	/* printf("==> <%s>\n", token); */
	_make_sm_frame(token, " ", cf->frame+cf->num);
	token = strtok_r(NULL, delimiter, &buf);
	cf->num++;
    }
}

float match_sm_tree(char *code, char *rel, SMCaseFrame* cf) {
    char *cp, *sm, backup, *cp1;
    int i, n = -1, value = 0, count = 0;

    if (!*code)
	return 0;

    for (i = 0; i < cf->num; i++) {
	if (str_eq((cf->frame+i)->relation, rel)) {
	    n = i;
	    break;
	}
    }

    /* 格がマッチしないとき */
    if (n < 0)
	return 0;

    for (cp = sm = code; *cp; cp += SM_CODE_SIZE) {
	count++;
	if (cp == sm)
	    continue;
	backup = *cp;
	*cp = '\0';
	cp1 = strdup(sm);
	sm = cp;
	*cp = backup;
	*cp1 = '1';

	for (i = 0; i < (cf->frame+n)->num; i++) {
	    if (comp_sm(((cf->frame+n)->list+i)->sm, cp1, 0) > 0) {
		value += ((cf->frame+n)->list+i)->frequency;
		/* fprintf(stdout, "%s: %s %s * %d\n", cf->predicate, cp1, ((cf->frame+n)->list+i)->sm, ((cf->frame+n)->list+i)->frequency); */
	    }
	}
	/* fprintf(stdout, "V:%d N:%d\n", value, count); */
    }
    if (count)
	return value/count;
    else
	return 0;
}

char *get_unsupervised_data(DBM_FILE db, char *key, char c, char p) {

    /* DB を検索するためのキーを作成 */
    if (c && p) {
	sprintf(buffer, "%s%c%c", key, c, p);
    }
    else if (c) {
	sprintf(buffer, "%s%c", key, c);
    }
    else if (p) {
	sprintf(buffer, "%s%c", key, p);
    }
    else {
	sprintf(buffer, "%s", key);
    }
    if (buffer[DATA_LEN-1] != '\n') {
	fprintf(stderr, "CorpusExampleDependencyFrequency: data length overflow.\n");
	exit(1);
    }

    /* DB 検索 */
    return db_get(db, buffer);
}

/* 意味素の係り受け頻度を調べる関数 */
float CorpusSMDependencyFrequency(SENTENCE_DATA *sp, BNST_DATA *ptr1, char *case1, BNST_DATA *ptr2, CORPUS_DATA *corpus, int target)
{
    int i, j, k, pos1, pos2;
    int fukugojiflag = 0;
    char *cp2 = NULL, *cp, *string = NULL, causative = 0, passive = 0;
    float score = 0;

    /* 文節番号 */
    pos1 = ptr1->num;
    pos2 = ptr2->num;

    /* 文節 feature のチェック */
    if (check_feature_for_optional_case(ptr1->f) == TRUE)
	return 0;

    /* 受け側が受動態の場合 */
    if (check_feature(ptr2->f, "〜れる") || check_feature(ptr2->f, "〜られる"))
	passive = 'P';

    /* 使役の場合 */
    if (check_feature(ptr2->f, "〜せる") || check_feature(ptr2->f, "〜させる"))
	causative = 'C';

    /* 補文 */
    if (str_eq(case1, "ト格") && (check_feature(ptr1->f, "ID:〜と（引用）") || 
				  check_feature(ptr1->f, "ID:（区切）"))) {
	return 0;
    }

    /* 時間 */
    else if (check_feature(ptr1->f, "時間")) {
	return 0;
    }
    /* 複合辞 */
    else if (check_feature(ptr1->f, "複合辞") && ptr1->num > 0) {
	cp = ptr1->Jiritu_Go;

	/* ひとつ前の文節の自立語をみる */
	pos1 = ptr1->num - 1;
	ptr1 = sp->bnst_data + pos1;

	/* 係の関係としては、前の付属語+自分の自立語 */
	case1 = (char *)malloc_data(strlen((ptr1->fuzoku_ptr + ptr1->fuzoku_num - 1)->Goi) + 
				    strlen(cp) + 1, "CorpusSMDependencyFrequency");
	sprintf(case1, "%s%s", (ptr1->fuzoku_ptr + ptr1->fuzoku_num - 1)->Goi, cp);
	fukugojiflag = 1;
    }

    /* Memory 確保 */
    if (!cp2) {
	cp2 = (char *)malloc_data(strlen(ptr2->Jiritu_Go)+1, "CorpusSMDependencyFrequency");
	cp2[0] = '\0';
    }

    /* DB を引くため、受け側自立語を調節 */
    for (j = 0; j < ptr2->jiritu_num; j++) {
	cp2[0] = '\0';

	/* 受け側自立語列の作成 */
	for (k = j; k < ptr2->jiritu_num; k++)
	    strcat(cp2, (ptr2->jiritu_ptr+k)->Goi);

	/* DB 検索 */
	if ((string = get_unsupervised_data(op_sm_db, cp2, causative, passive)))
	    break; /* あれば break */
    }

    /* 意味素 */
    if (string) {
	/* 初期化 */
	smcf.num = 0;
	smcf.predicate = strdup(cp2);

	make_sm_frame(string, ";", &smcf);
	score = match_sm_tree(ptr1->SM_code, case1, &smcf);

	/* 後始末 */
	if (smcf.frame) {
	    for (i = 0; i < smcf.num; i++) {
		if ((smcf.frame+i)->list) {
		    for (j = 0; j < (smcf.frame+i)->num; j++)
			if (((smcf.frame+i)->list+j)->sm)
			    free(((smcf.frame+i)->list+j)->sm);
		    free((smcf.frame+i)->list);
		}
		(smcf.frame+i)->num = 0;
		(smcf.frame+i)->maxnum = 0;
		if ((smcf.frame+i)->relation)
		    free((smcf.frame+i)->relation);
	    }
	    free(smcf.predicate);
	}
    }

    free(string);
    free(cp2);

    if (fukugojiflag)
	free(case1);

    return score;
}

float get_unsupervised_num(DBM_FILE db, char *cp1, char *case1, char *cp2, char c, char p) {

    /* DB を検索するためのキーを作成 */
    if (c && p) {
	sprintf(buffer, "%s:%s %s%c%c", cp1, case1, cp2, c, p);
    }
    else if (c) {
	sprintf(buffer, "%s:%s %s%c", cp1, case1, cp2, c);
    }
    else if (p) {
	sprintf(buffer, "%s:%s %s%c", cp1, case1, cp2, p);
    }
    else {
	sprintf(buffer, "%s:%s %s", cp1, case1, cp2);
    }
    if (buffer[DATA_LEN-1] != '\n') {
	fprintf(stderr, "CorpusExampleDependencyFrequency: data length overflow.\n");
	exit(1);
    }

    /* DB 検索 */
    return dbfetch_num(db, buffer);
}

/* 事例の係り受け頻度を返す関数 */
float CorpusExampleDependencyFrequency(SENTENCE_DATA *sp, BNST_DATA *ptr1, char *case1, BNST_DATA *ptr2, CORPUS_DATA *corpus, int target) {
    int i, k, score, flag, pos1, pos2, special = 0;
    int fukugojiflag = 0;
    char *cp1, *cp2, *cp;
    float maxscore = 0, tempscore;
    char *pbuffer = NULL, causative = 0, passive = 0;

    /* 文節番号 */
    pos1 = ptr1->num;
    pos2 = ptr2->num;

    /* 文節 feature のチェック */
    if (check_feature_for_optional_case(ptr1->f) == TRUE)
	return 0;

    /* 受け側が受動態の場合 */
    if (check_feature(ptr2->f, "〜れる") || check_feature(ptr2->f, "〜られる"))
	passive = 'P';

    /* 使役の場合 */
    if (check_feature(ptr2->f, "〜せる") || check_feature(ptr2->f, "〜させる"))
	causative = 'C';

    /* 補文 */
    if (str_eq(case1, "ト格") && (check_feature(ptr1->f, "ID:〜と（引用）") || 
				  check_feature(ptr1->f, "ID:（区切）"))) {
	cp1 = strdup("C:補文");
	special = 1;
    }
    /* 時間 */
    else if (check_feature(ptr1->f, "時間")) {
	cp1 = strdup("C:時間");
	special = 1;
    }
    /* 複合辞 */
    else if (check_feature(ptr1->f, "複合辞") && ptr1->num > 0) {
	cp = ptr1->Jiritu_Go;

	/* ひとつ前の文節の自立語をみる */
	pos1 = ptr1->num - 1;
	ptr1 = sp->bnst_data + pos1;

	/* 係の関係としては、前の付属語+自分の自立語 */
	case1 = (char *)malloc_data(strlen((ptr1->fuzoku_ptr + ptr1->fuzoku_num - 1)->Goi) + 
				    strlen(cp) + 1, "optional_case");
	sprintf(case1, "%s%s", (ptr1->fuzoku_ptr + ptr1->fuzoku_num - 1)->Goi, cp);
	fukugojiflag = 1;
    }

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

    /* count = dbfetch_num(wc_db, cp2); */

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

	score = get_unsupervised_num(op_db, cp1, case1, cp2, causative, passive);

	if (target) {
	    sprintf(buffer, "Unsupervised:%s:%s %s -> %d", cp1, case1, cp2, score);
	    if (buffer[DATA_LEN-1] != '\n') {
		fprintf(stderr, "corpus_optional_case_comp: data length overflow.\n");
		exit(1);
	    }
	}

#ifdef DEBUGMORE
	fprintf(Outfp, ";;;(O) %d %d %s:%s %s(%d) ->%d\n", pos1, pos2, cp1, case1, cp2, score);
#endif

	if (score) {
	    /* tempscore = (float)score/count; */
	    tempscore = (float)score;
	    if (maxscore < tempscore) {
		maxscore = tempscore;
		if (target) {
		    if (pbuffer)
			free(pbuffer);
		    pbuffer = strdup(buffer);
		}
	    }
	}
	if (special)
	    break;
    }

    /* Feature へのデータ出力 */
    if (target && corpus && pbuffer) {
	if (corpus->data) {
	    char *newstr;
	    newstr = (char *)malloc_data(strlen(corpus->data)+strlen(pbuffer)+2, "CorpusExampleDependencyFrequency");
	    sprintf(newstr, "%s %s", corpus->data, pbuffer);
	    free(corpus->data);
	    corpus->data = newstr;
	}
	else {
	    corpus->data = strdup(pbuffer);
	}
    }

    free(cp1);
    free(cp2);

    if (fukugojiflag)
	free(case1);

    return maxscore;	/* 最大スコアをかえす */
}

/* 係り先リストが与えられたときに、指定された係り先のスコアを計算する関数 */
int CorpusExampleDependencyCalculation(SENTENCE_DATA *sp, BNST_DATA *ptr1, char *case1, int h, CHECK_DATA *list, CORPUS_DATA *corpus)
{
    int i, flag;
    float score, currentscore = -1, totalscore = 0;
    float smscore, currentsmscore = -1, totalsmscore = 0;
    float lastscore, ratio, jiprob = 0, smprob = 0;
    float *candidates_score, *candidates_smscore;
    char *newstr;

    if (list->num < 0)
	return 0;

    candidates_score = (float *)malloc_data(sizeof(float)*list->num, "CorpusExampleDependencyCalculation");
    candidates_smscore = (float *)malloc_data(sizeof(float)*list->num, "CorpusExampleDependencyCalculation");

    /* fprintf(Outfp, "○ %2d ==>\n", ptr1->num); */

    /* 候補すべてのスコアを計算する */
    for (i = 0; i < list->num; i++) {
	if (list->pos[i] == h)
	    flag = 1;
	else
	    flag = 0;

	/* 自立語のスコアを計算する */
	score = CorpusExampleDependencyFrequency(sp, ptr1, case1, sp->bnst_data+list->pos[i], corpus, flag);
	totalscore += score;
	*(candidates_score+i) = score;

	/* 意味素のスコアを計算する */
	smscore = CorpusSMDependencyFrequency(sp, ptr1, case1, sp->bnst_data+list->pos[i], corpus, flag);
	totalsmscore += smscore;
	*(candidates_smscore+i) = smscore;

	/* fprintf(Outfp, "           %2d 自立:%f 意味:%f\n", list->pos[i], score, smscore); */

	/* 今調べている Head であるとき */
	if (flag) {
	    currentscore = score;
	    currentsmscore = smscore;
	}
    }

    if (currentscore < 0 || currentsmscore < 0) {
	fprintf(stderr, "A contradiction occured.\n");
	exit(1);
    }

    if (totalscore)
	jiprob = currentscore/totalscore;
    if (totalsmscore)
	smprob = currentsmscore/totalsmscore;

    ratio = currentscore/(currentscore+1);
    if (totalscore && totalsmscore)
	lastscore = ratio*jiprob+(1-ratio)*smprob;
    else if (totalscore)
	lastscore = jiprob;
    else if (totalsmscore)
	lastscore = smprob;
    else
	lastscore = 0;

    sprintf(buffer, "自立:%.2f,意味:%.2f,確:%.2f", jiprob, smprob, lastscore);
    if (buffer[DATA_LEN-1] != '\n') {
	fprintf(stderr, "CorpusExampleDependencyCalculation: data length overflow.\n");
	exit(1);
    }

    if (corpus) {
	if (corpus->data) {
	    newstr = (char *)malloc_data(strlen(corpus->data)+strlen(buffer)+2, "CorpusExampleDependencyCalculation");
	    sprintf(newstr, "%s %s", corpus->data, buffer);
	    free(corpus->data);
	    corpus->data = newstr;
	}
	else {
	    newstr = (char *)malloc_data(strlen(buffer)+14, "CorpusExampleDependencyCalculation");
	    sprintf(newstr, "Unsupervised:%s", buffer);
	    corpus->data = newstr;
	}

	corpus->candidatesdata = (char *)malloc_data(strlen(buffer)*list->num+DATA_LEN, "CorpusExampleDependencyCalculation");
	*(corpus->candidatesdata) = '\0';

	/* 候補それぞれのスコア */
	for (i = 0; i < list->num; i++) {
	    if (list->pos[i] == h)
		continue;

	    if (totalscore)
		jiprob = candidates_score[i]/totalscore;
	    if (totalsmscore)
		smprob = candidates_smscore[i]/totalsmscore;

	    ratio = candidates_score[i]/(candidates_score[i]+1);
	    if (totalscore && totalsmscore)
		score = ratio*jiprob+(1-ratio)*smprob;
	    else if (totalscore)
		score = jiprob;
	    else if (totalsmscore)
		score = smprob;
	    else
		score = 0;

	    sprintf(buffer, " %s(%d) 自立:%.2f,意味:%.2f,確:%.2f", (sp->bnst_data+list->pos[i])->Jiritu_Go, list->pos[i], jiprob, smprob, score);
	    if (buffer[DATA_LEN-1] != '\n') {
		fprintf(stderr, "CorpusExampleDependencyCalculation: data length overflow.\n");
		exit(1);
	    }
	    strcat(corpus->candidatesdata, buffer);
	}
    }

    free(candidates_score);
    free(candidates_smscore);

    return (int)(lastscore*10);
}

void unsupervised_debug_print(SENTENCE_DATA *sp) {
    int i;

    for (i = 0; i < sp->Bnst_num; i++) {
	/* 事例を用いた文節 */
	if (sp->Best_mgr->dpnd.op[i].flag) {
	    /* 出力して free */
	    if (sp->Best_mgr->dpnd.op[i].data) {
		fprintf(Outfp, "; %s(%d) %s\n", (sp->bnst_data+i)->Jiritu_Go, i, sp->Best_mgr->dpnd.op[i].data);
		if (*(sp->Best_mgr->dpnd.op[i].candidatesdata))
		    fprintf(Outfp, ";        %s\n", sp->Best_mgr->dpnd.op[i].candidatesdata);
		free(sp->Best_mgr->dpnd.op[i].data);
		sp->Best_mgr->dpnd.op[i].data = NULL;
	    }
	    if (sp->Best_mgr->dpnd.op[i].candidatesdata) {
		free(sp->Best_mgr->dpnd.op[i].candidatesdata);
		sp->Best_mgr->dpnd.op[i].candidatesdata = NULL;
	    }
	}
    }
}
