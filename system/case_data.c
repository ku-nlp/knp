/*====================================================================

			  格構造解析: 入力側

                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

char fukugoji_string[64];

char *FukugojiTable[] = {"を除く", "をのぞく", 
			 "を通じる", "をつうじる", 
			 "を通ずる", "をつうずる", 
			 "を含める", "をふくめる", 
			 "を始める", "をはじめる", 
			 "に絡む", "にからむ", 
			 "に沿う", "にそう", 
			 "に向ける", "にむける", 
			 "に伴う", "にともなう", 
			 "に基づく", "にもとづく", 
			 "に対する", "にたいする", 
			 "に関する", "にかんする", 
			 "に代わる", "にかわる", 
			 "に加える", "にくわえる", 
			 "に限る", "にかぎる", 
			 "に続く", "につづく", 
			 "に合わせる", "にあわせる", 
			 "に比べる", "にくらべる", 
			 "に並ぶ", "にならぶ", 
			 "に限るぬ", "にかぎるぬ", 
			 ""};

/*==================================================================*/
	     char *make_fukugoji_string(BNST_DATA *b_ptr)
/*==================================================================*/
{
    int i;

    fukugoji_string[0] = '\0';

    /* 前の文節の助詞 */
    strcat(fukugoji_string, 
	   ((b_ptr-1)->fuzoku_ptr+(b_ptr-1)->fuzoku_num-1)->Goi);

    /* この文節 */
    for (i = 0; i < b_ptr->jiritu_num; i++) {
	if ((b_ptr->jiritu_ptr+i)->Hinshi == 1)	/* 特殊以外 */
	    continue;
	strcat(fukugoji_string, 
	       (b_ptr->jiritu_ptr+i)->Goi);
    }

    /* 原形の読みに統一 */
    for (i = 0; *(FukugojiTable[i]); i+=2) {
	if (str_eq(fukugoji_string, FukugojiTable[i])) {
	    strcpy(fukugoji_string, FukugojiTable[i+1]);
	}
    }

    return fukugoji_string;
}

/*==================================================================*/
BNST_DATA *_make_data_cframe_pp(CF_PRED_MGR *cpm_ptr, BNST_DATA *b_ptr)
/*==================================================================*/
{
    int i, num, pp_num = 0, jiritsu_num = 0;
    CASE_FRAME *c_ptr = &(cpm_ptr->cf);

    if (b_ptr && !check_feature(b_ptr->f, "数量")) {
	jiritsu_num = b_ptr->jiritu_num;
    }

    if (b_ptr == NULL) {	/* 埋め込み文の被修飾詞 */
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("＊");
	c_ptr->pp[c_ptr->element_num][1] = END_M;
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:ガ格") || 
	     (!check_feature(cpm_ptr->pred_b_ptr->f, "用言:判") &&
	      check_feature(b_ptr->f, "係:ノ格"))) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("が");
	c_ptr->pp[c_ptr->element_num][1] = END_M;
	c_ptr->oblig[c_ptr->element_num] = TRUE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:ヲ格")) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("を");
	c_ptr->pp[c_ptr->element_num][1] = END_M;
	c_ptr->oblig[c_ptr->element_num] = TRUE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:ヘ格")) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("へ");
	c_ptr->pp[c_ptr->element_num][1] = END_M;
	c_ptr->oblig[c_ptr->element_num] = TRUE;
	return b_ptr;
    }

    else if (check_feature(b_ptr->f, "係:ニ格")) {
	/* ニ格で時間なら時間格 */
	if (check_feature(b_ptr->f, "時間")) {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("時間");
	    c_ptr->oblig[c_ptr->element_num] = FALSE;
	    if (jiritsu_num > 1) {
		c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("に");
	    }
	}
	else {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("に");
	    c_ptr->oblig[c_ptr->element_num] = TRUE;
	}
	c_ptr->pp[c_ptr->element_num][pp_num] = END_M;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:ヨリ格")) {
	if (check_feature(b_ptr->f, "時間")) {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("時間");
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("より");
	    c_ptr->oblig[c_ptr->element_num] = FALSE;
	}
	else {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("より");
	    c_ptr->oblig[c_ptr->element_num] = TRUE;
	}
	c_ptr->pp[c_ptr->element_num][pp_num] = END_M;
	return b_ptr;
    }

    else if (check_feature(b_ptr->f, "係:デ格")) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("で");
	c_ptr->pp[c_ptr->element_num][1] = END_M;
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	if (check_feature(b_ptr->f, "デモ")) {
	    c_ptr->pp[c_ptr->element_num][1] = -1;
	    c_ptr->pp[c_ptr->element_num][2] = END_M;
	}
	else if (check_feature(b_ptr->f, "デハ")) {
	    c_ptr->pp[c_ptr->element_num][1] = -1;
	    c_ptr->pp[c_ptr->element_num][2] = END_M;
	}
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:カラ格")) {
	if (check_feature(b_ptr->f, "時間")) {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("時間");
	    if (jiritsu_num > 1) {
		c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("から");
	    }
	}
	else {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("から");
	}
	c_ptr->pp[c_ptr->element_num][pp_num] = END_M;
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:ト格")) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("と");
	c_ptr->pp[c_ptr->element_num][1] = END_M;
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:マデ格")) {
	if (check_feature(b_ptr->f, "時間")) {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("時間");
	    if (jiritsu_num > 1) {
		c_ptr->pp[c_ptr->element_num][pp_num++] = -1;
	    }
	}
	else {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("まで");
	    c_ptr->pp[c_ptr->element_num][pp_num++] = -1;
	}
	c_ptr->pp[c_ptr->element_num][pp_num] = END_M;
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:無格")) {
	/* 無格で時間なら時間格 */
	if (check_feature(b_ptr->f, "時間")) {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("時間");
	    if (jiritsu_num > 1) {
		c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("φ");
	    }
	}
	else {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("φ");
	}
	c_ptr->pp[c_ptr->element_num][pp_num] = END_M;
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:未格")) {
	if (check_feature(b_ptr->f, "時間")) {
	    c_ptr->pp[c_ptr->element_num][pp_num++] = pp_hstr_to_code("時間");
	    c_ptr->oblig[c_ptr->element_num] = FALSE;
	    if (jiritsu_num > 1) {
		c_ptr->pp[c_ptr->element_num][pp_num++] = -1;
	    }
	}
	else {
	    /* 提題ではないときにする?
	    if (check_feature(b_ptr->f, "提題")) {
		return NULL;
	    } */
	    c_ptr->pp[c_ptr->element_num][pp_num++] = -1;
	    c_ptr->oblig[c_ptr->element_num] = FALSE;
	}
	c_ptr->pp[c_ptr->element_num][pp_num] = END_M;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "複合辞") && 
	     check_feature(b_ptr->f, "係:連用") && 
	     b_ptr->child[0]) {
	c_ptr->pp[c_ptr->element_num][0] = 
	    pp_hstr_to_code(make_fukugoji_string(b_ptr));
	c_ptr->pp[c_ptr->element_num][1] = END_M;
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	return b_ptr->child[0];
    }
    else {
	return NULL;
    }
}

