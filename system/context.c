/*====================================================================

			       文脈解析

                                               S.Kurohashi 98. 9. 8

    $Id$
====================================================================*/
#include "knp.h"

/*==================================================================*/
      void copy_cf_with_alloc(CASE_FRAME *dst, CASE_FRAME *src)
/*==================================================================*/
{
    int i, j;

    dst->element_num = src->element_num;
    for (i = 0; i < src->element_num; i++) {
	dst->oblig[i] = src->oblig[i];
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
			void clear_sentence()
/*==================================================================*/
{
    int i;
    SENTENCE_DATA *s;
    for (i = 0; i < sp->Sen_num-1; i++) {
	s = sentence_data+i;
	free(s->mrph_data);
	free(s->bnst_data);
	free(s->para_data);
	free(s->para_manager);
	free(s->cpm);
	free(s->cf);
	free(s->KNPSID);
    }
    sp->Sen_num = 1;
}

/*==================================================================*/
			 void copy_sentence()
/*==================================================================*/
{
    /* 文解析結果の保持 */

    int i, j, k, num, cfnum = 0;
    SENTENCE_DATA *sp_new;

    /* 一時的措置 */
    if (sp->Sen_num > 256) {
	fprintf(stderr, "Sentence buffer overflowed!\n");
	clear_sentence();
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

    sp_new->KNPSID = strdup(sp->KNPSID);

    sp_new->para_data = (PARA_DATA *)malloc_data(sizeof(PARA_DATA)*Para_num, 
				 "PARA DATA");
    for (i = 0; i < Para_num; i++) {
	sp_new->para_data[i] = sp->para_data[i];
	sp_new->para_data[i].manager_ptr += sp_new->para_manager - sp->para_manager;
    }

    sp_new->para_manager = 
	(PARA_MANAGER *)malloc_data(sizeof(PARA_MANAGER)*Para_M_num, 
				    "PARA MANAGER");
    for (i = 0; i < Para_M_num; i++) {
	sp_new->para_manager[i] = sp->para_manager[i];
	sp_new->para_manager[i].parent = sp_new->para_manager - sp->para_manager;
	for (j = 0; j < sp_new->para_manager[i].child_num; j++) {
	    sp_new->para_manager[i].child[j] = sp_new->para_manager - sp->para_manager;
	}
	sp_new->para_manager[i].bnst_ptr = sp_new->bnst_data - sp->bnst_data;
    }

    /* 格解析結果の保存 */
    sp_new->cpm = 
	(CF_PRED_MGR *)malloc_data(sizeof(CF_PRED_MGR)*Best_mgr.pred_num, 
				   "CF PRED MGR");

    /* 格フレームの個数分だけ確保 */
    for (i = 0; i < Best_mgr.pred_num; i++) {
	cfnum += Best_mgr.cpm[i].result_num;
    }
    sp_new->cf = (CASE_FRAME *)malloc_data(sizeof(CASE_FRAME)*cfnum, 
					   "CASE FRAME");

    cfnum = 0;
    for (i = 0; i < Best_mgr.pred_num; i++) {
	num = Best_mgr.cpm[i].pred_b_ptr->num;	/* この用言の文節番号 */
	*(sp_new->cpm+i) = Best_mgr.cpm[i];
	sp_new->bnst_data[num].cpm_ptr = sp_new->cpm+i;
	(sp_new->cpm+i)->pred_b_ptr = sp_new->bnst_data+num;
	for (j = 0; j < (sp_new->cpm+i)->cf.element_num; j++) {
	    (sp_new->cpm+i)->elem_b_ptr[j] = sp_new->bnst_data+(sp_new->cpm+i)->elem_b_num[j];
	}

	(sp_new->cpm+i)->pred_b_ptr->cf_ptr = sp_new->cf+cfnum;
	for (j = 0; j < (sp_new->cpm+i)->result_num; j++) {
	    copy_cf_with_alloc(sp_new->cf+cfnum, (sp_new->cpm+i)->cmm[j].cf_ptr);
	    (sp_new->cpm+i)->cmm[j].cf_ptr = sp_new->cf+cfnum;
	    cfnum++;
	}
    }
}

/*==================================================================*/
float CalcSimilarityForVerb(BNST_DATA *cand, CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{
    char *exd, *exp;
    int i, j, step;
    float score = -1, tempscore;

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

    /* どちらかに用例のコードがないとき */
    if (!(exd && exp && *exd && *exp)) {
	return 0;
    }

    /* 最大マッチスコアを求める */
    for (j = 0; exp[j]; j+=step) {
	for (i = 0; exd[i]; i+=step) {
	    if (Thesaurus == USE_BGH) {
		tempscore = _ex_match_score(exp+j, exd+i);
		tempscore /= 7;
	    }
	    else if (Thesaurus == USE_NTT) {
		tempscore = ntt_code_match(exp+j, exd+i);
	    }
	    if (tempscore > score) {
		score = tempscore;
	    }
	}
    }

    if (score > 0) {
	return score;
    }
    return 0;
}

/*==================================================================*/
void EllipsisDetectForVerb(CF_PRED_MGR *cpm_ptr, CASE_FRAME *cf_ptr, int n)
/*==================================================================*/
{
    /* cf_ptr = cpm_ptr->cmm[0].cf_ptr である */
    /* 用言 cpm_ptr の cf_ptr->pp[n][0] 格が省略されている
       cf_ptr->ex[n] に似ている文節を探す */

    int i, maxi, current = 1, bend;
    float score, maxscore = 0;
    char feature_buffer[DATA_LEN];
    SENTENCE_DATA *s, *maxs;

    /* 前の文の体言を探す (この用言の格要素になっているもの以外) */
    for (s = sentence_data + sp->Sen_num - 1; s >= sentence_data; s--) {
	if (current) {
	    bend = cpm_ptr->pred_b_ptr->num;
	}
	else {
	    bend = s->Bnst_num;
	}

	for (i = bend; i >= 0; i--) {
	    if (current && 
		(s->bnst_data+i)->dpnd_head == cpm_ptr->pred_b_ptr->num)
		continue;
	    if (check_feature((s->bnst_data+i)->f, "体言") && 
		!check_feature((s->bnst_data+i)->f, "形副名詞") && 
		!check_feature((s->bnst_data+i)->f, "時間") && 
		!check_feature((s->bnst_data+i)->f, "数量")) {
		score = CalcSimilarityForVerb(s->bnst_data+i, cf_ptr, n);
		if (score > maxscore) {
		    maxscore = score;
		    maxs = s;
		    maxi = i;
		}
		if (score > 0) {
		    /* 省略候補 */
		    sprintf(feature_buffer, "C%s;%s:%.3f", (s->bnst_data+i)->Jiritu_Go, 
			    pp_code_to_kstr(cf_ptr->pp[n][0]), 
			    score);
		    assign_cfeature(&(cpm_ptr->pred_b_ptr->f), feature_buffer);
		}
	    }
	}
	if (current)
	    current = 0;
    }

    if (maxscore > 0) {
	/* 決定した省略関係 */
	sprintf(feature_buffer, "C【%s】;%s:%.3f", (maxs->bnst_data+maxi)->Jiritu_Go, 
		pp_code_to_kstr(cf_ptr->pp[n][0]), 
		maxscore);
	assign_cfeature(&(cpm_ptr->pred_b_ptr->f), feature_buffer);
    }
}

/*==================================================================*/
	      void EllipsisDetectForNoun(BNST_DATA *bp)
/*==================================================================*/
{
    char **def;
    int i;

    def = GetDefinitionFromBunsetsu(bp);
    if (!def) {
	return;
    }

    for (i = 0; *(def+i); i++) {
	fprintf(stderr, "定義文[%s] %d: %s\n", bp->Jiritu_Go, i, *(def+i));
    }
}

/*==================================================================*/
		      void discourse_analysis()
/*==================================================================*/
{
    int i, j, num;
    CF_PRED_MGR *cpm_ptr;
    CF_MATCH_MGR *cmm_ptr;
    CASE_FRAME *cf_ptr;
    BNST_DATA *pred_b_ptr;

    copy_sentence();

    /* 各用言をチェック */
    for (j = 0; j < Best_mgr.pred_num; j++) {
	cpm_ptr = &(Best_mgr.cpm[j]);

	/* 格フレームがない場合 */
	if (cpm_ptr->result_num == 0) {
	    continue;
	}

	cmm_ptr = &(cpm_ptr->cmm[0]);
	cf_ptr = cmm_ptr->cf_ptr;
	pred_b_ptr = cpm_ptr->pred_b_ptr;

	for (i = 0; i < cf_ptr->element_num; i++) {
	    num = cmm_ptr->result_lists_p[0].flag[i];
	    /* とりあえず省略要素と認定する条件を設定 
	       1. 準用言ではない
	       2. 時間格ではない */
	    if (num == UNASSIGNED && cmm_ptr->score != -2 && 
		!check_feature(pred_b_ptr->f, "準用言") && 
		!str_eq((char *)pp_code_to_kstr(cmm_ptr->cf_ptr->pp[i][0]), "時間")) {
		EllipsisDetectForVerb(cpm_ptr, cmm_ptr->cf_ptr, i);
	    }
	}
    }

    /* 各体言をチェック */
    for (i = sp->Bnst_num-1; i >= 0; i--) {
	if (check_feature((sp->bnst_data+i)->f, "体言") && 
	    !check_feature((sp->bnst_data+i)->f, "形副名詞") && 
	    !check_feature((sp->bnst_data+i)->f, "時間") && 
	    !check_feature((sp->bnst_data+i)->f, "数量")) {
	    EllipsisDetectForNoun(sp->bnst_data+i);
	}
    }
}

/*====================================================================
                               END
====================================================================*/
