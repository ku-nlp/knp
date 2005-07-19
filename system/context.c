/*====================================================================

			       文脈解析

                                         Daisuke Kawahara 2001. 7. 13

    $Id$
====================================================================*/
#include "knp.h"
#include "context.h"

float maxscore;
float maxrawscore;
SENTENCE_DATA *maxs;
int maxi, maxpos;
char *maxtag, *maxfeatures;
int **Bcheck;
int **LC;
int PrintFeatures = 0;

char *ExtraTags[] = {"一人称", "不特定-人", "不特定-状況", ""};

char *ETAG_name[] = {
    "", "", "不特定:人", "一人称", "不特定:状況", 
    "前文", "後文"};

/* 探すのを止める閾値 */
float	AntecedentDecideThresholdPredGeneral = 0.60; /* 学習時は 0.01? */
float	AntecedentDecideThresholdForNoun = 0.60;
float	AntecedentDecideThresholdForNi = 0.90;

float	CFSimThreshold = 0.80;

float	SVM_FREQ_SD = 80.08846;	/* for np (cf-20040623) */
float	SVM_FREQ_SD_NO = 504.70998;	/* for noun, np */

PALIST palist[TBLSIZE];		/* 用言と格要素のセットのリスト */
CFLIST cflist[TBLSIZE];

E_CANDIDATE *ante_cands;
int cand_num = 0;
int cand_num_max = 0;

extern int	EX_match_subject;

#define CASE_ORDER_MAX	3
char *CaseOrder[CASE_ORDER_MAX][4] = {
    {"ガ", "ヲ", "ニ", ""}, 
    {"ヲ", "ニ", "ガ", ""}, 
    {"ニ", "ヲ", "ガ", ""}, 
};

int DiscAddedCases[PP_NUMBER] = {END_M};
int LocationLimit[PP_NUMBER] = {END_M, END_M, END_M, END_M};
int PrevSentenceLimit = 2;
int OptUseSmfix;


/*==================================================================*/
		    char *loc_code_to_str(int loc)
/*==================================================================*/
{
    if (loc == LOC_PARENTV) {
	return "PARENTV";
    }
    else if (loc == LOC_PARENTV_MC) {
	return "PARENTV_MC";
    }
    else if (loc == LOC_CHILDPV) {
	return "CHILDPV";
    }
    else if (loc == LOC_CHILDV) {
	return "CHILDV";
    }
    else if (loc == LOC_PARENTNPARENTV) {
	return "PARENTNPARENTV";
    }
    else if (loc == LOC_PARENTNPARENTV_MC) {
	return "PARENTNPARENTV_MC";
    }
    else if (loc == LOC_PV) {
	return "PV";
    }
    else if (loc == LOC_PV_MC) {
	return "PV_MC";
    }
    else if (loc == LOC_PARENTVPARENTV) {
	return "PARENTVPARENTV";
    }
    else if (loc == LOC_PARENTVPARENTV_MC) {
	return "PARENTVPARENTV_MC";
    }
    else if (loc == LOC_MC) {
	return "MC";
    }
    else if (loc == LOC_SC) {
	return "SC";
    }
    else if (loc == LOC_PRE_OTHERS) {
	return "PRE_OTHERS";
    }
    else if (loc == LOC_POST_OTHERS) {
	return "POST_OTHERS";
    }
    else if (loc == LOC_S1_MC) {
	return "S1_MC";
    }
    else if (loc == LOC_S1_SC) {
	return "S1_SC";
    }
    else if (loc == LOC_S1_OTHERS) {
	return "S1_OTHERS";
    }
    else if (loc == LOC_S2_MC) {
	return "S2_MC";
    }
    else if (loc == LOC_S2_SC) {
	return "S2_SC";
    }
    else if (loc == LOC_S2_OTHERS) {
	return "S2_OTHERS";
    }
    else if (loc == LOC_OTHERS) {
	return "OTHERS";
    }
    else if (loc == END_M) {
	return "NIL";
    }
    return NULL;
}

/*==================================================================*/
		   int get_utype(TAG_DATA *bp)
/*==================================================================*/
{
    // 親をたどって、発話タイプのfeatureがふられている文節を探す
    BNST_DATA *bc;
    char *utype;

    bc = bp->b_ptr;
    while (bc != NULL) {
	if (utype = check_feature(bc->f, "発話タイプ")) {
	    utype += strlen("発話タイプ:");
	    if (str_eq(utype, "作業:大")) {
		return UTYPE_ACTION_LARGE;
	    }
	    else if (str_eq(utype, "作業:中")) {
		return UTYPE_ACTION_MIDDLE;
	    }
	    else if (str_eq(utype, "作業:小")) {
		return UTYPE_ACTION_SMALL;
	    }
	    else if (str_eq(utype, "留意事項") || str_eq(utype, "留意事項・コツ") || str_eq(utype, "留意事項・注意")) {
		return UTYPE_NOTES;
	    }
	    else if (str_eq(utype, "食品・道具提示")) {
		return UTYPE_FOOD_PRESENTATION;
	    }
	    else if (str_eq(utype, "料理状態")) {
		return UTYPE_FOOD_STATE;
	    }
	    else if (str_eq(utype, "程度")) {
		return UTYPE_DEGREE;
	    }
	    else if (str_eq(utype, "効果")) {
		return UTYPE_EFFECT;
	    }
	    else if (str_eq(utype, "補足")) {
		return UTYPE_ADDITION;
	    }
	    else if (str_eq(utype, "代替可")) {
		return UTYPE_SUBSTITUTION;
	    }
	    else if (str_eq(utype, "終了")) {
		return UTYPE_END;
	    }
	    else {
		return UTYPE_OTHERS;
	    }
	    break;
	}
	bc = bc->parent;
    }

    return UTYPE_OTHERS;
}

/*==================================================================*/
		   int get_discourse_depth(TAG_DATA *bp)
/*==================================================================*/
{
    // 親をたどって、談話構造深さのfeatureがふられている文節を探す
    BNST_DATA *bc;
    char* depth_char;
    int depth = 1;

    bc = bp->b_ptr;
    while (bc != NULL) {
	if (depth_char = check_feature(bc->f, "談話構造深さ")) {
	    depth_char += strlen("談話構造深さ:");
	    depth = atoi(depth_char);
	    return depth;
	}
	bc = bc->parent;
    }

    return depth;
}

/*==================================================================*/
		   int loc_name_to_code(char *loc)
/*==================================================================*/
{
    int i;

    for (i = 0; LocationNames[i][0]; i++) {
	if (!strcmp(loc, LocationNames[i])) {
	    return LocationNums[i];
	}
    }
    return END_M;
}

/*==================================================================*/
		    void ClearCCList(PALIST *pap)
/*==================================================================*/
{
    int j;
    CASE_COMPONENT *ccp, *next;

    for (j = 0; j < CASE_MAX_NUM; j++) {
	if (pap->cc[j]) {
	    free(pap->cc[j]->word);
	    if (pap->cc[j]->pp_str) {
		free(pap->cc[j]->pp_str);
	    }
	    ccp = pap->cc[j]->next;
	    free(pap->cc[j]);
	    while (ccp) {
		free(ccp->word);
		if (ccp->pp_str) {
		    free(ccp->pp_str);
		}
		next = ccp->next;
		free(ccp);
		ccp = next;
	    }
	}
    }
}

/*==================================================================*/
		       void ClearAnaphoraList()
/*==================================================================*/
{
    int i, j;
    PALIST *pap, *next;

    for (i = 0; i < TBLSIZE; i++) {
	if (palist[i].key) {
	    free(palist[i].key);
	}
	ClearCCList(&palist[i]);
	pap = palist[i].next;
	while (pap) {
	    free(pap->key);
	    ClearCCList(pap);
	    next = pap->next;
	    free(pap);
	    pap = next;
	}
    }
}

/*==================================================================*/
			  void ClearCFList()
/*==================================================================*/
{
    int i, j;
    CFLIST *cfp, *next;

    for (i = 0; i < TBLSIZE; i++) {
	if (cflist[i].key) {
	    free(cflist[i].key);
	}

	for (j = 0; j < cflist[i].cfid_num; j++) {
	    free(*(cflist[i].cfid + j));
	}
	free(cflist[i].cfid);

	cfp = cflist[i].next;
	while (cfp) {
	    free(cfp->key);
	    next = cfp->next;
	    free(cfp);
	    cfp = next;
	}
    }
}

/*==================================================================*/
			void InitContextHash()
/*==================================================================*/
{
    memset(palist, 0, sizeof(PALIST)*TBLSIZE);
    memset(cflist, 0, sizeof(CFLIST)*TBLSIZE);
}

/*==================================================================*/
		void InitEllipsisMGR(ELLIPSIS_MGR *em)
/*==================================================================*/
{
    memset(em, 0, sizeof(ELLIPSIS_MGR));
}

/*==================================================================*/
	 void ClearEllipsisComponent(ELLIPSIS_COMPONENT *ec)
/*==================================================================*/
{
    ELLIPSIS_COMPONENT *emp, *next;

    if (ec->pp_str) {
	free(ec->pp_str);
    }
    emp = ec->next;
    while (emp) {
	if (emp->pp_str) {
	    free(emp->pp_str);
	}
	next = emp->next;
	free(emp);
	emp = next;
    }
}

/*==================================================================*/
	       void ClearEllipsisMGR(ELLIPSIS_MGR *em)
/*==================================================================*/
{
    int i;

    for (i = 0; i < CASE_TYPE_NUM; i++) {
	ClearEllipsisComponent(&(em->cc[i]));
    }

    clear_feature(&(em->f));
    InitEllipsisMGR(em);
}

/*==================================================================*/
void CopyEllipsisComponent(ELLIPSIS_COMPONENT *dst, ELLIPSIS_COMPONENT *src)
/*==================================================================*/
{
    dst->s = src->s;
    if (src->pp_str) {
	dst->pp_str = strdup(src->pp_str);
    }
    else {
	dst->pp_str = NULL;
    }
    dst->bnst = src->bnst;
    dst->score = src->score;
    dst->dist = src->dist;
    if (src->next) {
	dst->next = (ELLIPSIS_COMPONENT *)malloc_data(sizeof(ELLIPSIS_COMPONENT), "CopyEllipsisComponent");
	CopyEllipsisComponent(dst->next, src->next);
    }
    else {
	dst->next = NULL;
    }
}

/*==================================================================*/
		       int CheckBasicPP(int pp)
/*==================================================================*/
{
    /* ノ格 ok */
    if (pp == 41) {
	return 1;
    }

    /* 複合辞などの格は除く */
    if (pp == END_M || pp > 8 || pp < 0) {
	return 0;
    }
    return 1;
}

/*==================================================================*/
 void StoreCaseComponent(CASE_COMPONENT **ccpp, char *word, char *pp_str, 
			 int sent_n, int tag_n, int flag)
/*==================================================================*/
{
    /* 格要素を登録する */

    while (*ccpp) {
	/* ノ格格指定あり: 上書き */
	if (pp_str && (*ccpp)->pp_str && !strcmp((*ccpp)->pp_str, pp_str)) {
	    free((*ccpp)->word);
	    (*ccpp)->word = strdup(word);
	    (*ccpp)->sent_num = sent_n;
	    (*ccpp)->tag_num = tag_n;
	    (*ccpp)->count = 1;
	    (*ccpp)->flag = flag;
	    return;
	}
	/* すでに登録されているとき: 同じ単語があれば */
	else if (!pp_str && !(*ccpp)->pp_str && !strcmp((*ccpp)->word, word)) {
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
    if (pp_str) {
	(*ccpp)->pp_str = strdup(pp_str);
    }
    else {
	(*ccpp)->pp_str = NULL;
    }
    (*ccpp)->sent_num = sent_n;
    (*ccpp)->tag_num = tag_n;
    (*ccpp)->count = 1;
    (*ccpp)->flag = flag;
    (*ccpp)->next = NULL;
}

/*==================================================================*/
 void StoreEllipsisComponent(ELLIPSIS_COMPONENT *ccp, char *pp_str, 
			     SENTENCE_DATA *sp, int tag_n, float score, int dist)
/*==================================================================*/
{
    if (!pp_str) {
	ccp->s = sp;
	ccp->pp_str = NULL;
	ccp->bnst = tag_n;
	ccp->score = score;
	ccp->dist = dist;
	ccp->next = NULL;
	return;
    }
    else {
	ELLIPSIS_COMPONENT **ccpp = &ccp;

	while (*ccpp && (*ccpp)->s && (*ccpp)->bnst) {
	    /* ノ格格指定あり: 上書き */
	    if ((*ccpp)->pp_str && !strcmp((*ccpp)->pp_str, pp_str)) {
		(*ccpp)->s = sp;
		(*ccpp)->pp_str = strdup(pp_str);
		(*ccpp)->bnst = tag_n;
		(*ccpp)->score = score;
		(*ccpp)->dist = dist;
		return;
	    }
	    ccpp = &((*ccpp)->next);
	}

	if (!*ccpp) {
	    *ccpp = (ELLIPSIS_COMPONENT *)malloc_data(sizeof(ELLIPSIS_COMPONENT), "StoreEllipsisComponent");
	}

	(*ccpp)->s = sp;
	(*ccpp)->pp_str = strdup(pp_str);
	(*ccpp)->bnst = tag_n;
	(*ccpp)->score = score;
	(*ccpp)->dist = dist;
	(*ccpp)->next = NULL;
    }
}

/*==================================================================*/
ELLIPSIS_COMPONENT *CheckEllipsisComponent(ELLIPSIS_COMPONENT *ccp, char *pp_str)
/*==================================================================*/
{
    if (!pp_str) {
	return ccp;
    }
    else {
	while (ccp) {
	    if (ccp->pp_str && !strcmp(ccp->pp_str, pp_str)) {
		return ccp;
	    }
	    ccp = ccp->next;
	}
    }
    return NULL;
}

/*==================================================================*/
void RegisterTagTarget(char *key, int voice, int cf_addr, 
		       int pp, char *pp_str, char *word, int sent_n, int tag_n, int flag)
/*==================================================================*/
{
    /* 用言と格要素をセットで登録する */

    PALIST *pap;

    if (word == NULL) {
	return;
    }

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
		StoreCaseComponent(&((*papp)->cc[pp]), word, pp_str, sent_n, tag_n, flag);
		return;
	    }
	    papp = &((*papp)->next);
	} while (*papp);
	*papp = (PALIST *)malloc_data(sizeof(PALIST), "RegisterTagTarget");
	(*papp)->key = strdup(key);
	(*papp)->voice = voice;
	(*papp)->cf_addr = cf_addr;
	memset((*papp)->cc, 0, sizeof(CASE_COMPONENT *)*CASE_MAX_NUM);
	StoreCaseComponent(&((*papp)->cc[pp]), word, pp_str, sent_n, tag_n, flag);
	(*papp)->next = NULL;
    }
    else {
	pap->key = strdup(key);
	pap->voice = voice;
	pap->cf_addr = cf_addr;
	StoreCaseComponent(&(pap->cc[pp]), word, pp_str, sent_n, tag_n, flag);
    }
}

/*==================================================================*/
  CASE_COMPONENT *CheckTagTarget(char *key, int voice, int cf_addr,
				 int pp, char *pp_str)
/*==================================================================*/
{
    PALIST *pap;
    CASE_COMPONENT *ccp;

    if (CheckBasicPP(pp) == 0) {
	return NULL;
    }

    pap = &(palist[hash(key, strlen(key))]);
    if (!pap->key) {
	return NULL;
    }
    while (pap) {
	if (!strcmp(pap->key, key) && 
	    pap->voice == voice && 
	    pap->cf_addr == cf_addr) {
	    ccp = pap->cc[pp];
	    /* ノ格の格指定あるとき */
	    if (pp_str) {
		while (ccp) {
		    if (!ccp->pp_str || !strcmp(ccp->pp_str, pp_str)) {
			return ccp;
		    }
		    ccp = ccp->next;
		}
	    }
	    /* ノ格の格指定ないとき */
	    else if (ccp) {
		/* 最後の要素を返す */
		while (ccp->next) {
		    ccp = ccp->next;
		}
		return ccp;
	    }
	    return NULL;
	}
	pap = pap->next;
    }
    return NULL;
}

/*==================================================================*/
		    char *get_pred_id(char *cfid)
/*==================================================================*/
{
    char verb[SMALL_DATA_LEN], type[SMALL_DATA_LEN], voice[SMALL_DATA_LEN];
    char *ret;
    int index;

    /* with voice */
    if (sscanf(cfid, "%[^:]:%[^:]:%[^0-9]%d", verb, type, voice, &index) == 4) {
	ret = (char *)malloc_data(sizeof(char) * (strlen(verb) + strlen(type) + strlen(voice) + 3), 
				  "get_pred_id");
	sprintf(ret, "%s:%s:%s", verb, type, voice);
    }
    /* normal */
    else if (sscanf(cfid, "%[^:]:%[^0-9]%d", verb, type, &index) == 3) {
	ret = (char *)malloc_data(sizeof(char) * (strlen(verb) + strlen(type) + 2), 
				  "get_pred_id");
	sprintf(ret, "%s:%s", verb, type);
    }
    else {
	fprintf(stderr, ";; Unknown cfid format (%s)!\n", cfid);
	ret = NULL;
    }

    return ret;
}

/*==================================================================*/
		     void RegisterCF(char *cfid)
/*==================================================================*/
{
    char *key;
    CFLIST *cfp;

    if (cfid == NULL) {
	return;
    }

    if ((key = get_pred_id(cfid)) == NULL) {
	return;
    }

    cfp = &(cflist[hash(key, strlen(key))]);

    if (cfp->key) {
	CFLIST **cfpp;
	cfpp = &cfp;
	do {
	    if (!strcmp((*cfpp)->key, key)) {
		if ((*cfpp)->cfid_num >= (*cfpp)->cfid_max) {
		    (*cfpp)->cfid = (char **)realloc_data((*cfpp)->cfid, 
							  sizeof(char *) * ((*cfpp)->cfid_max <<= 1), 
							  "RegisterCF");
		}
		*((*cfpp)->cfid + (*cfpp)->cfid_num++) = strdup(cfid);
		free(key);
		return;
	    }
	    cfpp = &((*cfpp)->next);
	} while (*cfpp);
	*cfpp = (CFLIST *)malloc_data(sizeof(CFLIST), "RegisterCF");
	cfp = *cfpp;
    }

    cfp->key = strdup(key);
    cfp->cfid_num = 1;
    cfp->cfid_max = 2;
    cfp->cfid = (char **)malloc_data(sizeof(char *) * cfp->cfid_max, "RegisterCF");
    *(cfp->cfid) = strdup(cfid);
    cfp->next = NULL;

    free(key);
}

/*==================================================================*/
		      CFLIST *CheckCF(char *key)
/*==================================================================*/
{
    CFLIST *cfp;

    cfp = &(cflist[hash(key, strlen(key))]);
    if (!cfp->key) {
	return NULL;
    }
    while (cfp) {
	if (!strcmp(cfp->key, key)) {
	    return cfp;
	}
	cfp = cfp->next;
    }
    return NULL;
}

/*==================================================================*/
		void ClearSentence(SENTENCE_DATA *s)
/*==================================================================*/
{
    free(s->mrph_data);
    free(s->bnst_data);
    free(s->tag_data);
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
    for (i = 0; i < sp->Sen_num - 1; i++) {
	ClearSentence(sentence_data+i);
    }
    sp->Sen_num = 1;
    ClearAnaphoraList();
    ClearCFList();
    InitContextHash();
}

/*==================================================================*/
		 void InitSentence(SENTENCE_DATA *s)
