/*====================================================================

			  ³Ê¹½Â¤²òÀÏ: ÆþÎÏÂ¦

                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

/*==================================================================*/
       int _make_data_cframe_pp(CF_PRED_MGR *cpm_ptr, BNST_DATA *b_ptr)
/*==================================================================*/
{
    int i, num, flag;
    CASE_FRAME *c_ptr = &(cpm_ptr->cf);

    if (b_ptr == NULL) {	/* Ëä¤á¹þ¤ßÊ¸¤ÎÈï½¤¾þ»ì */
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("¡ö");
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	flag = TRUE;
    }
    else if (check_feature(b_ptr->f, "·¸:¥¬³Ê") || 
	     (!check_feature(cpm_ptr->pred_b_ptr->f, "ÍÑ¸À:¶¯:È½") &&
	      check_feature(b_ptr->f, "·¸:¥Î³Ê"))) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("¤¬");
	c_ptr->oblig[c_ptr->element_num] = TRUE;
	flag = TRUE;
    }
    else if (check_feature(b_ptr->f, "·¸:¥ò³Ê")) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("¤ò");
	c_ptr->oblig[c_ptr->element_num] = TRUE;
	flag = TRUE;
    }
    else if (check_feature(b_ptr->f, "·¸:¥Ø³Ê")) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("¤Ø");
	c_ptr->oblig[c_ptr->element_num] = TRUE;
	flag = TRUE;
    }

    else if (check_feature(b_ptr->f, "·¸:¥Ë³Ê")) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("¤Ë");
	if (check_feature(b_ptr->f, "»þ´Ö"))
	  c_ptr->oblig[c_ptr->element_num] = FALSE;
	else
	  c_ptr->oblig[c_ptr->element_num] = TRUE;
	flag = TRUE;
    }
    else if (check_feature(b_ptr->f, "·¸:¥è¥ê³Ê")) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("¤è¤ê");
	if (check_feature(b_ptr->f, "»þ´Ö"))
	  c_ptr->oblig[c_ptr->element_num] = FALSE;
	else
	  c_ptr->oblig[c_ptr->element_num] = TRUE;
	flag = TRUE;
    }

    else if (check_feature(b_ptr->f, "·¸:¥Ç³Ê")) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("¤Ç");
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	flag = TRUE;
    }
    else if (check_feature(b_ptr->f, "·¸:¥«¥é³Ê")) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("¤«¤é");
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	flag = TRUE;
    }
    else if (check_feature(b_ptr->f, "·¸:¥È³Ê")) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("¤È");
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	flag = TRUE;
    }
    else if (check_feature(b_ptr->f, "·¸:¥Þ¥Ç³Ê")) {
	c_ptr->pp[c_ptr->element_num][0] = -1;
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	flag = TRUE;
    }
    else if (check_feature(b_ptr->f, "·¸:Ìµ³Ê")) {
	c_ptr->pp[c_ptr->element_num][0] = pp_hstr_to_code("¦Õ");
	c_ptr->oblig[c_ptr->element_num] = FALSE;
	flag = TRUE;
    }
    else if (check_feature(b_ptr->f, "·¸:Ì¤³Ê")) {
	c_ptr->pp[c_ptr->element_num][0] = -1;
	if (check_feature(b_ptr->f, "»þ´Ö"))
	  c_ptr->oblig[c_ptr->element_num] = FALSE;
	else 
	  c_ptr->oblig[c_ptr->element_num] = TRUE;
	flag = TRUE;
    }
    else {
	flag = FALSE;
    }

    return flag;
}

/*==================================================================*/
   void _make_data_cframe_sm(CF_PRED_MGR *cpm_ptr, BNST_DATA *b_ptr)
/*==================================================================*/
{
    int i, sm_num = 0, qua_flag = FALSE, tim_flag = FALSE;
    CASE_FRAME *c_ptr = &(cpm_ptr->cf);

    if (check_feature(b_ptr->f, "·¸:¥È³Ê") && /* ³ÊÍ×ÁÇ -- Ê¸ */
	check_feature(b_ptr->f, "ÍÑ¸À:¶¯")) {
	strcpy(c_ptr->sm[c_ptr->element_num]+SM_CODE_SIZE*sm_num, 
	       (char *)sm2code("ÊäÊ¸"));
	sm_num++;
    }
    else {
	if (check_feature(b_ptr->f, "»þ´Ö")) {
	    strcpy(c_ptr->sm[c_ptr->element_num]+SM_CODE_SIZE*sm_num, 
		   (char *)sm2code("»þ´Ö"));
	    sm_num++;
	}
	if (check_feature(b_ptr->f, "¿ôÎÌ")) {
	    strcpy(c_ptr->sm[c_ptr->element_num]+SM_CODE_SIZE*sm_num, 
		   (char *)sm2code("¿ôÎÌ"));
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
    int i, j, k, child_num;
    
    /* É½ÁØ³Ê etc. ¤ÎÀßÄê */

    cpm_ptr->cf.element_num = 0;
    if (check_feature(b_ptr->f, "·¸:Ï¢³Ê")) {

	/* para_type == PARA_NORMAL ¤Ï¡ÖV¤·,V¤·¤¿ PARA N¡×¤Î¤È¤­
	   ¤³¤Î¤È¤­¤Ï¿Æ(PARA)¤Î¿Æ(N)¤ò³ÊÍ×ÁÇ¤È¤¹¤ë¡¥

	   ¿Æ¤¬para_top_p¤«¤É¤¦¤«¤ò¤ß¤Æ¤â¡ÖV¤·¤¿N¤ÈN PARA¡×¤Î
	   »þ¤È¶èÊÌ¤¬¤Ç¤­¤Ê¤¤
        */

	if (b_ptr->para_type != PARA_NORMAL) {
	    if (b_ptr->parent && 
		!check_feature(b_ptr->parent->f, "³°¤Î´Ø·¸")) {

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
		!check_feature(b_ptr->parent->parent->f, "³°¤Î´Ø·¸")) {

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
	if (_make_data_cframe_pp(cpm_ptr, b_ptr->child[i]) == TRUE) {
	    _make_data_cframe_sm(cpm_ptr, b_ptr->child[i]);
	    _make_data_cframe_ex(cpm_ptr, b_ptr->child[i]);
	    cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = b_ptr->child[i];
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
    /* ¥ô¥©¥¤¥¹¤ÎÀßÄê */ /* ¡ú¡ú¡ú ½¤ÀµÉ¬Í× ¡ú¡ú¡ú */

    int i;
    b_ptr->voice = NULL;

    if (check_feature(b_ptr->f, "¡Á¤»¤ë") ||
	check_feature(b_ptr->f, "¡Á¤µ¤»¤ë")) {
	b_ptr->voice = VOICE_SHIEKI;
    } else if (check_feature(b_ptr->f, "¡Á¤ì¤ë") ||
	       check_feature(b_ptr->f, "¡Á¤é¤ì¤ë")) {
	b_ptr->voice = VOICE_UKEMI;
    } else if (check_feature(b_ptr->f, "¡Á¤â¤é¤¦")) {
	b_ptr->voice = VOICE_MORAU;
    }
}

/*====================================================================
                               END
====================================================================*/



