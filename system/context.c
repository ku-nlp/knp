/*====================================================================

			       文脈解析

                                               S.Kurohashi 98. 9. 8

    $Id$
====================================================================*/
#include "knp.h"

float maxscore;
SENTENCE_DATA *maxs;
int maxi;
int Bcheck[BNST_MAX];

#define RANK0	0
#define RANK1	1
#define RANK2	2
#define RANK3	3

#define AssignReferentThreshold	0.3
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
   void RegisterPredicate(char *key, int voice, int pp, char *word, int flag)
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
	    if (!strcmp((*papp)->key, key) && (*papp)->voice == voice) {
		StoreCaseComponent(&((*papp)->cc[pp]), word, flag);
		return;
	    }
	    papp = &((*papp)->next);
	} while (*papp);
	*papp = (PALIST *)malloc_data(sizeof(PALIST), "RegisterPredicate");
	(*papp)->key = strdup(key);
	(*papp)->voice = voice;
	memset((*papp)->cc, 0, sizeof(CASE_COMPONENT *)*CASE_MAX_NUM);
	StoreCaseComponent(&((*papp)->cc[pp]), word, flag);
	(*papp)->next = NULL;
    }
    else {
	pap->key = strdup(key);
	pap->voice = voice;
	StoreCaseComponent(&(pap->cc[pp]), word, flag);
    }
}