/*==================================================================*/
{
    int i, j;

    s->mrph_data = (MRPH_DATA *)malloc_data(sizeof(MRPH_DATA)*MRPH_MAX, "InitSentence");
    s->bnst_data = (BNST_DATA *)malloc_data(sizeof(BNST_DATA)*BNST_MAX, "InitSentence");
    s->tag_data = (TAG_DATA *)malloc_data(sizeof(TAG_DATA)*TAG_MAX, "InitSentence");
    s->para_data = (PARA_DATA *)malloc_data(sizeof(PARA_DATA)*PARA_MAX, "InitSentence");
    s->para_manager = (PARA_MANAGER *)malloc_data(sizeof(PARA_MANAGER)*PARA_MAX, "InitSentence");
    s->Best_mgr = (TOTAL_MGR *)malloc_data(sizeof(TOTAL_MGR), "InitSentence");
    s->Sen_num = 0;
    s->Mrph_num = 0;
    s->Bnst_num = 0;
    s->New_Bnst_num = 0;
    s->Tag_num = 0;
    s->New_Tag_num = 0;
    s->KNPSID = NULL;
    s->Comment = NULL;
    s->cpm = NULL;
    s->cf = NULL;

    for (i = 0; i < MRPH_MAX; i++)
	(s->mrph_data + i)->f = NULL;
    for (i = 0; i < BNST_MAX; i++)
	(s->bnst_data + i)->f = NULL;
    for (i = 0; i < TAG_MAX; i++)
	(s->tag_data + i)->f = NULL;
    for (i = 0; i < PARA_MAX; i++) {
	for (j = 0; j < RF_MAX; j++) {
	    (s->para_data + i)->f_pattern.fp[j] = NULL;
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
    if (sp->Sen_num > SENTENCE_MAX) {
	fprintf(stderr, "Sentence buffer overflowed!\n");
	ClearSentences(sp);
    }

    sp_new = sentence_data + sp->Sen_num - 1;

    sp_new->available = sp->available;
    sp_new->Sen_num = sp->Sen_num;

    sp_new->Mrph_num = sp->Mrph_num;
    sp_new->mrph_data = (MRPH_DATA *)malloc_data(sizeof(MRPH_DATA)*sp->Mrph_num, 
						 "MRPH DATA");
    for (i = 0; i < sp->Mrph_num; i++) {
	sp_new->mrph_data[i] = sp->mrph_data[i];
    }

    sp_new->Bnst_num = sp->Bnst_num;
    sp_new->New_Bnst_num = sp->New_Bnst_num;
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
	sp_new->bnst_data[i].head_ptr = sp_new->mrph_data + (sp->bnst_data[i].head_ptr - sp->mrph_data);

	if (sp->bnst_data[i].parent)
	    sp_new->bnst_data[i].parent = sp_new->bnst_data + (sp->bnst_data[i].parent - sp->bnst_data);
	for (j = 0; sp_new->bnst_data[i].child[j]; j++) {
	    sp_new->bnst_data[i].child[j] = sp_new->bnst_data + (sp->bnst_data[i].child[j] - sp->bnst_data);
	}
	if (sp->bnst_data[i].pred_b_ptr) {
	    sp_new->bnst_data[i].pred_b_ptr = sp_new->bnst_data + (sp->bnst_data[i].pred_b_ptr - sp->bnst_data);
	}
    }

    sp_new->Tag_num = sp->Tag_num;
    sp_new->New_Tag_num = sp->New_Tag_num;
    sp_new->tag_data = 
	(TAG_DATA *)malloc_data(sizeof(TAG_DATA)*(sp->Tag_num + sp->New_Tag_num), 
				 "TAG DATA");
    for (i = 0; i < sp->Tag_num + sp->New_Tag_num; i++) {

	sp_new->tag_data[i] = sp->tag_data[i]; /* ここでtag_dataをコピー */

	sp_new->tag_data[i].mrph_ptr = sp_new->mrph_data + (sp->tag_data[i].mrph_ptr - sp->mrph_data);
	if (sp->tag_data[i].settou_ptr)
	    sp_new->tag_data[i].settou_ptr = sp_new->mrph_data + (sp->tag_data[i].settou_ptr - sp->mrph_data);
	sp_new->tag_data[i].jiritu_ptr = sp_new->mrph_data + (sp->tag_data[i].jiritu_ptr - sp->mrph_data);
	if (sp->tag_data[i].fuzoku_ptr)
	sp_new->tag_data[i].fuzoku_ptr = sp_new->mrph_data + (sp->tag_data[i].fuzoku_ptr - sp->mrph_data);
	sp_new->tag_data[i].head_ptr = sp_new->mrph_data + (sp->tag_data[i].head_ptr - sp->mrph_data);
	if (sp->tag_data[i].parent)
	    sp_new->tag_data[i].parent = sp_new->tag_data + (sp->tag_data[i].parent - sp->tag_data);
	for (j = 0; sp_new->tag_data[i].child[j]; j++) {
	    sp_new->tag_data[i].child[j] = sp_new->tag_data + (sp->tag_data[i].child[j] - sp->tag_data);
	}
	if (sp->tag_data[i].pred_b_ptr) {
	    sp_new->tag_data[i].pred_b_ptr = sp_new->tag_data + (sp->tag_data[i].pred_b_ptr - sp->tag_data);
	}

	sp_new->tag_data[i].b_ptr = sp_new->bnst_data + (sp->tag_data[i].b_ptr - sp->bnst_data);
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

    sp_new->cpm = NULL;
    sp_new->cf = NULL;

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
	*(sp_new->cpm + i) = sp->Best_mgr->cpm[i];
	num = sp->Best_mgr->cpm[i].pred_b_ptr->num;	/* この用言の文節番号 */
	sp_new->tag_data[num].cpm_ptr = sp_new->cpm + i;
	(sp_new->cpm + i)->pred_b_ptr = sp_new->tag_data + num;

	for (j = 0; j < (sp_new->cpm + i)->cf.element_num; j++) {
	    /* 省略じゃない格要素 */
	    if ((sp_new->cpm + i)->elem_b_num[j] > -2) {
		/* 内部文節じゃない */
		/* if ((sp_new->cpm + i)->elem_b_ptr[j]->inum == 0) */
		(sp_new->cpm + i)->elem_b_ptr[j] = 
		    sp_new->tag_data + ((sp_new->cpm + i)->elem_b_ptr[j]-sp->tag_data);
	    }
	}

	(sp_new->cpm + i)->pred_b_ptr->cf_ptr = sp_new->cf + cfnum;
	for (j = 0; j < (sp_new->cpm + i)->result_num; j++) {
	    copy_cf_with_alloc(sp_new->cf + cfnum, (sp_new->cpm + i)->cmm[j].cf_ptr);
	    (sp_new->cpm + i)->cmm[j].cf_ptr = sp_new->cf + cfnum;
	    sp->Best_mgr->cpm[i].cmm[j].cf_ptr = sp_new->cf + cfnum;
	    cfnum++;
	}
    }

    /* New領域の文節のcpmポインタをはりなおす */
    for (i = sp->Tag_num; i < sp->Tag_num + sp->New_Tag_num; i++) {
	if ((sp_new->tag_data + i)->cpm_ptr) {
	    (sp_new->tag_data + i)->cpm_ptr = 
		(sp_new->tag_data + (sp_new->tag_data + i)->cpm_ptr->pred_b_ptr->num)->cpm_ptr;
	}
    }

    /* 現在 cpm を保存しているが、Best_mgr を保存した方がいいかもしれない */
    sp_new->Best_mgr = NULL;
}

/*==================================================================*/
      int CheckCaseComponent(CF_PRED_MGR *cpm_ptr, TAG_DATA *tp)
/*==================================================================*/
{
    /* tpが用言の格要素であるかどうか */

    int i, j;

    for (i = 0; i < cpm_ptr->cf.element_num; i++) {
	if (cpm_ptr->elem_b_num[i] > -2) {
	    if (cpm_ptr->elem_b_ptr[i]->num == tp->num) {
		return TRUE;
	    }
	    /* 文節内 */
	    for (j = 0; cpm_ptr->elem_b_ptr[i]->child[j]; j++) {
		if (cpm_ptr->elem_b_ptr[i]->child[j]->bnum == cpm_ptr->elem_b_ptr[i]->bnum && 
		    cpm_ptr->elem_b_ptr[i]->child[j]->num == tp->num) {
		    return TRUE;
		}
	    }
	}
    }
    return FALSE;
}

/*==================================================================*/
int CheckHaveEllipsisComponent(CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int l, char *word)
/*==================================================================*/
{
    int i, num;

    /* 用言が候補と同じ表記を
       他の格の省略の指示対象としてもっているかどうか
       cpm_ptr->elem_b_num[num] <= -2 
       => 通常の格要素もチェック */

    for (i = 0; i < cmm_ptr->cf_ptr->element_num; i++) {
	num = cmm_ptr->result_lists_p[l].flag[i];
	/* 省略の指示対象 */
	if (num >= 0 && 
	    str_eq(cpm_ptr->elem_b_ptr[num]->head_ptr->Goi, word)) {
	    return 1;
	}
    }
    return 0;
}

/*==================================================================*/
int CheckObligatoryCase(CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int l, TAG_DATA *bp)
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
	    num = cmm_ptr->result_lists_p[l].flag[i];
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
int GetCandCase(CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, TAG_DATA *bp)
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
		cpm_ptr->elem_b_ptr[num]->num == bp->num) {
		return cmm_ptr->cf_ptr->pp[i][0];
	    }
	}
    }
    return -1;
}

