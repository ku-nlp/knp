/*====================================================================

			       文脈解析

                                         Daisuke Kawahara 2001. 7. 13

    $Id$
====================================================================*/
#include "knp.h"

float maxscore;
float maxrawscore;
SENTENCE_DATA *maxs;
int maxi, maxpos;
char *maxtag, *maxfeatures;
int Bcheck[BNST_MAX];

/* #define USE_RAWSCORE */

#define RANK0	0
#define RANK1	1
#define RANK2	2
#define RANK3	3
#define RANK4	4
#define RANKP	5

#define	PLOC_PV		0x0001
#define	PLOC_PARENTV	0x0010
#define	PLOC_CHILDV	0x0100
#define	PLOC_CHILDPV	0x0300
#define	PLOC_NPARENTV	0x0002	/* サ変名詞の親 */
#define	PLOC_NPARENTCV	0x0006	/* サ変名詞の親の従属節 */
#define	PLOC_NOTHERS	0x000e

/* ※ 位置の数を変えたら、LOC_NUMBER(const.h)を変えること */

#define	NON_IMPORTANT_WEIGHT	0.6

char *ExtraTags[] = {"対象外", "一人称", "不特定-人", "不特定-状況", ""};

char *ETAG_name[] = {
    "", "", "不特定:人", "一人称", "不特定:状況", 
    "前文", "後文", "対象外"};

float	AssignReferentThreshold = 0.67;
float	AssignReferentThresholdDecided = 0.50;
float	AssignGaCaseThreshold = 0.67;	/* ガ格を【主体一般】にする閾値 */
float	AssignReferentThresholdHigh = 0.80;
float	AssignReferentThresholdAnonymousThing = 0.90;

float	AssignReferentThresholdForSVM = -0.9999;
float	AntecedentDecideThreshold = 0.70;

int	EllipsisSubordinateClauseScore = 10;

ALIST alist[TBLSIZE];		/* リンクされた単語+頻度のリスト */
ALIST banlist[TBLSIZE];		/* 用言ごとの禁止単語 */
PALIST palist[TBLSIZE];		/* 用言と格要素のセットのリスト */
PALIST **ClauseList;		/* 各文の主節 */
int ClauseListMax = 0;

extern int	EX_match_subject;

#define CASE_ORDER_MAX	3
char *CaseOrder[CASE_ORDER_MAX][4] = {
    {"ガ", "ヲ", "ニ", ""}, 
    {"ヲ", "ニ", "ガ", ""}, 
    {"ニ", "ヲ", "ガ", ""}, 
};
int DiscAddedCases[PP_NUMBER] = {END_M};


/*==================================================================*/
		       void InitAnaphoraList()
/*==================================================================*/
{
    memset(alist, 0, sizeof(ALIST)*TBLSIZE);
    memset(banlist, 0, sizeof(ALIST)*TBLSIZE);
    memset(palist, 0, sizeof(PALIST)*TBLSIZE);
}

/*==================================================================*/
		void InitEllipsisMGR(ELLIPSIS_MGR *em)
/*==================================================================*/
{
    memset(em, 0, sizeof(ELLIPSIS_MGR));
}

/*==================================================================*/
	       void ClearEllipsisMGR(ELLIPSIS_MGR *em)
/*==================================================================*/
{
    clear_feature(&(em->f));
    InitEllipsisMGR(em);
}

/*==================================================================*/
		 void ClearAnaphoraList(ALIST *list)
/*==================================================================*/
{
    int i;
    ALIST *p, *next;
    for (i = 0; i < TBLSIZE; i++) {
	if (list[i].key) {
	    free(list[i].key);
	    list[i].key = NULL;
	}
	if (list[i].next) {
	    p = list[i].next;
	    list[i].next = NULL;
	    while (p) {
		free(p->key);
		next = p->next;
		free(p);
		p = next;
	    }
	}
	list[i].count = 0;
    }
}

/*==================================================================*/
	     void RegisterAnaphor(ALIST *list, char *key)
/*==================================================================*/
{
    ALIST *ap;

    ap = &(list[hash(key, strlen(key))]);
    if (ap->key) {
	ALIST **app;
	app = &ap;
	do {
	    if (!strcmp((*app)->key, key)) {
		(*app)->count++;
		return;
	    }
	    app = &((*app)->next);
	} while (*app);
	*app = (ALIST *)malloc_data(sizeof(ALIST), "RegisterAnaphor");
	(*app)->key = strdup(key);
	(*app)->count = 1;
	(*app)->next = NULL;
    }
    else {
	ap->key = strdup(key);
	ap->count = 1;
    }
}

/*==================================================================*/
	       int CheckAnaphor(ALIST *list, char *key)
/*==================================================================*/
{
    ALIST *ap;

    ap = &(list[hash(key, strlen(key))]);
    if (!ap->key) {
	return 0;
    }
    while (ap) {
	if (!strcmp(ap->key, key)) {
	    return ap->count;
	}
	ap = ap->next;
    }
    return 0;
}

/*==================================================================*/
		       int CheckBasicPP(int pp)
/*==================================================================*/
{
    /* 複合辞などの格は除く */
    if (pp == END_M || pp > 8 || pp < 0) {
	return 0;
    }
    return 1;
}

/*==================================================================*/
 void StoreCaseComponent(CASE_COMPONENT **ccpp, char *word, int flag)
/*==================================================================*/
{
    /* 格要素を登録する */

    while (*ccpp) {
	/* すでに登録されているとき */
	if (!strcmp((*ccpp)->word, word)) {
	    /* 元が省略関係で今が格関係なら、すべて格関係にする */
	    if ((*ccpp)->flag == EREL && flag == CREL) {
		(*ccpp)->flag = CREL;
	    }
	    (*ccpp)->count++;
	    return;
	}
	ccpp = &((*ccpp)->next);
    }
    *ccpp = (CASE_COMPONENT *)malloc_data(sizeof(CASE_COMPONENT), "StoreCaseComponent");
    (*ccpp)->word = strdup(word);
    (*ccpp)->count = 1;
    (*ccpp)->flag = flag;
    (*ccpp)->next = NULL;
}

/*==================================================================*/
void RegisterClause(PALIST **list, char *key, int pp, char *word, int flag)
/*==================================================================*/
{
    /* 主節を登録する */

    if (CheckBasicPP(pp) == 0) {
	return;
    }

    if (*list == NULL) {
	*list = (PALIST *)malloc_data(sizeof(PALIST), "RegisterClause");
	(*list)->key = strdup(key);
	memset((*list)->cc, 0, sizeof(CASE_COMPONENT *)*CASE_MAX_NUM);
	(*list)->next = NULL;
    }
    StoreCaseComponent(&((*list)->cc[pp]), word, flag);
}

/*==================================================================*/
void RegisterLastClause(int Snum, char *key, int pp, char *word, int flag)
/*==================================================================*/
{
    /* 前文の主節を登録する */

    /* 文番号は配列添字に対して 1 多い */
    Snum--;

    while (Snum >= ClauseListMax) {
	int i, start;
	start = ClauseListMax;
	if (ClauseListMax == 0) {
	    ClauseListMax = 1;
	    ClauseList = (PALIST **)malloc_data(sizeof(PALIST *)*(ClauseListMax), 
						"RegisterLastClause");
	}
	else {
	    ClauseList = (PALIST **)realloc_data(ClauseList, 
						 sizeof(PALIST *)*(ClauseListMax <<= 1), 
						 "RegisterLastClause");
	}
	/* 初期化 */
	for (i = start; i < ClauseListMax; i++) {
	    *(ClauseList+i) = NULL;
	}
    }
    RegisterClause(ClauseList+Snum, key, pp, word, flag);
}

/*==================================================================*/
	  int CheckClause(PALIST *list, int pp, char *word)
/*==================================================================*/
{
    int i;
    CASE_COMPONENT *ccp;

    if (list == NULL) {
	return 0;
    }

    if (CheckBasicPP(pp) == 0) {
	return 0;
    }

    /* 現在は格の一致はチェックしない */
    for (i = 0; i < CASE_MAX_NUM; i++) {
	ccp = list->cc[i];
	while (ccp) {
	    if (!strcmp(ccp->word, word)) {
		return 1;
	    }
	    ccp = ccp->next;
	}
    }
    return 0;
}

/*==================================================================*/
	  int CheckLastClause(int Snum, int pp, char *word)
/*==================================================================*/
{
    if (Snum < 1 || Snum > ClauseListMax) {
	return 0;
    }
    return CheckClause(*(ClauseList+Snum-1), pp, word);
}

/*==================================================================*/
   void RegisterPredicate(char *key, int voice, int cf_addr, 
			  int pp, char *word, int flag)
/*==================================================================*/
{
    /* 用言と格要素をセットで登録する */

    PALIST *pap;

    if (CheckBasicPP(pp) == 0) {
	return;
    }

    pap = &(palist[hash(key, strlen(key))]);
    if (pap->key) {
	PALIST **papp;
	papp = &pap;
	do {
	    if (!strcmp((*papp)->key, key) && 
		(*papp)->voice == voice && 
		(*papp)->cf_addr == (*papp)->cf_addr) {
		StoreCaseComponent(&((*papp)->cc[pp]), word, flag);
		return;
	    }
	    papp = &((*papp)->next);
	} while (*papp);
	*papp = (PALIST *)malloc_data(sizeof(PALIST), "RegisterPredicate");
	(*papp)->key = strdup(key);
	(*papp)->voice = voice;
	(*papp)->cf_addr = cf_addr;
	memset((*papp)->cc, 0, sizeof(CASE_COMPONENT *)*CASE_MAX_NUM);
	StoreCaseComponent(&((*papp)->cc[pp]), word, flag);
	(*papp)->next = NULL;
    }
    else {
	pap->key = strdup(key);
	pap->voice = voice;
	pap->cf_addr = cf_addr;
	StoreCaseComponent(&(pap->cc[pp]), word, flag);
    }
}

/*==================================================================*/
     int CheckPredicate(char *key, int voice, int cf_addr, 
			int pp, char *word)
/*==================================================================*/
{
    PALIST *pap;
    CASE_COMPONENT *ccp;

    if (CheckBasicPP(pp) == 0) {
	return 0;
    }

    pap = &(palist[hash(key, strlen(key))]);
    if (!pap->key) {
	return 0;
    }
    while (pap) {
	if (!strcmp(pap->key, key) && 
	    pap->voice == voice && 
	    pap->cf_addr == cf_addr) {
	    ccp = pap->cc[pp];
	    while (ccp) {
		if (!strcmp(ccp->word, word)) {
		    /* 格関係 */
		    if (ccp->flag == CREL) {
			return 2;
		    }
		    return 1;
		}
		ccp = ccp->next;
	    }
	    return 0;
	}
	pap = pap->next;
    }
    return 0;
}

/*==================================================================*/
		void ClearSentence(SENTENCE_DATA *s)
/*==================================================================*/
{
    free(s->mrph_data);
    free(s->bnst_data);
    free(s->para_data);
    free(s->para_manager);
    if (s->cpm)
	free(s->cpm);
    if (s->cf)
	free(s->cf);
    if (s->KNPSID)
	free(s->KNPSID);
    if (s->Best_mgr) {
	clear_mgr_cf(s);
	free(s->Best_mgr);
    }
}

/*==================================================================*/
		void ClearSentences(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;
    for (i = 0; i < sp->Sen_num-1; i++) {
	ClearSentence(sentence_data+i);
    }
    sp->Sen_num = 1;
    ClearAnaphoraList(alist);
    /* palist の clear */
    /* ClauseList の clear */
    InitAnaphoraList();
}

/*==================================================================*/
		 void InitSentence(SENTENCE_DATA *s)
/*==================================================================*/
{
    int i, j;

    s->mrph_data = (MRPH_DATA *)malloc_data(sizeof(MRPH_DATA)*MRPH_MAX, "InitSentence");
    s->bnst_data = (BNST_DATA *)malloc_data(sizeof(BNST_DATA)*BNST_MAX, "InitSentence");
    s->para_data = (PARA_DATA *)malloc_data(sizeof(PARA_DATA)*PARA_MAX, "InitSentence");
    s->para_manager = (PARA_MANAGER *)malloc_data(sizeof(PARA_MANAGER)*PARA_MAX, "InitSentence");
    s->Best_mgr = (TOTAL_MGR *)malloc_data(sizeof(TOTAL_MGR), "InitSentence");
    s->Sen_num = 0;
    s->Mrph_num = 0;
    s->Bnst_num = 0;
    s->New_Bnst_num = 0;
    s->KNPSID = NULL;
    s->Comment = NULL;
    s->cpm = NULL;
    s->cf = NULL;

    for (i = 0; i < MRPH_MAX; i++)
	(s->mrph_data+i)->f = NULL;
    for (i = 0; i < BNST_MAX; i++)
	(s->bnst_data+i)->f = NULL;
    for (i = 0; i < PARA_MAX; i++) {
	for (j = 0; j < RF_MAX; j++) {
	    (s->para_data+i)->f_pattern.fp[j] = NULL;
	}
    }

    init_mgr_cf(s->Best_mgr);
}

/*==================================================================*/
	  SENTENCE_DATA *PreserveSentence(SENTENCE_DATA *sp)