/*==================================================================*/
     int CheckPredicate(char *key, int voice, int pp, char *word)
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
	if (!strcmp(pap->key, key) && pap->voice == voice) {
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
      void copy_cf_with_alloc(CASE_FRAME *dst, CASE_FRAME *src)
/*==================================================================*/
{
    int i, j;

    dst->element_num = src->element_num;
    for (i = 0; i < src->element_num; i++) {
	dst->oblig[i] = src->oblig[i];
	dst->adjacent[i] = src->adjacent[i];
	for (j = 0; j < PP_ELEMENT_MAX; j++) {
	    dst->pp[i][j] = src->pp[i][j];
	}
	if (src->sm[i]) {
	    dst->sm[i] = strdup(src->sm[i]);
	}
	else {
	    dst->sm[i] = NULL;
	}
	if (Thesaurus == USE_BGH) {
	    if (src->ex[i]) {
		dst->ex[i] = strdup(src->ex[i]);
	    }
	    else {
		dst->ex[i] = NULL;
	    }
	}
	else if (Thesaurus == USE_NTT) {
	    if (src->ex2[i]) {
		dst->ex2[i] = strdup(src->ex2[i]);
	    }
	    else {
		dst->ex2[i] = NULL;
	    }
	}
	if (src->examples[i]) {
	    dst->examples[i] = strdup(src->examples[i]);
	}
	else {
	    dst->examples[i] = NULL;
	}
	if (src->semantics[i]) {
	    dst->semantics[i] = strdup(src->semantics[i]);
	}
	else {
	    dst->semantics[i] = NULL;
	}
    }
    dst->voice = src->voice;
    dst->ipal_address = src->ipal_address;
    dst->ipal_size = src->ipal_size;
    strcpy(dst->ipal_id, src->ipal_id);
    strcpy(dst->imi, src->imi);
    dst->concatenated_flag = src->concatenated_flag;
    dst->flag = src->flag;
    if (src->entry) {
	dst->entry = strdup(src->entry);
    }
    else {
	dst->entry = NULL;
    }
    /* weight, pred_b_ptr は未設定 */
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

    init_mgr_cf(s);
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
	num = sp->Best_mgr->cpm[i].pred_b_ptr->num;	/* この用言の文節番号 */
	*(sp_new->cpm+i) = sp->Best_mgr->cpm[i];
	sp_new->bnst_data[num].cpm_ptr = sp_new->cpm+i;
	(sp_new->cpm+i)->pred_b_ptr = sp_new->bnst_data+num;
	for (j = 0; j < (sp_new->cpm+i)->cf.element_num; j++) {
	    /* 省略じゃない格要素 */
	    if ((sp_new->cpm+i)->elem_b_num[j] != -2) {
		(sp_new->cpm+i)->elem_b_ptr[j] = sp_new->bnst_data+((sp_new->cpm+i)->elem_b_ptr[j]-sp->bnst_data);
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

    /* 現在 cpm を保存しているが、Best_mgr を保存した方がいいかもしれない */
    sp_new->Best_mgr = NULL;
}

/*==================================================================*/
float CalcSimilarityForVerb(BNST_DATA *cand, CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{
    char *exd, *exp;
    int i, j, step;
    float score = 0, ex_score;

    if (Thesaurus == USE_BGH) {
	exd = cand->BGH_code;
	exp = cf_ptr->ex[n];
	step = BGH_CODE_SIZE;
    }
    else if (Thesaurus == USE_NTT) {
	exd = cand->SM_code;
	exp = cf_ptr->ex2[n];
	step = SM_CODE_SIZE;
    }

    /* 最大マッチスコアを求める */
    ex_score = (float)CalcSimilarity(exd, exp);
    if (Thesaurus == USE_BGH) {
	ex_score /= 7;
    }

    /* 主体のマッチング (とりあえずガ格のときだけ) */
    if (cf_ptr->sm[n] && MatchPP(cf_ptr->pp[n][0], "ガ")) {
	for (j = 0; cf_ptr->sm[n][j]; j+=step) {
	    if (strncmp(cf_ptr->sm[n]+j, sm2code("主体"), SM_CODE_SIZE))
		continue;
	    /* 格フレーム側に <主体> があるときに、格要素側をチェック */
	    if (check_feature(cand->f, "人名") || 
		check_feature(cand->f, "組織名")) {
		/* 固有名詞のときにスコアを高く */
		if (EX_match_subject > 8) {
		    /* 主体スコアが高いときは、それと同じにする */
		    score = (float)EX_match_subject/11;
		}
		else {
		    score = (float)9/11;
		}
		break;
	    }
	    for (i = 0; cand->SM_code[i]; i+=step) {
		if (_sm_match_score(cf_ptr->sm[n]+j, cand->SM_code+i, SM_NO_EXPAND_NE)) {
		    score = (float)EX_match_subject/11;
		    break;
		}
	    }
	    break;
	}
    }


    if (ex_score > score) {
	return ex_score;
    }
    else {
	return score;
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
    ex_score = (float)CalcSimilarity(exd, exp);
    if (Thesaurus == USE_BGH) {
	ex_score /= 7;
    }

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
	!check_feature(bp->f, "時間") && 
	!check_feature(bp->f, "指示詞") && 
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
    if (check_feature(bp->f, "体言") && 
	!check_feature(bp->f, "数量") && 
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
	if (cpm_ptr->elem_b_num[i] != -2 && 
	    cpm_ptr->elem_b_ptr[i]->num == n) {
	    return TRUE;
	    /* 格要素になっているかどうかに
	       対応付け状態は関係ない
	    if (cpm_ptr->cmm[0].result_lists_d[0].flag[i] >= 0) {
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
   int CheckEllipsisComponent(CF_PRED_MGR *cpm_ptr, BNST_DATA *bp)
/*==================================================================*/
{
    int i, num;

    /* 用言が候補と同じ表記を
       他の格の省略の指示対象としてもっているかどうか */

    for (i = 0; i < cpm_ptr->cmm[0].cf_ptr->element_num; i++) {
	num = cpm_ptr->cmm[0].result_lists_p[0].flag[i];
	/* 省略の指示対象 */
	if (num >= 0 && 
	    cpm_ptr->elem_b_num[num] == -2 && 
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
     int CheckObligatoryCase(CF_PRED_MGR *cpm_ptr, BNST_DATA *bp)
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

    if (cpm_ptr->cmm[0].score != -2) {
	for (i = 0; i < cpm_ptr->cmm[0].cf_ptr->element_num; i++) {
	    num = cpm_ptr->cmm[0].result_lists_p[0].flag[i];
	    /* これが調べる格要素 */
	    if (num != UNASSIGNED && 
		cpm_ptr->elem_b_num[num] != -2 && /* 省略の格要素じゃない */
		cpm_ptr->elem_b_ptr[num]->num == bp->num) {
		if (MatchPP(cpm_ptr->cmm[0].cf_ptr->pp[i][0], "ガ") || 
		    MatchPP(cpm_ptr->cmm[0].cf_ptr->pp[i][0], "ヲ") || 
		    MatchPP(cpm_ptr->cmm[0].cf_ptr->pp[i][0], "ニ") || 
		    MatchPP(cpm_ptr->cmm[0].cf_ptr->pp[i][0], "ガ２")) {
		    return 1;
		}
		return 0;
	    }
	}
    }
    return 0;
}

/*==================================================================*/
int CheckCaseCorrespond(CF_PRED_MGR *cpm_ptr, BNST_DATA *bp, CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{
    /* 格の一致を調べる
       bp: 対象文節
       cpm_ptr: 対象文節の係る用言 (bp->parent->cpm_ptr)
    */

    int i, num;

    if (cpm_ptr->cmm[0].score != -2) {
	for (i = 0; i < cpm_ptr->cmm[0].cf_ptr->element_num; i++) {
	    num = cpm_ptr->cmm[0].result_lists_p[0].flag[i];
	    /* これが調べる格要素 */
	    if (num != UNASSIGNED && cpm_ptr->elem_b_ptr[num]->num == bp->num) {
		if (cf_ptr->pp[n][0] == cpm_ptr->cmm[0].cf_ptr->pp[i][0] || 
		    (MatchPP(cf_ptr->pp[n][0], "ガ") && MatchPP(cpm_ptr->cmm[0].cf_ptr->pp[i][0], "ガ２"))) {
		    return 1;
		}
		return 0;
	    }
	}
    }
    return 0;
}

/*==================================================================*/
void EllipsisDetectForVerbSubcontract(SENTENCE_DATA *s, SENTENCE_DATA *cs, ELLIPSIS_MGR *em_ptr, 
				      CF_PRED_MGR *cpm_ptr, BNST_DATA *bp, CASE_FRAME *cf_ptr, int n, int type)
/*==================================================================*/
{
    float score, weight, ascore, pascore, pcscore, mcscore, rawscore, topicscore, distscore;
    char feature_buffer[DATA_LEN];
    int ac, pac, pcc, mcc, topicflag, distance, agentflag, firstsc, subtopicflag, sameflag;
    int exception = 0;

    /* 対象用言と候補が同じ自立語のとき
       判定詞の場合だけ許す */
    if (str_eq(cpm_ptr->pred_b_ptr->Jiritu_Go, bp->Jiritu_Go)) {
	if (!check_feature(cpm_ptr->pred_b_ptr->f, "用言:判")) {
	    return;
	}
	sameflag = 1;
    }
    else {
	sameflag = 0;
    }

    if (CheckEllipsisComponent(cpm_ptr, bp)) {
	return;
    }

    /* 同格の前側は候補にしない */
    if (bp->dpnd_type == 'A') {
	return;
    }

    /* 現在の文から対象となっている格要素の文までの距離 */
    distance = cs-s;

    if (distance > 8) {
	distscore = 0.2;
    }
    else {
	distscore = 1-(float)distance*0.1;
    }

    /* 格の一致をチェック */
    if (distance == 0 && bp->parent && bp->parent->cpm_ptr && 
	bp->num < cpm_ptr->pred_b_ptr->num && 
	CheckCaseCorrespond(bp->parent->cpm_ptr, bp, cf_ptr, n) == 0) {
	weight = 0.9;
    }
    else {
	weight = 1;
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
			 cf_ptr->pp[n][0], bp->Jiritu_Go);
    pascore = 1.0+0.5*pac; /* 0.2 */

    /* 前文の主節のスコア */
    pcc = CheckLastClause(cs->Sen_num-1, cf_ptr->pp[n][0], bp->Jiritu_Go);

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
	subtopicflag = 1;
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
    if (sm_match_check(sm2code("主体"), cf_ptr->sm[n])) {
	agentflag = 1;
    }
    else {
	agentflag = 0;
    }

    /* N は 〜 N だ。 */
    if (sameflag) {
	rawscore = 1;
    }
    /* V したのは N だ。(ガ格だけでなくヲ格もある) */
    else if (check_feature(cpm_ptr->pred_b_ptr->f, "ID:〜の〜") && 
	     check_feature(bp->f, "用言:判")) {
	rawscore = 1;
	exception = 1;
    }
    else {
	rawscore = CalcSimilarityForVerb(bp, cf_ptr, n);
    }

    /* 先頭文の主節をチェック */
    firstsc = CheckLastClause(1, cf_ptr->pp[n][0], bp->Jiritu_Go);

    /* 救うもの */
    if (((s->Sen_num == 1 && firstsc == 0) || 
	 (distance == 1 && pcc == 0) || 
	 (distance == 0 && mcc == 0)
	) && 
	((bp->num == s->Bnst_num-1 && check_feature(bp->f, "用言:判")) || /* 文末の判定詞 */
	 (bp->parent && bp->parent->parent && check_feature(bp->parent->parent->f, "主節") && subordinate_level_check("B+", bp->parent) && 
	  CheckObligatoryCase(bp->parent->cpm_ptr, bp)) || /* 強い従属節 */
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

    /* 先頭文で用言の直前になく、デモ, デハではないデ格は対象にしない 
       他の任意格もそうかもしれない */
    if (firstsc && s->Sen_num == 1 && 
	check_feature(bp->f, "係:デ格") && 
	bp->dpnd_head != bp->num+1 && 
	!check_feature(bp->f, "デハ") && 
	!check_feature(bp->f, "デモ")) {
	firstsc = 0;
    }

    if (distance == 0 || /* 対象文 */
	(distance == 1 && (topicflag || subtopicflag || pcc)) || /* 前文 */
	(s->Sen_num == 1 && (topicflag || subtopicflag || firstsc)) || /* 先頭文 */
	(pcc || mcc || firstsc)) { /* それ以外で主節の省略の指示対象 */

	/* 対象文には、厳しい制約が必要 
	   主節の格要素の場合でも、対象用言より前に出現している必要がある */
	if (distance == 0 && !exception && !((topicflag || mcc || type >= RANK2) && bp->num < cpm_ptr->pred_b_ptr->num)) {
	    if (check_feature(bp->f, "抽象") && !check_feature(bp->f, "人名") && !check_feature(bp->f, "組織名")) {
		weight *= 0.6;
		score = weight*pascore*rawscore;
	    }
	    else {
		weight *= 0.6;
		score = weight*pascore*rawscore;
	    }
	}
	else {
	    if (subtopicflag == 1) {
		weight *= 0.6;
		score = weight*pascore*rawscore;
	    }
	    /* 判定詞の場合で RANK3 のもので類似度がない場合は救う
	       「結婚も 〜 上回り、〜 十九万五千組。」 
	       条件: rawscore == 0 はずした */
	    else if (type == RANK3 && check_feature(cpm_ptr->pred_b_ptr->f, "用言:判")) {
		score = weight*pascore*1.0;
	    }
	    else {
		score = weight*pascore*rawscore;
	    }
	}

    }
    /* 特別に N-V のセットのときは許す */
    else if (pascore > 1 && rawscore > 0.8) {
	score = weight*pascore*rawscore;
    }
    else {
	score = 0;
    }

    /* 距離を加味
    score *= distscore; */

    if (score > maxscore) {
	maxscore = score;
	maxs = s;
	if (bp->num < 0) {
	    maxi = bp->parent->num;
	}
	else {
	    maxi = bp->num;
	}
    }

    /* 省略候補 */
    if (score > 0) {
	sprintf(feature_buffer, "C用;%s;%s;%d;%d;%.3f|%.3f", bp->Jiritu_Go, 
		pp_code_to_kstr(cf_ptr->pp[n][0]), 
		distance, maxi, 
		score, rawscore);
	assign_cfeature(&(em_ptr->f), feature_buffer);
	sprintf(feature_buffer, "学習FEATURE;%s;%s;%.3f|%d|%d|%d|%d|%d|%d|%d|%d", 
		bp->Jiritu_Go, 
		pp_code_to_kstr(cf_ptr->pp[n][0]), 
		rawscore, type, ac, pac, pcc, mcc, topicflag, agentflag, distance);
	assign_cfeature(&(em_ptr->f), feature_buffer);
    }

    Bcheck[bp->num] = 1;
}

/*==================================================================*/
void SearchCaseComponent(SENTENCE_DATA *s, ELLIPSIS_MGR *em_ptr, CF_PRED_MGR *cpm_ptr, 
			 BNST_DATA *bp, CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{
    /* cpm_ptr: 省略格要素をもつ用言
       bp:      格要素の探索対象となっている用言文節
    */

    int i, num;

    /* 他の用言 (親用言など) の格要素をチェック */
    if (bp->cpm_ptr && bp->cpm_ptr->cmm[0].score != -2) {
	for (i = 0; i < bp->cpm_ptr->cmm[0].cf_ptr->element_num; i++) {
	    num = bp->cpm_ptr->cmm[0].result_lists_p[0].flag[i];
	    if (num != UNASSIGNED && 
		bp->cpm_ptr->elem_b_num[num] != -2 && /* 格要素が省略を補ったものであるときはだめ */
		bp->cpm_ptr->elem_b_ptr[num]->num != cpm_ptr->pred_b_ptr->num && /* 格要素が元用言のときはだめ (bp->cpm_ptr->elem_b_ptr[num] は並列のとき <PARA> となり、pointer はマッチしない) */
		CheckTargetNoun(bp->cpm_ptr->elem_b_ptr[num])) {
		/* 格要素の格の一致 (格によりけり) */
		if (cf_ptr->pp[n][0] == bp->cpm_ptr->cmm[0].cf_ptr->pp[i][0] || 
		    (MatchPP(cf_ptr->pp[n][0], "ガ") && MatchPP(bp->cpm_ptr->cmm[0].cf_ptr->pp[i][0], "ガ２"))) {
		    EllipsisDetectForVerbSubcontract(s, s, em_ptr, cpm_ptr, bp->cpm_ptr->elem_b_ptr[num], 
						     cf_ptr, n, RANK3);
		}
		/* 格は不一致 */
		else {
		    EllipsisDetectForVerbSubcontract(s, s, em_ptr, cpm_ptr, bp->cpm_ptr->elem_b_ptr[num], 
						     cf_ptr, n, RANK2);
		}
	    }
	}
    }
}

/*==================================================================*/
	int AppendToCF(CF_PRED_MGR *cpm_ptr, BNST_DATA *b_ptr,
		       CASE_FRAME *cf_ptr, int n, float maxscore)
/*==================================================================*/
{
    /* 省略の指示対象を入力側の格フレームに入れる */

    CASE_FRAME *c_ptr = &(cpm_ptr->cf);

    if (c_ptr->element_num >= CF_ELEMENT_MAX) {
	return 0;
    }

    /* 対応情報を追加 */
    cpm_ptr->cmm[0].result_lists_p[0].flag[n] = c_ptr->element_num;
    cpm_ptr->cmm[0].result_lists_d[0].flag[c_ptr->element_num] = n;
    /* cpm_ptr->cmm[0].result_lists_p[0].score[n] = -1; */
    if (maxscore > 1) {
	cpm_ptr->cmm[0].result_lists_p[0].score[n] = 11;
    }
    else {
	cpm_ptr->cmm[0].result_lists_p[0].score[n] = 11*maxscore;
    }

    c_ptr->pp[c_ptr->element_num][0] = cf_ptr->pp[n][0];
    c_ptr->pp[c_ptr->element_num][1] = END_M;
    c_ptr->oblig[c_ptr->element_num] = TRUE;
    _make_data_cframe_sm(cpm_ptr, b_ptr);
    _make_data_cframe_ex(cpm_ptr, b_ptr);
    cpm_ptr->elem_b_ptr[c_ptr->element_num] = b_ptr;
    cpm_ptr->elem_b_num[c_ptr->element_num] = -2;
    c_ptr->weight[c_ptr->element_num] = 0;
    c_ptr->adjacent[c_ptr->element_num] = FALSE;
    c_ptr->element_num++;
    return 1;
}

/*==================================================================*/
     int DeleteFromCF(ELLIPSIS_MGR *em_ptr, CF_PRED_MGR *cpm_ptr)
/*==================================================================*/
{
    int i, count = 0;

    /* 省略の指示対象を入力側の格フレームから削除する */

    for (i = 0; i < cpm_ptr->cf.element_num; i++) {
	if (cpm_ptr->elem_b_num[i] == -2) {
	    cpm_ptr->cmm[0].result_lists_p[0].flag[cpm_ptr->cmm[0].result_lists_d[0].flag[i]] = -1;
	    cpm_ptr->cmm[0].result_lists_d[0].flag[i] = -1;
	    count++;
	}
    }
    cpm_ptr->cf.element_num -= count;
    return 1;
}

/*==================================================================*/
int EllipsisDetectForVerb(SENTENCE_DATA *sp, ELLIPSIS_MGR *em_ptr, 
			  CF_PRED_MGR *cpm_ptr, CASE_FRAME *cf_ptr, int n, int last)
/*==================================================================*/
{
    /* 用言とその省略格が与えられる */

    /* cf_ptr = cpm_ptr->cmm[0].cf_ptr である */
    /* 用言 cpm_ptr の cf_ptr->pp[n][0] 格が省略されている
       cf_ptr->ex[n] に似ている文節を探す */

    int i, current = 1, bend;
    char feature_buffer[DATA_LEN], etc_buffer[DATA_LEN], *cp;
    SENTENCE_DATA *s, *cs;

    maxscore = 0;
    cs = sentence_data + sp->Sen_num - 1;
    memset(Bcheck, 0, BNST_MAX);

    /* 親をみる (PARA なら child 用言) */
    if (cpm_ptr->pred_b_ptr->parent) {
	/* 親が PARA */
	if (cpm_ptr->pred_b_ptr->parent->para_top_p) {
	    /* 自分と並列の用言 */
	    for (i = 0; cpm_ptr->pred_b_ptr->parent->child[i]; i++) {
		/* PARA の子供で、自分を以外の並列用言 */
		if (cpm_ptr->pred_b_ptr->parent->child[i] != cpm_ptr->pred_b_ptr &&
		    cpm_ptr->pred_b_ptr->parent->child[i]->para_type == PARA_NORMAL) {
		    SearchCaseComponent(cs, em_ptr, cpm_ptr, cpm_ptr->pred_b_ptr->parent->child[i], 
					cf_ptr, n);
		}
	    }

	    /* 連用で係る親用言 (並列のとき) */
	    if (cpm_ptr->pred_b_ptr->parent->parent && 
		check_feature(cpm_ptr->pred_b_ptr->f, "係:連用")) {
		SearchCaseComponent(cs, em_ptr, cpm_ptr, cpm_ptr->pred_b_ptr->parent->parent, 
				    cf_ptr, n);
	    }
	}
	/* とりあえず、連用で係るひとつ上の親用言のみ */
	else if (check_feature(cpm_ptr->pred_b_ptr->f, "係:連用")) {
	    SearchCaseComponent(cs, em_ptr, cpm_ptr, cpm_ptr->pred_b_ptr->parent, 
				cf_ptr, n);
	}
	/* 「〜した後」など */
	else if (check_feature(cpm_ptr->pred_b_ptr->f, "従属節扱い")) {
	    SearchCaseComponent(cs, em_ptr, cpm_ptr, cpm_ptr->pred_b_ptr->parent->parent, 
				cf_ptr, n);
	}
    }

    /* 子供 (用言) を見る */
    for (i = 0; cpm_ptr->pred_b_ptr->child[i]; i++) {
	if (check_feature(cpm_ptr->pred_b_ptr->child[i]->f, "用言")) {
	    SearchCaseComponent(cs, em_ptr, cpm_ptr, cpm_ptr->pred_b_ptr->child[i], 
				cf_ptr, n);
	}
    }

    if (cp = check_feature(cpm_ptr->pred_b_ptr->f, "照応ヒント")) {
	if (str_eq(cp, "照応ヒント:係")) {
	    /* 係り先の用言に係る格要素をみる (サ変名詞のとき) */
	    SearchCaseComponent(cs, em_ptr, cpm_ptr, cs->bnst_data+cpm_ptr->pred_b_ptr->dpnd_head, cf_ptr, n);

	    /* 係り先の用言に係る従属節に係る格要素をみる */
	    for (i = 0; (cs->bnst_data+cpm_ptr->pred_b_ptr->dpnd_head)->child[i]; i++) {
		if (check_feature((cs->bnst_data+cpm_ptr->pred_b_ptr->dpnd_head)->child[i]->f, "用言")) {
		    SearchCaseComponent(cs, em_ptr, cpm_ptr, (cs->bnst_data+cpm_ptr->pred_b_ptr->dpnd_head)->child[i], 
					cf_ptr, n);
		}
	    }
	}
	else {
	    i = cpm_ptr->pred_b_ptr->num+atoi(cp+11);
	    if (i >= 0 && i < cs->Bnst_num) {
		SearchCaseComponent(cs, em_ptr, cpm_ptr, cs->bnst_data+i, cf_ptr, n);
	    }
	}
    }

    /* 前の文の体言を探す (この用言の格要素になっているもの以外) */
    for (s = cs; s >= sentence_data; s--) {
	bend = s->Bnst_num;

	for (i = bend-1; i >= 0; i--) {
	    if (current) {
		if (Bcheck[i] || 
		    (!check_feature((s->bnst_data+i)->f, "係:連用") && 
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

		/* A の B を 〜 V */
		if (cp = check_feature((s->bnst_data+i)->f, "省略候補チェック")) {
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

	    EllipsisDetectForVerbSubcontract(s, cs, em_ptr, cpm_ptr, s->bnst_data+i, cf_ptr, n, RANK1);
	}
	if (current)
	    current = 0;
    }

    /* 【主体一般】
       1. 用言が受身でニ格 (もとはガ格) に <主体> をとるとき
       2. 「〜ため(に)」でガ格に <主体> をとるとき 
       3. 〜が V した N (外の関係, !判定詞), 形副名詞, 相対名詞は除く
       4. スコアが閾値より下で <主体> をとるとき */
    if ((maxscore <= AssignReferentThreshold && 
	 cf_match_element(cf_ptr->sm[n], "主体", FALSE)) || 
	(check_feature(cpm_ptr->pred_b_ptr->f, "ID:〜（ため）") && 
	 MatchPP(cf_ptr->pp[n][0], "ガ") && 
	 cf_match_element(cf_ptr->sm[n], "主体", TRUE)) || 
	(MatchPP(cf_ptr->pp[n][0], "ニ") && 
	cf_match_element(cf_ptr->sm[n], "主体", FALSE) && 
	(check_feature(cpm_ptr->pred_b_ptr->f, "〜れる") || 
	 check_feature(cpm_ptr->pred_b_ptr->f, "〜られる") || 
	 check_feature(cpm_ptr->pred_b_ptr->f, "サ変名詞格解析"))) || 
	(cpm_ptr->pred_b_ptr->parent && 
	 (check_feature(cpm_ptr->pred_b_ptr->parent->f, "外の関係") || 
	  check_feature(cpm_ptr->pred_b_ptr->parent->f, "外の関係可能性") || 
	  check_feature(cpm_ptr->pred_b_ptr->parent->f, "外の関係判定")) && 
	 !check_feature(cpm_ptr->pred_b_ptr->parent->f, "相対名詞") && 
	 !check_feature(cpm_ptr->pred_b_ptr->parent->f, "形副名詞") && 
	 !check_feature(cpm_ptr->pred_b_ptr->parent->f, "用言") && 
	 check_feature(cpm_ptr->pred_b_ptr->f, "係:連格") && 
	 cf_ptr->pp[n][0] == pp_kstr_to_code("ガ"))) {
	/* 格フレームを決めるループのときに feature を与えるのは問題 */
	sprintf(feature_buffer, "C用;【主体一般】;%s;-1;-1;1", 
		pp_code_to_kstr(cf_ptr->pp[n][0]));
	assign_cfeature(&(em_ptr->f), feature_buffer);
    }
    /* 次の場合は省略要素を探すが記録しない 
       (現時点ではデータを見るため、これらも省略解析を行っている) 
       デ格, ト格, ヨリ格, カラ格, マデ格 */
    else if (MatchPP(cf_ptr->pp[n][0],"デ") || 
	     MatchPP(cf_ptr->pp[n][0], "ト") || 
	     MatchPP(cf_ptr->pp[n][0], "ヨリ") || 
	     MatchPP(cf_ptr->pp[n][0], "カラ") || 
	     MatchPP(cf_ptr->pp[n][0], "マデ")) {
	sprintf(feature_buffer, "省略処理なし-%s", 
		pp_code_to_kstr(cf_ptr->pp[n][0]));
	assign_cfeature(&(em_ptr->f), feature_buffer);
    }
    else if (maxscore > 0 && 
	     maxscore < 0.9 && 
	     MatchPP(cf_ptr->pp[n][0],"ヲ") && 
	     sm_match_check(sm2code("抽象物"), cpm_ptr->pred_b_ptr->SM_code)) {
	sprintf(feature_buffer, "C用;【不特定物】;%s;-1;-1;1", 
		pp_code_to_kstr(cf_ptr->pp[n][0]));
	assign_cfeature(&(em_ptr->f), feature_buffer);
	/* 最大スコアの指示対象を dummy で格フレームに保存 
	   それが、ほかの格の候補にならなくなるのは問題 */
	AppendToCF(cpm_ptr, maxs->bnst_data+maxi, cf_ptr, n, maxscore);
	return 1;
    }
    else if (maxscore > AssignReferentThreshold) {
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
	AppendToCF(cpm_ptr, maxs->bnst_data+maxi, cf_ptr, n, maxscore);

	return 1;
    }
    return 0;
}

/*==================================================================*/
     void EllipsisDetectForNoun(SENTENCE_DATA *cs, BNST_DATA *bp)
/*==================================================================*/
{
    char **def;
    int i, j, ssize = 5, scount, current, bend;
    SENTENCE_DATA *sbuf, *sp, *s;
    float score;
    char feature_buffer[DATA_LEN], *buffer;

    /* 名詞の定義文を取得 */
    def = GetDefinitionFromBunsetsu(bp);
    if (!def) {
	return;
    }

    sbuf = (SENTENCE_DATA *)malloc_data(sizeof(SENTENCE_DATA)*ssize, 
					"EllipsisDetectForNoun");

    for (scount = 0; *(def+scount); scount++) {
#ifdef DEBUGMORE
	fprintf(Outfp, "定義文[%s] %d: %s\n", bp->Jiritu_Go, scount, *(def+scount));
#endif
	buffer = (char *)malloc_data(strlen(*(def+scount))+100, "EllipsisDetectForNoun");
	sprintf(buffer, "C定義文;%d:%s", scount, *(def+scount));
	assign_cfeature(&(bp->f), buffer);
	free(buffer);

	if (scount >= ssize) {
	    sbuf = (SENTENCE_DATA *)realloc_data(sbuf, sizeof(SENTENCE_DATA)*(ssize <<= 1), 
						 "EllipsisDetectForNoun");
	}
	sp = sbuf+scount;

	/* 定義文を解析 */
	InitSentence(sp);
	ParseSentence(sp, *(def+scount));

	/* 定義文に含まれる名詞に対して、元の文 (文章) の名詞で似ているものを探す */
	for (i = sp->Bnst_num-1; i >= 0; i--) {
	    if (CheckPureNoun(sp->bnst_data+i)) {
		/* sp->bnst_data+i: 定義文中の体言 */
		maxscore = 0;
		current = 1;
#ifdef DEBUGMORE
		fprintf(Outfp, "定義文 -- %s\n", (sp->bnst_data+i)->Jiritu_Go);
#endif
		/* 元の文 (文章) */
		for (s = cs; s >= sentence_data; s--) {
		    bend = s->Bnst_num;

		    for (j = bend-1; j >= 0; j--) {
			if (!CheckPureNoun(s->bnst_data+j))
			    continue;
			/* s->bnst_data+j: 元の文章中の体言 */
			score = CalcSimilarityForNoun(s->bnst_data+j, sp->bnst_data+i);
			if (score > maxscore) {
			    maxscore = score;
			    maxs = s;
			    maxi = j;
			}
			if (score > 0) {
			    /* 省略候補 */
			    sprintf(feature_buffer, "C体;%s;%s:%.3f", (s->bnst_data+j)->Jiritu_Go, 
				    (sp->bnst_data+i)->Jiritu_Go, 
				    score);
			    assign_cfeature(&(bp->f), feature_buffer);
#ifdef DEBUGMORE
			    fprintf(Outfp, "\t%.3f %s\n", score, (s->bnst_data+j)->Jiritu_Go);
#endif
			}
		    }
		    if (current)
			current = 0;
		}
		if (maxscore > 0) {
		    /* 決定した省略関係 */
		    sprintf(feature_buffer, "C体;【%s】;%s:%.3f", (maxs->bnst_data+maxi)->Jiritu_Go, 
			    (sp->bnst_data+i)->Jiritu_Go, 
			    maxscore);
		    assign_cfeature(&(bp->f), feature_buffer);
#ifdef DEBUGMORE
		    fprintf(Outfp, "\t◎ %s\n", (maxs->bnst_data+maxi)->Jiritu_Go);
#endif
		}
#ifdef DEBUGMORE
		fputc('\n', Outfp);
#endif
	    }
	}
	clear_cf();
    }

    /* ここで文データを scount 個 free */
    for (i = 0; i < scount; i++)
	ClearSentence(sbuf+i);
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

    /* 省略されている格をマーク */
    sprintf(feature_buffer, "C省略-%s", 
	    pp_code_to_kstr(cf_ptr->pp[n][0]));
    assign_cfeature(&(cpm_ptr->pred_b_ptr->f), feature_buffer);

    /* <時期・時間・状況> をガ格としてとる判定詞 */
    if (check_feature(cpm_ptr->pred_b_ptr->f, "時間ガ省略") && 
	MatchPP(cf_ptr->pp[n][0], "ガ")) {
	sprintf(feature_buffer, "C用;【時期・時間・状況】;%s;-1;-1;1", 
		pp_code_to_kstr(cf_ptr->pp[n][0]));
	assign_cfeature(&(cpm_ptr->pred_b_ptr->f), feature_buffer);
	return 0;
    }
    return 1;
}

/*==================================================================*/
float EllipsisDetectForVerbMain(SENTENCE_DATA *sp, ELLIPSIS_MGR *em_ptr, CF_PRED_MGR *cpm_ptr, 
				CASE_FRAME *cf_ptr, char **order, int mainflag, int toflag, int onceflag)
/*==================================================================*/
{
    int i, j, num, result;
    CF_MATCH_MGR *cmm_ptr;

    cmm_ptr = &(cpm_ptr->cmm[0]);

    /* 格を与えられた順番に */
    for (j = 0; *order[j]; j++) {
	for (i = 0; i < cf_ptr->element_num; i++) {
	    if (MatchPP(cf_ptr->pp[i][0], order[j]) && 
		cmm_ptr->result_lists_p[0].flag[i] == UNASSIGNED && 
		!(toflag && MatchPP(cf_ptr->pp[i][0], "ヲ"))) {
		if ((MarkEllipsisCase(cpm_ptr, cf_ptr, i)) == 0) {
		    continue;
		}
		result = EllipsisDetectForVerb(sp, em_ptr, cpm_ptr, cf_ptr, i, mainflag);
		if (result) {
		    em_ptr->cc[cf_ptr->pp[i][0]].score = maxscore;
		    em_ptr->score += maxscore;
		    if (onceflag) {
			/* ひとつの省略の指示対象をみつけたので、
			   ここでもっともスコアの高い格フレームを再調査する */
			find_best_cf(sp, cpm_ptr);
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
	    !MatchPP(cf_ptr->pp[i][0], "修飾")) {
	    if ((MarkEllipsisCase(cpm_ptr, cf_ptr, i)) == 0) {
		continue;
	    }
	    result = EllipsisDetectForVerb(sp, em_ptr, cpm_ptr, cf_ptr, i, mainflag);
	    if (result) {
		em_ptr->cc[cf_ptr->pp[i][0]].score = maxscore;
		em_ptr->score += maxscore;
		if (onceflag) {
		    find_best_cf(sp, cpm_ptr);
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
      int CompareDistance(ELLIPSIS_MGR *max, ELLIPSIS_MGR *work)
/*==================================================================*/
{
/*    int i, d = 0;

    for (i = 0; i < CASE_MAX_NUM; i++) {
	if (max->cc[i].s && max->cc[i].s == work->cc[i].s) {
	    max->cc[i].bnst
	}
    }
*/
}

/*==================================================================*/
	      void DiscourseAnalysis(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, k, num, toflag, lastflag = 1, mainflag, anum;
    float score;
    ELLIPSIS_MGR workem, maxem;
    SENTENCE_DATA *sp_new;
    CF_PRED_MGR *cpm_ptr;
    CF_MATCH_MGR *cmm_ptr;
    CASE_FRAME *cf_ptr, origcf;

    InitEllipsisMGR(&workem);
    InitEllipsisMGR(&maxem);

    sp_new = PreserveSentence(sp);

    /* 各用言をチェック (文末から) */
    for (j = 0; j < sp->Best_mgr->pred_num; j++) {
	cpm_ptr = &(sp->Best_mgr->cpm[j]);

	/* 格フレームがない場合 (ガ格ぐらい探してもいいかもしれない) */
	if (cpm_ptr->result_num == 0 || 
	    cpm_ptr->cmm[0].cf_ptr->ipal_address == -1 || 
	    cpm_ptr->cmm[0].score == -2) {
	    continue;
	}

	/* 省略解析しない用言
	   1. ルールで「省略処理なし」feature がついているもの
	   2. 準用言 (「サ変名詞格解析」は除く)
	   3. 格解析無視 (「〜みられる」 など)
	   4. カタカナのサ変名詞
	   5. レベル:A-
	   6. （〜を）〜に
	   7. 〜て（用言） (A), ※ 〜く（用言） は A- */
	if (check_feature(cpm_ptr->pred_b_ptr->f, "省略処理なし") || 
	    (!check_feature(cpm_ptr->pred_b_ptr->f, "サ変名詞格解析") && 
	     check_feature(cpm_ptr->pred_b_ptr->f, "準用言")) || 
	    check_feature(cpm_ptr->pred_b_ptr->f, "格解析無視") || 
	    (check_feature(cpm_ptr->pred_b_ptr->f, "サ変名詞格解析") && 
	     check_feature(L_Jiritu_M(cpm_ptr->pred_b_ptr)->f, "カタカナ")) || 
	    check_feature(cpm_ptr->pred_b_ptr->f, "レベル:A-") || 
	    check_feature(cpm_ptr->pred_b_ptr->f, "ID:（〜を）〜に") || 
	    check_feature(cpm_ptr->pred_b_ptr->f, "ID:〜て（用言）")) {
	    continue;
	}
	    

	cmm_ptr = &(cpm_ptr->cmm[0]);
	cf_ptr = cmm_ptr->cf_ptr;

	/* 「<補文>と 〜を V した」 
	   ト格があるとき、ヲ格を省略としない 
	   (格フレーム側の<補文>はチェックしていない) */
	toflag = 0;
	for (i = 0; i < cf_ptr->element_num; i++) {
	    num = cmm_ptr->result_lists_p[0].flag[i];
	    if (num != UNASSIGNED && 
		str_eq(pp_code_to_kstr(cmm_ptr->cf_ptr->pp[i][0]), "ト") && 
		check_feature(cpm_ptr->elem_b_ptr[num]->f, "補文")) {
		toflag = 1;
		break;
	    }
	}

	/* その文の主節 */
	if (lastflag == 1 && !check_feature(cpm_ptr->pred_b_ptr->f, "非主節")) {
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

	maxem.score = -1;
	origcf = *(cpm_ptr->cmm[0].cf_ptr);

	for (i = 0; i < CASE_ORDER_MAX; i++) {
	    ClearEllipsisMGR(&workem);
	    /* 格要素がひとつもないときは、省略の指示対象をひとつ決めてから
	       もっともスコアの高い格フレームを選ぶ */
	    if (anum == 0) {
		if ((score = EllipsisDetectForVerbMain(sp, &workem, cpm_ptr, &origcf, 
						       CaseOrder[i], mainflag, toflag, 1)) > -1) {
		    score += EllipsisDetectForVerbMain(sp, &workem, cpm_ptr, cpm_ptr->cmm[0].cf_ptr, 
						       CaseOrder[i], mainflag, toflag, 0);
		}
	    }
	    else {
		score = EllipsisDetectForVerbMain(sp, &workem, cpm_ptr, cpm_ptr->cmm[0].cf_ptr, 
						  CaseOrder[i], mainflag, toflag, 0);
	    }

	    if (workem.score > maxem.score /* || 
		(workem.score == maxem.score && CompareDistance(maxem, workem)) */) {
		maxem = workem;
		maxem.result_num = cpm_ptr->result_num;
		for (k = 0; k < maxem.result_num; k++) {
		    maxem.cmm[k] = cpm_ptr->cmm[k];
		}
		maxem.element_num = cpm_ptr->cf.element_num;
		for (k = 0; k < maxem.element_num; k++) {
		    maxem.elem_b_ptr[k] = cpm_ptr->elem_b_ptr[k];
		    maxem.elem_b_num[k] = cpm_ptr->elem_b_num[k];
		}
		workem.f = NULL;
	    }

	    if (VerboseLevel >= VERBOSE2) {
		fprintf(stdout, "CASE_ORDER %2d ==> %.3f\n", i, workem.score);
		print_data_cframe(cpm_ptr);
		for (k = 0; k < cpm_ptr->result_num; k++) {
		    print_crrspnd(cpm_ptr, &(cpm_ptr->cmm[k]));
		}
		fputc('\n', Outfp);
	    }

	    /* 格フレームの追加エントリの削除 */
	    DeleteFromCF(&workem, cpm_ptr);

	    ClearAnaphoraList(banlist);
	}

	/* もっとも score のよかった組み合わせを登録 */
	if (maxem.score > -1) {
	    /* cmm を復元 */
	    cpm_ptr->result_num = maxem.result_num;
	    for (k = 0; k < maxem.result_num; k++) {
		cpm_ptr->cmm[k] = maxem.cmm[k];
	    }
	    cpm_ptr->cf.element_num = maxem.element_num;
	    for (k = 0; k < maxem.element_num; k++) {
		cpm_ptr->elem_b_ptr[k] = maxem.elem_b_ptr[k];
		cpm_ptr->elem_b_num[k] = maxem.elem_b_num[k];
	    }
	    /* feature の伝搬 */
	    append_feature(&(cpm_ptr->pred_b_ptr->f), maxem.f);
	    maxem.f = NULL;

	    /* 格フレームに省略情報を復元
	    for (i = 0; i < CASE_MAX_NUM; i++) {
		if (maxem.cc[i].s) {
		    AppendToCF(cpm_ptr, maxem.cc[i].s->bnst_data+maxem.cc[i].bnst, cpm_ptr->cmm[0].cf_ptr, 
			       GetElementID(cpm_ptr->cmm[0].cf_ptr, i), maxem.cc[i].score);
		}
	    } */

	    /* サ変名詞のガ格はまだ怪しいので記録しない -> サ変名詞すべて記録しない
	       MatchPP(cf_ptr->pp[i][0], "ガ") */
	    if (!check_feature(cpm_ptr->pred_b_ptr->f, "サ変名詞格解析")) {
		for (i = 0; i < CASE_MAX_NUM; i++) {
		    if (maxem.cc[i].s) {
			/* リンクされたことを記録する */
			RegisterAnaphor(alist, (maxem.cc[i].s->bnst_data+maxem.cc[i].bnst)->Jiritu_Go);
			/* 用言と格要素のセットを記録 (格関係とは区別したい) */
			RegisterPredicate(L_Jiritu_M(cpm_ptr->pred_b_ptr)->Goi, 
					  cpm_ptr->pred_b_ptr->voice, 
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

    /* 各体言をチェック
    for (i = sp->Bnst_num-1; i >= 0; i--) {
	if (CheckPureNoun(sp->bnst_data+i)) {
	    EllipsisDetectForNoun(sentence_data+sp->Sen_num-1, sp->bnst_data+i);
	}
    }
    */

    PreserveCPM(sp_new, sp);
    clear_cf();
}

/*====================================================================
                               END
====================================================================*/