/*==================================================================*/
int CheckCaseCorrespond(CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int l, 
			TAG_DATA *bp, CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{
    /* 格の一致を調べる
       bp: 対象文節
       cpm_ptr: 対象文節の係る用言 (bp->parent->cpm_ptr)
    */

    int i, num;

    if (cpm_ptr->result_num > 0 && cmm_ptr->score != -2) {
	for (i = 0; i < cmm_ptr->cf_ptr->element_num; i++) {
	    num = cmm_ptr->result_lists_p[l].flag[i];
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
       TAG_DATA *GetRealParent(SENTENCE_DATA *sp, TAG_DATA *bp)
/*==================================================================*/
{
    if (bp->dpnd_head != -1) {
	return sp->tag_data + bp->dpnd_head;
    }
    return NULL;
}

/*==================================================================*/
int CountBnstDistance(SENTENCE_DATA *cs, int candn, SENTENCE_DATA *ps, int pn)
/*==================================================================*/
{
    int sdiff, i, diff = 0;

    sdiff = ps - cs;

    if (sdiff > 0) {
	for (i = 1; i < sdiff; i++) {
	    diff += (ps - i)->Tag_num;
	}
	diff += pn+cs->Tag_num - candn;
    }
    else {
	diff = pn - candn;
    }

    return diff;
}

/*==================================================================*/
int CheckPredicateChild(TAG_DATA *pred_b_ptr, TAG_DATA *child_ptr)
/*==================================================================*/
{
    /* pred_b_ptr のメモリは古い? */

    if (child_ptr->parent) {
	/* N -> V */
	if (child_ptr->parent->num == pred_b_ptr->num) {
	    return 1;
	}
	/* N(P) -> <PARA> -> V */
	else if (child_ptr->para_type == PARA_NORMAL && 
		 child_ptr->parent->para_top_p && 
		 child_ptr->parent->parent && 
		 child_ptr->parent->parent->num == pred_b_ptr->num) {
	    return 1;
	}
    }
    else if (pred_b_ptr->parent) {
	/* V -> N */
	if (pred_b_ptr->parent->num == child_ptr->num) {
	    return 1;
	}
	/* V -> <PARA>(Nを含む) => ★なし */
	else if (pred_b_ptr->parent->para_top_p && 
		 child_ptr->para_type == PARA_NORMAL && 
		 child_ptr->parent->num == pred_b_ptr->parent->num) {
	    return 1;
	}
    }
    return 0;
}

/*==================================================================*/
	char *EllipsisSvmFeatures2String(E_SVM_FEATURES *esf)
/*==================================================================*/
{
    int max, i, prenum;
    char *buffer, *sbuf;


#ifdef DISC_USE_EVENT
#ifndef DISC_DONT_USE_FREQ
    prenum = 5;
#else
    prenum = 4;
#endif
#else
#ifndef DISC_DONT_USE_FREQ
    prenum = 3;
#else
    prenum = 2;
#endif
#endif

    /* 桁数の計算は常用対数の代わりに自然対数でやる (1000でも6.9) */
    max = (sizeof(E_SVM_FEATURES) - prenum * sizeof(float)) / sizeof(int) + prenum;
    sbuf = (char *)malloc_data(sizeof(char) * (10 + log(max)), 
			       "EllipsisSvmFeatures2String");
    buffer = (char *)malloc_data((sizeof(char) * (10 + log(max))) * max + 20, 
				 "EllipsisSvmFeatures2String");
#ifdef DISC_USE_EVENT
    sprintf(buffer, "1:%.5f 2:%.5f 3:%.5f", esf->similarity, esf->event1, esf->event2);
#else
    sprintf(buffer, "1:%.5f", esf->similarity);
#endif

    prenum--;

    if (!OptAddSvmFeatureDiscourseDepth) {
	max--;
    }

#ifndef DISC_DONT_USE_FREQ
    if (OptLearn == TRUE) {
	sprintf(sbuf, " %d:%d", prenum, (int)esf->frequency);
    }
    else {
	sprintf(sbuf, " %d:%.5f", prenum, esf->frequency);
    }
    strcat(buffer, sbuf);

#endif

    if (OptAddSvmFeatureDiscourseDepth) {
	prenum++;
	sprintf(sbuf, " %d:%.5f", prenum, esf->discourse_depth_inverse);
	strcat(buffer, sbuf);
    }

    for (i = prenum + 1; i <= max; i++) {
	sprintf(sbuf, " %d:%d", i, *(esf->c_pp + i - prenum - 1));
	strcat(buffer, sbuf);
    }
    free(sbuf);

    return buffer;
}

/*==================================================================*/
   char *TwinCandSvmFeatures2String(E_TWIN_CAND_SVM_FEATURES *esf)
/*==================================================================*/
{
    int max, i, prenum;
    char *buffer, *sbuf;

    prenum = 2;

    max = (sizeof(E_TWIN_CAND_SVM_FEATURES) - prenum * sizeof(float)) / sizeof(int) + prenum;
    sbuf = (char *)malloc_data(sizeof(char) * (10 + log(max)), 
			       "TwinCandSvmFeatures2String");
    buffer = (char *)malloc_data((sizeof(char) * (10 + log(max))) * max + 20, 
				 "TwinCandSvmFeatures2String");

    sprintf(buffer, "1:%.5f 2:%.5f", esf->c1_similarity, esf->c2_similarity);
    for (i = prenum + 1; i <= max; i++) {
	sprintf(sbuf, " %d:%d", i, *(esf->c1_pp + i - prenum - 1));
	strcat(buffer, sbuf);
    }
    free(sbuf);

    return buffer;
}

/*==================================================================*/
void EllipsisSvmFeaturesString2Feature(ELLIPSIS_MGR *em_ptr, CF_PRED_MGR *cpm_ptr, char *ecp, 
				       char *word, int pp, char *sid, int num, int loc)
/*==================================================================*/
{
    char *buffer;

    /* -learn 時のみ学習用featureを表示する */
    if (!PrintFeatures) {
	return;
    }

    if (word == NULL) {
	return;
    }

    buffer = (char *)malloc_data(strlen(ecp) + 64 + strlen(word), 
				 "EllipsisSvmFeaturesString2FeatureString");
    sprintf(buffer, "SVM学習FEATURE;%s;%s;%s;%s;%d:%s", 
	    word, pp_code_to_kstr_in_context(cpm_ptr, pp), 
	    loc >= 0 ? loc_code_to_str(loc) : "NONE", sid, num, ecp);
    assign_cfeature(&(em_ptr->f), buffer);
    free(buffer);
}

/*==================================================================*/
void TwinCandSvmFeaturesString2Feature(ELLIPSIS_MGR *em_ptr, char *ecp, 
				       E_CANDIDATE *c1, E_CANDIDATE *c2)
/*==================================================================*/
{
    char *buffer, *w1, *w2, *p1, *p2, *sid1, *sid2;
    int n1, n2;

    if (c1->tp) {
	if (c1->tp->head_ptr->Goi == NULL) {
	    return;
	}
	else {
	    w1 = c1->tp->head_ptr->Goi;
	    p1 = pp_code_to_kstr(c1->ef->c_pp);
	    sid1 = c1->s->KNPSID ? c1->s->KNPSID + 5 : "?";
	    n1 = c1->tp->num;
	}
    }
    else {
	w1 = c1->tag;
	sid1 = "?";
	n1 = -1;
    }

    if (c2->tp) {
	if (c2->tp->head_ptr->Goi == NULL) {
	    return;
	}
	else {
	    w2 = c2->tp->head_ptr->Goi;
	    sid2 = c2->s->KNPSID ? c2->s->KNPSID + 5 : "?";
	    n2 = c2->tp->num;
	}
    }
    else {
	w2 = c2->tag;
	sid2 = "?";
	n2 = -1;
    }

    buffer = (char *)malloc_data(strlen(ecp) + 128 + strlen(w1) + strlen(w2), 
				 "TwinCandSvmFeaturesString2FeatureString");
    sprintf(buffer, "SVM学習FEATURE;%s;%s;%s;%s;%d;%s;%s;%s;%d:%s", 
	    pp_code_to_kstr(c1->ef->p_pp), w1, 
	    c1->ef->c_location >= 0 ? loc_code_to_str(c1->ef->c_location) : "NONE", 
	    sid1, n1, 
	    w2, 
	    c2->ef->c_location >= 0 ? loc_code_to_str(c2->ef->c_location) : "NONE", 
	    sid2, n2, 
	    ecp);
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
#ifdef DISC_USE_EVENT
    f->event1 = ef->event1;
    f->event2 = ef->event2;
#endif
#ifndef DISC_DONT_USE_FREQ
    if (OptLearn == TRUE) {
	f->frequency = ef->frequency;
    }
    else {
	/* 標準偏差で割る */
	if (ef->p_pp == pp_kstr_to_code("ノ")) {
	    f->frequency = (float)ef->frequency / SVM_FREQ_SD_NO;
	}
	else {
	    f->frequency = (float)ef->frequency / SVM_FREQ_SD;
	}
    }
#endif
    for (i = 0; i < PP_NUMBER; i++) {
	f->c_pp[i] = ef->c_pp == i ? 1 : 0;
    }
#ifdef DISC_USE_DIST
    f->c_distance = ef->c_distance;
    f->c_dist_bnst = ef->c_dist_bnst;
#else
    f->c_location[0] = ef->c_location == LOC_PARENTV ? 1 : 0;
    f->c_location[1] = ef->c_location == LOC_PARENTV_MC ? 1 : 0;
    f->c_location[2] = ef->c_location == LOC_CHILDPV ? 1 : 0;
    f->c_location[3] = ef->c_location == LOC_CHILDV ? 1 : 0;
    f->c_location[4] = ef->c_location == LOC_PARENTNPARENTV ? 1 : 0;
    f->c_location[5] = ef->c_location == LOC_PARENTNPARENTV_MC ? 1 : 0;
    f->c_location[6] = ef->c_location == LOC_PV ? 1 : 0;
    f->c_location[7] = ef->c_location == LOC_PV_MC ? 1 : 0;
    f->c_location[8] = ef->c_location == LOC_PARENTVPARENTV ? 1 : 0;
    f->c_location[9] = ef->c_location == LOC_PARENTVPARENTV_MC ? 1 : 0;
    f->c_location[10] = ef->c_location == LOC_MC ? 1 : 0;
    f->c_location[11] = ef->c_location == LOC_SC ? 1 : 0;
    f->c_location[12] = ef->c_location == LOC_PRE_OTHERS ? 1 : 0;
    f->c_location[13] = ef->c_location == LOC_POST_OTHERS ? 1 : 0;
    f->c_location[14] = ef->c_location == LOC_S1_MC ? 1 : 0;
    f->c_location[15] = ef->c_location == LOC_S1_SC ? 1 : 0;
    f->c_location[16] = ef->c_location == LOC_S1_OTHERS ? 1 : 0;
    f->c_location[17] = ef->c_location == LOC_S2_MC ? 1 : 0;
    f->c_location[18] = ef->c_location == LOC_S2_SC ? 1 : 0;
    f->c_location[19] = ef->c_location == LOC_S2_OTHERS ? 1 : 0;
    f->c_location[20] = ef->c_location == LOC_OTHERS ? 1 : 0;
#endif
    f->c_fs_flag = ef->c_fs_flag;
    f->c_topic_flag = ef->c_topic_flag;
    f->c_no_topic_flag = ef->c_no_topic_flag;
    f->c_in_cnoun_flag = ef->c_in_cnoun_flag;
    f->c_subject_flag = ef->c_subject_flag;
    f->c_dep_mc_flag = ef->c_dep_mc_flag;
    f->c_n_modify_flag = ef->c_n_modify_flag;
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

    /* ガ,ヲ,ニ */
    for (i = 0; i < 3; i++) {
	f->p_pp[i] = ef->p_pp == i+1 ? 1 : 0;
    }
    f->p_voice[0] = ef->p_voice & VOICE_SHIEKI ? 1 : 0;
    f->p_voice[1] = ef->p_voice & VOICE_UKEMI ? 1 : 0;
    f->p_voice[2] = ef->p_voice & VOICE_MORAU ? 1 : 0;
    f->p_type[0] = ef->p_type == 1 ? 1 : 0;
    f->p_type[1] = ef->p_type == 2 ? 1 : 0;
    f->p_type[2] = ef->p_type == 3 ? 1 : 0;
    f->p_sahen_flag = ef->p_sahen_flag;
    f->p_cf_subject_flag = ef->p_cf_subject_flag;
    f->p_cf_sentence_flag = ef->p_cf_sentence_flag;
    f->p_n_modify_flag = ef->p_n_modify_flag;
    /* f->p_dep_p_level[0] = str_eq(ef->p_dep_p_level, "A-") ? 1 : 0;
    f->p_dep_p_level[1] = str_eq(ef->p_dep_p_level, "A") ? 1 : 0;
    f->p_dep_p_level[2] = str_eq(ef->p_dep_p_level, "B-") ? 1 : 0;
    f->p_dep_p_level[3] = str_eq(ef->p_dep_p_level, "B") ? 1 : 0;
    f->p_dep_p_level[4] = str_eq(ef->p_dep_p_level, "B+") ? 1 : 0;
    f->p_dep_p_level[5] = str_eq(ef->p_dep_p_level, "C") ? 1 : 0; */

    /* f->c_ac = ef->c_ac; */

    /* 発話タイプ */
    if (OptAddSvmFeatureUtype) {
	f->utype[0] = ef->utype == UTYPE_ACTION_LARGE ? 1 : 0;
	f->utype[1] = ef->utype == UTYPE_ACTION_MIDDLE ? 1 : 0;
	f->utype[2] = ef->utype == UTYPE_ACTION_SMALL ? 1 : 0;
	f->utype[3] = ef->utype == UTYPE_NOTES ? 1 : 0;
	f->utype[4] = ef->utype == UTYPE_FOOD_PRESENTATION ? 1 : 0;
	f->utype[5] = ef->utype == UTYPE_FOOD_STATE ? 1 : 0;
	f->utype[6] = ef->utype == UTYPE_DEGREE ? 1 : 0;
	f->utype[7] = ef->utype == UTYPE_EFFECT ? 1 : 0;
	f->utype[8] = ef->utype == UTYPE_ADDITION ? 1 : 0;
	f->utype[9] = ef->utype == UTYPE_SUBSTITUTION ? 1 : 0;
	f->utype[10] = ef->utype == UTYPE_END ? 1 : 0;
	f->utype[11] = ef->utype == UTYPE_OTHERS ? 1 : 0;
    }
    else {
	memset(&(f->utype[0]), 0, sizeof(int) * UTYPE_NUMBER);
    }


    /* 談話構造深さ */
    if (OptAddSvmFeatureDiscourseDepth) {
	f->discourse_depth_inverse = (float) 1 / ef->discourse_depth;
    }
    else {
	f->discourse_depth_inverse = 0;
    }
    return f;
}

/*==================================================================*/
E_TWIN_CAND_SVM_FEATURES *MakeTwinCandSvmFeatures(E_FEATURES *ef1, E_FEATURES *ef2)
/*==================================================================*/
{
    E_TWIN_CAND_SVM_FEATURES *f;
    int i;

    f = (E_TWIN_CAND_SVM_FEATURES *)malloc_data(sizeof(E_TWIN_CAND_SVM_FEATURES), "MakeTwinCandSvmFeatures");

    /* ef1 */
    f->c1_similarity = ef1->similarity;

    for (i = 0; i < PP_NUMBER; i++) {
	f->c1_pp[i] = ef1->c_pp == i ? 1 : 0;
    }

    f->c1_location[0] = ef1->c_location == LOC_PARENTV ? 1 : 0;
    f->c1_location[1] = ef1->c_location == LOC_PARENTV_MC ? 1 : 0;
    f->c1_location[2] = ef1->c_location == LOC_CHILDPV ? 1 : 0;
    f->c1_location[3] = ef1->c_location == LOC_CHILDV ? 1 : 0;
    f->c1_location[4] = ef1->c_location == LOC_PARENTNPARENTV ? 1 : 0;
    f->c1_location[5] = ef1->c_location == LOC_PARENTNPARENTV_MC ? 1 : 0;
    f->c1_location[6] = ef1->c_location == LOC_PV ? 1 : 0;
    f->c1_location[7] = ef1->c_location == LOC_PV_MC ? 1 : 0;
    f->c1_location[8] = ef1->c_location == LOC_PARENTVPARENTV ? 1 : 0;
    f->c1_location[9] = ef1->c_location == LOC_PARENTVPARENTV_MC ? 1 : 0;
    f->c1_location[10] = ef1->c_location == LOC_MC ? 1 : 0;
    f->c1_location[11] = ef1->c_location == LOC_SC ? 1 : 0;
    f->c1_location[12] = ef1->c_location == LOC_PRE_OTHERS ? 1 : 0;
    f->c1_location[13] = ef1->c_location == LOC_POST_OTHERS ? 1 : 0;
    f->c1_location[14] = ef1->c_location == LOC_S1_MC ? 1 : 0;
    f->c1_location[15] = ef1->c_location == LOC_S1_SC ? 1 : 0;
    f->c1_location[16] = ef1->c_location == LOC_S1_OTHERS ? 1 : 0;
    f->c1_location[17] = ef1->c_location == LOC_S2_MC ? 1 : 0;
    f->c1_location[18] = ef1->c_location == LOC_S2_SC ? 1 : 0;
    f->c1_location[19] = ef1->c_location == LOC_S2_OTHERS ? 1 : 0;
    f->c1_location[20] = ef1->c_location == LOC_OTHERS ? 1 : 0;

    f->c1_fs_flag = ef1->c_fs_flag;
    f->c1_topic_flag = ef1->c_topic_flag;
    f->c1_no_topic_flag = ef1->c_no_topic_flag;
    f->c1_in_cnoun_flag = ef1->c_in_cnoun_flag;
    f->c1_subject_flag = ef1->c_subject_flag;
    f->c1_dep_mc_flag = ef1->c_dep_mc_flag;
    f->c1_n_modify_flag = ef1->c_n_modify_flag;
    f->c1_dep_p_level[0] = str_eq(ef1->c_dep_p_level, "A-") ? 1 : 0;
    f->c1_dep_p_level[1] = str_eq(ef1->c_dep_p_level, "A") ? 1 : 0;
    f->c1_dep_p_level[2] = str_eq(ef1->c_dep_p_level, "B-") ? 1 : 0;
    f->c1_dep_p_level[3] = str_eq(ef1->c_dep_p_level, "B") ? 1 : 0;
    f->c1_dep_p_level[4] = str_eq(ef1->c_dep_p_level, "B+") ? 1 : 0;
    f->c1_dep_p_level[5] = str_eq(ef1->c_dep_p_level, "C") ? 1 : 0;
    f->c1_prev_p_flag = ef1->c_prev_p_flag;
    f->c1_get_over_p_flag = ef1->c_get_over_p_flag;
    f->c1_sm_none_flag = ef1->c_sm_none_flag;
    f->c1_extra_tag[0] = ef1->c_extra_tag == 0 ? 1 : 0;
    f->c1_extra_tag[1] = ef1->c_extra_tag == 1 ? 1 : 0;
    f->c1_extra_tag[2] = ef1->c_extra_tag == 2 ? 1 : 0;

    /* ef2 */
    f->c2_similarity = ef2->similarity;

    for (i = 0; i < PP_NUMBER; i++) {
	f->c2_pp[i] = ef2->c_pp == i ? 1 : 0;
    }

    f->c2_location[0] = ef2->c_location == LOC_PARENTV ? 1 : 0;
    f->c2_location[1] = ef2->c_location == LOC_PARENTV_MC ? 1 : 0;
    f->c2_location[2] = ef2->c_location == LOC_CHILDPV ? 1 : 0;
    f->c2_location[3] = ef2->c_location == LOC_CHILDV ? 1 : 0;
    f->c2_location[4] = ef2->c_location == LOC_PARENTNPARENTV ? 1 : 0;
    f->c2_location[5] = ef2->c_location == LOC_PARENTNPARENTV_MC ? 1 : 0;
    f->c2_location[6] = ef2->c_location == LOC_PV ? 1 : 0;
    f->c2_location[7] = ef2->c_location == LOC_PV_MC ? 1 : 0;
    f->c2_location[8] = ef2->c_location == LOC_PARENTVPARENTV ? 1 : 0;
    f->c2_location[9] = ef2->c_location == LOC_PARENTVPARENTV_MC ? 1 : 0;
    f->c2_location[10] = ef2->c_location == LOC_MC ? 1 : 0;
    f->c2_location[11] = ef2->c_location == LOC_SC ? 1 : 0;
    f->c2_location[12] = ef2->c_location == LOC_PRE_OTHERS ? 1 : 0;
    f->c2_location[13] = ef2->c_location == LOC_POST_OTHERS ? 1 : 0;
    f->c2_location[14] = ef2->c_location == LOC_S1_MC ? 1 : 0;
    f->c2_location[15] = ef2->c_location == LOC_S1_SC ? 1 : 0;
    f->c2_location[16] = ef2->c_location == LOC_S1_OTHERS ? 1 : 0;
    f->c2_location[17] = ef2->c_location == LOC_S2_MC ? 1 : 0;
    f->c2_location[18] = ef2->c_location == LOC_S2_SC ? 1 : 0;
    f->c2_location[19] = ef2->c_location == LOC_S2_OTHERS ? 1 : 0;
    f->c2_location[20] = ef2->c_location == LOC_OTHERS ? 1 : 0;

    f->c2_fs_flag = ef2->c_fs_flag;
    f->c2_topic_flag = ef2->c_topic_flag;
    f->c2_no_topic_flag = ef2->c_no_topic_flag;
    f->c2_in_cnoun_flag = ef2->c_in_cnoun_flag;
    f->c2_subject_flag = ef2->c_subject_flag;
    f->c2_dep_mc_flag = ef2->c_dep_mc_flag;
    f->c2_n_modify_flag = ef2->c_n_modify_flag;
    f->c2_dep_p_level[0] = str_eq(ef2->c_dep_p_level, "A-") ? 1 : 0;
    f->c2_dep_p_level[1] = str_eq(ef2->c_dep_p_level, "A") ? 1 : 0;
    f->c2_dep_p_level[2] = str_eq(ef2->c_dep_p_level, "B-") ? 1 : 0;
    f->c2_dep_p_level[3] = str_eq(ef2->c_dep_p_level, "B") ? 1 : 0;
    f->c2_dep_p_level[4] = str_eq(ef2->c_dep_p_level, "B+") ? 1 : 0;
    f->c2_dep_p_level[5] = str_eq(ef2->c_dep_p_level, "C") ? 1 : 0;
    f->c2_prev_p_flag = ef2->c_prev_p_flag;
    f->c2_get_over_p_flag = ef2->c_get_over_p_flag;
    f->c2_sm_none_flag = ef2->c_sm_none_flag;
    f->c2_extra_tag[0] = ef2->c_extra_tag == 0 ? 1 : 0;
    f->c2_extra_tag[1] = ef2->c_extra_tag == 1 ? 1 : 0;
    f->c2_extra_tag[2] = ef2->c_extra_tag == 2 ? 1 : 0;

    /* ガ,ヲ,ニ */
    for (i = 0; i < 3; i++) {
	f->p_pp[i] = ef1->p_pp == i+1 ? 1 : 0;
    }
    f->p_voice[0] = ef1->p_voice & VOICE_SHIEKI ? 1 : 0;
    f->p_voice[1] = ef1->p_voice & VOICE_UKEMI ? 1 : 0;
    f->p_voice[2] = ef1->p_voice & VOICE_MORAU ? 1 : 0;
    f->p_type[0] = ef1->p_type == 1 ? 1 : 0;
    f->p_type[1] = ef1->p_type == 2 ? 1 : 0;
    f->p_type[2] = ef1->p_type == 3 ? 1 : 0;
    f->p_sahen_flag = ef1->p_sahen_flag;
    f->p_cf_subject_flag = ef1->p_cf_subject_flag;
    f->p_cf_sentence_flag = ef1->p_cf_sentence_flag;
    f->p_n_modify_flag = ef1->p_n_modify_flag;
    /* f->p_dep_p_level[0] = str_eq(ef->p_dep_p_level, "A-") ? 1 : 0;
    f->p_dep_p_level[1] = str_eq(ef->p_dep_p_level, "A") ? 1 : 0;
    f->p_dep_p_level[2] = str_eq(ef->p_dep_p_level, "B-") ? 1 : 0;
    f->p_dep_p_level[3] = str_eq(ef->p_dep_p_level, "B") ? 1 : 0;
    f->p_dep_p_level[4] = str_eq(ef->p_dep_p_level, "B+") ? 1 : 0;
    f->p_dep_p_level[5] = str_eq(ef->p_dep_p_level, "C") ? 1 : 0; */

    /* f->c_ac = ef->c_ac; */

    return f;
}

/*==================================================================*/
void SetEllipsisFeaturesForPred(E_FEATURES *f, CF_PRED_MGR *cpm_ptr, 
				CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{
    char *level;

    if (cpm_ptr->cf.type == CF_PRED) {
	f->p_pp = cf_ptr->pp[n][0];

	/* 能動(0), VOICE_SHIEKI(1), VOICE_UKEMI(2), VOICE_MORAU(3) */
	f->p_voice = cpm_ptr->pred_b_ptr->voice;

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
    }
    /* 名詞格フレームのとき */
    else {
	f->p_pp = -1;
	f->p_voice = -1;
	f->p_type = -1;
    }

    if (check_feature(cpm_ptr->pred_b_ptr->f, "サ変") && 
	check_feature(cpm_ptr->pred_b_ptr->f, "非用言格解析")) {
	f->p_sahen_flag = 1;
    }
    else {
	f->p_sahen_flag = 0;
    }

    f->p_cf_subject_flag = cf_match_element(cf_ptr->sm[n], "主体", FALSE) ? 1 : 0;
    f->p_cf_sentence_flag = cf_match_element(cf_ptr->sm[n], "補文", TRUE) ? 1 : 0;
    f->p_n_modify_flag = check_feature(cpm_ptr->pred_b_ptr->f, "係:連格") ? 1 : 0;

    if ((level = check_feature(cpm_ptr->pred_b_ptr->f, "レベル"))) {
	strcpy(f->p_dep_p_level, level + 7);
    }
    else {
	f->p_dep_p_level[0] = '\0';
    }
}

/*==================================================================*/
E_FEATURES *SetEllipsisFeatures(SENTENCE_DATA *s, SENTENCE_DATA *cs, 
				CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, 
				TAG_DATA *bp, CASE_FRAME *cf_ptr, int n, int loc, 
				SENTENCE_DATA *vs, TAG_DATA *vp)
/*==================================================================*/
{
    E_FEATURES *f;
    char *level;

    f = (E_FEATURES *)malloc_data(sizeof(E_FEATURES), "SetEllipsisFeatures");

    /* 類似度計算 */
    f->pos = MATCH_NONE;
    if (cpm_ptr->cf.type == CF_PRED) {
	f->similarity = calc_similarity_word_cf_with_sm(bp, cf_ptr, n, &f->pos);
    }
    else {
	f->similarity = calc_similarity_word_cf(bp, cf_ptr, n, &f->pos);

	/* 意味素マッチ (SVMには入っていない) */
	f->match_sm_flag = cf_match_sm_thesaurus(bp, cf_ptr, n);
    }
    f->frequency = f->similarity > 1.0 ? cf_ptr->ex_freq[n][f->pos] : 0; /* 用例の頻度 */

    if (vp) {
	f->event1 = get_cf_event_value(vp->cpm_ptr->cmm[0].cf_ptr, cmm_ptr->cf_ptr);
	f->event2 = get_cf_event_value(cmm_ptr->cf_ptr, vp->cpm_ptr->cmm[0].cf_ptr);

	f->c_pp = GetCandCase(vp->cpm_ptr, &(vp->cpm_ptr->cmm[0]), bp);

	if ((level = check_feature(vp->f, "レベル"))) {
	    strcpy(f->c_dep_p_level, level + 7);
	}
	else {
	    f->c_dep_p_level[0] = '\0';
	}
	f->c_dep_mc_flag = check_feature(vp->f, "主節") ? 1 : 0;
	f->c_n_modify_flag = check_feature(vp->f, "係:連格") ? 1 : 0;
    }
    else {
	f->event1 = -1;
	f->event2 = -1;

	f->c_pp = -1;
	f->c_dep_p_level[0] = '\0';
	f->c_dep_mc_flag = 0;
	f->c_n_modify_flag = 0;
    }

    f->c_distance = cs - vs;
    if (s == vs) {
	/* | n v | tv |  or  | n v tv | */
	f->c_dist_bnst = CountBnstDistance(s, bp->num, cs, cpm_ptr->pred_b_ptr->num);
	f->c_fs_flag = s->Sen_num == 1 ? 1 : 0;
	if (f->c_distance > 0 || 
	    (f->c_distance == 0 && bp->num < cpm_ptr->pred_b_ptr->num)) {
	    f->c_prev_p_flag = 1;
	}
	else {
	    f->c_prev_p_flag = 0;
	}
	if (f->c_distance == 0 && 
	    bp->num < cpm_ptr->pred_b_ptr->num && 
	    bp->dpnd_head > cpm_ptr->pred_b_ptr->num) {
	    f->c_get_over_p_flag = 1;
	}
	else {
	    f->c_get_over_p_flag = 0;
	}
    }
    else {
	/* | n | v | tv |  or  | n | v tv | : vに指示対象があると思う */
	f->c_dist_bnst = CountBnstDistance(vs, vp->num, cs, cpm_ptr->pred_b_ptr->num);
	f->c_fs_flag = vs->Sen_num == 1 ? 1 : 0; /* always 0 */
	if (f->c_distance > 0 || 
	    (f->c_distance == 0 && vp->num < cpm_ptr->pred_b_ptr->num)) {
	    f->c_prev_p_flag = 1;
	}
	else {
	    f->c_prev_p_flag = 0;
	}
	if (f->c_distance == 0 && 
	    vp->num < cpm_ptr->pred_b_ptr->num && 
	    vp->dpnd_head > cpm_ptr->pred_b_ptr->num) {
	    f->c_get_over_p_flag = 1;
	}
	else {
	    f->c_get_over_p_flag = 0;
	}
    }
    f->c_location = loc;
    f->c_topic_flag = check_feature(bp->f, "主題表現") ? 1 : 0;
    f->c_no_topic_flag = check_feature(bp->f, "準主題表現") ? 1 : 0;
    f->c_in_cnoun_flag = bp->inum != 0 ? 1 : 0;
    f->c_subject_flag = sm_match_check(sm2code("主体"), bp->SM_code, 
				       check_feature(bp->f, "Ｔ固有一般展開禁止") ? SM_NO_EXPAND_NE : SM_EXPAND_NE) ? 1 : 0;
    f->c_sm_none_flag = f->similarity < 0 ? 1 : 0;
    f->c_extra_tag = -1;

    /* 発話タイプに関するfeature */
    if (OptAddSvmFeatureUtype) {
	f->utype = get_utype(bp);
    }
    else {
	f->utype = 0;
    }

    /* 談話構造の深さ */
    if (OptAddSvmFeatureDiscourseDepth) {
	f->discourse_depth = get_discourse_depth(bp);
    }
    else {
	f->discourse_depth = 0;
    }

    /* 用言に関するfeatureを設定 */
    SetEllipsisFeaturesForPred(f, cpm_ptr, cf_ptr, n);

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

    if (!strcmp(ExtraTags[tag], "不特定-人") && 
	cf_match_element(cf_ptr->sm[n], "主体", FALSE)) {
	f->similarity = (float)EX_match_subject / 11;
    }
    else {
	f->similarity = -1;
    }
    f->c_pp = -1;
    f->c_distance = 0;
    f->c_dist_bnst = 0;
    f->c_extra_tag = tag;

    if (OptAddSvmFeatureUtype) {
	f->utype = UTYPE_OTHERS;
    }
    else {
	f->utype = 0;
    }

    if (OptAddSvmFeatureDiscourseDepth) {
	f->discourse_depth = 1.0;
    }
    else {
	f->discourse_depth = 0;
    }

    /* 用言に関するfeatureを設定 */
    SetEllipsisFeaturesForPred(f, cpm_ptr, cf_ptr, n);

    return f;
}

/*==================================================================*/
      float classify_by_learning(char *ecp, int pp, int method)
/*==================================================================*/
{
    if (OptLearn == TRUE) {
	return -1;
    }

    if (method == OPT_SVM) {
#ifdef USE_SVM
	return svm_classify(ecp, pp);
#endif
    }
    else if (method == OPT_DT) {
	return dt_classify(ecp, pp);
    }
    return -1;
}

/*==================================================================*/
 int ScoreCheckCore(CASE_FRAME *cf_ptr, int n, float score, int pos)
/*==================================================================*/
{
    if (MatchPP(cf_ptr->pp[n][0], "ニ")) {
	if (score > AntecedentDecideThresholdForNi) {
	    return 1;
	}
	else if (pos == MATCH_SUBJECT) {
	    return 1;
	}
    }
    else {
	if ((cf_ptr->type == CF_PRED && score > AntecedentDecideThresholdPredGeneral) || 
	    (cf_ptr->type == CF_NOUN && score > AntecedentDecideThresholdForNoun)) {
	    return 1;
	}
    }
    return 0;
}

/*==================================================================*/
	      int ScoreCheck(CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{
    /* 学習用featureを出力するときは候補をすべて出す */
    if (OptLearn == TRUE || (OptDiscFlag & OPT_DISC_TWIN_CAND)) {
	return 0;
    }
    /* 学習器の出力がpositiveなら 1 */
    else if (OptDiscFlag & OPT_DISC_CLASS_ONLY) {
	if (maxs) {
	    return 1;
	}
    }
    return ScoreCheckCore(cf_ptr, n, maxscore, maxpos);
}

/*==================================================================*/
void push_cand(E_FEATURES *ef, SENTENCE_DATA *s, TAG_DATA *tp, char *tag, 
	       CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{
    /* 解析時には、特殊タグ以外のスコアをチェック */
    if (OptLearn == FALSE && 
	tag == NULL && !ScoreCheckCore(cf_ptr, n, ef->similarity, 0)) {
	return;
    }

    while (cand_num >= cand_num_max) {
	if (cand_num_max == 0) {
	    cand_num_max = 1;
	    ante_cands = (E_CANDIDATE *)malloc_data(sizeof(E_CANDIDATE) * cand_num_max, 
						  "push_cand");
	}
	else {
	    ante_cands = (E_CANDIDATE *)realloc_data(ante_cands, 
						     sizeof(E_CANDIDATE) * (cand_num_max <<= 1), 
						     "push_cand");
	}
    }

    (ante_cands + cand_num)->ef = ef;
    (ante_cands + cand_num)->s = s;
    (ante_cands + cand_num)->tp = tp;
    (ante_cands + cand_num)->tag = tag;
    cand_num++;
}

/*==================================================================*/
			  void clear_cands()
/*==================================================================*/
{
    int i;

    for (i = 0; i < cand_num; i++) {
	free((ante_cands + i)->ef);
    }
    cand_num = 0;
}

/*==================================================================*/
 int classify_twin_candidate(SENTENCE_DATA *sp, ELLIPSIS_MGR *em_ptr,
			     CF_PRED_MGR *cpm_ptr)
/*==================================================================*/
{
    int i, j, max_num = 0;
    char *cp, feature_buffer[DATA_LEN];
    float score, max = 0;

    if (cand_num == 0) {
	return 0;
    }
    else if (cand_num > 1) {
	if (OptDiscFlag & OPT_DISC_RANKING) {
	    E_SVM_FEATURES *ecf;

	    max = -100000;
	    for (i = 0; i < cand_num; i++) {
		ecf = EllipsisFeatures2EllipsisSvmFeatures((ante_cands + i)->ef);
		cp = EllipsisSvmFeatures2String(ecf);

		if (OptLearn == TRUE) {
		    /* 学習FEATURE */
		    EllipsisSvmFeaturesString2Feature(em_ptr, cpm_ptr, cp, 
						      (ante_cands + i)->tp ? (ante_cands + i)->tp->head_ptr->Goi : (ante_cands + i)->tag, 
						      (ante_cands + i)->ef->p_pp, 
						      (ante_cands + i)->s ? ((ante_cands + i)->s->KNPSID ? (ante_cands + i)->s->KNPSID + 5 : "?") : "-1", 
						      (ante_cands + i)->tp ? (ante_cands + i)->tp->num : -1, 
						      (ante_cands + i)->ef->c_location);
		}
		else {
		    score = classify_by_learning(cp, (ante_cands + i)->ef->p_pp, OptDiscPredMethod);

		    if (max < score) {
			max = score;
			max_num = i;
		    }

		    /* 省略候補 */
		    sprintf(feature_buffer, "C用;%s;%s;%s;%d;%d;%.3f|%.3f", 
			    (ante_cands + i)->tp ? (ante_cands + i)->tp->head_ptr->Goi : (ante_cands + i)->tag, 
			    pp_code_to_kstr_in_context(cpm_ptr, (ante_cands + i)->ef->p_pp), 
			    loc_code_to_str((ante_cands + i)->ef->c_location), 
			    (ante_cands + i)->ef->c_distance, (ante_cands + i)->tp ? (ante_cands + i)->tp->num : -1, 
			    (ante_cands + i)->ef->similarity, score);
		    assign_cfeature(&(em_ptr->f), feature_buffer);
		}

		free(ecf);
		free(cp);
	    }

	    if (OptLearn == TRUE) {
		return 0;
	    }
	}
	else {
	    E_TWIN_CAND_SVM_FEATURES *f;
	    int *vote;

	    vote = (int *)malloc_data(sizeof(int) * cand_num, "classify_twin_candidate");
	    for (i = 0; i < cand_num; i++) {
		vote[i] = 0;
	    }

	    for (i = 0; i < cand_num - 1; i++) {
		for (j = i + 1; j < cand_num; j++) {
		    f = MakeTwinCandSvmFeatures((ante_cands + i)->ef, (ante_cands + j)->ef);
		    cp = TwinCandSvmFeatures2String(f);

		    if (OptLearn == TRUE) {
			/* 学習FEATURE */
			TwinCandSvmFeaturesString2Feature(em_ptr, cp, ante_cands + i, ante_cands + j);
		    }
		    else {
			score = classify_by_learning(cp, (ante_cands + i)->ef->p_pp, OptDiscPredMethod);

			if (score > 0) {
			    vote[i]++;
			}
			else {
			    vote[j]++;
			}
		    }

		    free(f);
		    free(cp);
		}
	    }

	    if (OptLearn == TRUE) {
		return 0;
	    }

	    for (i = 0; i < cand_num; i++) {
		if (max < vote[i]) {
		    max = vote[i];
		    max_num = i;
		}

		/* 省略候補 */
		sprintf(feature_buffer, "C用;%s;%s;%s;%d;%d;%.3f|%.3f", 
			(ante_cands + i)->tp ? (ante_cands + i)->tp->head_ptr->Goi : (ante_cands + i)->tag, 
			pp_code_to_kstr_in_context(cpm_ptr, (ante_cands + i)->ef->p_pp), 
			loc_code_to_str((ante_cands + i)->ef->c_location), 
			(ante_cands + i)->ef->c_distance, (ante_cands + i)->tp ? (ante_cands + i)->tp->num : -1, 
			(ante_cands + i)->ef->similarity, (float)vote[i]/cand_num);
		assign_cfeature(&(em_ptr->f), feature_buffer);
	    }
	    free(vote);
	}
    }
    else {
	/* cand_num == 1 */
	if (OptLearn == TRUE) {
	    return 0;
	}
	max = 1;
    }

    /* 決定 */
    maxrawscore = (ante_cands + max_num)->ef->similarity;
    maxscore = maxrawscore;
    maxs = (ante_cands + max_num)->s;
    maxpos = (ante_cands + max_num)->ef->pos;
    maxi = (ante_cands + max_num)->tp ? (ante_cands + max_num)->tp->num : -1;
    maxtag = (ante_cands + max_num)->tag;

    return 1;
}

/*==================================================================*/
void EllipsisDetectForVerbSubcontractExtraTagsWithLearning(SENTENCE_DATA *cs, ELLIPSIS_MGR *em_ptr, 
							   CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int l, 
							   int tag, CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{
    E_FEATURES *ef;
    E_SVM_FEATURES *esf;
    float score;
    char *ecp, feature_buffer[DATA_LEN];

    ef = SetEllipsisFeaturesExtraTags(tag, cpm_ptr, cf_ptr, n);

    if (OptDiscFlag & OPT_DISC_TWIN_CAND) {
	push_cand(ef, NULL, NULL, ExtraTags[tag], cf_ptr, n);
	return;
    }

    esf = EllipsisFeatures2EllipsisSvmFeatures(ef);
    ecp = EllipsisSvmFeatures2String(esf);

    /* 学習FEATURE */
    EllipsisSvmFeaturesString2Feature(em_ptr, cpm_ptr, ecp, ExtraTags[tag], cf_ptr->pp[n][0], 
				      "?", -1, -1);

    score = classify_by_learning(ecp, cpm_ptr->cf.type == CF_PRED ? cf_ptr->pp[n][0] : pp_kstr_to_code("ノ"), 
				 cpm_ptr->cf.type == CF_PRED ? OptDiscPredMethod : OptDiscNounMethod);

    if (score > maxscore) {
	maxscore = score;
	maxrawscore = 1.0;
	maxtag = ExtraTags[tag];
    }

    free(ef);
    free(esf);
    free(ecp);
}

/*==================================================================*/
void _EllipsisDetectForVerbSubcontractWithLearning(SENTENCE_DATA *s, SENTENCE_DATA *cs, ELLIPSIS_MGR *em_ptr, 
						   CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int l, 
						   TAG_DATA *bp, CASE_FRAME *cf_ptr, int n, int loc, 
						   SENTENCE_DATA *vs, TAG_DATA *vp)
/*==================================================================*/
{
    E_FEATURES *ef;
    E_SVM_FEATURES *esf;
    char *ecp, feature_buffer[DATA_LEN];
    float score, similarity;

    ef = SetEllipsisFeatures(s, cs, cpm_ptr, cmm_ptr, bp, cf_ptr, n, loc, vs, vp);

    if (cpm_ptr->cf.type == CF_PRED && (OptDiscFlag & OPT_DISC_TWIN_CAND)) {
	/* 解析時に、すでに他の格の指示対象になっているときはだめ */
	if (OptLearn == TRUE || 
	    !CheckHaveEllipsisComponent(cpm_ptr, cmm_ptr, l, bp->head_ptr->Goi)) {
	    push_cand(ef, s, bp, NULL, cf_ptr, n);
	}
	return;
    }

    esf = EllipsisFeatures2EllipsisSvmFeatures(ef);
    ecp = EllipsisSvmFeatures2String(esf);

    /* 学習FEATURE */
    EllipsisSvmFeaturesString2Feature(em_ptr, cpm_ptr, ecp, bp->head_ptr->Goi, cf_ptr->pp[n][0], 
				      s->KNPSID ? s->KNPSID + 5 : "?", bp->num, loc);

    /* すでに他の格の指示対象になっているときはだめ */
    if (CheckHaveEllipsisComponent(cpm_ptr, cmm_ptr, l, bp->head_ptr->Goi)) {
	free(ef);
	free(esf);
	free(ecp);
	return;
    }

    if (cpm_ptr->cf.type == CF_NOUN) {
	/* 名詞の場合: exact match or (<sm> match and sim > 0.6) */
	if (ef->similarity >= 1.0) {
	    score = classify_by_learning(ecp, pp_kstr_to_code("ノ"), OptDiscNounMethod);
	    similarity = ef->similarity;
	}
	else if (ef->match_sm_flag && ef->similarity > 0.6) {
	    score = classify_by_learning(ecp, pp_kstr_to_code("ノ"), OptDiscNounMethod);
	    similarity = (float)EX_match_subject / 11; /* 同点の候補比較のため一定の点を与える */
	    ef->pos = MATCH_SUBJECT;
	}
	else {
	    score = -1;
	    similarity = -1;
	}
    }
    else {
	score = classify_by_learning(ecp, cf_ptr->pp[n][0], OptDiscPredMethod);
	similarity = ef->similarity;
    }

    /* 省略候補 */
    sprintf(feature_buffer, "C用;%s;%s;%s;%d;%d;%.3f|%.3f", bp->head_ptr->Goi, 
	    pp_code_to_kstr_in_context(cpm_ptr, cf_ptr->pp[n][0]), 
	    loc_code_to_str(loc), 
	    ef->c_distance, bp->num, 
	    ef->similarity, score);
    assign_cfeature(&(em_ptr->f), feature_buffer);

    /* classifierがpositiveと分類 */
    if (score > 0) {
	if (!(OptDiscFlag & OPT_DISC_CLASS_ONLY)) {
	    score = similarity;
	}

	/* 類似度0を入れるにはここを >= にする */
	if (score > maxscore) {
	    maxscore = score;
	    maxrawscore = ef->similarity;
	    maxs = s;
	    maxpos = ef->pos;
	    maxi = bp->num;
	    maxtag = NULL;
	}
    }

    free(ef);
    free(esf);
    free(ecp);
}

/*==================================================================*/
int EllipsisDetectForVerbSubcontractExtraTags(SENTENCE_DATA *cs, ELLIPSIS_MGR *em_ptr, 
					      CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int l, 
					      int tag, CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{

    if (cpm_ptr->cf.type == CF_PRED && 
	(OptDiscPredMethod == OPT_SVM || OptDiscPredMethod == OPT_DT)) {
	EllipsisDetectForVerbSubcontractExtraTagsWithLearning(cs, em_ptr, cpm_ptr, cmm_ptr, l, 
							      tag, cf_ptr, n);
    }
    else {
	E_FEATURES *ef;
	E_SVM_FEATURES *esf;
	char *ecp;

	ef = SetEllipsisFeaturesExtraTags(tag, cpm_ptr, cf_ptr, n);

	if (OptDiscFlag & OPT_DISC_TWIN_CAND) {
	    push_cand(ef, NULL, NULL, ExtraTags[tag], cf_ptr, n);
	    return;
	}

	esf = EllipsisFeatures2EllipsisSvmFeatures(ef);
	ecp = EllipsisSvmFeatures2String(esf);
	EllipsisSvmFeaturesString2Feature(em_ptr, cpm_ptr, ecp, ExtraTags[tag], cf_ptr->pp[n][0], 
					  "?", -1, -1);

	free(ef);
	free(esf);
	free(ecp);
    }

    return 0;
}

/*==================================================================*/
void _EllipsisDetectForVerbSubcontract(SENTENCE_DATA *s, SENTENCE_DATA *cs, ELLIPSIS_MGR *em_ptr, 
				       CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int l, 
				       TAG_DATA *bp, CASE_FRAME *cf_ptr, int n, int loc, 
				       SENTENCE_DATA *vs, TAG_DATA *vp)
/*==================================================================*/
{
    E_FEATURES *ef;
    E_SVM_FEATURES *esf;
    char *ecp, feature_buffer[DATA_LEN];
    float score;

    /* 学習FEATURE */
    ef = SetEllipsisFeatures(s, cs, cpm_ptr, cmm_ptr, bp, cf_ptr, n, loc, vs, vp);

    if (cpm_ptr->cf.type == CF_PRED && (OptDiscFlag & OPT_DISC_TWIN_CAND)) {
	/* 解析時に、すでに他の格の指示対象になっているときはだめ */
	if (OptLearn == TRUE || 
	    !CheckHaveEllipsisComponent(cpm_ptr, cmm_ptr, l, bp->head_ptr->Goi)) {
	    push_cand(ef, s, bp, NULL, cf_ptr, n);
	}
	return;
    }

    esf = EllipsisFeatures2EllipsisSvmFeatures(ef);
    ecp = EllipsisSvmFeatures2String(esf);
    EllipsisSvmFeaturesString2Feature(em_ptr, cpm_ptr, ecp, bp->head_ptr->Goi, cf_ptr->pp[n][0], 
				      s->KNPSID ? s->KNPSID + 5 : "?", bp->num, loc);

    /* すでに他の格の指示対象になっているときはだめ */
    if (CheckHaveEllipsisComponent(cpm_ptr, cmm_ptr, l, bp->head_ptr->Goi)) {
	free(ef);
	free(esf);
	free(ecp);
	return;
    }

    if (cpm_ptr->cf.type == CF_NOUN) {
	/* 名詞の場合: exact match or (<sm> match and sim > 0.6) */
	if (ef->similarity >= 1.0) {
	    score = ef->similarity;
	}
	else if (ef->match_sm_flag && ef->similarity > 0.6) {
	    score = (float)EX_match_subject / 11; /* 同点の候補比較のため一定の点を与える */
	    ef->pos = MATCH_SUBJECT;
	}
	else {
	    score = -1;
	}
    }
    else {
	score = ef->similarity;
    }

    /* 省略候補 */
    sprintf(feature_buffer, "C用;%s;%s;%s;%d;%d;%.3f|%.3f", bp->head_ptr->Goi, 
	    pp_code_to_kstr_in_context(cpm_ptr, cf_ptr->pp[n][0]), 
	    loc_code_to_str(loc), 
	    ef->c_distance, bp->num, 
	    ef->similarity, ef->similarity);
    assign_cfeature(&(em_ptr->f), feature_buffer);

    if (score > maxscore) {
	maxscore = score;
	maxrawscore = score;
	maxs = s;
	maxpos = ef->pos;
	maxi = bp->num;
	maxtag = NULL;
    }

    free(ef);
    free(esf);
    free(ecp);
}

/*==================================================================*/
int EllipsisDetectForVerbSubcontract(SENTENCE_DATA *s, SENTENCE_DATA *cs, ELLIPSIS_MGR *em_ptr, 
				     CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int l, 
				     TAG_DATA *bp, CASE_FRAME *cf_ptr, int n, int loc, 
				     SENTENCE_DATA *vs, TAG_DATA *vp)
/*==================================================================*/
{
    if ((cpm_ptr->cf.type == CF_PRED && OptDiscPredMethod != OPT_NORMAL) || 
	(cpm_ptr->cf.type == CF_NOUN && OptDiscNounMethod != OPT_NORMAL)) {
	_EllipsisDetectForVerbSubcontractWithLearning(s, cs, em_ptr, 
						      cpm_ptr, cmm_ptr, l, 
						      bp, cf_ptr, n, loc, vs, vp);
    }
    else {
	_EllipsisDetectForVerbSubcontract(s, cs, em_ptr, 
					  cpm_ptr, cmm_ptr, l, 
					  bp, cf_ptr, n, loc, vs, vp);
    }

    return 0;
}

/*==================================================================*/
int AppendToCF(CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int l, 
	       TAG_DATA *b_ptr,
	       CASE_FRAME *cf_ptr, int n, float maxscore, int maxpos, SENTENCE_DATA *maxs)
/*==================================================================*/
{
    /* 省略の指示対象を入力側の格フレームに入れる */

    CASE_FRAME *c_ptr = &(cpm_ptr->cf);
    int d, demonstrative, old_score;

    if (c_ptr->element_num >= CF_ELEMENT_MAX) {
	return 0;
    }

    /* 指示詞の場合 */
    if (cmm_ptr->result_lists_p[l].flag[n] != UNASSIGNED) {
	d = cmm_ptr->result_lists_p[l].flag[n];
	old_score = cmm_ptr->result_lists_p[l].score[n];
	demonstrative = 1;
    }
    else {
	d = c_ptr->element_num;
	demonstrative = 0;
    }

    /* 対応情報を追加 */
    cmm_ptr->result_lists_p[l].flag[n] = d;
    cmm_ptr->result_lists_d[l].flag[d] = n;
    cmm_ptr->result_lists_p[l].pos[n] = maxpos;

    if (cpm_ptr->cf.type == CF_PRED && 
	(OptDiscPredMethod == OPT_SVM || OptDiscPredMethod == OPT_DT)) {
	if (maxscore < 0) {
	    cmm_ptr->result_lists_p[l].score[n] = 0;
	}
	else {
	    cmm_ptr->result_lists_p[l].score[n] = maxscore > 1 ? EX_match_exact : maxpos == MATCH_SUBJECT ? EX_match_subject : maxscore * 11;
	}
    }
    else {
	cmm_ptr->result_lists_p[l].score[n] = maxscore > 1 ? EX_match_exact : maxpos == MATCH_SUBJECT ? EX_match_subject : *(EX_match_score+(int)(maxscore * 7));
    }

    c_ptr->pp[d][0] = cf_ptr->pp[n][0];
    c_ptr->pp[d][1] = END_M;
    c_ptr->oblig[d] = TRUE;
    cpm_ptr->elem_b_ptr[d] = b_ptr;
    cpm_ptr->elem_s_ptr[d] = maxs;
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
	cmm_ptr->pure_score[l] -= old_score;
    }
    return 1;
}

/*==================================================================*/
int DeleteFromCF(ELLIPSIS_MGR *em_ptr, CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int l)
/*==================================================================*/
{
    int i, count = 0;

    /* 省略の指示対象を入力側の格フレームから削除する */

    for (i = 0; i < cpm_ptr->cf.element_num; i++) {
	if (cpm_ptr->elem_b_num[i] <= -2) {
	    cmm_ptr->result_lists_p[l].flag[cmm_ptr->result_lists_d[l].flag[i]] = -1;
	    cmm_ptr->result_lists_d[l].flag[i] = -1;
	    count++;
	}
    }
    cpm_ptr->cf.element_num -= count;
    return 1;
}


/*==================================================================*/
  int CheckAppropriateCandidate(SENTENCE_DATA *s, SENTENCE_DATA *cs,
				CF_PRED_MGR *cpm_ptr, TAG_DATA *bp, int pp, 
				CASE_FRAME *cf_ptr, int n, int loc, int flag)
/*==================================================================*/
{
    /* bp: candidate antecedent
       cpm_ptr->pred_b_ptr: target predicate */

    /* flag == 1 (名詞のとき): 用言より後でもOKにする */

    if (Bcheck[cs - s][bp->num] || /* すでにチェックした */
	!check_feature(bp->f, "先行詞候補") || 
	(s == cs && bp->num == cpm_ptr->pred_b_ptr->num)) {
	return FALSE;
    }

    /* 学習時は、基本的条件のチェックのみ */
    if (OptLearn == TRUE) {
	return TRUE;
    }

    /* 用言と同じ表記はだめ */
    if (!strcmp(bp->head_ptr->Goi, cpm_ptr->pred_b_ptr->head_ptr->Goi)) {
	return FALSE;
    }

    if (s == cs && /* 対象文 */
	((bp->num >= cpm_ptr->pred_b_ptr->num && /* 用言より後は許さない */
	  (cpm_ptr->cf.type == CF_PRED || 
	   (!flag && bp->dpnd_head != cpm_ptr->pred_b_ptr->dpnd_head))) || /* 名詞: 親が同じとき以外はだめ */
	 (!check_feature(bp->f, "係:連用") && 
	  bp->dpnd_head == cpm_ptr->pred_b_ptr->num) || /* 用言に直接係らない (連用は可) */
	 (cpm_ptr->pred_b_ptr->dpnd_head == bp->num) || /* 用言が対象に係らない */
	 CheckCaseComponent(cpm_ptr, bp))) { /* 元用言がその文節を格要素としてもたない */
	return FALSE;
    }

    if (0 && check_feature(cpm_ptr->pred_b_ptr->f, "用言")) {
	/* 省略格がガ格のときの条件
	   ガ格と格解析された要素しか許さない */
	if (MatchPP(cf_ptr->pp[n][0], "ガ")) {
	    /* ガ格で格解析されたものだけ OK */
	    if (bp->inum == 0 && bp->pred_b_ptr && 
		!check_feature(bp->f, "係:ノ格")) { /* 複合名詞の解析結果は用いない */
		/* 無条件許可 */
		if (pp == -2) {
		    return TRUE;
		}
		else if (pp == -1) {
		    pp = GetCandCase(bp->pred_b_ptr->cpm_ptr, &(bp->pred_b_ptr->cpm_ptr->cmm[0]), bp);
		}
		/* ガ, ガ２格のときのみ許可 */
		if (pp > 0 && 
		    (MatchPP(pp, "ガ") || 
		     MatchPP(pp, "ガ２"))) {
		    return TRUE;
		}
		else {
		    return FALSE;
		}
	    }
	    else {
		return FALSE;
	    }
	}
    }
    return TRUE;
}

/*==================================================================*/
int EllipsisDetectRecursive(SENTENCE_DATA *s, SENTENCE_DATA *cs, ELLIPSIS_MGR *em_ptr, 
			    CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int l, 
			    TAG_DATA *tp, CASE_FRAME *cf_ptr, int n, int loc)
/*==================================================================*/
{
    int i;

    /* 省略要素となるための条件 */
    if (tp->para_top_p == TRUE || 
	!CheckAppropriateCandidate(s, cs, cpm_ptr, tp, -1, cf_ptr, n, 0, FALSE)) {
	if (!Bcheck[cs - s][tp->num]) {
	    Bcheck[cs - s][tp->num] = 1;
	}
    }
    else {
	if ((OptDiscFlag & OPT_DISC_BEST) || 
	    cpm_ptr->cf.type == CF_NOUN || 
	    loc == LOC_OTHERS || 
	    ((loc == LOC_S1_OTHERS || loc == LOC_S2_OTHERS) && s != cs) || 
	    (loc == LOC_PRE_OTHERS && s == cs && tp->num < cpm_ptr->pred_b_ptr->num) || 
	    (loc == LOC_POST_OTHERS && s == cs && tp->num > cpm_ptr->pred_b_ptr->num)) {
	    EllipsisDetectForVerbSubcontract(s, cs, em_ptr, cpm_ptr, cmm_ptr, l, tp, cf_ptr, n, loc, s, tp->pred_b_ptr);
	    Bcheck[cs - s][tp->num] = 1;
	    /* BEST解を求めるとき以外は、スコアをチェックしてreturn */
	    if (!(OptDiscFlag & OPT_DISC_BEST) && 
		ScoreCheck(cf_ptr, n)) {
		return 1;
	    }
	}
    }

    for (i = 0; tp->child[i]; i++) {
	if (EllipsisDetectRecursive(s, cs, em_ptr, cpm_ptr, cmm_ptr, l, tp->child[i], cf_ptr, n, loc) == 1) {
	    return 1;
	}
    }
    return 0;
}

/*==================================================================*/
	int CheckLocation(SENTENCE_DATA *s, SENTENCE_DATA *cs,
			  CF_PRED_MGR *cpm_ptr, TAG_DATA *tp, int loc)
/*==================================================================*/
{
    if (loc == LOC_S1_OTHERS || loc == LOC_S2_OTHERS) {
	if (s != cs) {
	    return 1;
	}
	else {
	    return 0;
	}
    }
    else if (loc == LOC_PRE_OTHERS) {
	if (s == cs && tp->num < cpm_ptr->pred_b_ptr->num) {
	    return 1;
	}
	else {
	    return 0;
	}
    }
    else if (loc == LOC_POST_OTHERS) {
	if (s == cs && tp->num > cpm_ptr->pred_b_ptr->num) {
	    return 1;
	}
	else {
	    return 0;
	}
    }
    return 1;
}

/*==================================================================*/
int EllipsisDetectRecursive2(SENTENCE_DATA *s, SENTENCE_DATA *cs, ELLIPSIS_MGR *em_ptr, 
			     CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int l, 
			     TAG_DATA *tp, CASE_FRAME *cf_ptr, int n, int loc, int rec_flag)
/*==================================================================*/
{
    int i;
    TAG_DATA *tp2;

    /* 自分、用言の格要素(省略込み)、子供(再帰)の順番にチェックする */

    /* 省略要素となるための条件 */
    if (tp->para_top_p == TRUE || 
	!CheckAppropriateCandidate(s, cs, cpm_ptr, tp, -1, cf_ptr, n, 0, FALSE)) {
	if (!Bcheck[cs - s][tp->num]) {
	    Bcheck[cs - s][tp->num] = 1;
	}
    }
    else {
	if ((OptDiscFlag & OPT_DISC_BEST) || 
	    cpm_ptr->cf.type == CF_NOUN || 
	    loc == LOC_OTHERS || 
	    ((loc == LOC_S1_OTHERS || loc == LOC_S2_OTHERS) && s != cs) || 
	    (loc == LOC_PRE_OTHERS && s == cs && tp->num < cpm_ptr->pred_b_ptr->num) || 
	    (loc == LOC_POST_OTHERS && s == cs && tp->num > cpm_ptr->pred_b_ptr->num)) {
	    EllipsisDetectForVerbSubcontract(s, cs, em_ptr, cpm_ptr, cmm_ptr, l, tp, cf_ptr, n, loc, s, tp->pred_b_ptr);
	    Bcheck[cs - s][tp->num] = 1;
	    /* BEST解を求めるとき以外は、スコアをチェックしてreturn */
	    if (!(OptDiscFlag & OPT_DISC_BEST) && 
		ScoreCheck(cf_ptr, n)) {
		return 1;
	    }
	}
    }

    /* 用言の格要素をチェック (省略を含む) */
    tp2 = tp;
    while (tp2->para_top_p) {
	tp2 = tp2->child[0];
    }
    SearchCaseComponent(s, cs, em_ptr, cpm_ptr, cmm_ptr, l, 
			tp2, cf_ptr, n, loc);

    if (!(OptDiscFlag & OPT_DISC_BEST) && 
	ScoreCheck(cf_ptr, n)) {
	return 1;
    }

    /* 子供をたどる */
    if (rec_flag == TRUE) {
	for (i = 0; tp->child[i]; i++) {
	    if (EllipsisDetectRecursive2(s, cs, em_ptr, cpm_ptr, cmm_ptr, l, tp->child[i], cf_ptr, n, loc, TRUE) == 1) {
		return 1;
	    }
	}
    }

    return 0;
}

/*==================================================================*/
int EllipsisDetectOne(SENTENCE_DATA *s, SENTENCE_DATA *cs, ELLIPSIS_MGR *em_ptr, 
		      CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int l, 
		      TAG_DATA *tp, CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{
    int i;

    while (tp->para_top_p) {
	tp = tp->child[0];
    }

    /* 省略要素となるための条件 */
    if (CheckAppropriateCandidate(s, cs, cpm_ptr, tp, -1, cf_ptr, n, 0, FALSE)) {
	EllipsisDetectForVerbSubcontract(s, cs, em_ptr, cpm_ptr, cmm_ptr, l, tp, cf_ptr, n, LOC_OTHERS, s, tp->pred_b_ptr);
	if (ScoreCheck(cf_ptr, n)) {
	    return 1;
	}
    }
    return 0;
}

/*==================================================================*/
int SearchCompoundChild(SENTENCE_DATA *s, SENTENCE_DATA *cs, ELLIPSIS_MGR *em_ptr, 
			CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int l, 
			TAG_DATA *tp, CASE_FRAME *cf_ptr, int n, int loc, int eflag)
/*==================================================================*/
{
    int i;

    /* 並列を吸収 */
    while (tp->para_top_p) {
	tp = tp->child[0];
    }

    for (i = 0; tp->child[i]; i++) {
	/* ノ格, 連体の(直接の)子供をチェック (複合名詞の2つ以上前はみていない) */
	if ((check_feature(tp->child[i]->f, "係:ノ格") || 
	     check_feature(tp->child[i]->f, "係:連体") || 
	     check_feature(tp->child[i]->f, "係:隣")) && 
	    CheckAppropriateCandidate(s, cs, cpm_ptr, tp->child[i], -2, cf_ptr, n, loc, FALSE)) {
	    EllipsisDetectForVerbSubcontract(s, cs, em_ptr, cpm_ptr, cmm_ptr, l, 
					     tp->child[i], cf_ptr, n, loc, s, tp->child[i]->pred_b_ptr);
	    /* 省略を補ったものでなければ */
	    if (!eflag) {
		Bcheck[cs - s][tp->child[i]->num] = 1;
	    }
	}
    }
    return 1;
}

/*==================================================================*/
     int _SearchCompoundChild(TAG_DATA *tp, int *lc, int lc_num)
/*==================================================================*/
{
    int i;

    /* 並列を吸収 */
    while (tp->para_top_p) {
	tp = tp->child[0];
    }

    for (i = 0; tp->child[i]; i++) {
	/* ノ格, 連体の(直接の)子供をチェック (複合名詞の2つ以上前はみていない) */
	if (check_feature(tp->child[i]->f, "係:ノ格") || 
	    check_feature(tp->child[i]->f, "係:連体") || 
	    check_feature(tp->child[i]->f, "係:隣")) {
	    if (!lc[tp->child[i]->num]) {
		lc[tp->child[i]->num] = lc_num;
	    }
	}
    }
    return 1;
}

/*==================================================================*/
	      TAG_DATA** ListPredChildren(TAG_DATA *tp)
/*==================================================================*/
{
    int i, count = 0, size = PARA_PART_MAX;
    TAG_DATA **ret;

    ret = (TAG_DATA **)malloc_data(sizeof(TAG_DATA *) * size, "ListPredChildren");

    for (i = 0; tp->child[i]; i++) {
	if (check_feature(tp->child[i]->f, "格要素")) { /* 連用要素は除く */
	    ret[count++] = tp->child[i];
	}
    }

    if (tp->para_type == PARA_NORMAL) {
	TAG_DATA *tmp = tp->parent;
	while (tmp && tmp->para_top_p) {
	    for (i = 0; tmp->child[i]; i++) {
		/* 並列の用言(自分を含む)を除く */
		if (tmp->child[i]->para_type != PARA_NORMAL) {
		    if (count >= size - 1) {
			ret = (TAG_DATA **)realloc_data(ret, sizeof(TAG_DATA *) * (size <<= 1), 
							"ListPredChildren");
		    }
		    if (check_feature(tmp->child[i]->f, "格要素")) {
			ret[count++] = tmp->child[i];
		    }
		}
	    }
	    tmp = tmp->parent;
	}
    }

    ret[count] = NULL;
    return ret;
}

/*==================================================================*/
int SearchCaseComponent(SENTENCE_DATA *s, SENTENCE_DATA *cs, ELLIPSIS_MGR *em_ptr, 
			CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int l, 
			TAG_DATA *bp, CASE_FRAME *cf_ptr, int n, int loc)
/*==================================================================*/
{
    /* cpm_ptr: 省略格要素をもつ用言
       bp:      格要素の探索対象となっている用言文節
    */

    /* ★並列のNは? */

    int i, num, flag;
    TAG_DATA **children;

    /* 用言の格要素をチェック */
    if (bp->cpm_ptr) {
	if (bp->cpm_ptr->cmm[0].score != -2) {
	    /* 名詞: 親用言の格要素は位置が後でも許すflag */
	    flag = cpm_ptr->pred_b_ptr->dpnd_head == bp->cpm_ptr->pred_b_ptr->num ? TRUE : FALSE;

	    for (i = 0; i < bp->cpm_ptr->cmm[0].cf_ptr->element_num; i++) {
		num = bp->cpm_ptr->cmm[0].result_lists_p[0].flag[i];
		if (num != UNASSIGNED && 
		    CheckLocation(bp->cpm_ptr->elem_b_num[num] > -2 ? s : bp->cpm_ptr->elem_s_ptr[num], cs, 
				  cpm_ptr, bp->cpm_ptr->elem_b_ptr[num], loc) && 
		    CheckAppropriateCandidate(bp->cpm_ptr->elem_b_num[num] > -2 ? s : bp->cpm_ptr->elem_s_ptr[num], cs, 
					      cpm_ptr, bp->cpm_ptr->elem_b_ptr[num], bp->cpm_ptr->cmm[0].cf_ptr->pp[i][0], 
					      cf_ptr, n, loc, flag)) {
		    EllipsisDetectForVerbSubcontract(bp->cpm_ptr->elem_b_num[num] > -2 ? s : bp->cpm_ptr->elem_s_ptr[num], 
						     cs, em_ptr, cpm_ptr, cmm_ptr, l, 
						     bp->cpm_ptr->elem_b_ptr[num], 
						     cf_ptr, n, loc, s, bp);
		    /* 省略を補ったものでなければ */
		    if (bp->cpm_ptr->elem_b_num[num] > -2) {
			Bcheck[cs - s][bp->cpm_ptr->elem_b_ptr[num]->num] = 1;
		    }

		    /* ノ格の子供をチェック */
		    SearchCompoundChild(bp->cpm_ptr->elem_b_num[num] > -2 ? s : bp->cpm_ptr->elem_s_ptr[num], 
					cs, em_ptr, cpm_ptr, cmm_ptr, l, 
					bp->cpm_ptr->elem_b_ptr[num], 
					cf_ptr, n, loc, bp->cpm_ptr->elem_b_num[num] <= -2 ? 1 : 0);
		}
	    }
	}

	/* 格要素になっていない子供もチェック */
	children = ListPredChildren(bp->cpm_ptr->pred_b_ptr);
	for (i = 0; children[i]; i++) {
	    if (CheckAppropriateCandidate(s, cs, cpm_ptr, children[i], -2, cf_ptr, n, loc, FALSE)) {
		EllipsisDetectForVerbSubcontract(s, cs, em_ptr, cpm_ptr, cmm_ptr, l, 
						 children[i], 
						 cf_ptr, n, loc, s, bp);
		Bcheck[cs - s][children[i]->num] = 1;
		
		/* ノ格の子供をチェック */
		SearchCompoundChild(s, cs, em_ptr, cpm_ptr, cmm_ptr, l, 
				    children[i], 
				    cf_ptr, n, loc, 0);
	    }
	}
	free(children);
    }

    return 0;
}

/*==================================================================*/
int _SearchCaseComponent(SENTENCE_DATA *cs, TAG_DATA *bp, int **lc, int lc_num, int dist)
/*==================================================================*/
{
    /* cpm_ptr: 省略格要素をもつ用言
       bp:      格要素の探索対象となっている用言文節
    */

    int i, num, sent;
    TAG_DATA **children;

    /* 用言の格要素をチェック */
    if (bp->cpm_ptr) {
	if (bp->cpm_ptr->cmm[0].score != -2) {
	    for (i = 0; i < bp->cpm_ptr->cmm[0].cf_ptr->element_num; i++) {
		num = bp->cpm_ptr->cmm[0].result_lists_p[0].flag[i];
		if (num != UNASSIGNED) {
		    if (bp->cpm_ptr->elem_b_num[num] > -2) {
			sent = dist;
		    }
		    /* 省略 */
		    else {
			sent = dist + (cs - bp->cpm_ptr->elem_s_ptr[num]);
		    }

		    if (!lc[sent][bp->cpm_ptr->elem_b_ptr[num]->num]) {
			lc[sent][bp->cpm_ptr->elem_b_ptr[num]->num] = lc_num;
		    }

		    /* ノ格の子供をチェック */
		    _SearchCompoundChild(bp->cpm_ptr->elem_b_ptr[num], lc[sent], lc_num);
		}
	    }
	}

	/* 格要素になっていない子供もチェック */
	children = ListPredChildren(bp->cpm_ptr->pred_b_ptr);
	for (i = 0; children[i]; i++) {
	    if (!lc[dist][children[i]->num]) {
		lc[dist][children[i]->num] = lc_num;
	    }
	    /* ノ格の子供をチェック */
	    _SearchCompoundChild(children[i], lc[dist], lc_num);
	}
	free(children);
    }

    return 0;
}

/*==================================================================*/
int SearchRelatedComponent(SENTENCE_DATA *s, ELLIPSIS_MGR *em_ptr, 
			   CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int l, 
			   TAG_DATA *bp, CASE_FRAME *cf_ptr, int n, int loc)
/*==================================================================*/
{
    /* cpm_ptr: 省略格要素をもつ用言
       bp:      要素の探索対象となっている体言文節
    */

    int i, j;

    /* <PARA> */
    if (bp->para_top_p) {
	/* bpと並列になっている要素をチェック
	for (i = 0; bp->child[i]; i++) {
	    if (bp->child[i]->para_type == PARA_NORMAL && 
		bp->child[i]->num != bp->num && 
		!Bcheck[0][bp->child[i]->num]) {
		EllipsisDetectForVerbSubcontract(s, s, em_ptr, cpm_ptr, cmm_ptr, l, 
						     bp->child[i], cf_ptr, n);
		Bcheck[0][bp->child[i]->num] = 1;
	    }
	} */
	;
    }
    else {
	/* bpに係る要素をチェック */
	for (i = 0; bp->child[i]; i++) {
	    if (bp->child[i] == cpm_ptr->pred_b_ptr) continue;
	    if (bp->child[i]->para_top_p) {
		for (j = 0; bp->child[i]->child[j]; j++) {
		    if (bp->child[i]->child[j]->para_type == PARA_NORMAL && 
			!Bcheck[0][bp->child[i]->child[j]->num] && 
			CheckAppropriateCandidate(s, s, cpm_ptr, bp->child[i]->child[j], -1, cf_ptr, n, loc, FALSE)) {
			EllipsisDetectForVerbSubcontract(s, s, em_ptr, cpm_ptr, cmm_ptr, l, 
							 bp->child[i]->child[j], cf_ptr, n, loc, s, 
							 bp->child[i]->child[j]->pred_b_ptr);
			Bcheck[0][bp->child[i]->child[j]->num] = 1;
			/* return 1; */
		    }
		}
	    }
	    else if (!Bcheck[0][bp->child[i]->num] && 
		     CheckAppropriateCandidate(s, s, cpm_ptr, bp->child[i], -1, cf_ptr, n, loc, FALSE)) {
		EllipsisDetectForVerbSubcontract(s, s, em_ptr, cpm_ptr, cmm_ptr, l, 
						 bp->child[i], cf_ptr, n, loc, s, bp->child[i]->pred_b_ptr);
		Bcheck[0][bp->child[i]->num] = 1;
	    }
	}
    }
    return 0;
}

/*==================================================================*/
		      int check_mc(TAG_DATA *tp)
/*==================================================================*/
{
    if (check_feature(tp->f, "主節")) {
	/* check_feature(tp->f, "文末")) { */
	return 1;
    }
    return 0;
}

/*==================================================================*/
int SearchMC(SENTENCE_DATA *s, SENTENCE_DATA *cs, ELLIPSIS_MGR *em_ptr,
	     CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int l, 
	     CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{
    int i, flag = 0, dist;
    TAG_DATA *tp;

    dist = cs - s;

    for (i = s->Tag_num - 1; i >= 0; i--) {
	tp = s->tag_data + i;
	while (tp->para_top_p) {
	    tp = tp->child[0];
	}

	if (check_mc(tp)) {
	    flag = 1;
	    break;
	}
    }

    if (flag == 0) {
	tp = s->tag_data + s->Tag_num - 1;
	while (tp->para_top_p) {
	    tp = tp->child[0];
	}
    }

    SearchCaseComponent(s, cs, em_ptr, cpm_ptr, cmm_ptr, l, 
			tp, cf_ptr, n, dist == 2 ? LOC_S2_MC : dist == 1 ? LOC_S1_MC : LOC_MC);

    /* 文末にある体言(先行詞候補)は OK */
    if (CheckAppropriateCandidate(s, cs, cpm_ptr, tp, -2, cf_ptr, n, LOC_MC, FALSE)) {
	EllipsisDetectForVerbSubcontract(s, cs, em_ptr, cpm_ptr, cmm_ptr, l, tp, cf_ptr, n, 
					 dist == 2 ? LOC_S2_MC : dist == 1 ? LOC_S1_MC : LOC_MC, s, tp->pred_b_ptr);
    }

    return 0;
}

/*==================================================================*/
int mark_all_children(SENTENCE_DATA *cs, TAG_DATA *tp, int **lc, int lc_num, int sent)
/*==================================================================*/
{
    int i, j;

    if (!lc[sent][tp->num]) {
	lc[sent][tp->num] = lc_num;
    }

    _SearchCaseComponent(cs, tp, lc, lc_num, sent);

    /*
    for (i = 0; tp->child[i]; i++) {
	if (tp->child[i]->para_top_p) {
	    for (j = 0; tp->child[i]->child[j]; j++) {
		if (tp->child[i]->child[j]->para_type == PARA_NORMAL) {
		    if (check_feature(tp->child[i]->child[j]->f, "格要素")) {
			if (!lc[tp->child[i]->child[j]->num]) {
			    lc[tp->child[i]->child[j]->num] = lc_num;
			}
		    }
		}
		* ★ <PARA> のとき ★ *
	    }
	}
	else if (check_feature(tp->child[i]->f, "格要素")) {
	    if (!lc[tp->child[i]->num]) {
		lc[tp->child[i]->num] = lc_num;
	    }
	}
    }
    */
}

/*==================================================================*/
int _SearchMC(SENTENCE_DATA *s, TAG_DATA *ctp, int **lc, int dist)
/*==================================================================*/
{
    int i, flag = 0;
    TAG_DATA *tp;

    for (i = s->Tag_num - 1; i >= 0; i--) {
	tp = s->tag_data + i;
	while (tp->para_top_p) {
	    tp = tp->child[0];
	}

	if (check_mc(tp)) {
	    flag = 1;
	    break;
	}
    }

    if (flag == 0) {
	tp = s->tag_data + s->Tag_num - 1;
	while (tp->para_top_p) {
	    tp = tp->child[0];
	}
    }

    /* 対象ではない */
    if (ctp == NULL || tp->num != ctp->num) {
	mark_all_children(s, tp, lc, dist == 2 ? LOC_S2_MC : dist == 1 ? LOC_S1_MC : LOC_MC, dist);
    }

    return 0;
}

/*==================================================================*/
	 int SearchSC(SENTENCE_DATA *s, SENTENCE_DATA *cs, ELLIPSIS_MGR *em_ptr,
		      CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int l, 
		      CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{
    int i, j, start, dist;
    TAG_DATA *tp, *tp2;

    dist = cs - s;

    for (i = s->Tag_num - 1; i >= 0; i--) {
	tp = s->tag_data + i; 
	if (check_mc(tp)) {
	    if (tp->para_top_p) {
		/* 主節をチェックしないように */
		start = 1;
	    }
	    else {
		start = 0;
	    }
	    for (j = start; tp->child[j]; j++) {
		tp2 = tp->child[j];
		while (tp2->para_top_p) {
		    tp2 = tp2->child[0];
		}
		/* レベルがBより強い従属節 */
		if (check_feature(tp2->f, "係:連用") && 
		    subordinate_level_check("B", (BNST_DATA *)tp2)) {
		    SearchCaseComponent(s, cs, em_ptr, cpm_ptr, cmm_ptr, l, 
					tp2, cf_ptr, n, 
					dist == 2 ? LOC_S2_SC : dist == 1 ? LOC_S1_SC : LOC_SC);
		}
	    }
	    break;
	}
    }
    return 0;
}

/*==================================================================*/
  int _SearchSC(SENTENCE_DATA *s, TAG_DATA *ctp, int **lc, int dist)
/*==================================================================*/
{
    int i, j, start;
    TAG_DATA *tp, *tp2;

    for (i = s->Tag_num - 1; i >= 0; i--) {
	tp = s->tag_data + i; 
	if (check_mc(tp)) {
	    if (tp->para_top_p) {
		/* 主節をチェックしないように */
		start = 1;
	    }
	    else {
		start = 0;
	    }
	    for (j = start; tp->child[j]; j++) {
		tp2 = tp->child[j];
		while (tp2->para_top_p) {
		    tp2 = tp2->child[0];
		}
		/* レベルがBより強い従属節 */
		if ((ctp == NULL || tp2->num != ctp->num) && /* 対象ではない */
		    check_feature(tp2->f, "係:連用") && 
		    subordinate_level_check("B", (BNST_DATA *)tp2)) {
		    mark_all_children(s, tp2, lc, dist == 2 ? LOC_S2_SC : dist == 1 ? LOC_S1_SC : LOC_SC, dist);
		}
	    }
	    break;
	}
    }
    return 0;
}

/*==================================================================*/
int CheckMatchedLC(SENTENCE_DATA *s, SENTENCE_DATA *cs, ELLIPSIS_MGR *em_ptr, CF_PRED_MGR *cpm_ptr, 
		   CF_MATCH_MGR *cmm_ptr, int l, TAG_DATA *tp, 
		   CASE_FRAME *cf_ptr, int n, int loc)
/*==================================================================*/
{
    int sent, i, dist;
    SENTENCE_DATA *ts;

    for (sent = 0; sent < cs->Sen_num - (cs - s); sent++) {
	ts = s - sent;
	dist = (cs - s) + sent;
	for (i = 0; i < ts->Tag_num; i++) {
	    if (LC[dist][i] == loc) {
		if (CheckAppropriateCandidate(ts, cs, cpm_ptr, ts->tag_data + i, -2, cf_ptr, n, loc, 
					      FALSE)) {
		    EllipsisDetectForVerbSubcontract(ts, cs, em_ptr, cpm_ptr, cmm_ptr, l, ts->tag_data + i, cf_ptr, n, loc, ts, (ts->tag_data + i)->pred_b_ptr);
		    Bcheck[dist][i] = 1;
		}
	    }
	}
    }
}

/*==================================================================*/
int SearchParentV(SENTENCE_DATA *cs, ELLIPSIS_MGR *em_ptr, CF_PRED_MGR *cpm_ptr, 
		  CF_MATCH_MGR *cmm_ptr, int l, TAG_DATA *tp, 
		  CASE_FRAME *cf_ptr, int n, int mccheck)
/*==================================================================*/
{
    if (tp->parent && 
	check_feature(tp->parent->f, "用言")) {
	int i, mcflag;
	TAG_DATA *tp2;

	mcflag = check_mc(tp->parent);

	if (!(mccheck == 0 || 
	      (mccheck > 0 && mcflag) || 
	      (mccheck < 0 && !mcflag))) {
	    return 0;
	}

	/* 親が<PARA>なら<P>の子供を全部チェック */
	if (tp->parent->para_top_p) {
	    for (i = 0; tp->parent->child[i]; i++) {
		if (tp->parent->child[i]->para_type == PARA_NORMAL && 
		    tp->parent->child[i]->num > tp->num) {

		    /* <PARA>なら child[0]をみていく必要がある */
		    tp2 = tp->parent->child[i];
		    while (tp2->para_top_p) {
			tp2 = tp2->child[0];
		    }

		    SearchCaseComponent(cs, cs, em_ptr, cpm_ptr, cmm_ptr, l, 
					tp2, cf_ptr, n, mcflag ? LOC_PARENTV_MC : LOC_PARENTV);
		}
	    }
	}
	else {
	    SearchCaseComponent(cs, cs, em_ptr, cpm_ptr, cmm_ptr, l, 
				tp->parent, cf_ptr, n, mcflag ? LOC_PARENTV_MC : LOC_PARENTV);
	}
    }
    return 0;
}

/*==================================================================*/
     int _SearchParentV(SENTENCE_DATA *s, TAG_DATA *tp, int **lc)
/*==================================================================*/
{
    if (tp->parent && 
	check_feature(tp->parent->f, "用言")) {
	int i;
	TAG_DATA *tp2;

	/* 親が<PARA>なら<P>の子供を全部チェック */
	if (tp->parent->para_top_p) {
	    for (i = 0; tp->parent->child[i]; i++) {
		if (tp->parent->child[i]->para_type == PARA_NORMAL && 
		    tp->parent->child[i]->num > tp->num) {

		    /* <PARA>なら child[0]をみていく必要がある */
		    tp2 = tp->parent->child[i];
		    while (tp2->para_top_p) {
			tp2 = tp2->child[0];
		    }

		    mark_all_children(s, tp2, lc, check_mc(tp2) ? LOC_PARENTV_MC : LOC_PARENTV, 0);
		}
	    }
	}
	else {
	    mark_all_children(s, tp->parent, lc, check_mc(tp->parent) ? LOC_PARENTV_MC : LOC_PARENTV, 0);
	}
    }
    return 0;
}

/*==================================================================*/
int GoUpParaChild(SENTENCE_DATA *cs, ELLIPSIS_MGR *em_ptr, 
		  CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int l, 
		  TAG_DATA *tp, TAG_DATA *orig_tp, CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{
    int i;
    TAG_DATA *tp2;

    /* tp : <PARA> */

    if (tp && tp->para_top_p) {
	/* ★この子供の順番は? */
	for (i = 0; tp->child[i]; i++) {
	    if (tp->child[i]->num < orig_tp->num && /* 子供側 */
		tp->child[i]->para_type == PARA_NORMAL) {

		/* <PARA>でなくなるまでさかのぼる必要がある */
		tp2 = tp->child[i];
		while (tp2->para_top_p) {
		    tp2 = tp2->child[0];
		}

		SearchCaseComponent(cs, cs, em_ptr, cpm_ptr, cmm_ptr, l, 
				    tp2, cf_ptr, n, LOC_CHILDPV);

		/* 並列の子は全部<PARA>に係るので再帰に呼ぶのは間違い
		   GoUpParaChild(cs, em_ptr, cpm_ptr, cmm_ptr, 
		   tp->child[i], tp->child[i]->child[0], cf_ptr, n); */
	    }
	}
    }
}

/*==================================================================*/
int _GoUpParaChild(SENTENCE_DATA *s, TAG_DATA *tp, TAG_DATA *orig_tp, int **lc)
/*==================================================================*/
{
    int i;
    TAG_DATA *tp2;

    /* tp : <PARA> */

    if (tp && tp->para_top_p) {
	for (i = 0; tp->child[i]; i++) {
	    if (tp->child[i]->num < orig_tp->num && /* 子供側 */
		tp->child[i]->para_type == PARA_NORMAL) {

		/* <PARA>でなくなるまでさかのぼる必要がある */
		tp2 = tp->child[i];
		while (tp2->para_top_p) {
		    tp2 = tp2->child[0];
		}

		mark_all_children(s, tp2, lc, LOC_CHILDPV, 0);
	    }
	}
    }
}

/*==================================================================*/
int SearchChildPV(SENTENCE_DATA *cs, ELLIPSIS_MGR *em_ptr, CF_PRED_MGR *cpm_ptr, 
		  CF_MATCH_MGR *cmm_ptr, int l, TAG_DATA *tp, 
		  CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{
    if (tp->para_type == PARA_NORMAL && 
	tp->parent && 
	tp->parent->para_top_p) {
	GoUpParaChild(cs, em_ptr, cpm_ptr, cmm_ptr, l, 
		      tp->parent, tp, cf_ptr, n);
    }
    return 0;
}

/*==================================================================*/
     int _SearchChildPV(SENTENCE_DATA *s, TAG_DATA *tp, int **lc)
/*==================================================================*/
{
    if (tp->para_type == PARA_NORMAL && 
	tp->parent && 
	tp->parent->para_top_p) {
	_GoUpParaChild(s, tp->parent, tp, lc);
    }
    return 0;
}

/*==================================================================*/
int SearchChildV(SENTENCE_DATA *cs, ELLIPSIS_MGR *em_ptr, CF_PRED_MGR *cpm_ptr, 
		 CF_MATCH_MGR *cmm_ptr, int l, TAG_DATA *tp, 
		 CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{
    /* 自分は<PARA>でないので並列ではない */
    if (tp->para_type == PARA_NIL) {
	int i;

	for (i = 0; tp->child[i]; i++) {
	    if (check_feature(tp->child[i]->f, "用言")) {
		SearchCaseComponent(cs, cs, em_ptr, cpm_ptr, cmm_ptr, l, 
				    tp->child[i], cf_ptr, n, LOC_CHILDV);
	    }
	}
    }
    return 0;
}

/*==================================================================*/
     int _SearchChildV(SENTENCE_DATA *s, TAG_DATA *tp, int **lc)
/*==================================================================*/
{
    /* 自分は<PARA>でないので並列ではない */
    if (tp->para_type == PARA_NIL) {
	int i;

	for (i = 0; tp->child[i]; i++) {
	    if (check_feature(tp->child[i]->f, "用言")) {
		mark_all_children(s, tp->child[i], lc, LOC_CHILDV, 0);
	    }
	}
    }
    return 0;
}

/*==================================================================*/
int SearchParentNParentV(SENTENCE_DATA *cs, ELLIPSIS_MGR *em_ptr, CF_PRED_MGR *cpm_ptr, 
			 CF_MATCH_MGR *cmm_ptr, int l, TAG_DATA *tp, 
			 CASE_FRAME *cf_ptr, int n, int mccheck)
/*==================================================================*/
{
    if (tp->parent && 
	!tp->para_type && 
	tp->parent->parent && 
	!check_feature(tp->parent->f, "用言")) {
	int mcflag;
	TAG_DATA *tp2;

	mcflag = check_mc(tp->parent->parent);

	if (!(mccheck == 0 || 
	      (mccheck > 0 && mcflag) || 
	      (mccheck < 0 && !mcflag))) {
	    return 0;
	}

	if (check_feature(tp->parent->parent->f, "用言")) {
	    if (tp->parent->parent->para_top_p) {
		int i;

		for (i = 0; tp->parent->parent->child[i]; i++) {
		    if (tp->parent->parent->child[i]->para_type == PARA_NORMAL && 
			tp->parent->parent->child[i]->num > tp->parent->num) {

			/* <PARA>なら child[0]をみていく必要がある */
			tp2 = tp->parent->parent->child[i];
			while (tp2->para_top_p) {
			    tp2 = tp2->child[0];
			}

			SearchCaseComponent(cs, cs, em_ptr, cpm_ptr, cmm_ptr, l, 
					    tp2, cf_ptr, n, mcflag ? LOC_PARENTNPARENTV_MC : LOC_PARENTNPARENTV);
		    }
		}
	    }
	    else {
		SearchCaseComponent(cs, cs, em_ptr, cpm_ptr, cmm_ptr, l, 
				    tp->parent->parent, cf_ptr, n, mcflag ? LOC_PARENTNPARENTV_MC : LOC_PARENTNPARENTV);
	    }
	}
    }
    return 0;
}

/*==================================================================*/
 int _SearchParentNParentV(SENTENCE_DATA *s, TAG_DATA *tp, int **lc)
/*==================================================================*/
{
    if (tp->parent && 
	!tp->para_type && 
	tp->parent->parent && 
	!check_feature(tp->parent->f, "用言")) {
	TAG_DATA *tp2;

	if (check_feature(tp->parent->parent->f, "用言")) {
	    if (tp->parent->parent->para_top_p) {
		int i;

		for (i = 0; tp->parent->parent->child[i]; i++) {
		    if (tp->parent->parent->child[i]->para_type == PARA_NORMAL && 
			tp->parent->parent->child[i]->num > tp->parent->num) {

			/* <PARA>なら child[0]をみていく必要がある */
			tp2 = tp->parent->parent->child[i];
			while (tp2->para_top_p) {
			    tp2 = tp2->child[0];
			}

			mark_all_children(s, tp2, lc, check_mc(tp2) ? LOC_PARENTNPARENTV_MC : LOC_PARENTNPARENTV, 0);
		    }
		}
	    }
	    else {
		mark_all_children(s, tp->parent->parent, lc, 
				  check_mc(tp->parent->parent) ? LOC_PARENTNPARENTV_MC : LOC_PARENTNPARENTV, 0);
	    }
	}
    }
    return 0;
}

/*==================================================================*/
int SearchParentVParentV(SENTENCE_DATA *cs, ELLIPSIS_MGR *em_ptr, CF_PRED_MGR *cpm_ptr, 
			 CF_MATCH_MGR *cmm_ptr, int l, TAG_DATA *tp, 
			 CASE_FRAME *cf_ptr, int n, int mccheck)
/*==================================================================*/
{
    if (tp->parent && 
	!tp->para_type && 
	tp->parent->parent && 
	check_feature(tp->parent->f, "用言")) {
	int mcflag;
	TAG_DATA *tp2;

	mcflag = check_mc(tp->parent->parent);

	if (!(mccheck == 0 || 
	      (mccheck > 0 && mcflag) || 
	      (mccheck < 0 && !mcflag))) {
	    return 0;
	}

	if (check_feature(tp->parent->parent->f, "用言")) {
	    if (tp->parent->parent->para_top_p) {
		int i;

		for (i = 0; tp->parent->parent->child[i]; i++) {
		    if (tp->parent->parent->child[i]->para_type == PARA_NORMAL && 
			tp->parent->parent->child[i]->num > tp->parent->num) {

			/* <PARA>なら child[0]をみていく必要がある */
			tp2 = tp->parent->parent->child[i];
			while (tp2->para_top_p) {
			    tp2 = tp2->child[0];
			}

			SearchCaseComponent(cs, cs, em_ptr, cpm_ptr, cmm_ptr, l, 
					    tp2, cf_ptr, n, mcflag ? LOC_PARENTVPARENTV_MC : LOC_PARENTVPARENTV);
		    }
		}
	    }
	    else {
		SearchCaseComponent(cs, cs, em_ptr, cpm_ptr, cmm_ptr, l, 
				    tp->parent->parent, cf_ptr, n, mcflag ? LOC_PARENTVPARENTV_MC : LOC_PARENTVPARENTV);
	    }
	}
    }
    return 0;
}

/*==================================================================*/
 int _SearchParentVParentV(SENTENCE_DATA *s, TAG_DATA *tp, int **lc)
/*==================================================================*/
{
    if (tp->parent && 
	!tp->para_type && 
	tp->parent->parent && 
	check_feature(tp->parent->f, "用言")) {
	TAG_DATA *tp2;

	if (check_feature(tp->parent->parent->f, "用言")) {
	    if (tp->parent->parent->para_top_p) {
		int i;

		for (i = 0; tp->parent->parent->child[i]; i++) {
		    if (tp->parent->parent->child[i]->para_type == PARA_NORMAL && 
			tp->parent->parent->child[i]->num > tp->parent->num) {

			/* <PARA>なら child[0]をみていく必要がある */
			tp2 = tp->parent->parent->child[i];
			while (tp2->para_top_p) {
			    tp2 = tp2->child[0];
			}

			mark_all_children(s, tp2, lc, check_mc(tp2) ? LOC_PARENTVPARENTV_MC : LOC_PARENTVPARENTV, 0);
		    }
		}
	    }
	    else {
		mark_all_children(s, tp->parent->parent, lc, 
				  check_mc(tp->parent->parent) ? LOC_PARENTVPARENTV_MC : LOC_PARENTVPARENTV, 0);
	    }
	}
    }
    return 0;
}

/*==================================================================*/
       int _SearchPV(SENTENCE_DATA *s, TAG_DATA *tp, int **lc)
/*==================================================================*/
{
    int i;

    if (tp->para_type == PARA_NORMAL) {
	/* <PARA>に係る要素 */
	/* mark_all_children(tp->parent, lc, check_mc(tp->parent) ? LOC_PV_MC : LOC_PV); */
	for (i = 0; tp->parent->child[i]; i++) {
	    if (tp->parent->child[i]->num > tp->num && /* 親側 */
		tp->parent->child[i]->para_type == PARA_NORMAL) {
		mark_all_children(s, tp->parent->child[i], lc, check_mc(tp->parent->child[i]) ? LOC_PV_MC : LOC_PV, 0);
	    }
	}
    }    
}

/*==================================================================*/
int EllipsisDetectForVerb(SENTENCE_DATA *sp, ELLIPSIS_MGR *em_ptr, 
			  CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int l, 
			  CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{
    /* 用言とその省略格が与えられる */

    /* cf_ptr = cpm_ptr->cmm[0].cf_ptr である */
    /* 用言 cpm_ptr の cf_ptr->pp[n][0] 格が省略されている
       cf_ptr->ex[n] に似ている文節を探す */

    int i, j, mc = 0;
    char feature_buffer[DATA_LEN], etc_buffer[DATA_LEN], *cp;
    SENTENCE_DATA *s, *cs;
    TAG_DATA *tp, *tp2, *ptp;

    maxscore = 0;
    maxtag = NULL;
    maxs = NULL;
    maxpos = MATCH_NONE;

    cs = sentence_data + sp->Sen_num - 1;
    for (i = 0; i < sp->Sen_num; i++) {
	memset(Bcheck[i], 0, sizeof(int) * TAG_MAX);
    }

    /* best解を探す場合 */
    if (OptDiscFlag & OPT_DISC_BEST) {
	for (i = 0; i <= PrevSentenceLimit; i++) {
	    if (cs - sentence_data < i) {
		break;
	    }
	    EllipsisDetectRecursive(cs - i, cs, em_ptr, cpm_ptr, cmm_ptr, l, 
				    (cs - i)->tag_data + (cs - i)->Tag_num - 1, 
				    cf_ptr, n, LOC_OTHERS);
	}

	/* 閾値を越えるものが見つからなかった */
	if (!ScoreCheck(cf_ptr, n)) {
	    /* 閾値を越えるものがなく、格フレームに<主体>があるとき */
	    if (cf_match_element(cf_ptr->sm[n], "主体", FALSE)) {
		maxtag = ExtraTags[1]; /* 不特定-人 */
	    }
	    else {
		return 0;
	    }
	}

	goto EvalAntecedent;
    }
    /* flat */
    else if (OptDiscFlag & OPT_DISC_FLAT) {
	int max_n;
	int post_n = cs->Tag_num - cpm_ptr->pred_b_ptr->num - 1;
	int pre_n = cpm_ptr->pred_b_ptr->num;

	max_n = post_n > pre_n ? post_n : pre_n;

	/* 対象文 */
	for (i = 1; i < max_n; i++) {
	    if (cpm_ptr->pred_b_ptr->num - i >= 0) {
		EllipsisDetectOne(cs, cs, em_ptr, cpm_ptr, cmm_ptr, l, 
				  cs->tag_data + cpm_ptr->pred_b_ptr->num - i, cf_ptr, n);
	    }
	    if (cpm_ptr->pred_b_ptr->num + i < cs->Tag_num) {
		EllipsisDetectOne(cs, cs, em_ptr, cpm_ptr, cmm_ptr, l, 
				  cs->tag_data + cpm_ptr->pred_b_ptr->num + i, cf_ptr, n);
	    }
	    if (ScoreCheck(cf_ptr, n)) {
		goto EvalAntecedent;
	    }
	}

	/* 前文以前 */
	for (j = 1; j <= PrevSentenceLimit; j++) {
	    if (cs - sentence_data < j) {
		break;
	    }
	    for (i = (cs - j)->Tag_num - 1; i >= 0; i--) {
		EllipsisDetectOne(cs - j, cs, em_ptr, cpm_ptr, cmm_ptr, l, 
				  (cs - j)->tag_data + i, cf_ptr, n);
		if (ScoreCheck(cf_ptr, n)) {
		    goto EvalAntecedent;
		}
	    }
	}
    }

    /* 例外タグ */
    if (OptDiscFlag & OPT_DISC_TWIN_CAND) {
	/* for (i = 0; ExtraTags[i][0]; i++) */
	i = 1; /* とりあえず 不特定-人 */
	    EllipsisDetectForVerbSubcontractExtraTags(cs, em_ptr, cpm_ptr, cmm_ptr, l, 
						      i, cf_ptr, n);
	/*
	if (ScoreCheck(cf_ptr, n)) {
	    goto EvalAntecedent;
	}
	*/
    }


    /* 位置カテゴリの順番で探す */

    /* (用言の)並列を吸収 */
    tp = cpm_ptr->pred_b_ptr;
    while (tp->para_type == PARA_NORMAL && 
	   tp->parent && tp->parent->para_top_p) {
	tp = tp->parent;
    }
    /* 並列なら tp は <PARA> になる */
    ptp = tp;

    if (!(MatchPP(cf_ptr->pp[n][0], "ガ") || 
	  MatchPP(cf_ptr->pp[n][0], "ヲ") || 
	  MatchPP(cf_ptr->pp[n][0], "ニ"))) {
	fprintf(stderr, ";; Cannot handle <%s> of zero pronoun\n", pp_code_to_kstr(cf_ptr->pp[n][0]));
	return 0;
    }

    for (j = 0; LocationOrder[cf_ptr->pp[n][0]][j] != END_M && 
	     (OptLearn == TRUE || (OptDiscFlag & OPT_DISC_TWIN_CAND) || 
	      LocationLimit[cf_ptr->pp[n][0]] == END_M || j < LocationLimit[cf_ptr->pp[n][0]]); j++) {
	switch(LocationOrder[cf_ptr->pp[n][0]][j]) {
	case LOC_S1_MC:
	case LOC_S1_SC:
	case LOC_S1_OTHERS:
	    if (cs - sentence_data > 0) {
		CheckMatchedLC(cs - 1, cs, em_ptr, cpm_ptr, cmm_ptr, l, ptp, cf_ptr, n, LocationOrder[cf_ptr->pp[n][0]][j]);
	    }
	    break;
	case LOC_S2_MC:
	case LOC_S2_SC:
	case LOC_S2_OTHERS:
	    if (cs - sentence_data > 1) {
		CheckMatchedLC(cs - 2, cs, em_ptr, cpm_ptr, cmm_ptr, l, ptp, cf_ptr, n, LocationOrder[cf_ptr->pp[n][0]][j]);
	    }
	    break;
	default:
	    CheckMatchedLC(cs, cs, em_ptr, cpm_ptr, cmm_ptr, l, ptp, cf_ptr, n, LocationOrder[cf_ptr->pp[n][0]][j]);
	    break;
	}
	if (ScoreCheck(cf_ptr, n)) {
	    goto EvalAntecedent;
	}
    }

    /* 2文より以前 */
    for (i = 3; i <= PrevSentenceLimit; i++) {
	if (cs - sentence_data < i) {
	    break;
	}
	CheckMatchedLC(cs - i, cs, em_ptr, cpm_ptr, cmm_ptr, l, ptp, cf_ptr, n, LOC_OTHERS);
	if (ScoreCheck(cf_ptr, n)) {
	    goto EvalAntecedent;
	}
    }

    if (OptDiscFlag & OPT_DISC_TWIN_CAND) {
	if (classify_twin_candidate(cs, em_ptr, cpm_ptr)) {
	    if (maxtag != NULL || ScoreCheckCore(cf_ptr, n, maxscore, maxpos)) {
		clear_cands();
		goto EvalAntecedent;
	    }
	}
	clear_cands();
    }

    /* 閾値を越えるものが見つからなかった */
    if (!ScoreCheck(cf_ptr, n)) {
	/* 閾値を越えるものがなく、ガ格または格フレームに<主体>があるとき */
	if (MatchPP(cf_ptr->pp[n][0], "ガ") || 
	    cf_match_element(cf_ptr->sm[n], "主体", FALSE)) {
	    maxtag = ExtraTags[1]; /* 不特定-人 */
	}
	else {
	    return 0;
	}
    }

  EvalAntecedent:
    if (maxtag) {
	if (str_eq(maxtag, "不特定-人")) {
	    sprintf(feature_buffer, "C用;【不特定-人】;%s;-1;-1;1", 
		    pp_code_to_kstr_in_context(cpm_ptr, cf_ptr->pp[n][0]));
	    assign_cfeature(&(em_ptr->f), feature_buffer);
	    em_ptr->cc[cf_ptr->pp[n][0]].s = NULL;
	    em_ptr->cc[cf_ptr->pp[n][0]].bnst = ELLIPSIS_TAG_UNSPECIFIED_PEOPLE;
	    return 0;
	}
	else if (str_eq(maxtag, "一人称")) {
	    sprintf(feature_buffer, "C用;【一人称】;%s;-1;-1;1", 
		    pp_code_to_kstr_in_context(cpm_ptr, cf_ptr->pp[n][0]));
	    assign_cfeature(&(em_ptr->f), feature_buffer);
	    em_ptr->cc[cf_ptr->pp[n][0]].s = NULL;
	    em_ptr->cc[cf_ptr->pp[n][0]].bnst = ELLIPSIS_TAG_I_WE;
	    return 1;
	}
	else if (str_eq(maxtag, "不特定-状況")) {
	    sprintf(feature_buffer, "C用;【不特定-状況】;%s;-1;-1;1", 
		    pp_code_to_kstr_in_context(cpm_ptr, cf_ptr->pp[n][0]));
	    assign_cfeature(&(em_ptr->f), feature_buffer);
	    em_ptr->cc[cf_ptr->pp[n][0]].s = NULL;
	    em_ptr->cc[cf_ptr->pp[n][0]].bnst = ELLIPSIS_TAG_UNSPECIFIED_CASE;
	    return 1;
	}
    }
    else if (maxs) {
	int distance;
	char *word;

	word = make_print_string(maxs->tag_data + maxi, 0);

	distance = cs - maxs;
	if (distance == 0) {
	    strcpy(etc_buffer, "同一文");
	}
	else if (distance > 0) {
	    sprintf(etc_buffer, "%d文前", distance);
	}

	/* 決定した省略関係 */
	sprintf(feature_buffer, "C用;【%s】;%s;%d;%d;%.3f:%s(%s):%d文節", 
		word ? word : "?", 
		pp_code_to_kstr_in_context(cpm_ptr, cf_ptr->pp[n][0]), 
		distance, maxi, 
		maxscore, maxs->KNPSID ? maxs->KNPSID + 5 : "?", 
		etc_buffer, maxi);
	assign_cfeature(&(em_ptr->f), feature_buffer);

	StoreEllipsisComponent(&(em_ptr->cc[cf_ptr->pp[n][0]]), NULL, 
			       maxs, maxi, maxscore, distance);

	/* 指示対象を格フレームに保存 */
	AppendToCF(cpm_ptr, cmm_ptr, l, maxs->tag_data + maxi, cf_ptr, n, maxscore, maxpos, maxs);
	return 1;
    }
    return 0;
}

/*==================================================================*/
int EllipsisDetectForNoun(SENTENCE_DATA *sp, ELLIPSIS_MGR *em_ptr, 
			  CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int l, 
			  CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{
    int i;
    SENTENCE_DATA *cs;
    char feature_buffer[DATA_LEN], etc_buffer[DATA_LEN];
    CASE_COMPONENT *ccp;

    maxscore = 0;
    maxtag = NULL;
    maxs = NULL;
    maxpos = MATCH_NONE;

    cs = sentence_data + sp->Sen_num - 1;
    for (i = 0; i < sp->Sen_num; i++) {
	memset(Bcheck[i], 0, sizeof(int) * TAG_MAX);
    }

    /* 共参照リンクを辿ってタグつけ */
    if ((ccp = CheckTagTarget(cpm_ptr->pred_b_ptr->head_ptr->Goi, 
			      cpm_ptr->pred_b_ptr->voice, 
			      cmm_ptr->cf_ptr->cf_address, 
			      cf_ptr->pp[n][0], 
			      cf_ptr->pp_str[n]))
	&&  sp->Sen_num - ccp->sent_num < 5) {
	if (!CheckHaveEllipsisComponent(cpm_ptr, cmm_ptr, l, ccp->word)) {
	    maxs = sentence_data + ccp->sent_num - 1;
	    maxi = ccp->tag_num;
	    maxscore = 1.0;
	    goto EvalAntecedentNoun;
	}
    }

    /* best解を探す場合 */
    if (OptDiscFlag & OPT_DISC_BEST) {
	for (i = 0; i <= PrevSentenceLimit; i++) {
	    if (cs - sentence_data < i) {
		break;
	    }
	    EllipsisDetectRecursive2(cs - i, cs, em_ptr, cpm_ptr, cmm_ptr, l, 
				     (cs - i)->tag_data + (cs - i)->Tag_num - 1, 
				     cf_ptr, n, LOC_OTHERS, TRUE);
	}
	/* 閾値を越えるものが見つからなかった */
	if (!ScoreCheck(cf_ptr, n)) {
	    return 0;
	}
	goto EvalAntecedentNoun;
    }

    /* 親 */
    if (cpm_ptr->pred_b_ptr->parent &&
	EllipsisDetectRecursive2(cs, cs, em_ptr, cpm_ptr, cmm_ptr, l, 
				 cpm_ptr->pred_b_ptr->parent, 
				 cf_ptr, n, LOC_OTHERS, FALSE)) {
	goto EvalAntecedentNoun;
    }
    /* 主節 */
    else if (EllipsisDetectRecursive2(cs, cs, em_ptr, cpm_ptr, cmm_ptr, l, 
				 cs->tag_data + cs->Tag_num - 1, 
				 cf_ptr, n, LOC_OTHERS, TRUE)) {
	goto EvalAntecedentNoun;
    }
    /* 前文より前 */
    else {
	for (i = 1; i <= PrevSentenceLimit; i++) {
	    if (cs - sentence_data < i) {
		break;
	    }
	    if (EllipsisDetectRecursive2(cs - i, cs, em_ptr, cpm_ptr, cmm_ptr, l, 
					 (cs - i)->tag_data + (cs - i)->Tag_num - 1, 
					 cf_ptr, n, LOC_OTHERS, TRUE)) {
		goto EvalAntecedentNoun;
	    }
	}
    }

    /* 閾値を越えるものがないとき */
    return 0;

  EvalAntecedentNoun:
    /* 閾値を越えるものが見つかった */
    if (maxtag) {
	if (str_eq(maxtag, "不特定-人")) {
	    sprintf(feature_buffer, "C用;【不特定-人】;%s;-1;-1;1", 
		    pp_code_to_kstr_in_context(cpm_ptr, cf_ptr->pp[n][0]));
	    assign_cfeature(&(em_ptr->f), feature_buffer);
	    em_ptr->cc[cf_ptr->pp[n][0]].s = NULL;
	    em_ptr->cc[cf_ptr->pp[n][0]].bnst = ELLIPSIS_TAG_UNSPECIFIED_PEOPLE;
	    return 0;
	}
	else if (str_eq(maxtag, "一人称")) {
	    sprintf(feature_buffer, "C用;【一人称】;%s;-1;-1;1", 
		    pp_code_to_kstr_in_context(cpm_ptr, cf_ptr->pp[n][0]));
	    assign_cfeature(&(em_ptr->f), feature_buffer);
	    em_ptr->cc[cf_ptr->pp[n][0]].s = NULL;
	    em_ptr->cc[cf_ptr->pp[n][0]].bnst = ELLIPSIS_TAG_I_WE;
	    return 1;
	}
	else if (str_eq(maxtag, "不特定-状況")) {
	    sprintf(feature_buffer, "C用;【不特定-状況】;%s;-1;-1;1", 
		    pp_code_to_kstr_in_context(cpm_ptr, cf_ptr->pp[n][0]));
	    assign_cfeature(&(em_ptr->f), feature_buffer);
	    em_ptr->cc[cf_ptr->pp[n][0]].s = NULL;
	    em_ptr->cc[cf_ptr->pp[n][0]].bnst = ELLIPSIS_TAG_UNSPECIFIED_CASE;
	    return 1;
	}
    }
    else if (maxs) {
	int distance;
	char *word;

	word = make_print_string(maxs->tag_data + maxi, 0);

	distance = cs - maxs;
	if (distance == 0) {
	    strcpy(etc_buffer, "同一文");
	}
	else if (distance > 0) {
	    sprintf(etc_buffer, "%d文前", distance);
	}

	/* 決定した省略関係 */
	sprintf(feature_buffer, "C用;【%s】;%s;%d;%d;%.3f:%s(%s):%d文節", 
		word ? word : "?", 
		pp_code_to_kstr_in_context(cpm_ptr, cf_ptr->pp[n][0]), 
		distance, maxi, 
		maxscore, maxs->KNPSID ? maxs->KNPSID + 5 : "?", 
		etc_buffer, maxi);
	assign_cfeature(&(em_ptr->f), feature_buffer);

	StoreEllipsisComponent(&(em_ptr->cc[cf_ptr->pp[n][0]]), cf_ptr->pp_str[n], 
			       maxs, maxi, maxscore, distance);

	/* 指示対象を格フレームに保存 */
	AppendToCF(cpm_ptr, cmm_ptr, l, maxs->tag_data + maxi, cf_ptr, n, maxscore, maxpos, maxs);
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
int RuleRecognition(CF_PRED_MGR *cpm_ptr, CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{
    char feature_buffer[DATA_LEN];

    /* <不特定:状況> をガ格としてとる判定詞 */
    if (check_feature(cpm_ptr->pred_b_ptr->f, "時間ガ省略") && 
	MatchPP(cf_ptr->pp[n][0], "ガ")) {
	sprintf(feature_buffer, "C用;【不特定-状況】;%s;-1;-1;1", 
		pp_code_to_kstr_in_context(cpm_ptr, cf_ptr->pp[n][0]));
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
	sprintf(feature_buffer, "格フレーム-%s-主体準", pp_code_to_kstr_in_context(cpm_ptr, cf_ptr->pp[n][0]));
	assign_cfeature(&(em_ptr->f), feature_buffer);
    }
    /* 格フレームが <主体> をもつかどうか */
    else if (cf_match_element(cf_ptr->sm[n], "主体", FALSE)) {
	sprintf(feature_buffer, "格フレーム-%s-主体", pp_code_to_kstr_in_context(cpm_ptr, cf_ptr->pp[n][0]));
	assign_cfeature(&(em_ptr->f), feature_buffer);
    }


    /* 格フレームが <補文> をもつかどうか */
    if (cf_match_element(cf_ptr->sm[n], "補文", TRUE)) {
	sprintf(feature_buffer, "格フレーム-%s-補文", pp_code_to_kstr_in_context(cpm_ptr, cf_ptr->pp[n][0]));
	assign_cfeature(&(em_ptr->f), feature_buffer);
    }

    if (sm_match_check(sm2code("抽象物"), cpm_ptr->pred_b_ptr->SM_code, SM_NO_EXPAND_NE)) {
	sprintf(feature_buffer, "格フレーム-%s-抽象物", pp_code_to_kstr_in_context(cpm_ptr, cf_ptr->pp[n][0]));
	assign_cfeature(&(em_ptr->f), feature_buffer);
    }
}

/*==================================================================*/
 int CheckToCase(CF_PRED_MGR *cpm_ptr, CF_MATCH_MGR *cmm_ptr, int l, CASE_FRAME *cf_ptr)
/*==================================================================*/
{
    int i, num;

    /* 格フレームに補文ト格に割り当てがあるかどうか調べる */
    for (i = 0; i < cf_ptr->element_num; i++) {
	num = cmm_ptr->result_lists_p[l].flag[i];
	if (num != UNASSIGNED && 
	    MatchPP(cf_ptr->pp[i][0], "ト")) {
	    /* check_feature(cpm_ptr->elem_b_ptr[num]->f, "補文")) { */
	    return TRUE;
	}
    }

    /* 入力文に補文ト格があるか調べる */
    for (i = 0; i < cpm_ptr->cf.element_num; i++) {
	if (cpm_ptr->elem_b_num[i] > -2 && 
	    MatchPP(cpm_ptr->cf.pp[i][0], "ト")) {
	    /* check_feature(cpm_ptr->elem_b_ptr[i]->f, "補文")) { */
	    return TRUE;
	}
    }

    return FALSE;
}

/*==================================================================*/
float EllipsisDetectForVerbMain(SENTENCE_DATA *sp, ELLIPSIS_MGR *em_ptr, CF_PRED_MGR *cpm_ptr, 
				CF_MATCH_MGR *cmm_ptr, int l, 
				CASE_FRAME *cf_ptr, char **order)
/*==================================================================*/
{
    int i, j, num, result, demoflag, toflag;
    int cases[PP_NUMBER], count = 0;

//    toflag = CheckToCase(cpm_ptr, cmm_ptr, l, cf_ptr);
    toflag = 0;

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
	    /* 指示詞の解析 (割り当てあり) */
	    if ((OptEllipsis & OPT_DEMO) && 
		cmm_ptr->result_lists_p[l].flag[i] != UNASSIGNED && 
		cf_ptr->pp[i][0] == cases[j] && 
		check_feature(cpm_ptr->elem_b_ptr[cmm_ptr->result_lists_p[l].flag[i]]->f, "省略解析対象指示詞")) {
		demoflag = 1;
	    }
	    else {
		demoflag = 0;
	    }

	    if (demoflag == 1 || 
		/* 割り当てなし => 省略 */
		((OptEllipsis & OPT_ELLIPSIS) && 
		 cf_ptr->pp[i][0] == cases[j] && 
		 cmm_ptr->result_lists_p[l].flag[i] == UNASSIGNED && 
		 !(toflag && MatchPP(cf_ptr->pp[i][0], "ヲ")))) {
		result = EllipsisDetectForVerb(sp, em_ptr, cpm_ptr, cmm_ptr, l, cf_ptr, i);
		AppendCfFeature(em_ptr, cpm_ptr, cf_ptr, i);
		if (result) {
		    em_ptr->cc[cf_ptr->pp[i][0]].score = maxscore;

		    if (OptDiscPredMethod == OPT_SVM || OptDiscPredMethod == OPT_DT) {
			em_ptr->score += maxscore > 1.0 ? EX_match_exact : maxscore < 0 ? 0 : 11*maxscore;
		    }
		    else {
			if (maxpos == MATCH_SUBJECT) {
			    em_ptr->score += EX_match_subject;
			}
			else {
			    em_ptr->score += maxscore > 1.0 ? EX_match_exact : *(EX_match_score+(int)(maxscore * 7));
			}
		    }
		}
		else if (demoflag == 1) {
		    ; /* 割り当てなしにする */
		}
	    }
	}
    }
    return em_ptr->score;
}

/*==================================================================*/
float EllipsisDetectForNounMain(SENTENCE_DATA *sp, ELLIPSIS_MGR *em_ptr, CF_PRED_MGR *cpm_ptr, 
				CF_MATCH_MGR *cmm_ptr, int l, CASE_FRAME *cf_ptr)
/*==================================================================*/
{
    int i, j, num, result, demoflag, toflag;
    int count = 0;

    for (i = 0; i < cf_ptr->element_num; i++) {
	/* 割り当てなし => 省略 */
	if ((OptEllipsis & OPT_REL_NOUN) && 
	    cmm_ptr->result_lists_p[l].flag[i] == UNASSIGNED) {
	    result = EllipsisDetectForNoun(sp, em_ptr, cpm_ptr, cmm_ptr, l, cf_ptr, i);
	    AppendCfFeature(em_ptr, cpm_ptr, cf_ptr, i);
	    if (result) {
		em_ptr->cc[cf_ptr->pp[i][0]].score = maxscore;

		/* 現在は、rule base */
		if (0 && 
		    (OptDiscNounMethod == OPT_SVM || OptDiscNounMethod == OPT_DT)) {
		    em_ptr->score += maxscore > 1.0 ? EX_match_exact : maxscore < 0 ? 0 : 11*maxscore;
		}
		else {
		    if (maxpos == MATCH_SUBJECT) {
			em_ptr->score += EX_match_subject;
		    }
		    else {
			em_ptr->score += maxscore > 1.0 ? EX_match_exact : *(EX_match_score+(int)(maxscore * 7));
		    }
		}
	    }
	}
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
			  CF_PRED_MGR *bp, CF_MATCH_MGR *b, int l)
/*==================================================================*/
{
    int i;

    /* 異なる場合は 1 を返す */

    for (i = 0; i < a->cf_ptr->element_num; i++) {
	if (a->result_lists_p[0].flag[i] != UNASSIGNED && 
	    ap->elem_b_ptr[a->result_lists_p[0].flag[i]] != bp->elem_b_ptr[b->result_lists_p[l].flag[i]]) {
	    return 1;
	}
    }
    return 0;
}

/*==================================================================*/
int CompareAssignList(ELLIPSIS_MGR *maxem, CF_PRED_MGR *cpm, CF_MATCH_MGR *cmm, int l)
/*==================================================================*/
{
    int i;

    for (i = 0; i < maxem->result_num; i++) {
	/* 同じものがすでにある場合 */
	if (maxem->ecmm[i].cmm.cf_ptr == cmm->cf_ptr && 
	    maxem->ecmm[i].element_num == cpm->cf.element_num && 
	    !CompareCPM(&(maxem->ecmm[i].cpm), cpm) && 
	    !CompareCMM(&(maxem->ecmm[i].cpm), &(maxem->ecmm[i].cmm), cpm, cmm, l)) {
	    return 0;
	}
    }
    return 1;
}

/*==================================================================*/
      int CompareClosestScore(CF_MATCH_MGR *a, CF_MATCH_MGR *b, int l)
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
	if (b->result_lists_p[l].flag[i] != UNASSIGNED && 
	    b->cf_ptr->adjacent[i] == TRUE) {
	    bcount++;
	    bscore = b->result_lists_p[l].score[i];
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
	  int CheckClosestAssigned(CF_MATCH_MGR *cmm, int l)
/*==================================================================*/
{
    int i, flag = 0;

    for (i = 0; i < cmm->cf_ptr->element_num; i++) {
	if (cmm->cf_ptr->adjacent[i] == TRUE) {
	    if (cmm->result_lists_p[0].flag[i] != UNASSIGNED) {
		return TRUE;
	    }
	    flag = 1;
	}
    }

    if (flag) {
	return FALSE;
    }
    return TRUE;
}

/*==================================================================*/
void FindBestCFforContext(SENTENCE_DATA *sp, ELLIPSIS_MGR *maxem, 
			  CF_PRED_MGR *cpm_ptr, char **order)
/*==================================================================*/
{
    int i, k, l, type, frame_num;
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
	    frame_num = 0;
	    cf_array = (CASE_FRAME **)malloc_data(sizeof(CASE_FRAME *)*cpm_ptr->pred_b_ptr->cf_num, 
						  "FindBestCFforContext");

	    if (OptUseSmfix == TRUE && CFSimExist == TRUE) {
		CFLIST *cfp;
		char *key;

		key = get_pred_id(cpm_ptr->pred_b_ptr->cf_ptr->cf_id);
		cfp = CheckCF(key);
		free(key);

		if (cfp) {
		    for (l = 0; l < cpm_ptr->pred_b_ptr->cf_num; l++) {
			for (i = 0; i < cfp->cfid_num; i++) {
			    if (((cpm_ptr->pred_b_ptr->cf_ptr + l)->type == cpm_ptr->cf.type) &&
				 ((cpm_ptr->pred_b_ptr->cf_ptr + l)->cf_similarity = 
				get_cfs_similarity((cpm_ptr->pred_b_ptr->cf_ptr + l)->cf_id, 
						   *(cfp->cfid + i))) > CFSimThreshold) {
				*(cf_array + frame_num++) = cpm_ptr->pred_b_ptr->cf_ptr + l;
				break;
			    }
			}
		    }

		    cpm_ptr->pred_b_ptr->e_cf_num = frame_num;
		    fprintf(stderr, ";; ★ %s [%s] CF -> %d/%d\n", sp->KNPSID, 
			    cpm_ptr->pred_b_ptr->head_ptr->Goi, 
			    frame_num, cpm_ptr->pred_b_ptr->cf_num);
		}
	    }

	    if (frame_num == 0) {
		for (l = 0; l < cpm_ptr->pred_b_ptr->cf_num; l++) {
		    if ((cpm_ptr->pred_b_ptr->cf_ptr + l)->type == cpm_ptr->cf.type) {
			*(cf_array + frame_num++) = cpm_ptr->pred_b_ptr->cf_ptr + l;
		    }
		}
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
	if (OptEllipsis & OPT_DEMO || cpm_ptr->cf.type_flag) {
	    make_data_cframe(sp, cpm_ptr);
	}

	/* 今ある格要素を対応づけ */
	case_frame_match(cpm_ptr, &cmm, OptCFMode, -1);
	cpm_ptr->score = cmm.score;

	/* for (i = 0; i < cmm.result_num; i++) */ {
	i = 0;

	ClearEllipsisMGR(&workem);

	    if (cpm_ptr->cf.type == CF_NOUN) {
		EllipsisDetectForNounMain(sp, &workem, cpm_ptr, &cmm, i, *(cf_array+l));
	    }
	    else {
		EllipsisDetectForVerbMain(sp, &workem, cpm_ptr, &cmm, i, 
					  *(cf_array+l), order);
	    }

	    if (0 && !CheckClosestAssigned(&cmm, i)) {
		workem.score = -1;
	    }
	    else if (cmm.score >= 0) {
		/* 直接の格要素の正規化していないスコアを足す */
		workem.score += cmm.pure_score[i];
		workem.pure_score = workem.score;
		/* 正規化 */
		if (cpm_ptr->cf.type == CF_PRED) {
		    workem.score /= sqrt((double)(count_pat_element(cmm.cf_ptr, 
								    &(cmm.result_lists_p[i]))));
		}
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
		 CompareClosestScore(&(maxem->ecmm[0].cmm), &cmm, i))) {
		maxem->cpm = workem.cpm;
		for (k = 0; k < CASE_TYPE_NUM; k++) {
		    ClearEllipsisComponent(&(maxem->cc[k]));
		    CopyEllipsisComponent(&(maxem->cc[k]), &(workem.cc[k]));
		}
		maxem->score = workem.score;
		maxem->pure_score = workem.pure_score;
		maxem->f = workem.f;
		workem.f = NULL;

		/* ひとつずつずらす */
		for (k = maxem->result_num >= CMM_MAX - 1 ? maxem->result_num - 1 : maxem->result_num; k >= 0; k--) {
		    maxem->ecmm[k + 1] = maxem->ecmm[k];
		}

		/* 今回が最大マッチ */
		maxem->ecmm[0].cmm = cmm;
		maxem->ecmm[0].cpm = *cpm_ptr;
		maxem->ecmm[0].element_num = cpm_ptr->cf.element_num;

		maxem->ecmm[0].cmm.result_num = 1;
		maxem->ecmm[0].cmm.result_lists_p[0] = cmm.result_lists_p[i];
		maxem->ecmm[0].cmm.result_lists_d[0] = cmm.result_lists_d[i];
		maxem->ecmm[0].cmm.pure_score[0] = cmm.pure_score[i];

		if (maxem->result_num < CMM_MAX - 1) {
		    maxem->result_num++;
		}
	    }
	    /* 新しい種類の格フレーム(割り当て)を保存 */
	    else if (CompareAssignList(maxem, cpm_ptr, &cmm, i)) {
		maxem->ecmm[maxem->result_num].cmm = cmm;
		maxem->ecmm[maxem->result_num].cpm = *cpm_ptr;
		maxem->ecmm[maxem->result_num].element_num = cpm_ptr->cf.element_num;

		maxem->ecmm[maxem->result_num].cmm.result_num = 1;
		maxem->ecmm[maxem->result_num].cmm.result_lists_p[0] = cmm.result_lists_p[i];
		maxem->ecmm[maxem->result_num].cmm.result_lists_d[0] = cmm.result_lists_d[i];
		maxem->ecmm[maxem->result_num].cmm.pure_score[0] = cmm.pure_score[i];

		for (k = maxem->result_num - 1; k >= 0; k--) {
		    if (maxem->ecmm[k].cmm.score < maxem->ecmm[k + 1].cmm.score) {
			tempecmm = maxem->ecmm[k];
			maxem->ecmm[k] = maxem->ecmm[k + 1];
			maxem->ecmm[k + 1] = tempecmm;
		    }
		    else {
			break;
		    }
		}

		if (maxem->result_num < CMM_MAX - 1) {
		    maxem->result_num++;
		}
	    }

	    /* 格フレームの追加エントリの削除 */
	    if (!(OptEllipsis & OPT_DEMO)) {
		DeleteFromCF(&workem, cpm_ptr, &cmm, i);
	    }
	}
    }
    free(cf_array);
}

/*==================================================================*/
	   void AssignFeaturesByProgram(SENTENCE_DATA *sp)
/*==================================================================*/
{
    /* 撲滅予定 */

    int i;

    for (i = 0; i < sp->Tag_num; i++) {
	/* 隣以外の AのB はルールで与えられていない */
	if (!check_feature((sp->tag_data + i)->f, "準主題表現") && 
	    check_feature((sp->tag_data + i)->f, "係:ノ格") && 
	    (sp->tag_data + i)->parent && 
	    check_feature((sp->tag_data + i)->parent->f, "主題表現")) {
	    assign_cfeature(&((sp->tag_data + i)->f), "準主題表現");
	}
    }
}

/*==================================================================*/
      int mark_location_classes(SENTENCE_DATA *sp, TAG_DATA *tp)
/*==================================================================*/
{
    int i, j;
    SENTENCE_DATA *cs;

    cs = sentence_data + sp->Sen_num - 1;

    LC = (int **)malloc_data(sizeof(int *) * sp->Sen_num, "mark_location_classes");
    for (i = 0; i < sp->Sen_num; i++) {
	LC[i] = (int *)malloc_data(sizeof(int) * TAG_MAX, "mark_location_classes");
	memset(LC[i], 0, sizeof(int) * TAG_MAX);
    }

    LC[0][tp->num] = END_M; /* 自分は対象外 */
    _SearchCaseComponent(cs, tp, LC, END_M, 0); /* 自分の子供は対象外 */
    _SearchPV(cs, tp, LC);
    _SearchParentV(cs, tp, LC);
    _SearchParentNParentV(cs, tp, LC);
    _SearchParentVParentV(cs, tp, LC);
    _SearchChildPV(cs, tp, LC);
    _SearchChildV(cs, tp, LC);
    _SearchMC(cs, tp, LC, 0);
    _SearchSC(cs, tp, LC, 0);

    for (i = 0; i < cs->Tag_num; i++) {
	if (LC[0][i] != 0) {
	    continue;
	}
	if (i < tp->num) {
	    LC[0][i] = LOC_PRE_OTHERS;
	}
	else {
	    LC[0][i] = LOC_POST_OTHERS;
	}
    }

    if (cs - sentence_data > 0) {
	_SearchMC(cs - 1, NULL, LC, 1);
	_SearchSC(cs - 1, NULL, LC, 1);
	for (i = 0; i < (cs - 1)->Tag_num; i++) {
	    if (LC[1][i] != 0) {
		continue;
	    }
	    LC[1][i] = LOC_S1_OTHERS;
	}
    }

    if (cs - sentence_data > 1) {
	_SearchMC(cs - 2, NULL, LC, 2);
	_SearchSC(cs - 2, NULL, LC, 2);
	for (i = 0; i < (cs - 2)->Tag_num; i++) {
	    if (LC[2][i] != 0) {
		continue;
	    }
	    LC[2][i] = LOC_S2_OTHERS;
	}
    }

    /* 2文以前 */
    for (j = 3; j <= PrevSentenceLimit; j++) {
	if (cs - sentence_data < j) {
	    break;
	}
	for (i = 0; i < (cs - j)->Tag_num; i++) {
	    LC[j][i] = LOC_OTHERS;
	}
    }

    if (VerboseLevel >= VERBOSE2) {
	int j;

	fprintf(stderr, ";;; %s for %s(%d):", cs->KNPSID ? cs->KNPSID : "?", tp->head_ptr->Goi, tp->num);
	for (i = 0; i < sp->Sen_num; i++) {
	    for (j = 0; j < (cs - i)->Tag_num; j++) {
		fprintf(stderr, " %s(%d):%s", ((cs - i)->tag_data + j)->head_ptr->Goi, j, loc_code_to_str(LC[i][j]));
	    }
	}
	fprintf(stderr, "\n");
    }
}

/*==================================================================*/
	 void merge_cf_ptr(CASE_FRAME *cf_ptr1, CASE_FRAME *cf_ptr2)
/*==================================================================*/
{
    int i, j, k;

    for (i = 0; i < cf_ptr2->element_num; i++) {
	j = cf_ptr1->element_num + i;
	if (j >= CF_ELEMENT_MAX) {
	    break;
	}
	cf_ptr1->oblig[j] = cf_ptr2->oblig[i];                       /* oblig */
	cf_ptr1->adjacent[j] = cf_ptr2->adjacent[i];                 /* adjacent */
	for (k = 0; k < PP_ELEMENT_MAX; k++) { 
	    cf_ptr1->pp[j][k] = cf_ptr2->pp[i][k];                   /* pp */
	}
	cf_ptr1->sp[j] = cf_ptr2->sp[i];                             /* sp */
	cf_ptr1->pp_str[j] = strdup_with_check(cf_ptr2->pp_str[i]);  /* pp_str */
	cf_ptr1->sm[j] = strdup_with_check(cf_ptr2->sm[i]);          /* sm */
	cf_ptr1->sm_delete[j] = strdup_with_check(cf_ptr2->sm_delete[i]); /* sm_delete */
	if (cf_ptr2->sm_delete[i]) {
	    cf_ptr1->sm_delete_size[j] = cf_ptr2->sm_delete_size[i]; /* sm_delete_size */
	    cf_ptr1->sm_delete_num[j] = cf_ptr2->sm_delete_num[i];   /* sm_delete_num */
	}
	cf_ptr1->sm_specify[j] = strdup_with_check(cf_ptr2->sm_specify[i]); /* sm_specify */
	if (cf_ptr1->sm_specify[i]) {
	    cf_ptr1->sm_specify_size[j] = cf_ptr2->sm_specify_size[i];   /* sm_specify_size */
	    cf_ptr1->sm_specify_num[j] = cf_ptr2->sm_specify_num[i];     /* sm_specify_num */
	}
	cf_ptr1->ex[j] = strdup_with_check(cf_ptr2->ex[i]);                     /* ex */
	if (cf_ptr2->ex_num[i]) {
	    cf_ptr1->ex_list[j] = (char **)malloc_data(sizeof(char *)*cf_ptr2->ex_num[i], "merge_cf_ptr");
	    for (k = 0; k < cf_ptr2->ex_num[i]; k++) {
		cf_ptr1->ex_list[j][k] = strdup_with_check(cf_ptr2->ex_list[i][k]); /* ex_list */
	    }
	    /* ex_freq */
	    cf_ptr1->ex_freq[j] = (int *)malloc_data(sizeof(int)*cf_ptr2->ex_num[i], "merge_cf_ptr");
	    for (k = 0; k < cf_ptr2->ex_num[i]; k++) {
		cf_ptr1->ex_freq[j][k] = cf_ptr2->ex_freq[i][k];
	    }
	    cf_ptr1->ex_size[j] = cf_ptr2->ex_size[i];                   /* ex_size */
	    cf_ptr1->ex_num[j] = cf_ptr2->ex_num[i];                     /* ex_num */
	}
	cf_ptr1->semantics[j] = strdup_with_check(cf_ptr2->semantics[i]); /* semantics */
    }
    cf_ptr1->element_num += i;                                          /* element_num */
}

/*==================================================================*/
	 void merge_em(ELLIPSIS_MGR *em1, ELLIPSIS_MGR *em2)
/*==================================================================*/
{
    int i, j, k;

    em1->score += em2->score;
    em1->pure_score += em2->pure_score;

    for (i = 0; i < CASE_TYPE_NUM; i++) {
	if (em2->cc[i].s) {
	    em1->cc[i] = em2->cc[i];
	    em2->cc[i].s = NULL;
	    em2->cc[i].pp_str = NULL;
	    em2->cc[i].bnst = 0;
	    em2->cc[i].score = 0;
	    em2->cc[i].dist = 0;
	}
    }

    em1->ecmm[0].cpm.score += em2->ecmm[0].cpm.score;

    for (i = 0; i < em2->ecmm[0].element_num; i++) {
	j = em1->ecmm[0].element_num + i;
	if (j >= CF_ELEMENT_MAX) {
	    break;
	}
	/* CF_PRED */
	for (k = 0; k < PP_ELEMENT_MAX; k++) { 
	    em1->ecmm[0].cpm.cf.pp[j][k] = em2->ecmm[0].cpm.cf.pp[i][k];
	}
	strcpy(em1->ecmm[0].cpm.cf.sm[j], em2->ecmm[0].cpm.cf.sm[i]);
	em1->ecmm[0].cpm.elem_b_ptr[j] = em2->ecmm[0].cpm.elem_b_ptr[i];
	em1->ecmm[0].cpm.elem_s_ptr[j] = em2->ecmm[0].cpm.elem_s_ptr[i];
	em1->ecmm[0].cpm.elem_b_num[j] = em2->ecmm[0].cpm.elem_b_num[i];
	
	/* CF_MATCH */
	em1->ecmm[0].cmm.result_lists_d[0].flag[j]
	    = em2->ecmm[0].cmm.result_lists_p[0].flag[i] 
	    + em1->ecmm[0].cmm.cf_ptr->element_num;
	em1->ecmm[0].cmm.result_lists_d[0].score[j]
	    = em2->ecmm[0].cmm.result_lists_p[0].score[i];
	em1->ecmm[0].cmm.result_lists_d[0].pos[j]
	    = em2->ecmm[0].cmm.result_lists_p[0].pos[i];
    }
    if (em2->ecmm[0].cmm.cf_ptr) {
	for (i = 0; i < em2->ecmm[0].cmm.cf_ptr->element_num; i++) {
	    j = em1->ecmm[0].cmm.cf_ptr->element_num + i;
	    em1->ecmm[0].cmm.result_lists_p[0].flag[j]
		= em2->ecmm[0].cmm.result_lists_p[0].flag[i]
		+ em1->ecmm[0].element_num;
	    em1->ecmm[0].cmm.result_lists_p[0].score[j]
		= em2->ecmm[0].cmm.result_lists_p[0].score[i];
	    em1->ecmm[0].cmm.result_lists_p[0].pos[j]
		= em2->ecmm[0].cmm.result_lists_p[0].pos[i];
	}
	merge_cf_ptr(em1->ecmm[0].cmm.cf_ptr, em2->ecmm[0].cmm.cf_ptr);
    }
    em1->ecmm[0].element_num += em2->ecmm[0].element_num;
}

/*==================================================================*/
	      void DiscourseAnalysis(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, k, l, mainflag;
    float score;
    ELLIPSIS_MGR workem, maxem, maxem_copula;
    SENTENCE_DATA *sp_new;
    CF_PRED_MGR *cpm_ptr;
    CF_MATCH_MGR *cmm_ptr;
    CASE_FRAME *cf_ptr;

    InitEllipsisMGR(&workem);
    InitEllipsisMGR(&maxem);
    InitEllipsisMGR(&maxem_copula);

    AssignFeaturesByProgram(sp);

    sp_new = PreserveSentence(sp);

    if (sp->available) {
	Bcheck = (int **)malloc_data(sizeof(int *) * sp->Sen_num, "DiscourseAnalysis");
	for (i = 0; i < sp->Sen_num; i++) {
	    Bcheck[i] = (int *)malloc_data(sizeof(int) * TAG_MAX, "DiscourseAnalysis");
	}

	/* for (j = 0; j < sp->Best_mgr->pred_num; j++) { */
	/* 各用言をチェック (文頭から) */
	for (j = sp->Best_mgr->pred_num - 1; j >= 0; j--) {
	    cpm_ptr = &(sp->Best_mgr->cpm[j]);

	    /* 格フレームがない場合 (ガ格ぐらい探してもいいかもしれない) 
	       格解析が失敗した場合 */
	    if (cpm_ptr->result_num == 0 || 
		cpm_ptr->cmm[0].cf_ptr->cf_address == -1 || 
		cpm_ptr->cmm[0].score < 0) {
		continue;
	    }

	    /* 省略解析しない用言 */
	    if (check_feature(cpm_ptr->pred_b_ptr->f, "省略解析なし")) {
		continue;
	    }
	    /* 固有名詞は省略解析しない (用言に対して) */
	    else if (cpm_ptr->cf.type == CF_PRED && 
		     (check_feature((sp->bnst_data + cpm_ptr->pred_b_ptr->bnum)->f, "人名") || 
		      check_feature((sp->bnst_data + cpm_ptr->pred_b_ptr->bnum)->f, "地名") || 
		      check_feature((sp->bnst_data + cpm_ptr->pred_b_ptr->bnum)->f, "組織名"))) {
		assign_cfeature(&(cpm_ptr->pred_b_ptr->f), "省略解析なし");
		continue;
	    }

	    mark_location_classes(sp, cpm_ptr->pred_b_ptr);

	    cmm_ptr = &(cpm_ptr->cmm[0]);
	    cf_ptr = cmm_ptr->cf_ptr;

	    /* その文の主節 */
	    if (check_feature(cpm_ptr->pred_b_ptr->f, "主節")) {
		mainflag = 1;
	    }
	    else {
		mainflag = 0;
	    }

	    /* もっともスコアがよくなる順番で省略の指示対象を決定する */

	    maxem.score = -2;

	    if (cpm_ptr->cf.type == CF_NOUN) {
		FindBestCFforContext(sp, &maxem, cpm_ptr, NULL);
	    }
	    else {
		for (i = 0; i < CASE_ORDER_MAX; i++) {
		    if (cpm_ptr->decided == CF_DECIDED) {

			/* 入力側格要素を設定
			   照応解析時はすでにある格要素を上書きしてしまうのでここで再設定
			   それ以外のときは下の DeleteFromCF() で省略要素をクリア */
			if (OptEllipsis & OPT_DEMO) {
			    make_data_cframe(sp, cpm_ptr);
			}

			ClearEllipsisMGR(&workem);
			score = EllipsisDetectForVerbMain(sp, &workem, cpm_ptr, &(cpm_ptr->cmm[0]), 0, 
							  cpm_ptr->cmm[0].cf_ptr, CaseOrder[i]);
			/* 直接の格要素の正規化していないスコアを足す */
			workem.score += cpm_ptr->cmm[0].pure_score[0];
			workem.pure_score += workem.score;
			workem.score /= sqrt((double)(count_pat_element(cpm_ptr->cmm[0].cf_ptr, 
									&(cpm_ptr->cmm[0].result_lists_p[0]))));
			if (workem.score > maxem.score) {
			    maxem = workem;
			    maxem.result_num = cpm_ptr->result_num;
			    for (k = 0; k < maxem.result_num; k++) {
				maxem.ecmm[k].cmm = cpm_ptr->cmm[k];
				maxem.ecmm[k].cpm = *cpm_ptr;
				maxem.ecmm[k].element_num = cpm_ptr->cf.element_num;
			    }
			    workem.f = NULL;
			}

			/* 格フレームの追加エントリの削除 */
			if (!(OptEllipsis & OPT_DEMO)) {
			    DeleteFromCF(&workem, cpm_ptr, &(cpm_ptr->cmm[0]), 0);
			}
			
		    }
		    /* 格フレーム未決定のとき */
		    else {
			FindBestCFforContext(sp, &maxem, cpm_ptr, CaseOrder[i]);
		    }
		}
		if (cpm_ptr->cf.type_flag && (OptEllipsis & OPT_REL_NOUN)) {
		    cpm_ptr->cf.type = CF_NOUN;
		    maxem_copula.score = -2;
		    FindBestCFforContext(sp, &maxem_copula, cpm_ptr, NULL);
		    if (maxem_copula.score > -2) {
			merge_em(&maxem, &maxem_copula);
		    }
		    cpm_ptr->cf.type = CF_PRED;
		} 
	    }
	    
	    /* もっとも score のよかった組み合わせを登録 */
	    if (maxem.score > -2) {
		cpm_ptr->score = maxem.score;
		maxem.ecmm[0].cmm.score = maxem.score;
		maxem.ecmm[0].cmm.pure_score[0] = maxem.pure_score;
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
		    cpm_ptr->elem_s_ptr[k] = maxem.ecmm[0].cpm.elem_s_ptr[k];
		    for (l = 0; l < PP_ELEMENT_MAX; l++) {
			cpm_ptr->cf.pp[k][l] = maxem.ecmm[0].cpm.cf.pp[k][l];
		    }
		    strcpy(cpm_ptr->cf.sm[k], maxem.ecmm[0].cpm.cf.sm[k]);
		}
		/* feature の伝搬 */
		append_feature(&(cpm_ptr->pred_b_ptr->f), maxem.f);
		maxem.f = NULL;

		/* 文脈解析において格フレームを決定した場合 */
		if (cpm_ptr->decided != CF_DECIDED) {
		    after_case_analysis(sp, cpm_ptr);
		    if (OptCaseFlag & OPT_CASE_ASSIGN_GA_SUBJ) {
			assign_ga_subject(sp_new, cpm_ptr); /* CF_CAND_DECIDED の場合は行っているが */
		    }
		    if (OptUseSmfix == TRUE) {
			specify_sm_from_cf(sp_new, cpm_ptr);
		    }
		}

		record_match_ex(sp, cpm_ptr);

		/* 格解析の結果を feature へ */
		record_case_analysis(sp, cpm_ptr, &maxem, mainflag);
	    }
	    ClearEllipsisMGR(&maxem);
	    ClearEllipsisMGR(&maxem_copula);

	    /* 格フレームの保存 */
	    if (cpm_ptr->cmm[0].score > 0) {
		RegisterCF(cpm_ptr->cmm[0].cf_ptr->cf_id);
	    }

	    for (i = 0; i < sp->Sen_num; i++) {
		free(LC[i]);
	    }
	    free(LC);
	}

	PreserveCPM(sp_new, sp);

	for (i = 0; i < sp->Sen_num; i++) {
	    free(Bcheck[i]);
	}
	free(Bcheck);
    }
    clear_cf(0);
}

/*====================================================================
                               END
====================================================================*/