/*==================================================================*/
{
    /* 文解析結果の保持 */

    int i, j;
    SENTENCE_DATA *sp_new;

    /* 一時的措置 */
    if (sp->Sen_num > 256) {
	fprintf(stderr, "Sentence buffer overflowed!\n");
	ClearSentences(sp);
    }

    sp_new = sentence_data + sp->Sen_num - 1;
    sp_new->Sen_num = sp->Sen_num;

    sp_new->Mrph_num = sp->Mrph_num;
    sp_new->mrph_data = (MRPH_DATA *)malloc_data(sizeof(MRPH_DATA)*sp->Mrph_num, 
						 "MRPH DATA");
    for (i = 0; i < sp->Mrph_num; i++) {
	sp_new->mrph_data[i] = sp->mrph_data[i];
    }

    sp_new->Bnst_num = sp->Bnst_num;
    sp_new->bnst_data = 
	(BNST_DATA *)malloc_data(sizeof(BNST_DATA)*(sp->Bnst_num + sp->New_Bnst_num), 
				 "BNST DATA");
    for (i = 0; i < sp->Bnst_num + sp->New_Bnst_num; i++) {

	sp_new->bnst_data[i] = sp->bnst_data[i]; /* ここでbnst_dataをコピー */

	/* SENTENCE_DATA 型 の sp は, MRPH_DATA をメンバとして持っている    */
	/* 同じく sp のメンバである BNST_DATA は MRPH_DATA をメンバとして   */
        /* 持っている。                                                     */
	/* で、単に BNST_DATA をコピーしただけだと、BNST_DATA 内の MRPH_DATA */
        /* は, sp のほうの MRPH_DATA を差したままコピーされる */
	/* よって、以下でポインタアドレスのずれを補正        */


        /*
             sp -> SENTENCE_DATA                                              sp_new -> SENTENCE_DATA 
                  +-------------+				                   +-------------+
                  |             |				                   |             |
                  +-------------+				                   +-------------+
                  |             |				                   |             |
       BNST_DATA  +=============+		   ┌─────────────    +=============+ BNST_DATA
                0 |             |────────┐│                            0 |             |
                  +-------------+                ↓↓		                   +-------------+
                1 |             |              BNST_DATA	                 1 |             |
                  +-------------+                  +-------------+                 +-------------+
                  |   ・・・    |	           |             |                 |   ・・・    |
                  +-------------+	           +-------------+                 +-------------+
                n |             |  ┌─ MRPH_DATA  |* mrph_ptr   |- ┐           n |             |
                  +=============+  │	           +-------------+  │             +=============+
                  |             |  │	MRPH_DATA  |* settou_ptr |  │             |             |
       MRPH_DATA  +=============+  │	           +-------------+  │             +=============+ MRPH_DATA
                0 | * mrph_data |  │	MRPH_DATA  |* jiritu_ptr |  └ - - - - - 0 | * mrph_data |
                  +-------------+  │              +-------------+    ↑           +-------------+
                  |   ・・・    |←┘	 			      │           |   ・・・    |
                  +-------------+      		                      │           +-------------+
                n | * mrph_data |	 			      │         n | * mrph_data |
                  +=============+	 			      │           +=============+
                                                                      │
		                                            単にコピーしたままだと,
		                                            sp_new->bnst_data[i] の
		                                      	    mrph_data は, sp のデータを
		                                            指してしまう。
		                                            元のデータ構造を保つためには、
		                                            自分自身(sp_new)のデータ(メンバ)
		                              		    を指すように,修正する必要がある。
	*/



	sp_new->bnst_data[i].mrph_ptr = sp_new->mrph_data + (sp->bnst_data[i].mrph_ptr - sp->mrph_data);
	if (sp->bnst_data[i].settou_ptr)
	    sp_new->bnst_data[i].settou_ptr = sp_new->mrph_data + (sp->bnst_data[i].settou_ptr - sp->mrph_data);
	sp_new->bnst_data[i].jiritu_ptr = sp_new->mrph_data + (sp->bnst_data[i].jiritu_ptr - sp->mrph_data);
	if (sp->bnst_data[i].fuzoku_ptr)
	sp_new->bnst_data[i].fuzoku_ptr = sp_new->mrph_data + (sp->bnst_data[i].fuzoku_ptr - sp->mrph_data);
	if (sp->bnst_data[i].parent)
	    sp_new->bnst_data[i].parent = sp_new->bnst_data + (sp->bnst_data[i].parent - sp->bnst_data);
	for (j = 0; sp_new->bnst_data[i].child[j]; j++) {
	    sp_new->bnst_data[i].child[j] = sp_new->bnst_data + (sp->bnst_data[i].child[j] - sp->bnst_data);
	}
	if (sp->bnst_data[i].pred_b_ptr) {
	    sp_new->bnst_data[i].pred_b_ptr = sp_new->bnst_data + (sp->bnst_data[i].pred_b_ptr - sp->bnst_data);
	}
    }

    if (sp->KNPSID)
	sp_new->KNPSID = strdup(sp->KNPSID);
    else
	sp_new->KNPSID = NULL;

    sp_new->para_data = (PARA_DATA *)malloc_data(sizeof(PARA_DATA)*sp->Para_num, 
				 "PARA DATA");
    for (i = 0; i < sp->Para_num; i++) {
	sp_new->para_data[i] = sp->para_data[i];
	sp_new->para_data[i].manager_ptr += sp_new->para_manager - sp->para_manager;
    }

    sp_new->para_manager = 
	(PARA_MANAGER *)malloc_data(sizeof(PARA_MANAGER)*sp->Para_M_num, 
				    "PARA MANAGER");
    for (i = 0; i < sp->Para_M_num; i++) {
	sp_new->para_manager[i] = sp->para_manager[i];
	sp_new->para_manager[i].parent += sp_new->para_manager - sp->para_manager;
	for (j = 0; j < sp_new->para_manager[i].child_num; j++) {
	    sp_new->para_manager[i].child[j] += sp_new->para_manager - sp->para_manager;
	}
	sp_new->para_manager[i].bnst_ptr += sp_new->bnst_data - sp->bnst_data;
    }
    return sp_new;
}

/*==================================================================*/
      void PreserveCPM(SENTENCE_DATA *sp_new, SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, num, cfnum = 0;

    /* 格解析結果の保存 */
    sp_new->cpm = 
	(CF_PRED_MGR *)malloc_data(sizeof(CF_PRED_MGR)*sp->Best_mgr->pred_num, 
				   "CF PRED MGR");

    /* 格フレームの個数分だけ確保 */
    for (i = 0; i < sp->Best_mgr->pred_num; i++) {
	cfnum += sp->Best_mgr->cpm[i].result_num;
    }
    sp_new->cf = (CASE_FRAME *)malloc_data(sizeof(CASE_FRAME)*cfnum, 
					   "CASE FRAME");

    cfnum = 0;
    for (i = 0; i < sp->Best_mgr->pred_num; i++) {
	*(sp_new->cpm+i) = sp->Best_mgr->cpm[i];
	num = sp->Best_mgr->cpm[i].pred_b_ptr->num;	/* この用言の文節番号 */
	if (num != -1) {
	    sp_new->bnst_data[num].cpm_ptr = sp_new->cpm+i;
	    (sp_new->cpm+i)->pred_b_ptr = sp_new->bnst_data+num;
	}
	/* 内部文節 */
	else {
	    /* internalはfreeしない */
	    (sp_new->cpm+i)->pred_b_ptr->cpm_ptr = sp_new->cpm+i;
	}
	for (j = 0; j < (sp_new->cpm+i)->cf.element_num; j++) {
	    /* 省略じゃない格要素 */
	    if ((sp_new->cpm+i)->elem_b_num[j] > -2) {
		/* 内部文節じゃない */
		if ((sp_new->cpm+i)->elem_b_ptr[j]->num != -1) {
		    (sp_new->cpm+i)->elem_b_ptr[j] = sp_new->bnst_data+((sp_new->cpm+i)->elem_b_ptr[j]-sp->bnst_data);
		}
	    }
	}

	(sp_new->cpm+i)->pred_b_ptr->cf_ptr = sp_new->cf+cfnum;
	for (j = 0; j < (sp_new->cpm+i)->result_num; j++) {
	    copy_cf_with_alloc(sp_new->cf+cfnum, (sp_new->cpm+i)->cmm[j].cf_ptr);
	    (sp_new->cpm+i)->cmm[j].cf_ptr = sp_new->cf+cfnum;
	    sp->Best_mgr->cpm[i].cmm[j].cf_ptr = sp_new->cf+cfnum;
	    cfnum++;
	}
    }

    /* New領域の文節のcpmポインタをはりなおす */
    for (i = sp->Bnst_num; i < sp->Bnst_num + sp->New_Bnst_num; i++) {
	if ((sp_new->bnst_data+i)->cpm_ptr) {
	    (sp_new->bnst_data+i)->cpm_ptr = (sp_new->bnst_data+(sp_new->bnst_data+i)->cpm_ptr->pred_b_ptr->num)->cpm_ptr;
	}
    }

    /* 現在 cpm を保存しているが、Best_mgr を保存した方がいいかもしれない */
    sp_new->Best_mgr = NULL;
}

/*==================================================================*/
float CalcSimilarityForVerb(BNST_DATA *cand, CASE_FRAME *cf_ptr, int n, int *pos)
/*==================================================================*/
{
    char *exd, *exp;
    int i, j, step, expand;
    float score = 0, ex_score;

    exp = cf_ptr->ex[n];
    if (Thesaurus == USE_BGH) {
	exd = cand->BGH_code;
	step = BGH_CODE_SIZE;
    }
    else if (Thesaurus == USE_NTT) {
	exd = cand->SM_code;
	step = SM_CODE_SIZE;
    }

    if (check_feature(cand->f, "Ｔ固有一般展開禁止")) {
	expand = SM_NO_EXPAND_NE;
    }
    else {
	expand = SM_EXPAND_NE;
    }

    /* 意味素なし
       候補にするために -1 を返す */
    if (!exd[0]) {
	ex_score = -1;
    }
    /* exact match */
    else if (cf_match_exactly(cand, cf_ptr->ex_list[n], cf_ptr->ex_num[n], pos)) {
	ex_score = 1.1;
    }
    else {
	/* 最大マッチスコアを求める */
	ex_score = CalcSmWordsSimilarity(exd, cf_ptr->ex_list[n], cf_ptr->ex_num[n], pos, 
					 cf_ptr->sm_delete[n], expand);
    }

    /* 主体のマッチング (とりあえずガ格のときだけ) */
    if (cf_ptr->sm[n] && 
	(MatchPP(cf_ptr->pp[n][0], "ガ") || 
	 (MatchPP(cf_ptr->pp[n][0], "ニ") && 
	  cf_ptr->voice == FRAME_PASSIVE_1))) {
	int flag;
	for (j = 0; cf_ptr->sm[n][j]; j+=step) {
	    if (!strncmp(cf_ptr->sm[n]+j, sm2code("主体"), SM_CODE_SIZE)) {
		flag = 3;
	    }
	    else if (!strncmp(cf_ptr->sm[n]+j, sm2code("人"), SM_CODE_SIZE)) {
		flag = 1;
	    }
	    else if (!strncmp(cf_ptr->sm[n]+j, sm2code("組織"), SM_CODE_SIZE)) {
		flag = 2;
	    }
	    else {
		continue;
	    }
	    if (check_feature(cand->f, "非主体")) {
		return 0;
	    }
	    /* 格フレーム側に <主体> があるときに、格要素側をチェック */
	    if (((flag & 1) && check_feature(cand->f, "人名")) || 
		((flag & 2) && check_feature(cand->f, "組織名"))) {
		score = (float)EX_match_subject/11;
		/* 固有名詞のときにスコアを高く
		if (EX_match_subject > 8) {
		    * 主体スコアが高いときは、それと同じにする *
		    score = (float)EX_match_subject/11;
		}
		else {
		    score = (float)9/11;
		} */
		break;
	    }
	    for (i = 0; cand->SM_code[i]; i+=step) {
		if (_sm_match_score(cf_ptr->sm[n]+j, cand->SM_code+i, expand)) {
		    score = (float)EX_match_subject/11;
		    break;
		}
	    }
	    break;
	}
    }

    if (score > 0) {
	*pos = MATCH_SUBJECT;
	return score;
    }
    else {
	return ex_score;
    }
}

/*==================================================================*/
     float CalcSimilarityForNoun(BNST_DATA *dat, BNST_DATA *pat)
/*==================================================================*/
{
    char *exd, *exp;
    float score = 0, ex_score;

    if (Thesaurus == USE_BGH) {
	exd = dat->BGH_code;
	exp = pat->BGH_code;
    }
    else if (Thesaurus == USE_NTT) {
	exd = dat->SM_code;
	exp = pat->SM_code;
    }

    /* 最大マッチスコアを求める */
    ex_score = (float)calc_similarity(exd, exp, 0);

    if (ex_score > score) {
	return ex_score;
    }
    else {
	return score;
    }
}

/*==================================================================*/
		  int CheckTargetNoun(BNST_DATA *bp)
/*==================================================================*/
{
    /* いつでもチェックされる */

    if (!check_feature(bp->f, "修飾") && 
	!check_feature(bp->f, "修飾的") && 
	!check_feature(bp->f, "形副名詞") && 
	!check_feature(bp->f, "強時間") && 
	!check_feature(bp->f, "指示詞") && 
	!check_feature(bp->f, "ID:（〜を）〜に") && 
	!check_feature(bp->f, "外の関係") && 
	!check_feature(bp->f, "外の関係可能性")) {
	return TRUE;
    }
    return FALSE;
}

/*==================================================================*/
		   int CheckPureNoun(BNST_DATA *bp)
/*==================================================================*/
{
    /* !check_feature(bp->f, "数量") &&  */
    if ((check_feature(bp->f, "体言") || 
	 check_feature(bp->f, "サ変")) && 
	CheckTargetNoun(bp)) {
	return TRUE;
    }
    return FALSE;
}

/*==================================================================*/
	 int CheckCaseComponent(CF_PRED_MGR *cpm_ptr, int n)
/*==================================================================*/
{
    int i;

    for (i = 0; i < cpm_ptr->cf.element_num; i++) {
	if (cpm_ptr->elem_b_num[i] > -2 && 
	    cpm_ptr->elem_b_ptr[i]->num == n) {
	    return TRUE;
	    /* 格要素になっているかどうかに
	       対応付け状態は関係ない
	    if (cmm_ptr->result_lists_d[0].flag[i] >= 0) {
		return TRUE;
	    }
	    else {
		return FALSE;
	    } */
	}
    }
    return FALSE;
}

/*==================================================================*/
int CheckEllipsisComponent(CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, BNST_DATA *bp)
/*==================================================================*/
{
    int i, num;

    /* 用言が候補と同じ表記を
       他の格の省略の指示対象としてもっているかどうか
       cpm_ptr->elem_b_num[num] <= -2 
       => 通常の格要素もチェック */

    for (i = 0; i < cmm_ptr->cf_ptr->element_num; i++) {
	num = cmm_ptr->result_lists_p[0].flag[i];
	/* 省略の指示対象 */
	if (num >= 0 && 
	    str_eq(cpm_ptr->elem_b_ptr[num]->Jiritu_Go, bp->Jiritu_Go)) {
	    return 1;
	}
    }
    return 0;
}

/*==================================================================*
   int CheckEllipsisComponent(ELLIPSIS_MGR *em_ptr, BNST_DATA *bp)
 *==================================================================*
{
    int i;

    * 用言が候補と同じ表記を
       他の格の省略の指示対象としてもっているかどうか *

    for (i = 0; i < CASE_MAX_NUM; i++) {
	if (em_ptr->cc[i].s && 
	    str_eq((em_ptr->cc[i].s->bnst_data+em_ptr->cc[i].bnst)->Jiritu_Go, bp->Jiritu_Go)) {
	    return 1;
	}
    }
    return 0;
}
*/

/*==================================================================*/
int CheckObligatoryCase(CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, BNST_DATA *bp)
/*==================================================================*/
{
    /* 
       bp: 対象文節
       cpm_ptr: 対象文節の係る用言 (bp->parent->cpm_ptr)
    */

    int i, num;

    if (cpm_ptr == NULL) {
	return 0;
    }

    if (cmm_ptr->score != -2) {
	for (i = 0; i < cmm_ptr->cf_ptr->element_num; i++) {
	    num = cmm_ptr->result_lists_p[0].flag[i];
	    /* これが調べる格要素 */
	    if (num != UNASSIGNED && 
		cpm_ptr->elem_b_num[num] > -2 && /* 省略の格要素じゃない */
		cpm_ptr->elem_b_ptr[num]->num == bp->num) {
		if (MatchPP(cmm_ptr->cf_ptr->pp[i][0], "ガ") || 
		    MatchPP(cmm_ptr->cf_ptr->pp[i][0], "ヲ") || 
		    MatchPP(cmm_ptr->cf_ptr->pp[i][0], "ニ") || 
		    MatchPP(cmm_ptr->cf_ptr->pp[i][0], "ガ２")) {
		    return 1;
		}
		return 0;
	    }
	}
    }
    return 0;
}