/*==================================================================*/
   void _make_data_cframe_sm(CF_PRED_MGR *cpm_ptr, BNST_DATA *b_ptr)
/*==================================================================*/
{
    int i, sm_num = 0, qua_flag = FALSE, tim_flag = FALSE;
    CASE_FRAME *c_ptr = &(cpm_ptr->cf);

    if (check_feature(b_ptr->f, "係:ト格") && /* 格要素 -- 文 */
	check_feature(b_ptr->f, "用言")) {
	strcpy(c_ptr->sm[c_ptr->element_num]+SM_CODE_SIZE*sm_num, 
	       (char *)sm2code("補文"));
	assign_cfeature(&(b_ptr->f), "補文");
	sm_num++;
    }
    else {
	if (check_feature(b_ptr->f, "時間")) {
	    strcpy(c_ptr->sm[c_ptr->element_num]+SM_CODE_SIZE*sm_num, 
		   (char *)sm2code("時間"));
	    sm_num++;
	}
	if (check_feature(b_ptr->f, "数量")) {
	    strcpy(c_ptr->sm[c_ptr->element_num]+SM_CODE_SIZE*sm_num, 
		   (char *)sm2code("数量"));
	    sm_num++;
	}
	
	/* for (i = 0; i < b_ptr->SM_num; i++) */
	strcpy(c_ptr->sm[c_ptr->element_num]+SM_CODE_SIZE*sm_num, 
	       b_ptr->SM_code);
	sm_num += strlen(b_ptr->SM_code)/SM_CODE_SIZE;
    }
}

/*==================================================================*/
   void _make_data_cframe_ex(CF_PRED_MGR *cpm_ptr, BNST_DATA *b_ptr)
/*==================================================================*/
{
    CASE_FRAME *c_ptr = &(cpm_ptr->cf);

    if (Thesaurus == USE_BGH) {
	strcpy(c_ptr->ex[c_ptr->element_num], b_ptr->BGH_code);
    }
    else if (Thesaurus == USE_NTT) {
	strcpy(c_ptr->ex2[c_ptr->element_num], b_ptr->SM_code);
    }
}

/*==================================================================*/
	      int make_data_cframe(CF_PRED_MGR *cpm_ptr)
