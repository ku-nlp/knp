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

#define RANK1	1
#define RANK2	2
#define RANK3	3

ALIST alist[TBLSIZE];
PALIST palist[TBLSIZE];

int hash(unsigned char *key, int keylen);
extern int	EX_match_subject;

/*==================================================================*/
		       void InitAnaphoraList()
/*==================================================================*/
{
    memset(alist, 0, sizeof(ALIST)*TBLSIZE);
    memset(palist, 0, sizeof(PALIST)*TBLSIZE);
}

/*==================================================================*/
		       void ClearAnaphoraList()
/*==================================================================*/
{
    int i;
    ALIST *p, *next;
    for (i = 0; i < TBLSIZE; i++) {
	if (alist[i].key) {
	    free(alist[i].key);
	    alist[i].key = NULL;
	}
	if (alist[i].next) {
	    p = alist[i].next;
	    alist[i].next = NULL;
	    while (p) {
		free(p->key);
		next = p->next;
		free(p);
		p = next;
	    }
	}
	alist[i].count = 0;
    }
}

/*==================================================================*/
		   void RegisterAnaphor(char *key)
/*==================================================================*/
{
    ALIST *ap;

    ap = &(alist[hash(key, strlen(key))]);
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
		     int CheckAnaphor(char *key)
/*==================================================================*/
{
    ALIST *ap;

    ap = &(alist[hash(key, strlen(key))]);
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
      void StoreCaseComponent(CASE_COMPONENT **ccpp, char *word)
/*==================================================================*/
{
    while (*ccpp) {
	if (!strcmp((*ccpp)->word, word)) {
	    (*ccpp)->count++;
	    return;
	}
	ccpp = &((*ccpp)->next);
    }
    *ccpp = (CASE_COMPONENT *)malloc_data(sizeof(CASE_COMPONENT), "StoreCaseComponent");
    (*ccpp)->word = strdup(word);
    (*ccpp)->count = 1;
    (*ccpp)->next = NULL;
}

/*==================================================================*/
	void RegisterPredicate(char *key, int pp, char *word)
/*==================================================================*/
{
    PALIST *pap;

    /* 複合辞などの格は除く */
    if (pp == END_M || pp > 8) {
	return;
    }

    pap = &(palist[hash(key, strlen(key))]);
    if (pap->key) {
	PALIST **papp;
	papp = &pap;
	do {
	    if (!strcmp((*papp)->key, key)) {
		StoreCaseComponent(&((*papp)->cc[pp]), word);
		return;
	    }
	    papp = &((*papp)->next);
	} while (*papp);
	*papp = (PALIST *)malloc_data(sizeof(PALIST), "RegisterPredicate");
	(*papp)->key = strdup(key);
	memset((*papp)->cc, 0, sizeof(CASE_COMPONENT *)*CASE_MAX_NUM);
	StoreCaseComponent(&((*papp)->cc[pp]), word);
	(*papp)->next = NULL;
    }
    else {
	pap->key = strdup(key);
	StoreCaseComponent(&(pap->cc[pp]), word);
    }
}

/*==================================================================*/
	  int CheckPredicate(char *key, int pp, char *word)
/*==================================================================*/
{
    PALIST *pap;
    CASE_COMPONENT *ccp;

    /* 複合辞などの格は除く */
    if (pp == END_M || pp > 8) {
	return 0;
    }

    pap = &(palist[hash(key, strlen(key))]);
    if (!pap->key) {
	return 0;
    }
    while (pap) {
	if (!strcmp(pap->key, key)) {
	    ccp = pap->cc[pp];
	    while (ccp) {
		if (!strcmp(ccp->word, word)) {
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
    ClearAnaphoraList();
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
		void copy_sentence(SENTENCE_DATA *sp)
/*==================================================================*/
{
    /* 文解析結果の保持 */

    int i, j, num, cfnum = 0;
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



	sp_new->bnst_data[i].mrph_ptr += sp_new->mrph_data - sp->mrph_data;
	sp_new->bnst_data[i].settou_ptr += sp_new->mrph_data - sp->mrph_data;
	sp_new->bnst_data[i].jiritu_ptr += sp_new->mrph_data - sp->mrph_data;
	sp_new->bnst_data[i].fuzoku_ptr += sp_new->mrph_data - sp->mrph_data;
	sp_new->bnst_data[i].parent += sp_new->bnst_data - sp->bnst_data;
	for (j = 0; sp_new->bnst_data[i].child[j]; j++) {
	    sp_new->bnst_data[i].child[j]+= sp_new->bnst_data - sp->bnst_data;
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
	    (sp_new->cpm+i)->elem_b_ptr[j] = sp_new->bnst_data+(sp_new->cpm+i)->elem_b_num[j];
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

    /* 主体のマッチング */
    if (cf_ptr->sm[n]) {
	for (j = 0; cf_ptr->sm[n][j]; j+=step) {
	    if (strncmp(cf_ptr->sm[n]+j, sm2code("主体"), SM_CODE_SIZE))
		continue;
	    for (i = 0; cand->SM_code[i]; i+=step) {
		if (_sm_match_score(cf_ptr->sm[n]+j, cand->SM_code+i, SM_NO_EXPAND_NE)) {
		    score = (float)EX_match_subject/11;
		}
	    }
	}
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
    if (!check_feature(bp->f, "修飾")) {
	return TRUE;
    }
    return FALSE;
}

/*==================================================================*/
		   int CheckPureNoun(BNST_DATA *bp)
/*==================================================================*/
{
    if (check_feature(bp->f, "体言") && 
	!check_feature(bp->f, "形副名詞") && 
	!check_feature(bp->f, "時間") && 
	!check_feature(bp->f, "数量") && 
	CheckTargetNoun(bp)) {
	return TRUE;
    }
    return FALSE;
}

/*==================================================================*/
void EllipsisDetectForVerbSubcontract(SENTENCE_DATA *s, SENTENCE_DATA *cs, CF_PRED_MGR *cpm_ptr, 
				      BNST_DATA *bp, CASE_FRAME *cf_ptr, int n, int type)
/*==================================================================*/
{
    float score, weight, ascore, pascore, rawscore, topicscore;
    char feature_buffer[DATA_LEN];
    int ac, pac, topicflag;

    /* リンクする場所によるスコア */
    weight = 1.0+0.2*(type-1);

    /* リンクされた回数によるスコア */
    ac = CheckAnaphor(bp->Jiritu_Go);
    ascore = 1.0+0.1*ac;

    /* すでに出現した用言とその格要素のスコア */
    pac = CheckPredicate(L_Jiritu_M(cpm_ptr->pred_b_ptr)->Goi, cf_ptr->pp[n][0], bp->Jiritu_Go);
    pascore = 1.0+0.2*pac;

    /* 提題のスコア */
    if (check_feature(bp->f, "提題")) {
	topicflag = 1;
	topicscore = 1.2;
    }
    else {
	topicflag = 0;
	topicscore = 1.0;
    }

    rawscore = CalcSimilarityForVerb(bp, cf_ptr, n);
    score = ascore*pascore*weight*rawscore*topicscore;

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
    sprintf(feature_buffer, "C用;%s;%s:%.3f|%.3f", bp->Jiritu_Go, 
	    pp_code_to_kstr(cf_ptr->pp[n][0]), 
	    score, rawscore);
    assign_cfeature(&(cpm_ptr->pred_b_ptr->f), feature_buffer);
    sprintf(feature_buffer, "学習FEATURE;%s;%s:%.3f|%d|%d|%d|%d|%d", 
	    bp->Jiritu_Go, 
	    pp_code_to_kstr(cf_ptr->pp[n][0]), 
	    rawscore, type, ac, pac, topicflag, cs-s);
    assign_cfeature(&(cpm_ptr->pred_b_ptr->f), feature_buffer);

    Bcheck[bp->num] = 1;
}

/*==================================================================*/
void SearchCaseComponent(SENTENCE_DATA *s, CF_PRED_MGR *cpm_ptr, 
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
		bp->cpm_ptr->elem_b_ptr[num] != cpm_ptr->pred_b_ptr && 	/* 格要素が元用言のときはだめ */
		CheckTargetNoun(bp->cpm_ptr->elem_b_ptr[num])) {
		/* 格要素の格の一致 (格によりけり) */
		if (cf_ptr->pp[n][0] == bp->cpm_ptr->cmm[0].cf_ptr->pp[i][0]) {
		    EllipsisDetectForVerbSubcontract(s, s, cpm_ptr, bp->cpm_ptr->elem_b_ptr[num], 
						     cf_ptr, n, RANK3);
		}
		/* 格は不一致 */
		else {
		    EllipsisDetectForVerbSubcontract(s, s, cpm_ptr, bp->cpm_ptr->elem_b_ptr[num], 
						     cf_ptr, n, RANK2);
		}
	    }
	}
    }
}

/*==================================================================*/
	 int CheckCaseComponent(CF_PRED_MGR *cpm_ptr, int n)
/*==================================================================*/
{
    int i;

    for (i = 0; i < cpm_ptr->cf.element_num; i++) {
	if (cpm_ptr->elem_b_ptr[i]->num == n) {
	    if (cpm_ptr->cmm[0].result_lists_d[0].flag[i] >= 0) {
		return TRUE;
	    }
	    else {
		return FALSE;
	    }
	}
    }
    return FALSE;
}

/*==================================================================*/
void EllipsisDetectForVerb(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr, CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{
    /* 用言とその省略格が与えられる */

    /* cf_ptr = cpm_ptr->cmm[0].cf_ptr である */
    /* 用言 cpm_ptr の cf_ptr->pp[n][0] 格が省略されている
       cf_ptr->ex[n] に似ている文節を探す */

    int i, current = 1, bend;
    char feature_buffer[DATA_LEN], etc_buffer[DATA_LEN];
    SENTENCE_DATA *s, *cs;

    maxscore = 0;
    cs = sentence_data + sp->Sen_num - 1;
    memset(Bcheck, 0, BNST_MAX);

    /* 省略されている格をマーク */
    sprintf(feature_buffer, "C省略-%s", 
	    pp_code_to_kstr(cf_ptr->pp[n][0]));
    assign_cfeature(&(cpm_ptr->pred_b_ptr->f), feature_buffer);

    /* 親をみる (PARA なら child 用言) */
    if (cpm_ptr->pred_b_ptr->parent) {
	/* 親が PARA */
	if (cpm_ptr->pred_b_ptr->parent->para_top_p) {
	    /* 自分と並列の用言 */
	    for (i = 0; cpm_ptr->pred_b_ptr->parent->child[i]; i++) {
		/* PARA の子供で、自分を以外の並列用言 */
		if (cpm_ptr->pred_b_ptr->parent->child[i] != cpm_ptr->pred_b_ptr &&
		    cpm_ptr->pred_b_ptr->parent->child[i]->para_type == PARA_NORMAL) {
		    SearchCaseComponent(cs, cpm_ptr, cpm_ptr->pred_b_ptr->parent->child[i], 
					cf_ptr, n);
		}
	    }

	    /* 連用で係る親用言 (並列のとき) */
	    if (cpm_ptr->pred_b_ptr->parent->parent && 
		check_feature(cpm_ptr->pred_b_ptr->f, "係:連用")) {
		SearchCaseComponent(cs, cpm_ptr, cpm_ptr->pred_b_ptr->parent->parent, cf_ptr, n);
	    }
	}
	/* とりあえず、連用で係るひとつ上の親用言のみ */
	else if (check_feature(cpm_ptr->pred_b_ptr->f, "係:連用")) {
	    SearchCaseComponent(cs, cpm_ptr, cpm_ptr->pred_b_ptr->parent, cf_ptr, n);
	}
    }

    /* 子供 (用言) を見る */
    for (i = 0; cpm_ptr->pred_b_ptr->child[i]; i++) {
	if (check_feature(cpm_ptr->pred_b_ptr->child[i]->f, "用言")) {
	    SearchCaseComponent(cs, cpm_ptr, cpm_ptr->pred_b_ptr->child[i], cf_ptr, n);
	}
    }

    /* 前の文の体言を探す (この用言の格要素になっているもの以外) */
    for (s = cs; s >= sentence_data; s--) {
	bend = s->Bnst_num;

	for (i = bend-1; i >= 0; i--) {
	    if (current && 
		(Bcheck[i] || 
		 (s->bnst_data+i)->dpnd_head == cpm_ptr->pred_b_ptr->num || /* 用言に直接係らない */
		 i == cpm_ptr->pred_b_ptr->num || /* 自分自身じゃない */
		 CheckCaseComponent(cpm_ptr, i))) { /* 元用言がその文節を格要素としてもたない */
		continue;
	    }

	    /* 省略要素となるためのとりあえずの条件 */
	    if (!CheckPureNoun(s->bnst_data+i)) {
		continue;
	    }

	    EllipsisDetectForVerbSubcontract(s, cs, cpm_ptr, s->bnst_data+i, cf_ptr, n, RANK1);
	}
	if (current)
	    current = 0;
    }

    /* 【主体一般】
       1. 用言が受身でニ格 (もとはガ格) に <主体> をとるとき
       2. 「〜ため(に)」でガ格に <主体> をとるとき 
       3. 〜が V した N (外の関係), 形副名詞, 相対名詞は除く */
    if ((check_feature(cpm_ptr->pred_b_ptr->f, "ID:〜（ため）") && 
	 cf_ptr->pp[n][0] == pp_kstr_to_code("ガ") && 
	 cf_match_element(cf_ptr->sm[n], "主体", SM_CODE_SIZE)) || 
	(cf_ptr->pp[n][0] == pp_kstr_to_code("ニ") && 
	cf_match_element(cf_ptr->sm[n], "主体", SM_CODE_SIZE) && 
	(check_feature(cpm_ptr->pred_b_ptr->f, "〜れる") || 
	 check_feature(cpm_ptr->pred_b_ptr->f, "〜られる") || 
	 check_feature(cpm_ptr->pred_b_ptr->f, "サ変名詞格解析"))) || 
	(cpm_ptr->pred_b_ptr->parent && 
	 (check_feature(cpm_ptr->pred_b_ptr->parent->f, "外の関係") || 
	  check_feature(cpm_ptr->pred_b_ptr->parent->f, "外の関係判定")) && 
	 !check_feature(cpm_ptr->pred_b_ptr->parent->f, "相対名詞") && 
	 !check_feature(cpm_ptr->pred_b_ptr->parent->f, "形副名詞") && 
	 check_feature(cpm_ptr->pred_b_ptr->f, "係:連格") && 
	 cf_ptr->pp[n][0] == pp_kstr_to_code("ガ"))) {
	sprintf(feature_buffer, "C用;【主体一般】;%s:1", 
		pp_code_to_kstr(cf_ptr->pp[n][0]));
	assign_cfeature(&(cpm_ptr->pred_b_ptr->f), feature_buffer);
	sprintf(feature_buffer, "省略処理なし-%s", 
		pp_code_to_kstr(cf_ptr->pp[n][0]));
	assign_cfeature(&(cpm_ptr->pred_b_ptr->f), feature_buffer);
    }
    /* 次の場合は省略要素を探すが記録しない 
       (現時点ではデータを見るため、これらも省略解析を行っている) 
       1. デ格
       2. ト格
       3. ヨリ格 */
    else if (cf_ptr->pp[n][0] == pp_kstr_to_code("デ") ||  
	     cf_ptr->pp[n][0] == pp_kstr_to_code("ト") || 
	     cf_ptr->pp[n][0] == pp_kstr_to_code("ヨリ")) {
	sprintf(feature_buffer, "省略処理なし-%s", 
		pp_code_to_kstr(cf_ptr->pp[n][0]));
	assign_cfeature(&(cpm_ptr->pred_b_ptr->f), feature_buffer);
    }
    else if (maxscore > 0) {
	if (cs == maxs) {
	    strcpy(etc_buffer, "同一文");
	}
	else if (cs-maxs > 0) {
	    sprintf(etc_buffer, "%d文前", cs-maxs);
	}

	/* 決定した省略関係 */
	sprintf(feature_buffer, "C用;【%s】;%s:%.3f:%s(%s):%d文節", 
		(maxs->bnst_data+maxi)->Jiritu_Go, 
		pp_code_to_kstr(cf_ptr->pp[n][0]), 
		maxscore, maxs->KNPSID ? maxs->KNPSID+5 : "?", 
		etc_buffer, maxi);
	assign_cfeature(&(cpm_ptr->pred_b_ptr->f), feature_buffer);

	/* リンクされたことを記録する */
	RegisterAnaphor((maxs->bnst_data+maxi)->Jiritu_Go);
	/* RegisterPredicate(L_Jiritu_M(cpm_ptr->pred_b_ptr)->Goi, cf_ptr->pp[n][0], 
	   (maxs->bnst_data+maxi)->Jiritu_Go); */
    }
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
	      void discourse_analysis(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, num, toflag;
    CF_PRED_MGR *cpm_ptr;
    CF_MATCH_MGR *cmm_ptr;
    CASE_FRAME *cf_ptr;

    copy_sentence(sp);
    clear_cf();

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
	   7. 〜て（用言） */
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

	for (i = 0; i < cf_ptr->element_num; i++) {
	    num = cmm_ptr->result_lists_p[0].flag[i];
	    /* 以下の格の場合、省略要素と認定しない
	       時間格, 修飾格, 無格, 複合辞
	    */
	    if (num == UNASSIGNED && 
		!(toflag && str_eq(pp_code_to_kstr(cmm_ptr->cf_ptr->pp[i][0]), "ヲ")) && 
		!(cmm_ptr->cf_ptr->pp[i][0] > 8 && cmm_ptr->cf_ptr->pp[i][0] < 38) && 
		!str_eq((char *)pp_code_to_kstr(cmm_ptr->cf_ptr->pp[i][0]), "時間") && 
		!str_eq((char *)pp_code_to_kstr(cmm_ptr->cf_ptr->pp[i][0]), "φ") && 
		!str_eq((char *)pp_code_to_kstr(cmm_ptr->cf_ptr->pp[i][0]), "修飾")) {
		EllipsisDetectForVerb(sp, cpm_ptr, cmm_ptr->cf_ptr, i);
	    }
	}
    }

    /* 各体言をチェック
    for (i = sp->Bnst_num-1; i >= 0; i--) {
	if (CheckPureNoun(sp->bnst_data+i)) {
	    EllipsisDetectForNoun(sentence_data+sp->Sen_num-1, sp->bnst_data+i);
	}
    }
    */
}

/*====================================================================
                               END
====================================================================*/