/*==================================================================*/
int GetCandCase(CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, BNST_DATA *bp)
/*==================================================================*/
{
    /* 候補の格を得る
       bp: 対象文節
       cpm_ptr: 対象文節の係る用言 (bp->parent->cpm_ptr)
    */

    int i, num;

    if (cpm_ptr && cpm_ptr->result_num > 0 && cmm_ptr->score != -2) {
	for (i = 0; i < cmm_ptr->cf_ptr->element_num; i++) {
	    num = cmm_ptr->result_lists_p[0].flag[i];
	    /* これが調べる格要素 */
	    if (num != UNASSIGNED && 
		cpm_ptr->elem_b_num[num] > -2 && /* 省略の格要素じゃない */
		cpm_ptr->elem_b_ptr[num]->num == bp->num) {
		return cmm_ptr->cf_ptr->pp[i][0];
	    }
	}
    }
    return -1;
}

/*==================================================================*/
int CheckCaseCorrespond(CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, 
			BNST_DATA *bp, CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{
    /* 格の一致を調べる
       bp: 対象文節
       cpm_ptr: 対象文節の係る用言 (bp->parent->cpm_ptr)
    */

    int i, num;

    if (cpm_ptr->result_num > 0 && cmm_ptr->score != -2) {
	for (i = 0; i < cmm_ptr->cf_ptr->element_num; i++) {
	    num = cmm_ptr->result_lists_p[0].flag[i];
	    /* これが調べる格要素 */
	    if (num != UNASSIGNED && 
		cpm_ptr->elem_b_num[num] > -2 && /* 省略の格要素じゃない */
		cpm_ptr->elem_b_ptr[num]->num == bp->num) {
		if (cf_ptr->pp[n][0] == cmm_ptr->cf_ptr->pp[i][0] || 
		    (MatchPP(cf_ptr->pp[n][0], "ガ") && MatchPP(cmm_ptr->cf_ptr->pp[i][0], "ガ２"))) {
		    return 1;
		}
		return 0;
	    }
	}
    }
    return 0;
}

/*==================================================================*/
      BNST_DATA *GetRealParent(SENTENCE_DATA *sp, BNST_DATA *bp)
/*==================================================================*/
{
    if (bp->dpnd_head != -1) {
	return sp->bnst_data+bp->dpnd_head;
    }
    return NULL;
}

/*==================================================================*/
int CountBnstDistance(SENTENCE_DATA *cs, int candn, SENTENCE_DATA *ps, int pn)
/*==================================================================*/
{
    int sdiff, i, diff = 0;

    sdiff = ps-cs;

    if (sdiff > 0) {
	for (i = 1; i < sdiff; i++) {
	    diff += (ps-i)->Bnst_num;
	}
	diff += pn+cs->Bnst_num-candn;
    }
    else {
	diff = pn-candn;
    }

    return diff;
}

/*==================================================================*/
	char *EllipsisSvmFeatures2String(E_SVM_FEATURES *esf)
/*==================================================================*/
{
    int max, i;
    char *buffer, *sbuf;

    /* max = (sizeof(E_SVM_FEATURES)-sizeof(float))/sizeof(int)+1; */
    max = (sizeof(E_SVM_FEATURES)-sizeof(float))/sizeof(int); /* ac を除くテスト */
    sbuf = (char *)malloc_data(sizeof(char)*(10+log(max)), 
			       "EllipsisSvmFeatures2String");
    buffer = (char *)malloc_data((sizeof(char)*(10+log(max)))*max+10, 
				 "EllipsisSvmFeatures2String");
    sprintf(buffer, "1:%.5f", esf->similarity);
    for (i = 2; i <= max; i++) {
	sprintf(sbuf, " %d:%d", i, *(esf->c_pp+i-2));
	strcat(buffer, sbuf);
    }
    free(sbuf);

    return buffer;
}

/*==================================================================*/
void EllipsisSvmFeaturesString2Feature(ELLIPSIS_MGR *em_ptr, char *ecp, 
				       char *word, int pp, char *sid, int num)
/*==================================================================*/
{
    char *buffer;

    /* -learn 時のみ学習用featureを表示する */
    if (OptLearn == FALSE) {
	return;
    }

    buffer = (char *)malloc_data(strlen(ecp)+46+strlen(word), 
				 "EllipsisSvmFeaturesString2FeatureString");
    sprintf(buffer, "SVM学習FEATURE;%s;%s;%s;%d:%s", 
	    word, pp_code_to_kstr(pp), sid, num, ecp);
    assign_cfeature(&(em_ptr->f), buffer);
    free(buffer);
}

/*==================================================================*/
 E_SVM_FEATURES *EllipsisFeatures2EllipsisSvmFeatures(E_FEATURES *ef)
/*==================================================================*/
{
    E_SVM_FEATURES *f;
    int i;

    f = (E_SVM_FEATURES *)malloc_data(sizeof(E_SVM_FEATURES), "SetEllipsisFeatures");

    f->similarity = ef->similarity;
    for (i = 0; i < PP_NUMBER; i++) {
	f->c_pp[i] = ef->c_pp == i ? 1 : 0;
    }
    f->c_distance = ef->c_distance;
    f->c_dist_bnst = ef->c_dist_bnst;
    f->c_fs_flag = ef->c_fs_flag;
    f->c_location[0] = ef->c_location == PLOC_PV ? 1 : 0;
    f->c_location[1] = ef->c_location == PLOC_PARENTV ? 1 : 0;
    f->c_location[2] = ef->c_location == PLOC_CHILDV ? 1 : 0;
    f->c_location[3] = ef->c_location == PLOC_CHILDPV ? 1 : 0;
    f->c_location[4] = ef->c_location == PLOC_NPARENTV ? 1 : 0;
    f->c_location[5] = ef->c_location == PLOC_NPARENTCV ? 1 : 0;
    f->c_location[6] = ef->c_location == PLOC_NOTHERS ? 1 : 0;
    f->c_topic_flag = ef->c_topic_flag;
    f->c_no_topic_flag = ef->c_no_topic_flag;
    f->c_subject_flag = ef->c_subject_flag;
    f->c_dep_mc_flag = ef->c_dep_mc_flag;
    f->c_dep_p_level[0] = str_eq(ef->c_dep_p_level, "A-") ? 1 : 0;
    f->c_dep_p_level[1] = str_eq(ef->c_dep_p_level, "A") ? 1 : 0;
    f->c_dep_p_level[2] = str_eq(ef->c_dep_p_level, "B-") ? 1 : 0;
    f->c_dep_p_level[3] = str_eq(ef->c_dep_p_level, "B") ? 1 : 0;
    f->c_dep_p_level[4] = str_eq(ef->c_dep_p_level, "B+") ? 1 : 0;
    f->c_dep_p_level[5] = str_eq(ef->c_dep_p_level, "C") ? 1 : 0;
    f->c_prev_p_flag = ef->c_prev_p_flag;
    f->c_get_over_p_flag = ef->c_get_over_p_flag;
    f->c_sm_none_flag = ef->c_sm_none_flag;
    f->c_extra_tag[0] = ef->c_extra_tag == 0 ? 1 : 0;
    f->c_extra_tag[1] = ef->c_extra_tag == 1 ? 1 : 0;
    f->c_extra_tag[2] = ef->c_extra_tag == 2 ? 1 : 0;
    f->c_extra_tag[3] = ef->c_extra_tag == 3 ? 1 : 0;

    /* ガ,ヲ,ニ */
    for (i = 0; i < 3; i++) {
	f->p_pp[i] = ef->p_pp == i+1 ? 1 : 0;
    }
    f->p_voice[0] = ef->p_voice == VOICE_SHIEKI ? 1 : 0;
    f->p_voice[1] = ef->p_voice == VOICE_UKEMI ? 1 : 0;
    f->p_voice[2] = ef->p_voice == VOICE_MORAU ? 1 : 0;
    f->p_type[0] = ef->p_type == 1 ? 1 : 0;
    f->p_type[1] = ef->p_type == 2 ? 1 : 0;
    f->p_type[2] = ef->p_type == 3 ? 1 : 0;
    f->p_sahen_flag = ef->p_sahen_flag;
    f->p_cf_subject_flag = ef->p_cf_subject_flag;
    f->p_cf_sentence_flag = ef->p_cf_sentence_flag;
    f->p_n_modify_flag = ef->p_n_modify_flag;

    f->c_ac = ef->c_ac;

    return f;
}

/*==================================================================*/
void SetEllipsisFeaturesForPred(E_FEATURES *f, CF_PRED_MGR *cpm_ptr, 
				CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{
    f->p_pp = cf_ptr->pp[n][0];

    if (check_feature(cpm_ptr->pred_b_ptr->f, "追加受身")) {
	f->p_voice = VOICE_UKEMI;
    }
    else {
	/* 能動(0), VOICE_SHIEKI(1), VOICE_UKEMI(2), VOICE_MORAU(3) */
	f->p_voice = cpm_ptr->pred_b_ptr->voice;
    }

    if (check_feature(cpm_ptr->pred_b_ptr->f, "用言:動")) {
	f->p_type = 1;
    }
    else if (check_feature(cpm_ptr->pred_b_ptr->f, "用言:形")) {
	f->p_type = 2;
    }
    else if (check_feature(cpm_ptr->pred_b_ptr->f, "用言:判")) {
	f->p_type = 3;
    }
    else {
	f->p_type = 0;
    }

    if (check_feature(cpm_ptr->pred_b_ptr->f, "サ変")) {
	f->p_sahen_flag = 1;
    }
    else {
	f->p_sahen_flag = 0;
    }

    f->p_cf_subject_flag = cf_match_element(cf_ptr->sm[n], "主体", FALSE) ? 1 : 0;
    f->p_cf_sentence_flag = cf_match_element(cf_ptr->sm[n], "補文", TRUE) ? 1 : 0;
    f->p_n_modify_flag = check_feature(cpm_ptr->pred_b_ptr->f, "係:連格") ? 1 : 0;
}

/*==================================================================*/
E_FEATURES *SetEllipsisFeatures(SENTENCE_DATA *s, SENTENCE_DATA *cs, 
				CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, 
				BNST_DATA *bp, CASE_FRAME *cf_ptr, int n, int loc)
/*==================================================================*/
{
    E_FEATURES *f;
    char *level;

    f = (E_FEATURES *)malloc_data(sizeof(E_FEATURES), "SetEllipsisFeatures");

    f->pos = MATCH_NONE;
    f->similarity = CalcSimilarityForVerb(bp, cf_ptr, n, &f->pos);

    if (bp->pred_b_ptr) {
	f->c_pp = GetCandCase(bp->pred_b_ptr->cpm_ptr, &(bp->pred_b_ptr->cpm_ptr->cmm[0]), bp);

	if ((level = check_feature(bp->pred_b_ptr->f, "レベル"))) {
	    strcpy(f->c_dep_p_level, level+7);
	}
	else {
	    f->c_dep_p_level[0] = '\0';
	}
    }
    else {
	f->c_pp = -1;
	f->c_dep_p_level[0] = '\0';
    }

    f->c_distance = cs-s;
    f->c_dist_bnst = CountBnstDistance(s, bp->num, cs, cpm_ptr->pred_b_ptr->num);
    f->c_fs_flag = s->Sen_num == 1 ? 1 : 0;
    f->c_location = loc;
    f->c_topic_flag = check_feature(bp->f, "主題表現") ? 1 : 0;
    f->c_no_topic_flag = check_feature(bp->f, "準主題表現") ? 1 : 0;
    f->c_subject_flag = sm_match_check(sm2code("主体"), bp->SM_code) ? 1 : 0;
    f->c_dep_mc_flag = CheckLastClause(cs->Sen_num-f->c_distance, f->c_pp, bp->Jiritu_Go) ? 1 : 0;

    if (f->c_distance > 0 || 
	(f->c_distance == 0 && bp->num < cpm_ptr->pred_b_ptr->num)) {
	f->c_prev_p_flag = 1;
    }
    else {
	f->c_prev_p_flag = 0;
    }

    if (f->c_distance == 0 && bp->dpnd_head > cpm_ptr->pred_b_ptr->num) {
	f->c_get_over_p_flag = 1;
    }
    else {
	f->c_get_over_p_flag = 0;
    }

    f->c_sm_none_flag = f->similarity < 0 ? 1 : 0;
    f->c_extra_tag = -1;

    /* 用言に関するfeatureを設定 */
    SetEllipsisFeaturesForPred(f, cpm_ptr, cf_ptr, n);

    /* 参照回数 */
    f->c_ac = CheckAnaphor(alist, bp->Jiritu_Go);

    return f;
}

/*==================================================================*/
E_FEATURES *SetEllipsisFeaturesExtraTags(int tag, CF_PRED_MGR *cpm_ptr, 
					 CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{
    E_FEATURES *f;

    f = (E_FEATURES *)malloc_data(sizeof(E_FEATURES), "SetEllipsisFeaturesExtraTags");
    memset(f, 0, sizeof(E_FEATURES));
    f->similarity = -1;
    f->c_pp = -1;
    f->c_distance = -1;
    f->c_dist_bnst = -1;
    f->c_extra_tag = tag;

    /* 用言に関するfeatureを設定 */
    SetEllipsisFeaturesForPred(f, cpm_ptr, cf_ptr, n);

    return f;
}

/*==================================================================*/
void EllipsisDetectForVerbSubcontractExtraTagsWithLearning(SENTENCE_DATA *cs, ELLIPSIS_MGR *em_ptr, 
							   CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, 
							   int tag, CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{
    E_FEATURES *ef;
    E_SVM_FEATURES *esf;
    float score;
    char *ecp, feature_buffer[DATA_LEN];

    ef = SetEllipsisFeaturesExtraTags(tag, cpm_ptr, cf_ptr, n);
    esf = EllipsisFeatures2EllipsisSvmFeatures(ef);
    ecp = EllipsisSvmFeatures2String(esf);

    if (OptDiscMethod == OPT_SVM) {
#ifdef USE_SVM
	score = svm_classify(ecp);
#endif
    }
    else if (OptDiscMethod == OPT_DT) {
	score = dt_classify(ecp);
    }

    if (score > maxscore) {
	char feature_buffer2[50000];
	maxscore = score;
	maxrawscore = 1.0;
	maxtag = ExtraTags[tag];
	sprintf(feature_buffer2, "SVM-%s:%s", pp_code_to_kstr(cf_ptr->pp[n][0]), ecp);
	assign_cfeature(&(em_ptr->f), feature_buffer2);
    }

    /* 省略候補 */
    sprintf(feature_buffer, "C用;%s;%s;-1;-1;%.3f|-1", ExtraTags[tag], 
	    pp_code_to_kstr(cf_ptr->pp[n][0]), 
	    score);
    assign_cfeature(&(em_ptr->f), feature_buffer);

    /* 学習FEATURE */
    EllipsisSvmFeaturesString2Feature(em_ptr, ecp, ExtraTags[tag], cf_ptr->pp[n][0], 
				      "?", -1);
    free(ef);
    free(esf);
    free(ecp);
}

/*==================================================================*/
void _EllipsisDetectForVerbSubcontractWithLearning(SENTENCE_DATA *s, SENTENCE_DATA *cs, ELLIPSIS_MGR *em_ptr, 
						   CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, 
						   BNST_DATA *bp, CASE_FRAME *cf_ptr, int n, int type, int loc)
/*==================================================================*/
{
    E_FEATURES *ef;
    E_SVM_FEATURES *esf;
    float score;
    char *ecp, feature_buffer[DATA_LEN];

    ef = SetEllipsisFeatures(s, cs, cpm_ptr, cmm_ptr, bp, cf_ptr, n, loc);
    esf = EllipsisFeatures2EllipsisSvmFeatures(ef);
    ecp = EllipsisSvmFeatures2String(esf);

    /* 学習FEATURE */
    EllipsisSvmFeaturesString2Feature(em_ptr, ecp, bp->Jiritu_Go, cf_ptr->pp[n][0], 
				      s->KNPSID ? s->KNPSID+5 : "?", bp->num);

    /* すでに他の格の指示対象になっているときはだめ */
    if (CheckEllipsisComponent(cpm_ptr, cmm_ptr, bp)) {
	free(ef);
	free(esf);
	free(ecp);
	return;
    }

    if (OptDiscMethod == OPT_SVM) {
#ifdef USE_SVM
	score = svm_classify(ecp);
#endif
    }
    else if (OptDiscMethod == OPT_DT) {
	score = dt_classify(ecp);
    }

    /* if (rawscore > 0 || smnone == 1) { */
    if (score > maxscore) {
	char feature_buffer2[50000];
	maxscore = score;
	maxrawscore = ef->similarity;
	maxs = s;
	maxpos = ef->pos;
	if (bp->num < 0) {
	    maxi = bp->parent->num;
	}
	else {
	    maxi = bp->num;
	}
	sprintf(feature_buffer2, "SVM-%s:%s", pp_code_to_kstr(cf_ptr->pp[n][0]), ecp);
	assign_cfeature(&(em_ptr->f), feature_buffer2);
	maxtag = NULL;
    }

    /* 省略候補 */
    sprintf(feature_buffer, "C用;%s;%s;%d;%d;%.3f|%.3f", bp->Jiritu_Go, 
	    pp_code_to_kstr(cf_ptr->pp[n][0]), 
	    ef->c_distance, maxi, 
	    score, ef->similarity);
    assign_cfeature(&(em_ptr->f), feature_buffer);

    free(ef);
    free(esf);
    free(ecp);
}

/*==================================================================*/
int EllipsisDetectForVerbSubcontractExtraTags(SENTENCE_DATA *cs, ELLIPSIS_MGR *em_ptr, 
					      CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, 
					      int tag, CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{

    if (OptDiscMethod == OPT_SVM || OptDiscMethod == OPT_DT) {
	EllipsisDetectForVerbSubcontractExtraTagsWithLearning(cs, em_ptr, cpm_ptr, cmm_ptr, 
							      tag, cf_ptr, n);
	if (maxscore > AntecedentDecideThreshold) {
	    return 1;
	}
    }
    else {
	E_FEATURES *ef;
	E_SVM_FEATURES *esf;
	char *ecp;

	ef = SetEllipsisFeaturesExtraTags(tag, cpm_ptr, cf_ptr, n);
	esf = EllipsisFeatures2EllipsisSvmFeatures(ef);
	ecp = EllipsisSvmFeatures2String(esf);
	EllipsisSvmFeaturesString2Feature(em_ptr, ecp, ExtraTags[tag], cf_ptr->pp[n][0], 
					  "?", -1);
	free(ef);
	free(esf);
	free(ecp);
    }
    return 0;
}

/*==================================================================*/
void _EllipsisDetectForVerbSubcontract(SENTENCE_DATA *s, SENTENCE_DATA *cs, ELLIPSIS_MGR *em_ptr, 
				       CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, 
				       BNST_DATA *bp, CASE_FRAME *cf_ptr, int n, int type, int loc)
/*==================================================================*/
{
    float score, weight, pascore, pcscore, mcscore, rawscore, topicscore, distscore;
    float addscore = 0;
    char feature_buffer[DATA_LEN], *ecp;
    int ac, pac, pcc, pcc2, mcc, topicflag, distance, agentflag, firstsc, subtopicflag, sameflag;
    int exception = 0, pos = MATCH_NONE, casematch, candagent, hobunflag, predabstract, smnone;
    int important = 0;
    E_FEATURES *ef;
    E_SVM_FEATURES *esf;

    /* 学習FEATURE */
    ef = SetEllipsisFeatures(s, cs, cpm_ptr, cmm_ptr, bp, cf_ptr, n, loc);
    esf = EllipsisFeatures2EllipsisSvmFeatures(ef);
    ecp = EllipsisSvmFeatures2String(esf);
    EllipsisSvmFeaturesString2Feature(em_ptr, ecp, bp->Jiritu_Go, cf_ptr->pp[n][0], 
				      s->KNPSID ? s->KNPSID+5 : "?", bp->num);
    free(ef);
    free(esf);
    free(ecp);

    /* cs のときだけ意味がある */
    Bcheck[bp->num] = 1;

    /* 対象用言と候補が同じ自立語のとき
       判定詞の場合だけ許す */
    if (str_eq(cpm_ptr->pred_b_ptr->Jiritu_Go, bp->Jiritu_Go)) {
	if (!(check_feature(cpm_ptr->pred_b_ptr->f, "用言:判") && 
	      MatchPP(cf_ptr->pp[n][0], "ガ"))) {
	    return;
	}
	/* 判定詞でガ格のときだけ */
	sameflag = 1;
    }
    else {
	sameflag = 0;
    }

    /* すでに他の格の指示対象になっているときはだめ */
    if (CheckEllipsisComponent(cpm_ptr, cmm_ptr, bp)) {
	return;
    }

    /* 現在の文から対象となっている格要素の文までの距離 */
    distance = cs-s;

    /* 同一文ではない同格の前側は候補にしない */
    if (bp->dpnd_type == 'A' && distance != 0) {
	return;
    }

    if (distance > 8) {
	distscore = 0.2;
    }
    else {
	distscore = 1-(float)distance*0.1;
    }

    /* 格の一致をチェック 
     bp->parent && bp->parent->cpm_ptr &&  */
    if (distance == 0 && 
	bp->pred_b_ptr && bp->pred_b_ptr->cpm_ptr && 
	bp->num < cpm_ptr->pred_b_ptr->num && 
	check_feature(bp->pred_b_ptr->f, "用言")) {
	if (CheckCaseCorrespond(bp->pred_b_ptr->cpm_ptr, &(bp->pred_b_ptr->cpm_ptr->cmm[0]), 
				bp, cf_ptr, n) == 0) {
	    /* 格不一致 */
	    weight = 0.9;
	    casematch = -1;
	}
	else {
	    /* 格一致 */
	    weight = 1;
	    casematch = 1;
	}
    }
    else {
	weight = 1;
	casematch = 0;
    }

    /* リンクする場所によるスコア
    if (type == RANK2) {
	* weight = (float)EllipsisSubordinateClauseScore/10; *
	weight = 0.95;
    }
    else {
	weight = 1.0;
    } */

    /* リンクされた回数によるスコア */
    ac = CheckAnaphor(alist, bp->Jiritu_Go);

    /* すでに出現した用言とその格要素のセット */
    pac = CheckPredicate(L_Jiritu_M(cpm_ptr->pred_b_ptr)->Goi, cpm_ptr->pred_b_ptr->voice, 
			 cf_ptr->cf_address, 
			 cf_ptr->pp[n][0], bp->Jiritu_Go);
    pascore = 1.0+0.5*pac; /* 0.2 */

    /* 前文の主節のスコア */
    pcc = CheckLastClause(cs->Sen_num-1, cf_ptr->pp[n][0], bp->Jiritu_Go);

    /* 2文前の主節のスコア */
    pcc2 = CheckLastClause(cs->Sen_num-2, cf_ptr->pp[n][0], bp->Jiritu_Go);

    /* 重要ではない格要素の場合 */
    if (check_feature(bp->f, "非主題")) {
	pcscore = 1.0+0.2*pcc; /* 0.1 */
    }
    else {
	pcscore = 1.0+0.5*pcc; /* 0.1 */
    }

    /* 現在の文の主節のスコア */
    mcc = CheckLastClause(cs->Sen_num, cf_ptr->pp[n][0], bp->Jiritu_Go);

    /* 重要ではない格要素の場合 */
    if (check_feature(bp->f, "非主題")) {
	mcscore = 1.0+0.2*mcc; /* 0.2 */
    }
    else {
	mcscore = 1.0+0.5*mcc; /* 0.2 */
    }

    /* 提題のスコア */
    if (check_feature(bp->f, "主題表現")) {
	topicflag = 1;
	if (distance == 0) {
	    topicscore = 1.5; /* 1.2 */
	}
	else {
	    topicscore = 1.5; /* 1.2 */
	}
    }
    else {
	topicflag = 0;
	topicscore = 1.0;
    }

    if (check_feature(bp->f, "準主題表現")) {
	if (distance == 0 && 
	    (check_feature(bp->f, "人名") || 
	     check_feature(bp->f, "地名") || 
	     check_feature(bp->f, "組織名"))) { 
	    topicflag = 1;
	    subtopicflag = 0;
	}
	else {
	    subtopicflag = 1;
	}
    }
    else {
	subtopicflag = 0;
    }

    /* 補正 */
    if (topicscore > 1.0 && 
	(pcscore > 1.0 || mcscore > 1.0)) {
	pcscore = 1.0;
	mcscore = 1.0;
    }

    /* 格フレームが <主体> をもつかどうか */
    if (cf_match_element(cf_ptr->sm[n], "主体", FALSE)) {
	/* sm_match_check(sm2code("主体"), cf_ptr->sm[n]) */
	agentflag = 1;
    }
    else {
	agentflag = 0;
    }

    /* 格フレームが <補文> をもつかどうか */
    if (cf_match_element(cf_ptr->sm[n], "補文", TRUE)) {
	hobunflag = 1;
    }
    else {
	hobunflag = 0;
    }

    if (sm_match_check(sm2code("主体"), bp->SM_code)) {
	candagent = 1;
    }
    else {
	candagent = 0;
    }

    if (sm_match_check(sm2code("抽象物"), cpm_ptr->pred_b_ptr->SM_code)) {
	predabstract = 1;
    }
    else {
	predabstract = 0;
    }

    /* N は 〜 N だ。 */
    if (sameflag) {
	rawscore = 1;
    }
    /* V したのは N だ。: ガ格, ヲ格 
       やめた: check_feature(bp->f, "用言:判") */
    else if (cpm_ptr->pred_b_ptr->dpnd_head == bp->num && /* ちゃんと指定してみる */
	     check_feature(cpm_ptr->pred_b_ptr->f, "ID:〜の〜") && 
	     (check_feature(cpm_ptr->pred_b_ptr->f, "係:未格") || 
	      check_feature(cpm_ptr->pred_b_ptr->f, "係:ガ格"))) {
	rawscore = CalcSimilarityForVerb(bp, cf_ptr, n, &pos);
	/* ★ rawscore == 0 で bp 側に意味素がない場合を考える */
	exception = 1;
    }
    else {
	rawscore = CalcSimilarityForVerb(bp, cf_ptr, n, &pos);
    }

    /* 意味素なし */
    if (rawscore < 0) {
	smnone = 1;
    }
    else {
	smnone = 0;
    }

    /* 先頭文の主節をチェック */
    firstsc = CheckLastClause(1, cf_ptr->pp[n][0], bp->Jiritu_Go);

    /* 救うもの */
    if (((s->Sen_num == 1 && firstsc == 0) || 
	 (distance == 1 && pcc == 0) || 
	 (distance == 0 && mcc == 0)
	) && 
	((bp->num == s->Bnst_num-1 && check_feature(bp->f, "用言:判")) || /* 文末の判定詞 */
	 (bp->parent && bp->parent->parent && check_feature(bp->parent->parent->f, "主節") && subordinate_level_check("B", bp->parent) && 
	  CheckObligatoryCase(bp->parent->cpm_ptr, &(bp->parent->cpm_ptr->cmm[0]), bp)) || /* 強い従属節 */
	 (check_feature(bp->f, "係:ノ格") && check_feature((s->bnst_data+bp->dpnd_head)->f, "主節")))) { /* 主節に係るノ格(名詞句) */
	if (s->Sen_num == 1) {
	    firstsc = 1;
	}
	else if (distance == 0) {
	    mcc = 1;
	}
	else {
	    pcc = 1;
	}
    }

    if (casematch != -1 && (loc == PLOC_PV || loc == PLOC_PARENTV)) {
	addscore = 0.10;
    }

    /* 先頭文で用言の直前になく、デモ, デハではないデ格は対象にしない 
       他の任意格もそうかもしれない
    if (firstsc && s->Sen_num == 1 && 
	check_feature(bp->f, "係:デ格") && 
	bp->dpnd_head != bp->num+1 && 
	!check_feature(bp->f, "デハ") && 
	!check_feature(bp->f, "デモ")) {
	firstsc = 0;
    } */

    if (distance == 0 || /* 対象文 */
	(distance == 1 && (topicflag || subtopicflag || pcc)) || /* 前文 */
	/* (distance == 2 && pcc2) || * 2文前 */
	(s->Sen_num == 1 && (topicflag || subtopicflag || firstsc)) || /* 先頭文 */
	(pcc || mcc || firstsc)) { /* それ以外で主節の省略の指示対象 */

	/* 対象文には、厳しい制約が必要 
	   主節の格要素の場合でも、対象用言より前に出現している必要がある */
	if (distance == 0 && !exception && type < RANK4 && !((topicflag || mcc || type >= RANK2) && bp->num < cpm_ptr->pred_b_ptr->num)) {
	    if (check_feature(bp->f, "抽象") && !check_feature(bp->f, "人名") && !check_feature(bp->f, "組織名")) {
		weight *= NON_IMPORTANT_WEIGHT;
		score = weight*pascore*rawscore;
	    }
	    else {
		weight *= 1; /* NON_IMPORTANT_WEIGHT; */
		score = weight*pascore*rawscore;
	    }
	}
	else {
	    if (subtopicflag == 1) {
		weight *= NON_IMPORTANT_WEIGHT;
		score = weight*pascore*rawscore;
	    }
	    /* 判定詞の場合で RANK3 のもので類似度がない場合は救う
	       「結婚も 〜 上回り、〜 十九万五千組。」 
	       条件: rawscore == 0 はずした */
	    else if (casematch != -1 && type >= RANK3 && check_feature(cpm_ptr->pred_b_ptr->f, "用言:判")) {
		score = weight*pascore*1.0;
	    }
	    else {
		score = weight*pascore*rawscore;
	    }
	}
	important = 1;
    }
    /* 特別に N-V のセットのときは許す */
    else if (pascore > 1 && rawscore > 0.8) {
	score = weight*pascore*rawscore;
    }
    else {
	/* 非重要要素 */
	score = -1;
	/* return; */
    }

    /* 距離を加味
    score *= distscore; */

    /* ボーナス */
    score += addscore;

    if (score > maxscore) {
	BNST_DATA *tp = bp;
	maxscore = score;
	maxs = s;
	maxpos = pos;

	/* 文節内の場合は親文節を探しにいく */
	while (tp->num < 0) {
	    tp = tp->parent;
	}
	maxi = tp->num;
    }

    /* 省略候補 (rawscore == 0 の場合も候補として出力) */
    sprintf(feature_buffer, "C用;%s;%s;%d;%d;%.3f|%.3f", bp->Jiritu_Go, 
	    pp_code_to_kstr(cf_ptr->pp[n][0]), 
	    distance, maxi, 
	    score, rawscore);
    assign_cfeature(&(em_ptr->f), feature_buffer);
}

/*==================================================================*/
int EllipsisDetectForVerbSubcontract(SENTENCE_DATA *s, SENTENCE_DATA *cs, ELLIPSIS_MGR *em_ptr, 
				     CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, 
				     BNST_DATA *bp, CASE_FRAME *cf_ptr, int n, int type, int loc)
/*==================================================================*/
{
    if (OptDiscMethod == OPT_SVM || OptDiscMethod == OPT_DT) {
	_EllipsisDetectForVerbSubcontractWithLearning(s, cs, em_ptr, 
						      cpm_ptr, cmm_ptr, 
						      bp, cf_ptr, n, type, loc);
	if (maxscore > AntecedentDecideThreshold) {
	    return 1;
	}
    }
    else {
	_EllipsisDetectForVerbSubcontract(s, cs, em_ptr, 
					  cpm_ptr, cmm_ptr, 
					  bp, cf_ptr, n, type, loc);
    }
    return 0;
}

/*==================================================================*/
int SearchCaseComponent(SENTENCE_DATA *s, ELLIPSIS_MGR *em_ptr, 
			CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, 
			BNST_DATA *bp, CASE_FRAME *cf_ptr, int n, int rank, int loc)
/*==================================================================*/
{
    /* cpm_ptr: 省略格要素をもつ用言
       bp:      格要素の探索対象となっている用言文節
    */

    int i, num;

    /* 他の用言 (親用言など) の格要素をチェック 
       ★親の格フレームはすでに決まっているが、
       それ以外の場合は?★ */
    if (bp->cpm_ptr && bp->cpm_ptr->cmm[0].score != -2) {
	for (i = 0; i < bp->cpm_ptr->cmm[0].cf_ptr->element_num; i++) {
	    num = bp->cpm_ptr->cmm[0].result_lists_p[0].flag[i];
	    if (num != UNASSIGNED && 
		bp->cpm_ptr->elem_b_num[num] > -2 && /* 格要素が省略を補ったものであるときはだめ */
		bp->cpm_ptr->elem_b_ptr[num]->num != cpm_ptr->pred_b_ptr->num && /* 格要素が元用言のときはだめ (bp->cpm_ptr->elem_b_ptr[num] は並列のとき <PARA> となり、pointer はマッチしない) */
		CheckTargetNoun(bp->cpm_ptr->elem_b_ptr[num])) {
		/* 格要素の格の一致 (格によりけり) */
		if (!check_feature(bp->cpm_ptr->elem_b_ptr[num]->f, "係:ノ格") && /* ノ格の格解析は信用しない */
		    !check_feature(bp->cpm_ptr->elem_b_ptr[num]->f, "デハ") && /* 「では」の格解析は信用しない */
		    !check_feature(bp->cpm_ptr->elem_b_ptr[num]->f, "デモ") && /* 「でも」の格解析は信用しない */
		    (cf_ptr->pp[n][0] == bp->cpm_ptr->cmm[0].cf_ptr->pp[i][0] || 
		    (MatchPP(cf_ptr->pp[n][0], "ガ") && MatchPP(bp->cpm_ptr->cmm[0].cf_ptr->pp[i][0], "ガ２")))) {
		    /* 〜して、〜した N 〜 (格が一致している連体修飾) */
		    if (bp->cpm_ptr->elem_b_ptr[num]->num > bp->cpm_ptr->pred_b_ptr->num) {
			/* rank = RANK4; */
			rank++;
		    }
		    else {
			;
			/* rank = RANK3; */
		    }
		}
		else {
		    /* 格は不一致 */
		    rank = RANK2;
		    /* rank--; */
		}
		if (EllipsisDetectForVerbSubcontract(s, s, em_ptr, cpm_ptr, cmm_ptr, 
						     bp->cpm_ptr->elem_b_ptr[num], 
						     cf_ptr, n, rank, loc)) {
		    return 1;
		}
	    }
	}
    }
    return 0;
}

/*==================================================================*/
	int AppendToCF(CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, 
		       BNST_DATA *b_ptr,
		       CASE_FRAME *cf_ptr, int n, float maxscore, int maxpos)
/*==================================================================*/
{
    /* 省略の指示対象を入力側の格フレームに入れる */

    CASE_FRAME *c_ptr = &(cpm_ptr->cf);
    int d, demonstrative;

    if (c_ptr->element_num >= CF_ELEMENT_MAX) {
	return 0;
    }

    /* 指示詞の場合 */
    if (cmm_ptr->result_lists_p[0].flag[n] != UNASSIGNED) {
	d = cmm_ptr->result_lists_p[0].flag[n];
	demonstrative = 1;
    }
    else {
	d = c_ptr->element_num;
	demonstrative = 0;
    }

    /* 対応情報を追加 */
    cmm_ptr->result_lists_p[0].flag[n] = d;
    cmm_ptr->result_lists_d[0].flag[d] = n;
    cmm_ptr->result_lists_p[0].pos[n] = maxpos;
    /* cpm_ptr->cmm[0].result_lists_p[0].score[n] = -1; */

    if (OptDiscMethod == OPT_SVM || OptDiscMethod == OPT_DT) {
#ifdef USE_RAWSCORE
	if (maxrawscore < 0) {
	    cmm_ptr->result_lists_p[0].score[n] = 0;
	}
	else if (maxrawscore > 1.0) {
	    cmm_ptr->result_lists_p[0].score[n] = EX_match_exact;
	}
	else {
	    cmm_ptr->result_lists_p[0].score[n] = *(EX_match_score+(int)(maxrawscore*7));
	}
#else
	if (maxscore < 0) {
	    cmm_ptr->result_lists_p[0].score[n] = 0;
	}
	else {
	    cmm_ptr->result_lists_p[0].score[n] = maxscore > 1 ? EX_match_exact : maxscore*11;
	}
#endif
    }
    else {
	cmm_ptr->result_lists_p[0].score[n] = maxscore > 1 ? EX_match_exact : *(EX_match_score+(int)(maxscore*7));
    }

    c_ptr->pp[d][0] = cf_ptr->pp[n][0];
    c_ptr->pp[d][1] = END_M;
    c_ptr->oblig[d] = TRUE;
    cpm_ptr->elem_b_ptr[d] = b_ptr;
    c_ptr->weight[d] = 0;
    c_ptr->adjacent[d] = FALSE;
    if (!demonstrative) {
	cpm_ptr->elem_b_num[d] = -2;	/* 省略を表す */
	_make_data_cframe_sm(cpm_ptr, b_ptr);	/* 問題: 格納場所が c_ptr->element_num 固定 */
	_make_data_cframe_ex(cpm_ptr, b_ptr);
	c_ptr->element_num++;
    }
    else {
	cpm_ptr->elem_b_num[d] = -3;	/* 照応を表す */

	/* 指示詞の場合、もとの指示詞の分のスコアを引いておく */
	cmm_ptr->pure_score[0] -= cmm_ptr->result_lists_p[0].score[n];
    }
    return 1;
}

/*==================================================================*/
int DeleteFromCF(ELLIPSIS_MGR *em_ptr, CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr)
/*==================================================================*/
{
    int i, count = 0;

    /* 省略の指示対象を入力側の格フレームから削除する */

    for (i = 0; i < cpm_ptr->cf.element_num; i++) {
	if (cpm_ptr->elem_b_num[i] <= -2) {
	    cmm_ptr->result_lists_p[0].flag[cmm_ptr->result_lists_d[0].flag[i]] = -1;
	    cmm_ptr->result_lists_d[0].flag[i] = -1;
	    count++;
	}
    }
    cpm_ptr->cf.element_num -= count;
    return 1;
}

/*==================================================================*/
int EllipsisDetectForVerb(SENTENCE_DATA *sp, ELLIPSIS_MGR *em_ptr, 
			  CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, 
			  CASE_FRAME *cf_ptr, int n, int last)
/*==================================================================*/
{
    /* 用言とその省略格が与えられる */

    /* cf_ptr = cpm_ptr->cmm[0].cf_ptr である */
    /* 用言 cpm_ptr の cf_ptr->pp[n][0] 格が省略されている
       cf_ptr->ex[n] に似ている文節を探す */

    int i, j, current = 1, bend;
    char feature_buffer[DATA_LEN], etc_buffer[DATA_LEN], *cp;
    SENTENCE_DATA *s, *cs;
    BNST_DATA *bp;

    if (OptDiscMethod == OPT_SVM || OptDiscMethod == OPT_DT) {
	maxscore = AssignReferentThresholdForSVM;
	maxtag = NULL;
    }
    else {
	maxscore = 0;
    }

    cs = sentence_data + sp->Sen_num - 1;
    memset(Bcheck, 0, sizeof(int)*BNST_MAX);

    for (i = 0; ExtraTags[i][0]; i++) {
	if (EllipsisDetectForVerbSubcontractExtraTags(cs, em_ptr, cpm_ptr, cmm_ptr, 
						      i, cf_ptr, n)) {
	    goto EvalAntecedent;
	}
    }

    /* 親をみる (PARA なら child 用言) */
    if (cpm_ptr->pred_b_ptr->parent) {
	/* 親が PARA */
	if (cpm_ptr->pred_b_ptr->parent->para_top_p) {
	    /* 自分と並列の用言 */
	    for (i = 0; cpm_ptr->pred_b_ptr->parent->child[i]; i++) {
		/* PARA の子供で、自分を以外の並列用言 */
		if (cpm_ptr->pred_b_ptr->parent->child[i] != cpm_ptr->pred_b_ptr &&
		    cpm_ptr->pred_b_ptr->parent->child[i]->para_type == PARA_NORMAL) {
		    if (SearchCaseComponent(cs, em_ptr, cpm_ptr, cmm_ptr, 
					    cpm_ptr->pred_b_ptr->parent->child[i], cf_ptr, n, RANKP, PLOC_PV)) {
			goto EvalAntecedent;
		    }
		}
	    }

	    /* 連用で係る親用言 (並列のとき) */
	    if (cpm_ptr->pred_b_ptr->parent->parent && 
		check_feature(cpm_ptr->pred_b_ptr->f, "係:連用")) {
		if (SearchCaseComponent(cs, em_ptr, cpm_ptr, cmm_ptr, 
					cpm_ptr->pred_b_ptr->parent->parent, cf_ptr, n, RANK3, PLOC_PARENTV)) {
		    goto EvalAntecedent;
		}
	    }
	}
	/* とりあえず、連用で係るひとつ上の親用言のみ */
	else if (check_feature(cpm_ptr->pred_b_ptr->f, "係:連用")) {
	    if (SearchCaseComponent(cs, em_ptr, cpm_ptr, cmm_ptr, 
				    cpm_ptr->pred_b_ptr->parent, cf_ptr, n, RANK3, PLOC_PARENTV)) {
		goto EvalAntecedent;
	    }
	}
	/* 「〜した後」など */
	else if (check_feature(cpm_ptr->pred_b_ptr->f, "従属節扱い")) {
	    if (SearchCaseComponent(cs, em_ptr, cpm_ptr, cmm_ptr, 
				    cpm_ptr->pred_b_ptr->parent->parent, cf_ptr, n, RANK3, PLOC_PARENTV)) {
		goto EvalAntecedent;
	    }
	}
    }

    /* 子供 (用言) を見る
       check する feature: 用言 -> 係:連用 */
    for (i = 0; cpm_ptr->pred_b_ptr->child[i]; i++) {
	/* 子供が <PARA> */
	if (cpm_ptr->pred_b_ptr->child[i]->para_top_p) {
	    for (j = 0; cpm_ptr->pred_b_ptr->child[i]->child[j]; j++) {
		if (!cpm_ptr->pred_b_ptr->child[i]->child[j]->para_top_p && 
		    check_feature(cpm_ptr->pred_b_ptr->child[i]->child[j]->f, "係:連用")) {
		    if (SearchCaseComponent(cs, em_ptr, cpm_ptr, cmm_ptr, 
					    cpm_ptr->pred_b_ptr->child[i]->child[j], cf_ptr, n, RANK3, PLOC_CHILDPV)) {
			goto EvalAntecedent;
		    }
		}
	    }
	}
	else {
	    if (check_feature(cpm_ptr->pred_b_ptr->child[i]->f, "係:連用")) {
		if (SearchCaseComponent(cs, em_ptr, cpm_ptr, cmm_ptr, 
					cpm_ptr->pred_b_ptr->child[i], cf_ptr, n, RANK3, PLOC_CHILDV)) {
		    goto EvalAntecedent;
		}
	    }
	}
    }

    if ((cp = check_feature(cpm_ptr->pred_b_ptr->f, "照応ヒント"))) {
	if (str_eq(cp, "照応ヒント:係")) {
	    /* 係り先の用言に係る格要素をみる (サ変名詞のとき) */
	    if (SearchCaseComponent(cs, em_ptr, cpm_ptr, cmm_ptr, 
				    cs->bnst_data+cpm_ptr->pred_b_ptr->dpnd_head, cf_ptr, n, RANK3, PLOC_NPARENTV)) {
		goto EvalAntecedent;
	    }

	    /* 係り先の用言に係る従属節に係る格要素をみる */
	    for (i = 0; (cs->bnst_data+cpm_ptr->pred_b_ptr->dpnd_head)->child[i]; i++) {
		if (check_feature((cs->bnst_data+cpm_ptr->pred_b_ptr->dpnd_head)->child[i]->f, "用言")) {
		    if (SearchCaseComponent(cs, em_ptr, cpm_ptr, cmm_ptr, 
					    (cs->bnst_data+cpm_ptr->pred_b_ptr->dpnd_head)->child[i], 
					    cf_ptr, n, RANK3, PLOC_NPARENTCV)) {
			goto EvalAntecedent;
		    }
		}
	    }
	}
	else {
	    i = cpm_ptr->pred_b_ptr->num+atoi(cp+11);
	    if (i >= 0 && i < cs->Bnst_num) {
		if (SearchCaseComponent(cs, em_ptr, cpm_ptr, cmm_ptr, cs->bnst_data+i, cf_ptr, n, RANK3, PLOC_NOTHERS)) {
		    goto EvalAntecedent;
		}
	    }
	}
    }

    /* 前の文の体言を探す (この用言の格要素になっているもの以外) */
    for (s = cs; s >= sentence_data; s--) {
	bend = s->Bnst_num;

	for (i = bend-1; i >= 0; i--) {
	    if (current) {
		if (Bcheck[i]) {
		    continue;
		}
		else if ((!check_feature((s->bnst_data+i)->f, "係:連用") && 
		     (s->bnst_data+i)->dpnd_head == cpm_ptr->pred_b_ptr->num) || /* 用言に直接係らない (連用は可) */
		    (!check_feature(cpm_ptr->pred_b_ptr->f, "係:未格") && /* 「〜 V のは N」を許す */
		     !check_feature(cpm_ptr->pred_b_ptr->f, "係:ガ格") && 
		     (cpm_ptr->pred_b_ptr->dpnd_head == (s->bnst_data+i)->num)) || /* 用言が対象に係らない */
		    CheckCaseComponent(cpm_ptr, i) || /* 元用言がその文節を格要素としてもたない */
		    CheckAnaphor(banlist, (s->bnst_data+i)->Jiritu_Go)) {
		    RegisterAnaphor(banlist, (s->bnst_data+i)->Jiritu_Go);
		    continue;
		}

		/* 自分自身じゃない (「(N が) 〜 N だ」があるので禁止リストに登録しない) */
		if (i == cpm_ptr->pred_b_ptr->num) {
		    continue;
		}

		/* 用言の格要素となるノ格 */
		if (0 && i < cpm_ptr->pred_b_ptr->num && 
		    check_feature((s->bnst_data+i)->f, "ノ格用言チェック") && 
		    !MatchPP(cf_ptr->pp[n][0], "ガ")) {
		    bp = s->bnst_data+i;
		    /* ノ格の連続を飛ばす */
		    while (check_feature(bp->f, "係:ノ格")) {
			bp = s->bnst_data+bp->dpnd_head;
		    }
		    /* A の B を V 
		       B(bp) の head が V であれば */
		    if (bp->dpnd_head == cpm_ptr->pred_b_ptr->num) {
			if (EllipsisDetectForVerbSubcontract(s, cs, em_ptr, cpm_ptr, cmm_ptr, 
							     s->bnst_data+i, cf_ptr, n, RANK2, 0)) {
			    goto EvalAntecedent;
			}
			continue;
		    }
		}

		/* A の B を 〜 V */
		if ((cp = check_feature((s->bnst_data+i)->f, "省略候補チェック"))) {
		    if ((s->bnst_data+i+atoi(cp+17))->dpnd_head == cpm_ptr->pred_b_ptr->num) {
			RegisterAnaphor(banlist, (s->bnst_data+i)->Jiritu_Go);
			continue;
		    }
		}
	    }

	    /* 省略要素となるためのとりあえずの条件 */
	    if (!CheckPureNoun(s->bnst_data+i) || 
		CheckAnaphor(banlist, (s->bnst_data+i)->Jiritu_Go)) {
		continue;
	    }

	    if (EllipsisDetectForVerbSubcontract(s, cs, em_ptr, cpm_ptr, cmm_ptr, 
						 s->bnst_data+i, cf_ptr, n, RANK1, 0)) {
		goto EvalAntecedent;
	    }
	}
	if (current)
	    current = 0;
    }

  EvalAntecedent:
    if (OptDiscMethod == OPT_SVM || OptDiscMethod == OPT_DT) {
	if (MatchPP(cf_ptr->pp[n][0],"デ") || 
	    MatchPP(cf_ptr->pp[n][0], "ト") || 
	    MatchPP(cf_ptr->pp[n][0], "ヘ") || 
	    MatchPP(cf_ptr->pp[n][0], "ヨリ") || 
	    MatchPP(cf_ptr->pp[n][0], "カラ") || 
	    MatchPP(cf_ptr->pp[n][0], "マデ") || 
	    MatchPP(cf_ptr->pp[n][0], "ガ２") || 
	    MatchPP(cf_ptr->pp[n][0], "ノ") || 
	    MatchPP(cf_ptr->pp[n][0], "外の関係")) {
	    sprintf(feature_buffer, "省略解析なし-%s", 
		    pp_code_to_kstr(cf_ptr->pp[n][0]));
	    assign_cfeature(&(em_ptr->f), feature_buffer);
	    return 0;
	}

	if (maxscore > AssignReferentThresholdForSVM) {
	    if (maxtag) {
		if (str_eq(maxtag, "不特定-人")) {
		    sprintf(feature_buffer, "C用;【不特定:人】;%s;-1;-1;1", 
			    pp_code_to_kstr(cf_ptr->pp[n][0]));
		    assign_cfeature(&(em_ptr->f), feature_buffer);
		    em_ptr->cc[cf_ptr->pp[n][0]].bnst = ELLIPSIS_TAG_UNSPECIFIED_PEOPLE;
		    return 0;
		}
		else if (str_eq(maxtag, "対象外")) {
		    sprintf(feature_buffer, "C用;【対象外】;%s;-1;-1;1", 
			    pp_code_to_kstr(cf_ptr->pp[n][0]));
		    assign_cfeature(&(em_ptr->f), feature_buffer);
		    em_ptr->cc[cf_ptr->pp[n][0]].bnst = ELLIPSIS_TAG_EXCEPTION;
		    /* AppendToCF(cpm_ptr, cmm_ptr, cpm_ptr->pred_b_ptr, cf_ptr, n, maxscore, -1); */
		    return 1;
		}
		/*
		else if (str_eq(maxtag, "不特定物")) {
		    sprintf(feature_buffer, "C用;【例外:なし】;%s;-1;-1;1", 
			    pp_code_to_kstr(cf_ptr->pp[n][0]));
		    assign_cfeature(&(em_ptr->f), feature_buffer);
		    * ★最大スコアの指示対象を dummy で格フレームに保存 
		       それが、ほかの格の候補にならなくなるのは問題★ *
		    AppendToCF(cpm_ptr, cmm_ptr, cpm_ptr->pred_b_ptr, cf_ptr, n, maxscore, -1);
		    return 1;
		}
		*/
		else if (str_eq(maxtag, "一人称")) {
		    sprintf(feature_buffer, "C用;【一人称】;%s;-1;-1;1", 
			    pp_code_to_kstr(cf_ptr->pp[n][0]));
		    assign_cfeature(&(em_ptr->f), feature_buffer);
		    em_ptr->cc[cf_ptr->pp[n][0]].bnst = ELLIPSIS_TAG_I_WE;
		    /* AppendToCF(cpm_ptr, cmm_ptr, cpm_ptr->pred_b_ptr, cf_ptr, n, maxscore, -1); */
		    return 1;
		}
		else if (str_eq(maxtag, "不特定-状況")) {
		    sprintf(feature_buffer, "C用;【不特定:状況】;%s;-1;-1;1", 
			    pp_code_to_kstr(cf_ptr->pp[n][0]));
		    assign_cfeature(&(em_ptr->f), feature_buffer);
		    em_ptr->cc[cf_ptr->pp[n][0]].bnst = ELLIPSIS_TAG_UNSPECIFIED_CASE;
		    return 1;
		}
	    }

	    /* 
	    if (check_feature(cpm_ptr->pred_b_ptr->f, "省略格指定")) {
		;
	    }
	    else */ {
		int distance;

		distance = cs-maxs;
		if (distance == 0) {
		    strcpy(etc_buffer, "同一文");
		}
		else if (distance > 0) {
		    sprintf(etc_buffer, "%d文前", distance);
		}

		/* 決定した省略関係 */
		sprintf(feature_buffer, "C用;【%s】;%s;%d;%d;%.3f:%s(%s):%d文節", 
			(maxs->bnst_data+maxi)->Jiritu_Go, 
			pp_code_to_kstr(cf_ptr->pp[n][0]), 
			distance, maxi, 
			maxscore, maxs->KNPSID ? maxs->KNPSID+5 : "?", 
			etc_buffer, maxi);
		assign_cfeature(&(em_ptr->f), feature_buffer);
		em_ptr->cc[cf_ptr->pp[n][0]].s = maxs;
		em_ptr->cc[cf_ptr->pp[n][0]].bnst = maxi;

		/* 指示対象を格フレームに保存 */
		AppendToCF(cpm_ptr, cmm_ptr, maxs->bnst_data+maxi, cf_ptr, n, maxscore, maxpos);

		return 1;
	    }
	}
	return 0;
    }

    /* 一人称 */
    if ((cp = check_feature(cpm_ptr->pred_b_ptr->f, "一人称")) && 
	     MatchPP(cf_ptr->pp[n][0], cp+7)) {
	sprintf(feature_buffer, "C用;【一人称】;%s;-1;-1;1", 
		    pp_code_to_kstr(cf_ptr->pp[n][0]));
	assign_cfeature(&(em_ptr->f), feature_buffer);
	em_ptr->cc[cf_ptr->pp[n][0]].bnst = ELLIPSIS_TAG_I_WE;
	/* AppendToCF(cpm_ptr, cmm_ptr, maxs->bnst_data+maxi, cf_ptr, n, maxscore, maxpos); */
	return 1;
    }
    /* 対象外 */
    else if ((cp = check_feature(cpm_ptr->pred_b_ptr->f, "対象外")) && 
	     MatchPP(cf_ptr->pp[n][0], cp+7)) {
	sprintf(feature_buffer, "C用;【対象外】;%s;-1;-1;1", 
		    pp_code_to_kstr(cf_ptr->pp[n][0]));
	assign_cfeature(&(em_ptr->f), feature_buffer);
	em_ptr->cc[cf_ptr->pp[n][0]].bnst = ELLIPSIS_TAG_EXCEPTION;
	/* AppendToCF(cpm_ptr, cmm_ptr, maxs->bnst_data+maxi, cf_ptr, n, maxscore, maxpos); */
	return 1;
    }
    /* 【不特定:人】
       1. 用言が受身でニ格 (もとはガ格) に <主体> をとり、閾値を越えるとき
       2. ガ格 <主体> で、閾値を越えるとき
       3. 〜が V した N (外の関係, !判定詞), 形副名詞, 相対名詞は除く
       4. スコアが閾値より下でガ格 <主体> をとるとき */
    else if (((cp = check_feature(cpm_ptr->pred_b_ptr->f, "不特定人")) && 
	      MatchPP(cf_ptr->pp[n][0], cp+9)) || 
	     (cmm_ptr->result_lists_p[0].flag[n] == UNASSIGNED && /* 指示詞ではない */
	      ((MatchPP(cf_ptr->pp[n][0], "ガ") && 
		cf_match_element(cf_ptr->sm[n], "主体", FALSE) && 
		(maxscore <= AssignGaCaseThreshold)) || 
	       (MatchPP(cf_ptr->pp[n][0], "ニ") && 
		cf_match_element(cf_ptr->sm[n], "主体", FALSE) && 
		!cf_match_element(cf_ptr->sm[n], "場所", FALSE) && 
		maxscore <= AssignGaCaseThreshold && 
		(check_feature(cpm_ptr->pred_b_ptr->f, "〜れる") || 
		 check_feature(cpm_ptr->pred_b_ptr->f, "〜られる") || 
		 check_feature(cpm_ptr->pred_b_ptr->f, "追加受身") || 
		 check_feature(cpm_ptr->pred_b_ptr->f, "非用言格解析"))) || 
	       (cpm_ptr->pred_b_ptr->parent && 
		(check_feature(cpm_ptr->pred_b_ptr->parent->f, "外の関係") || 
		 check_feature(cpm_ptr->pred_b_ptr->parent->f, "外の関係可能性") || 
		 check_feature(cpm_ptr->pred_b_ptr->parent->f, "外の関係判定")) && 
		!check_feature(cpm_ptr->pred_b_ptr->parent->f, "時間") && 
		!check_feature(cpm_ptr->pred_b_ptr->parent->f, "相対名詞") && 
		!check_feature(cpm_ptr->pred_b_ptr->parent->f, "形副名詞") && 
		!check_feature(cpm_ptr->pred_b_ptr->parent->f, "用言") && 
		check_feature(cpm_ptr->pred_b_ptr->f, "係:連格") && 
		cf_ptr->pp[n][0] == pp_kstr_to_code("ガ"))))) {
	/* 格フレームを決めるループのときに feature を与えるのは問題 */
	sprintf(feature_buffer, "C用;【不特定:人】;%s;-1;-1;1", 
		pp_code_to_kstr(cf_ptr->pp[n][0]));
	assign_cfeature(&(em_ptr->f), feature_buffer);
	em_ptr->cc[cf_ptr->pp[n][0]].bnst = ELLIPSIS_TAG_UNSPECIFIED_PEOPLE;
    }
    /* 次の場合は省略要素を探すが記録しない 
       (現時点ではデータを見るため、これらも省略解析を行っている) 
       デ格, ト格, ヨリ格, カラ格, マデ格 */
    else if (!MatchPPn(cf_ptr->pp[n][0], DiscAddedCases) && 
	     (MatchPP(cf_ptr->pp[n][0],"デ") || 
	      MatchPP(cf_ptr->pp[n][0], "ト") || 
	      MatchPP(cf_ptr->pp[n][0], "ヘ") || 
	      MatchPP(cf_ptr->pp[n][0], "ヨリ") || 
	      MatchPP(cf_ptr->pp[n][0], "カラ") || 
	      MatchPP(cf_ptr->pp[n][0], "マデ") || 
	      MatchPP(cf_ptr->pp[n][0], "ガ２") || 
	      MatchPP(cf_ptr->pp[n][0], "ノ") || 
	      MatchPP(cf_ptr->pp[n][0], "外の関係"))) {
	sprintf(feature_buffer, "省略解析なし-%s", 
		pp_code_to_kstr(cf_ptr->pp[n][0]));
	assign_cfeature(&(em_ptr->f), feature_buffer);
    }
    else if (check_feature(cpm_ptr->pred_b_ptr->f, "省略格指定")) {
	;
    }
    /* ガ格:時間 があるとき、「不特定:状況」とする
    else if (maxscore > 0 && 
	     MatchPP(cf_ptr->pp[n][0], "ガ") && 
	     cf_match_element(cf_ptr->sm[n], "時間", TRUE)) {
	sprintf(feature_buffer, "C用;【不特定:状況】;%s;-1;-1;1", 
		pp_code_to_kstr(cf_ptr->pp[n][0]));
	assign_cfeature(&(em_ptr->f), feature_buffer);
	em_ptr->cc[cf_ptr->pp[n][0]].bnst = ELLIPSIS_TAG_UNSPECIFIED_CASE;
	AppendToCF(cpm_ptr, cmm_ptr, maxs->bnst_data+maxi, cf_ptr, n, maxscore, maxpos);
	return 1;
    } */
    /* ヲ格:補文 があるときで、良いマッチがないときは「前文」とする */
    else if (sp->Sen_num > 1 && 
	     maxscore > 0 && 
	     maxscore < 0.8 && 
	     MatchPP(cf_ptr->pp[n][0], "ヲ") && 
	     cf_match_element(cf_ptr->sm[n], "補文", TRUE)) {
	sprintf(feature_buffer, "C用;【前文】;%s;-1;-1;1", 
		pp_code_to_kstr(cf_ptr->pp[n][0]));
	assign_cfeature(&(em_ptr->f), feature_buffer);
	em_ptr->cc[cf_ptr->pp[n][0]].bnst = ELLIPSIS_TAG_PRE_SENTENCE;
	/* AppendToCF(cpm_ptr, cmm_ptr, maxs->bnst_data+maxi, cf_ptr, n, maxscore, maxpos); */
	return 1;
    }
    /* ★考える必要あり */
    else if (maxscore > 0 && 
	     maxscore < AssignReferentThresholdAnonymousThing && 
	     MatchPP(cf_ptr->pp[n][0], "ヲ") && 
	     check_feature(cpm_ptr->pred_b_ptr->f, "非用言格解析") && 
	     sm_match_check(sm2code("抽象物"), cpm_ptr->pred_b_ptr->SM_code)) {
	/* sprintf(feature_buffer, "C用;【例外:なし】;%s;-1;-1;1", 
		pp_code_to_kstr(cf_ptr->pp[n][0]));
	assign_cfeature(&(em_ptr->f), feature_buffer); */
	/* ★最大スコアの指示対象を dummy で格フレームに保存 
	   それが、ほかの格の候補にならなくなるのは問題★ */
	/* AppendToCF(cpm_ptr, cmm_ptr, maxs->bnst_data+maxi, cf_ptr, n, maxscore, maxpos); */
	return 1;
    }
    /* 格解析で格フレームを決定していたら閾値なし
       その他の場合は閾値: AssignReferentThreshold
       指示詞のときは閾値なし */
    else if (maxscore > 0 && 
	     (cmm_ptr->result_lists_p[0].flag[n] != UNASSIGNED || /* 指示詞 */
	      (cpm_ptr->decided == CF_DECIDED && 
	       maxscore > AssignReferentThresholdDecided) || /* maxscore == 0 のとき省略の割り当てはないので > 0 にする必要がある */
	      maxscore > AssignReferentThreshold)) {
	int distance;
	char *word;

	if (distance == 0 && cpm_ptr->pred_b_ptr->num > maxi && 
	    check_feature((maxs->bnst_data+maxi)->f, "ノ格用言チェック") && 
	    maxscore < AssignReferentThresholdHigh) {
	    return 0;
	}

	distance = cs-maxs;
	if (distance == 0) {
	    strcpy(etc_buffer, "同一文");
	}
	else if (distance > 0) {
	    sprintf(etc_buffer, "%d文前", distance);
	}

	word = make_print_string(maxs->bnst_data+maxi, 0);

	/* 決定した省略関係 */
	sprintf(feature_buffer, "C用;【%s】;%s;%d;%d;%.3f:%s(%s):%d文節", 
		word ? word : "?", 
		pp_code_to_kstr(cf_ptr->pp[n][0]), 
		distance, maxi, 
		maxscore, maxs->KNPSID ? maxs->KNPSID+5 : "?", 
		etc_buffer, maxi);
	assign_cfeature(&(em_ptr->f), feature_buffer);
	if (word) free(word);
	em_ptr->cc[cf_ptr->pp[n][0]].s = maxs;
	em_ptr->cc[cf_ptr->pp[n][0]].bnst = maxi;
	em_ptr->cc[cf_ptr->pp[n][0]].dist = distance;

	/* 指示対象を格フレームに保存 */
	AppendToCF(cpm_ptr, cmm_ptr, maxs->bnst_data+maxi, cf_ptr, n, maxscore, maxpos);

	return 1;
    }
    return 0;
}

/*==================================================================*/
	       int GetElementID(CASE_FRAME *cfp, int c)
/*==================================================================*/
{
    /* 格の番号から、格フレームの要素番号に変換する */

    int i;

    for (i = 0; i < cfp->element_num; i++) {
	if (cfp->pp[i][0] == c) {
	    return i;
	}
    }
    return -1;
}

/*==================================================================*/
int MarkEllipsisCase(CF_PRED_MGR *cpm_ptr, CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{
    char feature_buffer[DATA_LEN];

    /* 省略されている格をマーク
    sprintf(feature_buffer, "C省略-%s", 
	    pp_code_to_kstr(cf_ptr->pp[n][0]));
    assign_cfeature(&(cpm_ptr->pred_b_ptr->f), feature_buffer);
    */

    /* <不特定:状況> をガ格としてとる判定詞 */
    if (check_feature(cpm_ptr->pred_b_ptr->f, "時間ガ省略") && 
	MatchPP(cf_ptr->pp[n][0], "ガ")) {
	sprintf(feature_buffer, "C用;【不特定:状況】;%s;-1;-1;1", 
		pp_code_to_kstr(cf_ptr->pp[n][0]));
	assign_cfeature(&(cpm_ptr->pred_b_ptr->f), feature_buffer);
	return 0;
    }
    return 1;
}

/*==================================================================*/
void AppendCfFeature(ELLIPSIS_MGR *em_ptr, CF_PRED_MGR *cpm_ptr, CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{
    char feature_buffer[DATA_LEN];

    /* 格フレームが <主体準> をもつかどうか */
    if (cf_ptr->etcflag & CF_GA_SEMI_SUBJECT) {
	sprintf(feature_buffer, "格フレーム-%s-主体準", pp_code_to_kstr(cf_ptr->pp[n][0]));
	assign_cfeature(&(em_ptr->f), feature_buffer);
    }
    /* 格フレームが <主体> をもつかどうか */
    else if (cf_match_element(cf_ptr->sm[n], "主体", FALSE)) {
	sprintf(feature_buffer, "格フレーム-%s-主体", pp_code_to_kstr(cf_ptr->pp[n][0]));
	assign_cfeature(&(em_ptr->f), feature_buffer);
    }


    /* 格フレームが <補文> をもつかどうか */
    if (cf_match_element(cf_ptr->sm[n], "補文", TRUE)) {
	sprintf(feature_buffer, "格フレーム-%s-補文", pp_code_to_kstr(cf_ptr->pp[n][0]));
	assign_cfeature(&(em_ptr->f), feature_buffer);
    }

    if (sm_match_check(sm2code("抽象物"), cpm_ptr->pred_b_ptr->SM_code)) {
	sprintf(feature_buffer, "格フレーム-%s-抽象物", pp_code_to_kstr(cf_ptr->pp[n][0]));
	assign_cfeature(&(em_ptr->f), feature_buffer);
    }
}

/*==================================================================*/
float EllipsisDetectForVerbMain(SENTENCE_DATA *sp, ELLIPSIS_MGR *em_ptr, CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, 
				CASE_FRAME *cf_ptr, char **order, int mainflag, int onceflag)
/*==================================================================*/
{
    int i, j, num, result, toflag = 0;
    int cases[PP_NUMBER], count = 0;

    /* onceflag が指定された場合には、
       省略要素をひとつ発見し、そのときに最もよい格フレームを探す
       ★ 故障中 (cmm 関連) */

    /* 「<補文>と 〜を V した」 
       ト格があるとき、ヲ格を省略としない 
       (格フレーム側の<補文>はチェックしていない) */
    for (i = 0; i < cf_ptr->element_num; i++) {
	num = cmm_ptr->result_lists_p[0].flag[i];
	if (num != UNASSIGNED && 
	    MatchPP(cf_ptr->pp[i][0], "ト") && 
	    check_feature(cpm_ptr->elem_b_ptr[num]->f, "補文")) {
	    toflag = 1;
	    break;
	}
    }

    for (j = 0; *order[j]; j++) {
	cases[count++] = pp_kstr_to_code(order[j]);
    }
    for (j = 0; DiscAddedCases[j] != END_M; j++) {
	cases[count++] = DiscAddedCases[j];
    }
    cases[count] = END_M;

    /* 格を与えられた順番に */
    for (j = 0; cases[j] != END_M; j++) {
	for (i = 0; i < cf_ptr->element_num; i++) {
	    if (((cf_ptr->pp[i][0] == cases[j] && 
		 cmm_ptr->result_lists_p[0].flag[i] == UNASSIGNED) || 
		(OptDemo == TRUE && /* 割り当てがあって、指示詞のとき */
		 cmm_ptr->result_lists_p[0].flag[i] != UNASSIGNED && 
		 cf_ptr->pp[i][0] == cases[j] && 
		 check_feature(cpm_ptr->elem_b_ptr[cmm_ptr->result_lists_p[0].flag[i]]->f, "省略解析対象指示詞"))) && 
		!(toflag && MatchPP(cf_ptr->pp[i][0], "ヲ"))) {
		if ((MarkEllipsisCase(cpm_ptr, cf_ptr, i)) == 0) {
		    continue;
		}
		result = EllipsisDetectForVerb(sp, em_ptr, cpm_ptr, cmm_ptr, cf_ptr, i, mainflag);
		AppendCfFeature(em_ptr, cpm_ptr, cf_ptr, i);
		if (result) {
		    em_ptr->cc[cf_ptr->pp[i][0]].score = maxscore;

		    if (OptDiscMethod == OPT_SVM || OptDiscMethod == OPT_DT) {
#ifdef USE_RAWSCORE
			em_ptr->score += maxrawscore > 1.0 ? EX_match_exact : maxrawscore < 0 ? 0 : *(EX_match_score+(int)(maxrawscore*7));
#else
			em_ptr->score += maxscore > 1.0 ? EX_match_exact : maxscore < 0 ? 0 : 11*maxscore;
#endif
		    }
		    else {
			if (maxscore == (float)EX_match_subject/11) {
			    em_ptr->score += EX_match_subject;
			}
			else {
			    em_ptr->score += maxscore > 1.0 ? EX_match_exact : *(EX_match_score+(int)(maxscore*7));
			}
		    }

		    if (onceflag) {
			/* ひとつの省略の指示対象をみつけたので、
			   ここでもっともスコアの高い格フレームを再調査する */
			find_best_cf(sp, cpm_ptr, -1, 0);
			return em_ptr->score;
		    }
		}
	    }
	}
    }

    /* onceflag 時に、指定された格がどれも見つからなければ、
       以下でどの格でもよいから見つける */

    for (i = 0; i < cf_ptr->element_num; i++) {
	num = cmm_ptr->result_lists_p[0].flag[i];
	/* 以下の格の場合、省略要素と認定しない
	   時間格, 修飾格, 無格, 複合辞
	*/
	if (num == UNASSIGNED && 
	    !(toflag && str_eq(pp_code_to_kstr(cf_ptr->pp[i][0]), "ヲ")) && 
	    !(cmm_ptr->cf_ptr->pp[i][0] > 8 && cf_ptr->pp[i][0] < 38) && 
	    !MatchPP(cf_ptr->pp[i][0], "時間") && 
	    !MatchPP(cf_ptr->pp[i][0], "φ") && 
	    !MatchPP(cf_ptr->pp[i][0], "修飾") && 
	    !MatchPP(cf_ptr->pp[i][0], "外の関係") && 
	    !MatchPP(cf_ptr->pp[i][0], "ノ")) {
	    if ((MarkEllipsisCase(cpm_ptr, cf_ptr, i)) == 0) {
		continue;
	    }
	    result = EllipsisDetectForVerb(sp, em_ptr, cpm_ptr, cmm_ptr, cf_ptr, i, mainflag);
	    AppendCfFeature(em_ptr, cpm_ptr, cf_ptr, i);
	    if (result) {
		em_ptr->cc[cf_ptr->pp[i][0]].score = maxscore;
		/* 
		if (maxscore == (float)EX_match_subject/11) {
		    em_ptr->score += EX_match_subject;
		}
		else */
		em_ptr->score += maxscore > 1.0 ? EX_match_exact : *(EX_match_score+(int)(maxscore*7));
		if (onceflag) {
		    find_best_cf(sp, cpm_ptr, -1, 0);
		    return em_ptr->score;
		}
	    }
	}
    }

    if (onceflag) {
	return -1;
    }
    return em_ptr->score;
}

/*==================================================================*/
	    int CompareCPM(CF_PRED_MGR *a, CF_PRED_MGR *b)
/*==================================================================*/
{
    int i, j, flag;

    /* 異なる場合は 1 を返す */

    if (a->cf.element_num != b->cf.element_num) {
	return 1;
    }

    /* 順番が違うことがあるので単純比較ではだめ */

    for (i = 0; i < a->cf.element_num; i++) {
	flag = 0;
	for (j = 0; j < b->cf.element_num; j++) {
	    if (a->elem_b_ptr[i] == b->elem_b_ptr[j]) {
		flag = 1;
		break;
	    }
	}
	if (!flag) {
	    return 1;
	}
    }
    return 0;
}
/*==================================================================*/
	   int CompareCMM(CF_PRED_MGR *ap, CF_MATCH_MGR *a, 
			  CF_PRED_MGR *bp, CF_MATCH_MGR *b)
/*==================================================================*/
{
    int i;

    /* 異なる場合は 1 を返す */

    for (i = 0; i < a->cf_ptr->element_num; i++) {
	if (a->result_lists_p[0].flag[i] != UNASSIGNED && 
	    ap->elem_b_ptr[a->result_lists_p[0].flag[i]] != bp->elem_b_ptr[b->result_lists_p[0].flag[i]]) {
	    return 1;
	}
    }
    return 0;
}

/*==================================================================*/
int CompareAssignList(ELLIPSIS_MGR *maxem, CF_PRED_MGR *cpm, CF_MATCH_MGR *cmm)
/*==================================================================*/
{
    int i;

    for (i = 0; i < maxem->result_num; i++) {
	/* 同じものがすでにある場合 */
	if (maxem->ecmm[i].cmm.cf_ptr == cmm->cf_ptr && 
	    maxem->ecmm[i].element_num == cpm->cf.element_num && 
	    !CompareCPM(&(maxem->ecmm[i].cpm), cpm) && 
	    !CompareCMM(&(maxem->ecmm[i].cpm), &(maxem->ecmm[i].cmm), cpm, cmm)) {
	    return 0;
	}
    }
    return 1;
}

/*==================================================================*/
      int CompareClosestScore(CF_MATCH_MGR *a, CF_MATCH_MGR *b)
/*==================================================================*/
{
    int i, acount = 0, bcount = 0;
    float ascore = 0, bscore = 0;

    for (i = 0; i < a->cf_ptr->element_num; i++) {
	if (a->result_lists_p[0].flag[i] != UNASSIGNED && 
	    a->cf_ptr->adjacent[i] == TRUE) {
	    acount++;
	    ascore = a->result_lists_p[0].score[i];
	}
    }

    for (i = 0; i < b->cf_ptr->element_num; i++) {
	if (b->result_lists_p[0].flag[i] != UNASSIGNED && 
	    b->cf_ptr->adjacent[i] == TRUE) {
	    bcount++;
	    bscore = b->result_lists_p[0].score[i];
	}
    }

    if (acount < bcount || 
	(acount == bcount && 
	    ascore < bscore)) {
	return 1;
    }
    return 0;
}

/*==================================================================*/
void FindBestCFforContext(SENTENCE_DATA *sp, ELLIPSIS_MGR *maxem, CF_PRED_MGR *cpm_ptr, 
			  char **order, int mainflag)
/*==================================================================*/
{
    int k, l, frame_num;
    CASE_FRAME **cf_array;
    CF_MATCH_MGR cmm;
    ELLIPSIS_CMM tempecmm;
    ELLIPSIS_MGR workem;

    InitEllipsisMGR(&workem);

    if (OptDiscFlag & OPT_DISC_OR_CF) {
	frame_num = 0;
	cf_array = (CASE_FRAME **)malloc_data(sizeof(CASE_FRAME *), "FindBestCFforContext");
	for (l = 0; l < cpm_ptr->pred_b_ptr->cf_num; l++) {
	    if ((cpm_ptr->pred_b_ptr->cf_ptr+l)->etcflag & CF_SUM) {
		*cf_array = cpm_ptr->pred_b_ptr->cf_ptr+l;
		frame_num = 1;
		break;
	    }
	}
    }
    else {
	if (cpm_ptr->decided == CF_CAND_DECIDED) {
	    frame_num = cpm_ptr->tie_num;
	    cf_array = (CASE_FRAME **)malloc_data(sizeof(CASE_FRAME *)*frame_num, "FindBestCFforContext");
	    for (l = 0; l < frame_num; l++) {
		*(cf_array+l) = cpm_ptr->cmm[l].cf_ptr;
	    }
	}
	else {
	    frame_num = cpm_ptr->pred_b_ptr->cf_num;
	    cf_array = (CASE_FRAME **)malloc_data(sizeof(CASE_FRAME *)*frame_num, "FindBestCFforContext");
	    for (l = 0; l < frame_num; l++) {
		*(cf_array+l) = cpm_ptr->pred_b_ptr->cf_ptr+l;
	    }
	}
    }

    /* 候補の格フレームについて省略解析を実行 */

    for (l = 0; l < frame_num; l++) {
	/* OR の格フレームを除く */
	if (((*(cf_array+l))->etcflag & CF_SUM) && frame_num != 1) {
	    continue;
	}

	/* 格フレームを仮指定 */
	cmm.cf_ptr = *(cf_array+l);
	cpm_ptr->result_num = 1;

	/* 入力側格要素を設定
	   照応解析時はすでにある格要素を上書きしてしまうのでここで再設定
	   それ以外のときは下の DeleteFromCF() で省略要素をクリア */
	if (OptDemo == TRUE) {
	    make_data_cframe(sp, cpm_ptr);
	}

	/* 今ある格要素を対応づけ */
	case_frame_match(cpm_ptr, &cmm, OptCFMode, -1);
	cpm_ptr->score = cmm.score;

	ClearEllipsisMGR(&workem);
	EllipsisDetectForVerbMain(sp, &workem, cpm_ptr, &cmm, 
				  *(cf_array+l), 
				  order, mainflag, 0);

	if (cmm.score >= 0) {
	    /* 直接の格要素の正規化していないスコアを足す */
	    workem.score += cmm.pure_score[0];
	    /* 正規化 */
	    workem.score /= sqrt((double)(count_pat_element(cmm.cf_ptr, 
							     &(cmm.result_lists_p[0]))));
	    cmm.score = workem.score;
	}
	/* 格解析失敗のとき -- 解析をひとつだけ結果に入れるために
	   最大スコアのデフォルトを -2 にしている */
	else {
	    workem.score = cmm.score;
	}

	/* DEBUG 表示 */
	if (VerboseLevel >= VERBOSE3) {
	    fprintf(stdout, "★ 格フレーム %d\n", l);
	    print_data_cframe(cpm_ptr, &cmm);
	    print_good_crrspnds(cpm_ptr, &cmm, 1);
	    fprintf(stdout, "   FEATURES: ");
	    print_feature(workem.f, Outfp);
	    fputc('\n', Outfp);
	}

	if (workem.score > maxem->score || 
	    (workem.score == maxem->score && 
	     CompareClosestScore(&(maxem->ecmm[0].cmm), &cmm))) {
	    maxem->cpm = workem.cpm;
	    for (k = 0; k < CASE_MAX_NUM; k++) {
		maxem->cc[k] = workem.cc[k];
	    }
	    maxem->score = workem.score;
	    maxem->f = workem.f;
	    workem.f = NULL;

	    /* ひとつずつずらす */
	    for (k = maxem->result_num >= CMM_MAX-1 ? maxem->result_num-1 : maxem->result_num; k >= 0; k--) {
		maxem->ecmm[k+1] = maxem->ecmm[k];
	    }

	    /* 今回が最大マッチ */
	    maxem->ecmm[0].cmm = cmm;
	    maxem->ecmm[0].cpm = *cpm_ptr;
	    maxem->ecmm[0].element_num = cpm_ptr->cf.element_num;

	    if (maxem->result_num < CMM_MAX-1) {
		maxem->result_num++;
	    }
	}
	/* 新しい種類の格フレーム(割り当て)を保存 */
	else if (CompareAssignList(maxem, cpm_ptr, &cmm)) {
	    maxem->ecmm[maxem->result_num].cmm = cmm;
	    maxem->ecmm[maxem->result_num].cpm = *cpm_ptr;
	    maxem->ecmm[maxem->result_num].element_num = cpm_ptr->cf.element_num;
	    for (k = maxem->result_num-1; k >= 0; k--) {
		if (maxem->ecmm[k].cmm.score < maxem->ecmm[k+1].cmm.score) {
		    tempecmm = maxem->ecmm[k];
		    maxem->ecmm[k] = maxem->ecmm[k+1];
		    maxem->ecmm[k+1] = tempecmm;
		}
		else {
		    break;
		}
	    }

	    if (maxem->result_num < CMM_MAX-1) {
		maxem->result_num++;
	    }
	}

	/* 格フレームの追加エントリの削除 */
	if (OptDemo == FALSE) {
	    DeleteFromCF(&workem, cpm_ptr, &cmm);
	}
	ClearAnaphoraList(banlist);
    }
    free(cf_array);
}

/*==================================================================*/
	   void AssignFeaturesByProgram(SENTENCE_DATA *sp)
/*==================================================================*/
{
    /* 撲滅予定 */

    int i;

    for (i = 0; i < sp->Bnst_num; i++) {
	/* 隣以外の AのB はルールで与えられていない */
	if (!check_feature((sp->bnst_data+i)->f, "準主題表現") && 
	    check_feature((sp->bnst_data+i)->f, "係:ノ格") && 
	    (sp->bnst_data+i)->parent && 
	    check_feature((sp->bnst_data+i)->parent->f, "主題表現")) {
	    assign_cfeature(&((sp->bnst_data+i)->f), "準主題表現");
	}
    }
}

/*==================================================================*/
	      void DiscourseAnalysis(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, k, lastflag = 1, mainflag, anum;
    float score;
    ELLIPSIS_MGR workem, maxem;
    SENTENCE_DATA *sp_new;
    CF_PRED_MGR *cpm_ptr;
    CF_MATCH_MGR *cmm_ptr;
    CASE_FRAME *cf_ptr, *origcf;

    InitEllipsisMGR(&workem);
    InitEllipsisMGR(&maxem);

    AssignFeaturesByProgram(sp);

    sp_new = PreserveSentence(sp);

    /* 各用言をチェック (文末から) */
    for (j = 0; j < sp->Best_mgr->pred_num; j++) {
	cpm_ptr = &(sp->Best_mgr->cpm[j]);

	/* 格フレームがない場合 (ガ格ぐらい探してもいいかもしれない) */
	if (cpm_ptr->result_num == 0 || 
	    cpm_ptr->cmm[0].cf_ptr->cf_address == -1 || 
	    cpm_ptr->cmm[0].score == -2) {
	    continue;
	}

	/* 省略解析しない用言
	   1. ルールで「省略解析なし」feature がついているもの (「〜みられる」 など)
	   2. 準用言 (「非用言格解析」は除く)
	   3. カタカナのサ変名詞
	   4. レベル:A- (〜て（用言） (A), ※ 〜く（用言） は A-)
	   5. （〜を）〜に */
	if (check_feature(cpm_ptr->pred_b_ptr->f, "省略解析なし")) {
	    continue;
	}
	else if((!check_feature(cpm_ptr->pred_b_ptr->f, "非用言格解析") && 
		 check_feature(cpm_ptr->pred_b_ptr->f, "準用言")) || 
		check_feature(cpm_ptr->pred_b_ptr->f, "レベル:A-") || 
		check_feature(cpm_ptr->pred_b_ptr->f, "ID:（〜を）〜に")) {
	    assign_cfeature(&(cpm_ptr->pred_b_ptr->f), "省略解析なし");
	    continue;
	}
	    

	cmm_ptr = &(cpm_ptr->cmm[0]);
	cf_ptr = cmm_ptr->cf_ptr;

	/* その文の主節 */
	if (lastflag == 1 && 
	    !check_feature(cpm_ptr->pred_b_ptr->f, "非主節") && 
	    !check_feature(cpm_ptr->pred_b_ptr->f, "省略解析なし")) {
	    mainflag = 1;
	    lastflag = 0;
	    assign_cfeature(&(cpm_ptr->pred_b_ptr->f), "主節");
	}
	else {
	    mainflag = 0;
	}

	/* 格要素の個数 */
	anum = 0;
	for (i = 0; i < cf_ptr->element_num; i++) {
	    if (cmm_ptr->result_lists_p[0].flag[i] != UNASSIGNED) {
		anum++;
	    }
	}

	/* もっともスコアがよくなる順番で省略の指示対象を決定する */

	maxem.score = -2;
	origcf = cpm_ptr->cmm[0].cf_ptr;
	/* 今の cmm を保存していないけど => 必要ない */

	for (i = 0; i < CASE_ORDER_MAX; i++) {
	    if (cpm_ptr->decided == CF_DECIDED) {

		/* 入力側格要素を設定
		   照応解析時はすでにある格要素を上書きしてしまうのでここで再設定
		   それ以外のときは下の DeleteFromCF() で省略要素をクリア */
		if (OptDemo == TRUE) {
		    make_data_cframe(sp, cpm_ptr);
		}

		ClearEllipsisMGR(&workem);
		score = EllipsisDetectForVerbMain(sp, &workem, cpm_ptr, &(cpm_ptr->cmm[0]), 
						  cpm_ptr->cmm[0].cf_ptr, 
						  CaseOrder[i], mainflag, 0);
		/* 直接の格要素の正規化していないスコアを足す */
		workem.score += cpm_ptr->cmm[0].pure_score[0];
		workem.score /= sqrt((double)(count_pat_element(cpm_ptr->cmm[0].cf_ptr, 
								&(cpm_ptr->cmm[0].result_lists_p[0]))));
		if (workem.score > maxem.score) {
		    maxem = workem;
		    /* ★find_cf しなかったらいらない?★
		       この関数で格フレーム決定した場合は
		       その結果をスコア順に表示する */
		    maxem.result_num = cpm_ptr->result_num;
		    for (k = 0; k < maxem.result_num; k++) {
			maxem.ecmm[k].cmm = cpm_ptr->cmm[k];
			maxem.ecmm[k].cpm = *cpm_ptr;
			maxem.ecmm[k].element_num = cpm_ptr->cf.element_num;
		    }
		    workem.f = NULL;
		}

		/* 格フレームの追加エントリの削除 */
		if (OptDemo == FALSE) {
		    DeleteFromCF(&workem, cpm_ptr, &(cpm_ptr->cmm[0]));
		}
		ClearAnaphoraList(banlist);
	    }
	    else {
		FindBestCFforContext(sp, &maxem, cpm_ptr, CaseOrder[i], mainflag);
	    }

	    /*
	    if (VerboseLevel >= VERBOSE2) {
		fprintf(stdout, "CASE_ORDER %2d ==> %.3f\n", i, workem.score);
		print_data_cframe(cpm_ptr, &(cpm_ptr->cmm[0]));
		for (k = 0; k < cpm_ptr->result_num; k++) {
		    print_crrspnd(cpm_ptr, &(cpm_ptr->cmm[k]));
		}
		fputc('\n', Outfp);
	    }
	    */
	}

	/* もっとも score のよかった組み合わせを登録 */
	if (maxem.score > -2) {
	    cpm_ptr->score = maxem.score;
	    maxem.ecmm[0].cmm.score = maxem.score;
	    /* cmm を復元 */
	    cpm_ptr->result_num = maxem.result_num;
	    for (k = 0; k < cpm_ptr->result_num; k++) {
		cpm_ptr->cmm[k] = maxem.ecmm[k].cmm;
		cpm_ptr->cmm[k].cpm = (CF_PRED_MGR *)malloc_data(sizeof(CF_PRED_MGR), 
								 "DiscourseAnalysis");
		*cpm_ptr->cmm[k].cpm = maxem.ecmm[k].cpm;
	    }
	    cpm_ptr->cf.element_num = maxem.ecmm[0].element_num;
	    for (k = 0; k < maxem.ecmm[0].element_num; k++) {
		cpm_ptr->elem_b_ptr[k] = maxem.ecmm[0].cpm.elem_b_ptr[k];
		cpm_ptr->elem_b_num[k] = maxem.ecmm[0].cpm.elem_b_num[k];
	    }
	    /* feature の伝搬 */
	    append_feature(&(cpm_ptr->pred_b_ptr->f), maxem.f);
	    maxem.f = NULL;

	    /* 文脈解析において格フレームを決定した場合 */
	    if (cpm_ptr->decided != CF_DECIDED) {
		after_case_analysis(sp, cpm_ptr);
		assign_ga_subject(sp, cpm_ptr); /* CF_CAND_DECIDED の場合は行っているが */
		/* fix_sm_place(sp, cpm_ptr); */
		/* 格解析の結果を feature へ */
		record_match_ex(sp, cpm_ptr);
		record_case_analysis(sp, cpm_ptr, &maxem, mainflag);
	    }
	    else {
		/* 格解析の結果を feature へ */
		record_case_analysis(sp, cpm_ptr, &maxem, mainflag);
	    }

	    /* サ変名詞のガ格はまだ怪しいので記録しない -> サ変名詞すべて記録しない */
	    if (!check_feature(cpm_ptr->pred_b_ptr->f, "非用言格解析")) {
		for (i = 0; i < CASE_MAX_NUM; i++) {
		    if (maxem.cc[i].s) {
			/* リンクされたことを記録する */
			RegisterAnaphor(alist, (maxem.cc[i].s->bnst_data+maxem.cc[i].bnst)->Jiritu_Go);
			/* 用言と格要素のセットを記録 (格関係とは区別したい) */
			RegisterPredicate(L_Jiritu_M(cpm_ptr->pred_b_ptr)->Goi, 
					  cpm_ptr->pred_b_ptr->voice, 
					  cpm_ptr->cmm[0].cf_ptr->cf_address, 
					  i, 
					  (maxem.cc[i].s->bnst_data+maxem.cc[i].bnst)->Jiritu_Go, 
					  EREL);
		    }
		}
	    }

	    /* 主節の場合、記録 */
	    if (mainflag) {
		for (i = 0; i < CASE_MAX_NUM; i++) {
		    if (maxem.cc[i].s) {
			RegisterLastClause(sp->Sen_num, 
					   L_Jiritu_M(cpm_ptr->pred_b_ptr)->Goi, i, 
					   (maxem.cc[i].s->bnst_data+maxem.cc[i].bnst)->Jiritu_Go, EREL);
		    }
		}
	    }
	}
	ClearEllipsisMGR(&maxem);
    }

    PreserveCPM(sp_new, sp);
    clear_cf(0);
}

/*====================================================================
                               END
====================================================================*/