/*==================================================================*/
{
    BNST_DATA *b_ptr = cpm_ptr->pred_b_ptr;
    BNST_DATA *cel_b_ptr;
    int i, j, k, child_num, score = 0;
    char *vtype = NULL;

    if (vtype = (char *)check_feature(b_ptr->f, "用言")) {
	vtype += 5;
	strcpy(cpm_ptr->cf.ipal_id, vtype);
    }
    else {
	cpm_ptr->cf.ipal_id[0] = '\0';
    }

    cpm_ptr->cf.pred_b_ptr = b_ptr;
    b_ptr->cpm_ptr = cpm_ptr;

    /* 表層格 etc. の設定 */

    cpm_ptr->cf.element_num = 0;
    if (check_feature(b_ptr->f, "係:連格")) {

	/* para_type == PARA_NORMAL は「Vし,Vした PARA N」のとき
	   このときは親(PARA)の親(N)を格要素とする．

	   親がpara_top_pかどうかをみても「VしたNとN PARA」の
	   時と区別ができない
        */

	if (b_ptr->para_type != PARA_NORMAL) {
	    if (b_ptr->parent) {
		if (!check_feature(b_ptr->parent->f, "外の関係")) {
		    _make_data_cframe_pp(cpm_ptr, NULL);
		    _make_data_cframe_sm(cpm_ptr, b_ptr->parent);
		    _make_data_cframe_ex(cpm_ptr, b_ptr->parent);
		    cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = b_ptr->parent;
		    cpm_ptr->elem_b_num[cpm_ptr->cf.element_num] = -1;
		    cpm_ptr->cf.weight[cpm_ptr->cf.element_num] = 0;
		    cpm_ptr->cf.element_num ++;
		}
		else {
		    score = SOTO_SCORE;
		}
	    }
	} else {
	    if (b_ptr->parent && 
		b_ptr->parent->parent) {
		if (!check_feature(b_ptr->parent->parent->f, "外の関係")) {
		    _make_data_cframe_pp(cpm_ptr, NULL);
		    _make_data_cframe_sm(cpm_ptr, b_ptr->parent->parent);
		    _make_data_cframe_ex(cpm_ptr, b_ptr->parent->parent);
		    cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = b_ptr->parent->parent;
		    cpm_ptr->elem_b_num[cpm_ptr->cf.element_num] = -1;
		    cpm_ptr->cf.weight[cpm_ptr->cf.element_num] = 0;
		    cpm_ptr->cf.element_num ++;
		}
		else {
		    score = SOTO_SCORE;
		}
	    }
	}
    }

    for (child_num=0; b_ptr->child[child_num]; child_num++);
    for (i = child_num - 1; i >= 0; i--) {
	if (cel_b_ptr = _make_data_cframe_pp(cpm_ptr, b_ptr->child[i])) {
	    _make_data_cframe_sm(cpm_ptr, cel_b_ptr);
	    _make_data_cframe_ex(cpm_ptr, cel_b_ptr);
	    cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = cel_b_ptr;
	    cpm_ptr->elem_b_num[cpm_ptr->cf.element_num] = i;
	    cpm_ptr->cf.weight[cpm_ptr->cf.element_num] = 0;
	    cpm_ptr->cf.element_num ++;
	}
	if (cpm_ptr->cf.element_num > CF_ELEMENT_MAX) {
	    cpm_ptr->cf.element_num = 0;
	    return score;
	}
    }

    /* 用言が並列のとき、格要素を expand する */
    if (b_ptr->para_type == PARA_NORMAL && 
	b_ptr->parent && 
	b_ptr->parent->para_top_p) {
	child_num = 0;
	for (i = 0; b_ptr->parent->child[i]; i++) {
	    if (b_ptr->parent->child[i]->para_type == PARA_NORMAL) {
		child_num++;
	    }
	}
	for (i = 0; b_ptr->parent->child[i]; i++) {
	    if (b_ptr->parent->child[i]->para_type == PARA_NIL) {
		if (cel_b_ptr = _make_data_cframe_pp(cpm_ptr, b_ptr->parent->child[i])) {
		    _make_data_cframe_sm(cpm_ptr, cel_b_ptr);
		    _make_data_cframe_ex(cpm_ptr, cel_b_ptr);
		    cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = cel_b_ptr;
		    cpm_ptr->elem_b_num[cpm_ptr->cf.element_num] = i;
		    cpm_ptr->cf.weight[cpm_ptr->cf.element_num] = child_num;
		    cpm_ptr->cf.element_num ++;
		}
		if (cpm_ptr->cf.element_num > CF_ELEMENT_MAX) {
		    cpm_ptr->cf.element_num = 0;
		    return score;
		}
	    }
	}
    }

    return score;
}

/*==================================================================*/
     void set_pred_voice(BNST_DATA *b_ptr)
/*==================================================================*/
{
    /* ヴォイスの設定 */ /* ★★★ 修正必要 ★★★ */

    int i;
    b_ptr->voice = NULL;

    if (check_feature(b_ptr->f, "〜せる") ||
	check_feature(b_ptr->f, "〜させる")) {
	b_ptr->voice = VOICE_SHIEKI;
    } else if (check_feature(b_ptr->f, "〜れる") ||
	       check_feature(b_ptr->f, "〜られる")) {
	b_ptr->voice = VOICE_UKEMI;
    } else if (check_feature(b_ptr->f, "〜もらう")) {
	b_ptr->voice = VOICE_MORAU;
    }
}

/*====================================================================
                               END
====================================================================*/



