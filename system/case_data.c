/*====================================================================

			  格構造解析: 入力側

                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

char fukugoji_string[64];

/*==================================================================*/
	     char *make_fukugoji_string(BNST_DATA *b_ptr)
/*==================================================================*/
{
    int i;

    fukugoji_string[0] = '\0';

    /* 前の文節の助詞 */
    strcat(fukugoji_string, 
	   ((sp->bnst_data+b_ptr->num-1)->fuzoku_ptr+(sp->bnst_data+b_ptr->num-1)->fuzoku_num-1)->Goi);

    /* この文節 */
    for (i = 0; i < b_ptr->mrph_num; i++) {
	if ((b_ptr->mrph_ptr+i)->Hinshi == 1)	/* 特殊以外 */
	    continue;
	strcat(fukugoji_string, 
	       (b_ptr->mrph_ptr+i)->Goi);
    }

    return fukugoji_string;
}

/*==================================================================*/
BNST_DATA *_make_data_cframe_pp(CF_PRED_MGR *cpm_ptr, BNST_DATA *b_ptr)
/*==================================================================*/
{
    int i, num;
    CASE_FRAME *c_ptr = &(cpm_ptr->cf);

    if (b_ptr == NULL) {	/* 埋め込み文の被修飾詞 */
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("＊");
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:ガ格") || 
	     (!check_feature(cpm_ptr->pred_b_ptr->f, "用言:判") &&
	      check_feature(b_ptr->f, "係:ノ格"))) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("が");
	c_ptr->oblig[c_ptr->element_num] = TRUE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:ヲ格")) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("を");
	c_ptr->oblig[c_ptr->element_num] = TRUE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:ヘ格")) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("へ");
	c_ptr->oblig[c_ptr->element_num] = TRUE;
	return b_ptr;
    }

    else if (check_feature(b_ptr->f, "係:ニ格")) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("に");
	if (check_feature(b_ptr->f, "時間"))
	  c_ptr->oblig[c_ptr->element_num] = FALSE;
	else
	  c_ptr->oblig[c_ptr->element_num] = TRUE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:ヨリ格")) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("より");
	if (check_feature(b_ptr->f, "時間"))
	  c_ptr->oblig[c_ptr->element_num] = FALSE;
	else
	  c_ptr->oblig[c_ptr->element_num] = TRUE;
	return b_ptr;
    }

    else if (check_feature(b_ptr->f, "係:デ格")) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("で");
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:カラ格")) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("から");
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:ト格")) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("と");
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:マデ格")) {
	c_ptr->pp[c_ptr->element_num][0] = -1;
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:無格")) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("φ");
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "係:未格")) {
	c_ptr->pp[c_ptr->element_num][0] = -1;
	if (check_feature(b_ptr->f, "時間"))
	  c_ptr->oblig[c_ptr->element_num] = FALSE;
	else 
	  c_ptr->oblig[c_ptr->element_num] = TRUE;
	return b_ptr;
    }
    else if (check_feature(b_ptr->f, "複合辞") && b_ptr->child[0]) {
	c_ptr->pp[c_ptr->element_num][0] = 
	    pp_hstr_to_code(make_fukugoji_string(b_ptr));
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

    strcpy(c_ptr->ex[c_ptr->element_num], b_ptr->BGH_code);
}

/*==================================================================*/
     void make_data_cframe(CF_PRED_MGR *cpm_ptr)
/*==================================================================*/
{
    BNST_DATA *b_ptr = cpm_ptr->pred_b_ptr;
    BNST_DATA *cel_b_ptr;
    int i, j, k, child_num;
    
    /* 表層格 etc. の設定 */

    cpm_ptr->cf.element_num = 0;
    if (check_feature(b_ptr->f, "係:連格")) {

	/* para_type == PARA_NORMAL は「Vし,Vした PARA N」のとき
	   このときは親(PARA)の親(N)を格要素とする．

	   親がpara_top_pかどうかをみても「VしたNとN PARA」の
	   時と区別ができない
        */

	if (b_ptr->para_type != PARA_NORMAL) {
	    if (b_ptr->parent && 
		!check_feature(b_ptr->parent->f, "外の関係")) {

		_make_data_cframe_pp(cpm_ptr, NULL);
		_make_data_cframe_sm(cpm_ptr, b_ptr->parent);
		_make_data_cframe_ex(cpm_ptr, b_ptr->parent);
		cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = b_ptr->parent;
		cpm_ptr->elem_b_num[cpm_ptr->cf.element_num] = -1;
		cpm_ptr->cf.element_num ++;
	    }
	} else {
	    if (b_ptr->parent && 
		b_ptr->parent->parent && 
		!check_feature(b_ptr->parent->parent->f, "外の関係")) {

		_make_data_cframe_pp(cpm_ptr, NULL);
		_make_data_cframe_sm(cpm_ptr, b_ptr->parent->parent);
		_make_data_cframe_ex(cpm_ptr, b_ptr->parent->parent);
		cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = b_ptr->parent->parent;
		cpm_ptr->elem_b_num[cpm_ptr->cf.element_num] = -1;
		cpm_ptr->cf.element_num ++;
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
	    cpm_ptr->cf.element_num ++;
	}
	if (cpm_ptr->cf.element_num > CF_ELEMENT_MAX) {
	    cpm_ptr->cf.element_num = 0;
	    return;
	}
    }
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



