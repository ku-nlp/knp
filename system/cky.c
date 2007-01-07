/*====================================================================

				 CKY

    $Id$
====================================================================*/

#include "knp.h"

typedef struct _CKY *CKYptr;
typedef struct _CKY {
    int		i;
    int		j;
    char	cp;
    double	score;		/* score at this point */
    double	para_score;	/* coordination score */
    int		para_flag;	/* coordination flag */
    char	dpnd_type;	/* type of dependency (D or P) */
    int		direction;	/* direction of dependency */
    BNST_DATA	*b_ptr;
    int 	scase_check[SCASE_CODE_SIZE];
    int		un_count;
    CF_PRED_MGR *cpm_ptr;	/* case components */
    CKYptr	left;		/* pointer to the left child */
    CKYptr	right;		/* pointer to the right child */
    CKYptr	next;		/* pointer to the next CKY data at this point */
} CKY;

#define	CKY_TABLE_MAX	50000
CKY *cky_matrix[BNST_MAX][BNST_MAX];/* CKY¹ÔÎó¤Î³Æ°ÌÃÖ¤ÎºÇ½é¤ÎCKY¥Ç¡¼¥¿¤Ø¤Î¥Ý¥¤¥ó¥¿ */
CKY cky_table[CKY_TABLE_MAX];	  /* an array of CKY data */
int cpm_allocated_cky_num = -1;

/* add a clausal modifiee to CPM */
void make_data_cframe_rentai_simple(CF_PRED_MGR *pre_cpm_ptr, TAG_DATA *d_ptr, TAG_DATA *t_ptr) {

    /* ³°¤Î´Ø·¸°Ê³°¤Î¤È¤­¤Ï³ÊÍ×ÁÇ¤Ë (³°¤Î´Ø·¸¤Ç¤â·ÁÍÆ»ì¤Î¤È¤­¤Ï³ÊÍ×ÁÇ¤Ë¤¹¤ë) */
    if (!check_feature(t_ptr->f, "³°¤Î´Ø·¸") || check_feature(d_ptr->f, "ÍÑ¸À:·Á")) {
	_make_data_cframe_pp(pre_cpm_ptr, d_ptr, FALSE);
    }
    /* °ì°Õ¤Ë³°¤Î´Ø·¸¤Ë¤¹¤ë */
    else {
	pre_cpm_ptr->cf.pp[pre_cpm_ptr->cf.element_num][0] = pp_hstr_to_code("³°¤Î´Ø·¸");
	pre_cpm_ptr->cf.pp[pre_cpm_ptr->cf.element_num][1] = END_M;
	pre_cpm_ptr->cf.oblig[pre_cpm_ptr->cf.element_num] = FALSE;
    }

    _make_data_cframe_sm(pre_cpm_ptr, t_ptr);
    _make_data_cframe_ex(pre_cpm_ptr, t_ptr);
    pre_cpm_ptr->elem_b_ptr[pre_cpm_ptr->cf.element_num] = t_ptr;
    pre_cpm_ptr->elem_b_num[pre_cpm_ptr->cf.element_num] = -1;
    pre_cpm_ptr->cf.weight[pre_cpm_ptr->cf.element_num] = 0;
    pre_cpm_ptr->cf.adjacent[pre_cpm_ptr->cf.element_num] = FALSE;
    pre_cpm_ptr->cf.element_num++;
}

char check_dpnd_possibility (SENTENCE_DATA *sp, int dep, int gov, int begin, int relax_flag) {
    if ((OptParaFix == 0 && 
	 begin >= 0 && 
	 (sp->bnst_data + dep)->para_num != -1 && 
	 Para_matrix[(sp->bnst_data + dep)->para_num][begin][gov] >= 0) || /* para score is not minus */
	(OptParaFix == 1 && 
	 Mask_matrix[dep][gov] == 2)) {   /* ÊÂÎóP */
	return 'P';
    }
    else if (OptParaFix == 1 && 
	     Mask_matrix[dep][gov] == 3) { /* ÊÂÎóI */
	return 'I';
    }
    else if (Dpnd_matrix[dep][gov] && Quote_matrix[dep][gov] && 
	     (OptParaFix == 0 || Mask_matrix[dep][gov] == 1)) {
	return Dpnd_matrix[dep][gov];
    }
    else if ((Dpnd_matrix[dep][gov] == 'R' || relax_flag) && Language != CHINESE) { /* relax */
	return 'R';
    }
    return '\0';
}

/* make an array of dependency possibilities */
void make_work_mgr_dpnd_check(SENTENCE_DATA *sp, CKY *cky_ptr, BNST_DATA *d_ptr) {
    int i, count = 0, start;

/*    if (cky_ptr->right && cky_ptr->right->dpnd_type == 'P')
	start = cky_ptr->right->j;
	else */
	start = d_ptr->num + 1;

    for (i = start; i < sp->Bnst_num; i++) {
	if (check_dpnd_possibility(sp, d_ptr->num, i, -1, ((i == sp->Bnst_num - 1) && count == 0) ? TRUE : FALSE)) {
	    Work_mgr.dpnd.check[d_ptr->num].pos[count] = i;
	    count++;
	}
    }

    Work_mgr.dpnd.check[d_ptr->num].num = count;
}

int convert_to_dpnd(SENTENCE_DATA *sp, TOTAL_MGR *Best_mgr, CKY *cky_ptr) {
    int i;
    char *cp;

    /* make case analysis result for clausal modifiee */
    if (OptAnalysis == OPT_CASE) {
	if (cky_ptr->cpm_ptr->pred_b_ptr && 
	    Best_mgr->cpm[cky_ptr->cpm_ptr->pred_b_ptr->pred_num].pred_b_ptr == NULL) {
	    copy_cpm(&(Best_mgr->cpm[cky_ptr->cpm_ptr->pred_b_ptr->pred_num]), cky_ptr->cpm_ptr, 0);
	    cky_ptr->cpm_ptr->pred_b_ptr->cpm_ptr = &(Best_mgr->cpm[cky_ptr->cpm_ptr->pred_b_ptr->pred_num]);
	}

	if (cky_ptr->left && cky_ptr->right && 
	    cky_ptr->left->cpm_ptr->pred_b_ptr && 
	    check_feature(cky_ptr->left->cpm_ptr->pred_b_ptr->f, "·¸:Ï¢³Ê")) { /* clausal modifiee */
	    CF_PRED_MGR *cpm_ptr = &(Best_mgr->cpm[cky_ptr->left->cpm_ptr->pred_b_ptr->pred_num]);

	    if (cpm_ptr->pred_b_ptr == NULL) { /* ¤Þ¤ÀBest_mgr¤Ë¥³¥Ô¡¼¤·¤Æ¤¤¤Ê¤¤¤È¤­ */
		copy_cpm(cpm_ptr, cky_ptr->left->cpm_ptr, 0);
		cky_ptr->left->cpm_ptr->pred_b_ptr->cpm_ptr = cpm_ptr;
	    }

	    make_work_mgr_dpnd_check(sp, cky_ptr->left, cky_ptr->right->b_ptr);
	    make_data_cframe_rentai_simple(cpm_ptr, cpm_ptr->pred_b_ptr, 
					   cky_ptr->right->b_ptr->tag_ptr + cky_ptr->right->b_ptr->tag_num - 1);
	    find_best_cf(sp, cpm_ptr, get_closest_case_component(sp, cpm_ptr), 1);
	}
    }

    if (cky_ptr->left && cky_ptr->right) {
	if (OptDisplay == OPT_DEBUG) {
	    printf("(%d, %d): (%d, %d) (%d, %d)\n", cky_ptr->i, cky_ptr->j, cky_ptr->left->i, cky_ptr->left->j, cky_ptr->right->i, cky_ptr->right->j);
	}

	if (cky_ptr->para_flag == 0) {
	    if (cky_ptr->dpnd_type != 'P' && 
		(cp = check_feature(cky_ptr->left->b_ptr->f, "·¸:Ìµ³Ê½¾Â°")) != NULL) {
		sscanf(cp, "%*[^:]:%*[^:]:%d", &(Best_mgr->dpnd.head[cky_ptr->left->b_ptr->num]));
		Best_mgr->dpnd.type[cky_ptr->left->b_ptr->num] = 
		    Dpnd_matrix[cky_ptr->left->b_ptr->num][cky_ptr->right->b_ptr->num];
	    }
	    else {
		if (cky_ptr->direction == RtoL) { /* <- */
		    Best_mgr->dpnd.head[cky_ptr->right->b_ptr->num] = cky_ptr->left->b_ptr->num;
		    Best_mgr->dpnd.type[cky_ptr->right->b_ptr->num] = cky_ptr->dpnd_type;
		}
		else { /* -> */
		    Best_mgr->dpnd.head[cky_ptr->left->b_ptr->num] = cky_ptr->right->b_ptr->num;
		    Best_mgr->dpnd.type[cky_ptr->left->b_ptr->num] = cky_ptr->dpnd_type;
		}
	    }
	}

	convert_to_dpnd(sp, Best_mgr, cky_ptr->left);
	convert_to_dpnd(sp, Best_mgr, cky_ptr->right);
    }
    else {
	if (OptDisplay == OPT_DEBUG) {
	    printf("(%d, %d)\n", cky_ptr->i, cky_ptr->j);
	}
    }
}

int check_scase (BNST_DATA *g_ptr, int *scase_check, int rentai, int un_count) {
    int vacant_slot_num = 0;

    /* ¶õ¤¤¤Æ¤¤¤ë¥¬³Ê,¥ò³Ê,¥Ë³Ê,¥¬£² */
    if ((g_ptr->SCASE_code[case2num("¥¬³Ê")]
	 - scase_check[case2num("¥¬³Ê")]) == 1) {
	vacant_slot_num++;
    }
    if ((g_ptr->SCASE_code[case2num("¥ò³Ê")]
	 - scase_check[case2num("¥ò³Ê")]) == 1) {
	vacant_slot_num++;
    }
    if ((g_ptr->SCASE_code[case2num("¥Ë³Ê")]
	 - scase_check[case2num("¥Ë³Ê")]) == 1 &&
	rentai == 1 &&
	check_feature(g_ptr->f, "ÍÑ¸À:Æ°")) {
	vacant_slot_num++;
	/* ¥Ë³Ê¤ÏÆ°»ì¤ÇÏ¢ÂÎ½¤¾þ¤Î¾ì¹ç¤À¤±¹ÍÎ¸¡¤¤Ä¤Þ¤êÏ¢ÂÎ
	   ½¤¾þ¤Ë³ä¤êÅö¤Æ¤ë¤À¤±¤Ç¡¤Ì¤³Ê¤Î¥¹¥í¥Ã¥È¤È¤Ï¤·¤Ê¤¤ */
    }
    if ((g_ptr->SCASE_code[case2num("¥¬£²")]
	 - scase_check[case2num("¥¬£²")]) == 1) {
	vacant_slot_num++;
    }

    /* ¶õ¤­¥¹¥í¥Ã¥ÈÊ¬¤À¤±Ï¢ÂÎ½¤¾þ¡¤Ì¤³Ê¤Ë¥¹¥³¥¢¤òÍ¿¤¨¤ë */
    if ((rentai + un_count) <= vacant_slot_num) {
	return (rentai + un_count) * 10;
    }
    else {
	return vacant_slot_num * 10;
    }
}

/* conventional scoring function */
double calc_score(SENTENCE_DATA *sp, CKY *cky_ptr) {
    CKY *right_ptr = cky_ptr->right, *tmp_cky_ptr = cky_ptr;
    BNST_DATA *g_ptr = cky_ptr->b_ptr, *d_ptr;
    int i, k, pred_p = 0, topic_score = 0;
    int ha_check = 0, *un_count;
    int rentai, vacant_slot_num, *scase_check;
    int count, pos, default_pos;
    int verb, comma;
    double one_score = 0;
    char *cp, *cp2;

    /* ÂÐ¾Ý¤ÎÍÑ¸À°Ê³°¤Î¥¹¥³¥¢¤ò½¸¤á¤ë (right¤ò¤¿¤É¤ê¤Ê¤¬¤éleft¤Î¥¹¥³¥¢¤òÂ­¤¹) */
    while (tmp_cky_ptr) {
	if (tmp_cky_ptr->direction == LtoR ? tmp_cky_ptr->left : tmp_cky_ptr->right) {
	    one_score += tmp_cky_ptr->direction == LtoR ? tmp_cky_ptr->left->score : tmp_cky_ptr->right->score;
	}
	tmp_cky_ptr = tmp_cky_ptr->direction == LtoR ? tmp_cky_ptr->right : tmp_cky_ptr->left;
    }
    if (OptDisplay == OPT_DEBUG) {
	printf("%.3f=>", one_score);
    }

    if (check_feature(g_ptr->f, "ÍÑ¸À") ||
	check_feature(g_ptr->f, "½àÍÑ¸À")) {
	pred_p = 1;
	for (k = 0; k < SCASE_CODE_SIZE; k++) cky_ptr->scase_check[k] = 0;
	scase_check = &(cky_ptr->scase_check[0]);
	cky_ptr->un_count = 0;
	un_count = &(cky_ptr->un_count);
    }

    /* ¤¹¤Ù¤Æ¤Î»Ò¶¡¤Ë¤Ä¤¤¤Æ */
    while (cky_ptr) {
	if (cky_ptr->direction == LtoR ? cky_ptr->left : cky_ptr->right) {
	    d_ptr = cky_ptr->direction == LtoR ? cky_ptr->left->b_ptr : cky_ptr->right->b_ptr;

	    if ((d_ptr->num < g_ptr->num &&
		 (Mask_matrix[d_ptr->num][g_ptr->num] == 2 || /* ÊÂÎóP */
		  Mask_matrix[d_ptr->num][g_ptr->num] == 3)) || /* ÊÂÎóI */
		(g_ptr->num < d_ptr->num &&
		 (Mask_matrix[g_ptr->num][d_ptr->num] == 2 || /* ÊÂÎóP */
		  Mask_matrix[g_ptr->num][d_ptr->num] == 3))) { /* ÊÂÎóI */
		;
	    }
	    else {
		/* µ÷Î¥¥³¥¹¥È¤ò·×»»¤¹¤ë¤¿¤á¤Ë¡¢¤Þ¤º·¸¤êÀè¤Î¸õÊä¤òÄ´¤Ù¤ë */
		count = 0;
		pos = 0;
		verb = 0;
		comma = 0;
		if (d_ptr->num < g_ptr->num) {
		    for (i = d_ptr->num + 1; i < sp->Bnst_num; i++) {
			if (check_dpnd_possibility(sp, d_ptr->num, i, cky_ptr->i, ((i == sp->Bnst_num - 1) && count == 0) ? TRUE : FALSE)) {
			    if (i == g_ptr->num) {
				pos = count;
			    }
			    count++;
			}
			if (i >= g_ptr->num) {
			    continue;
			}
			if (Language == CHINESE &&
			    (check_feature((sp->bnst_data+i)->f, "VV") ||
			     check_feature((sp->bnst_data+i)->f, "VA") ||
			     check_feature((sp->bnst_data+i)->f, "VC") ||
			     check_feature((sp->bnst_data+i)->f, "VE"))) {
			    verb++;
			}
			if (Language == CHINESE &&
			    check_feature((sp->bnst_data+i)->f, "PU") &&
			    (!strcmp((sp->bnst_data+i)->head_ptr->Goi, ",") ||
			     !strcmp((sp->bnst_data+i)->head_ptr->Goi, "¡§") ||
			     !strcmp((sp->bnst_data+i)->head_ptr->Goi, "$A!C(B") ||
			     !strcmp((sp->bnst_data+i)->head_ptr->Goi, ":") ||
			     !strcmp((sp->bnst_data+i)->head_ptr->Goi, "¡¨") ||
			     !strcmp((sp->bnst_data+i)->head_ptr->Goi, "¡¤"))) {
			    comma++;
			}
		    }
		}
		else {
		    for (i = d_ptr->num - 1; i >= 0; i--) {
			if (check_dpnd_possibility(sp, i ,d_ptr->num, cky_ptr->i, FALSE)) {
			    if (i == g_ptr->num) {
				pos = count;
			    }
			    count++;
			}
			if (i <= g_ptr->num) {
			    continue;
			}
			if (Language == CHINESE &&
			    (check_feature((sp->bnst_data+i)->f, "VV") ||
			     check_feature((sp->bnst_data+i)->f, "VA") ||
			     check_feature((sp->bnst_data+i)->f, "VC") ||
			     check_feature((sp->bnst_data+i)->f, "VE"))) {
			    verb++;
			}
			if (Language == CHINESE &&
			    check_feature((sp->bnst_data+i)->f, "PU") &&
			    (!strcmp((sp->bnst_data+i)->head_ptr->Goi, ",") ||
			     !strcmp((sp->bnst_data+i)->head_ptr->Goi, "¡§") ||
			     !strcmp((sp->bnst_data+i)->head_ptr->Goi, "$A!C(B") ||
			     !strcmp((sp->bnst_data+i)->head_ptr->Goi, ":") ||
			     !strcmp((sp->bnst_data+i)->head_ptr->Goi, "¡¨") ||
			     !strcmp((sp->bnst_data+i)->head_ptr->Goi, "¡¤"))) {
			    comma++;
			}
		    }
		}

		if (Language == CHINESE) {
		    one_score -= 30 * verb; 
		    one_score -= 40 * comma;
		}

		default_pos = (d_ptr->dpnd_rule->preference == -1) ?
		    count : d_ptr->dpnd_rule->preference;
		

		/* ·¸¤êÀè¤ÎDEFAULT¤Î°ÌÃÖ¤È¤Îº¹¤ò¥Ú¥Ê¥ë¥Æ¥£¤Ë
		   ¢¨ ÄóÂê¤ÏC,B'¤òµá¤á¤Æ±ó¤¯¤Ë·¸¤ë¤³¤È¤¬¤¢¤ë¤¬¡¤¤½¤ì¤¬
		   Â¾¤Î·¸¤êÀè¤Ë±Æ¶Á¤·¤Ê¤¤¤è¤¦,¥Ú¥Ê¥ë¥Æ¥£¤Ëº¹¤ò¤Ä¤±¤ë */
		if (check_feature(d_ptr->f, "ÄóÂê")) {
		    one_score -= abs(default_pos - 1 - pos);
		}
		else if (Language != CHINESE){
		    one_score -= abs(default_pos - 1 - pos) * 2;
		}
		else if (Language == CHINESE) {
		    if (abs(default_pos - 1 - pos) > 20) {
			one_score -= 20;
		    }
		}

		/* ÆÉÅÀ¤ò¤â¤Ä¤â¤Î¤¬ÎÙ¤Ë¤«¤«¤ë¤³¤È¤òËÉ¤° */
		if (d_ptr->num + 1 == g_ptr->num && 
		    abs(default_pos - 1 - pos) > 0 && 
		    (check_feature(d_ptr->f, "ÆÉÅÀ"))) {
		    one_score -= 5;
		}
	    }
	    
	    if (pred_p && (cp = check_feature(d_ptr->f, "·¸")) != NULL) {
		    
		/* Ì¤³Ê ÄóÂê(¡Ö¡Á¤Ï¡×)¤Î°·¤¤ */
		if (check_feature(d_ptr->f, "ÄóÂê") && !strcmp(cp, "·¸:Ì¤³Ê")) {

		    /* Ê¸Ëö, ¡Ö¡Á¤¬¡×¤Ê¤É, ÊÂÎóËö, C, B'¤Ë·¸¤ë¤³¤È¤òÍ¥Àè */
		    if ((cp2 = check_feature(g_ptr->f, "ÄóÂê¼õ")) != NULL) {
			sscanf(cp2, "%*[^:]:%d", &topic_score);
			one_score += topic_score;
		    }

		    /* °ì¤Ä¤á¤ÎÄóÂê¤Ë¤À¤±ÅÀ¤òÍ¿¤¨¤ë (»þ´Ö,¿ôÎÌ¤ÏÊÌ)
		       ¢ª Ê£¿ô¤ÎÄóÂê¤¬Æ±°ì½Ò¸ì¤Ë·¸¤ë¤³¤È¤òËÉ¤° */
		    if (check_feature(d_ptr->f, "»þ´Ö") ||
			check_feature(d_ptr->f, "¿ôÎÌ")) {
			one_score += 10;
		    }
		    else if (ha_check == 0){
			one_score += 10;
			ha_check = 1;
		    }
		}

		k = case2num(cp + 3);

		/* ³ÊÍ×ÁÇ°ìÈÌ¤Î°·¤¤ */

		/* Ì¤³Ê : ¿ô¤¨¤Æ¤ª¤­¡¤¸å¤Ç¶õ¥¹¥í¥Ã¥È¤òÄ´¤Ù¤ë (»þ´Ö,¿ôÎÌ¤ÏÊÌ) */
		if (!strcmp(cp, "·¸:Ì¤³Ê")) {
		    if (check_feature(d_ptr->f, "»þ´Ö") ||
			check_feature(d_ptr->f, "¿ôÎÌ")) {
			one_score += 10;
		    }
		    else {
			(*un_count)++;
		    }
		}

		/* ¥Î³Ê : ÂÎ¸À°Ê³°¤Ê¤é break 
		   ¢ª ¤½¤ì¤è¤êÁ°¤Î³ÊÍ×ÁÇ¤Ë¤ÏÅÀ¤òÍ¿¤¨¤Ê¤¤¡¥
		   ¢ª ¥Î³Ê¤¬¤«¤«¤ì¤Ð¤½¤ì¤è¤êÁ°¤Î³Ê¤Ï¤«¤«¤é¤Ê¤¤

		   ¢¨ ¡ÖÂÎ¸À¡×¤È¤¤¤¦¤Î¤ÏÈ½Äê»ì¤Î¤³¤È¡¤¤¿¤À¤·
		   Ê¸Ëö¤Ê¤É¤Ç¤ÏÍÑ¸À:Æ°¤È¤Ê¤Ã¤Æ¤¤¤ë¤³¤È¤â
		   ¤¢¤ë¤Î¤Ç¡¤¡ÖÂÎ¸À¡×¤Ç¥Á¥§¥Ã¥¯ */
		else if (!strcmp(cp, "·¸:¥Î³Ê")) {
		    if (!check_feature(g_ptr->f, "ÂÎ¸À")) {
			/* one_score += 10;
			   break; */
			if (g_ptr->SCASE_code[case2num("¥¬³Ê")] &&
			    scase_check[case2num("¥¬³Ê")] == 0) {
			    one_score += 10;
			    scase_check[case2num("¥¬³Ê")] = 1;
			}
		    }
		} 

		/* ¥¬³Ê : ¥¬¥¬¹½Ê¸¤¬¤¢¤ë¤Î¤Ç¾¯¤·Ê£»¨ */
		else if (!strcmp(cp, "·¸:¥¬³Ê")) {
		    if (g_ptr->SCASE_code[case2num("¥¬³Ê")] &&
			scase_check[case2num("¥¬³Ê")] == 0) {
			one_score += 10;
			scase_check[case2num("¥¬³Ê")] = 1;
		    }
		    else if (g_ptr->SCASE_code[case2num("¥¬£²")] &&
			     scase_check[case2num("¥¬£²")] == 0) {
			one_score += 10;
			scase_check[case2num("¥¬³Ê")] = 1;
		    }
		}

		/* Â¾¤Î³Ê : ³Æ³Ê1¤Ä¤ÏÅÀ¿ô¤ò¤¢¤¿¤¨¤ë
		   ¢¨ ¥Ë³Ê¤Î¾ì¹ç¡¤»þ´Ö¤È¤½¤ì°Ê³°¤Ï¶èÊÌ¤¹¤ëÊý¤¬¤¤¤¤¤«¤â¡© */
		else if (k != -1) {
		    if (scase_check[k] == 0) {
			scase_check[k] = 1;
			one_score += 10;
		    } 
		}

		/* ¡Ö¡Á¤¹¤ë¤Î¤Ï¡Á¤À¡×¤Ë¥Ü¡¼¥Ê¥¹ 01/01/11
		   ¤Û¤È¤ó¤É¤Î¾ì¹ç²þÁ±¡¥

		   ²þÁ±Îã)
		   ¡Ö¹³µÄ¤·¤¿¤Î¤â Ç¤´±¤ò µñÈÝ¤µ¤ì¤ë ÍýÍ³¤Î °ì¤Ä¤é¤·¤¤¡×

		   ¡Ö»È¤¦¤Î¤Ï ¶²¤í¤·¤¤ ¤³¤È¤À¡£¡×
		   ¡Ö±ßËþ·èÃå¤Ë ¤Ê¤ë¤«¤É¤¦¤«¤Ï ÈùÌ¯¤Ê ¤È¤³¤í¤À¡£¡×
		   ¢¨ ¤³¤ì¤é¤ÎÎã¤Ï¡Ö¤³¤È/¤È¤³¤í¤À¡×¤Ë·¸¤ë¤È°·¤¦

		   ¡ÖÂ¾¿Í¤Ë ¶µ¤¨¤ë¤Î¤¬ ¹¥¤­¤Ë¤Ê¤ë ¤ä¤êÊý¤Ç¤¹¡×
		   ¢¨ ¤³¤ÎÎã¤ÏÛ£Ëæ¤À¤¬¡¤Ê¸Ì®¾åÀµ¤·¤¤

		   ÉûºîÍÑÎã)
		   ¡Ö¤À¤ì¤¬ £Í£Ö£Ð¤« Ê¬¤«¤é¤Ê¤¤ »î¹ç¤Ç¤·¤ç¤¦¡×
		   ¡Ö¡Á ²¥¤ë¤Ê¤É ¤·¤¿ µ¿¤¤¡£¡×
		   ¡Ö¥Ó¥¶¤ò ¼è¤ë¤Î¤â ÂçÊÑ¤Ê »þÂå¡£¡×
		   ¡ÖÇÈ¤¬ ¹â¤Þ¤ë¤Î¤Ï Èò¤±¤é¤ì¤½¤¦¤Ë¤Ê¤¤ ±À¹Ô¤­¤À¡£¡×
		   ¡Ö¤¢¤Þ¤ê ÌòÎ©¤Ä¤È¤Ï »×¤ï¤ì¤Ê¤¤ ÏÀÍý¤À¡£¡×
		   ¡Ö¤É¤¦ ÀÞ¤ê¹ç¤¦¤«¤¬ ÌäÂê»ë¤µ¤ì¤Æ¤­¤¿ Ë¡¤À¡£¡×
		   ¡ÖÇ§¤á¤é¤ì¤ë¤«¤É¤¦¤«¤¬ Áè¤ï¤ì¤¿ ºÛÈ½¤Ç¡×

		   ¢¨ÌäÂê¢¨
		   ¡Ö¤¢¤ÎÀïÁè¡×¤¬¡Á ¤Î¤è¤¦¤Ê¾ì¹ç¤âÍÑ¸À¤È¤ß¤Ê¤µ¤ì¤ë¤Î¤¬ÌäÂê
		*/

		if (check_feature(d_ptr->f, "ÍÑ¸À") &&
		    (check_feature(d_ptr->f, "·¸:Ì¤³Ê") ||
		     check_feature(d_ptr->f, "·¸:¥¬³Ê")) &&
		    check_feature(g_ptr->f, "ÍÑ¸À:È½")) {
		    one_score += 3;
		}
	    }

	    /* Ï¢ÂÎ½¤¾þ¤Î¾ì¹ç¡¤·¸Àè¤¬
	       ¡¦·Á¼°Ì¾»ì,Éû»ìÅªÌ¾»ì
	       ¡¦¡ÖÍ½Äê¡×,¡Ö¸«¹þ¤ß¡×¤Ê¤É
	       ¤Ç¤Ê¤±¤ì¤Ð°ì¤Ä¤Î³ÊÍ×ÁÇ¤È¹Í¤¨¤ë */
	    if (check_feature(d_ptr->f, "·¸:Ï¢³Ê")) {
		if (check_feature(g_ptr->f, "³°¤Î´Ø·¸") || 
		    check_feature(g_ptr->f, "¥ë¡¼¥ë³°¤Î´Ø·¸")) {
		    one_score += 10;	/* ³°¤Î´Ø·¸¤Ê¤é¤³¤³¤Ç²ÃÅÀ */
		}
		else {
		    /* ¤½¤ì°Ê³°¤Ê¤é¶õ¤­¥¹¥í¥Ã¥È¤ò¥Á¥§¥Ã¥¯ (Ï¢ÂÎ½¤¾þ¤òÉÕ¤±¤¿¤È¤­¤Î¥¹¥³¥¢¤Îº¹Ê¬) */
		    one_score += check_scase(d_ptr, &(cky_ptr->left->scase_check[0]), 1, cky_ptr->left->un_count) - 
			check_scase(d_ptr, &(cky_ptr->left->scase_check[0]), 0, cky_ptr->left->un_count);
		}
	    }

	    /* calc score for Chinese */
	    if (Language == CHINESE) {
		/* add score from case frame */
		if ((check_feature(g_ptr->f, "VV") ||
		     check_feature(g_ptr->f, "VA") ||
		     check_feature(g_ptr->f, "VC") ||
		     check_feature(g_ptr->f, "VE") ||
		     check_feature(g_ptr->f, "P")) &&
		    !check_feature(d_ptr->f, "PU")) {
		    /* calc case frame score for Chinese */
		    one_score += TIME_CASE_FRAME * Chi_case_prob_matrix[g_ptr->num][d_ptr->num];
		}

		if (cky_ptr->direction == LtoR) {
		    one_score += Dpnd_prob_matrix[d_ptr->num][g_ptr->num];
		    /* add score for stable dpnd */
		    if (d_ptr->num + 1 == g_ptr->num &&
			(((check_feature(d_ptr->f, "CD")) &&
			  (check_feature(g_ptr->f, "DEG") ||
			   check_feature(g_ptr->f, "M"))) ||
			 
			 ((check_feature(d_ptr->f, "DEG")) &&
			  (check_feature(g_ptr->f, "NR"))) ||

			 ((check_feature(d_ptr->f, "JJ")) &&
			  (check_feature(g_ptr->f, "NN") ||
			   check_feature(g_ptr->f, "DEG"))) ||
			 
			 ((check_feature(d_ptr->f, "DEV")) &&
			   check_feature(g_ptr->f, "VV")) ||
			 
			 ((check_feature(d_ptr->f, "NR-SHORT")) &&
			  (check_feature(g_ptr->f, "NR"))) ||

			 ((check_feature(d_ptr->f, "NT-SHORT")) &&
			  (check_feature(g_ptr->f, "NT"))) ||
			 
			 ((check_feature(d_ptr->f, "NR")) &&
			  (check_feature(g_ptr->f, "DEG") ||
			   check_feature(g_ptr->f, "NN"))) ||

			 ((check_feature(d_ptr->f, "NT")) &&
			  (check_feature(g_ptr->f, "DEG") ||
			   check_feature(g_ptr->f, "NT"))) ||

			 ((check_feature(d_ptr->f, "OD")) &&
			  (check_feature(g_ptr->f, "M"))) ||

			 ((check_feature(d_ptr->f, "PN")) &&
			  (check_feature(g_ptr->f, "DEG"))) ||

			 ((check_feature(d_ptr->f, "SB")) &&
			  (check_feature(g_ptr->f, "VV"))) ||

			 ((check_feature(d_ptr->f, "VA")) &&
			  (check_feature(g_ptr->f, "DEV"))))) {
			one_score += 10;
		    }
		    if (d_ptr->num < g_ptr->num &&
			(((check_feature(d_ptr->f, "AD")) &&
			  (check_feature(g_ptr->f, "BA") ||
			   check_feature(g_ptr->f, "DEG") ||
			   check_feature(g_ptr->f, "DT") ||
			   check_feature(g_ptr->f, "JJ") ||
			   check_feature(g_ptr->f, "LB") ||
			   check_feature(g_ptr->f, "VA") ||
			   check_feature(g_ptr->f, "VC") ||
			   check_feature(g_ptr->f, "VE"))) ||
			 
			 ((check_feature(d_ptr->f, "CC")) &&
			  (check_feature(g_ptr->f, "AD") ||
			   check_feature(g_ptr->f, "CD") ||
			   check_feature(g_ptr->f, "NR") ||
			   check_feature(g_ptr->f, "NT") ||
			   check_feature(g_ptr->f, "PN") ||
			   check_feature(g_ptr->f, "BA"))) ||
			      
			 ((check_feature(d_ptr->f, "AS")) &&
			  (check_feature(g_ptr->f, "VV"))) ||

			 ((check_feature(d_ptr->f, "CD")) &&
			  (check_feature(g_ptr->f, "BA") ||
			   check_feature(g_ptr->f, "DEG") ||
			   check_feature(g_ptr->f, "DEV") ||
			   check_feature(g_ptr->f, "LC") ||
			   check_feature(g_ptr->f, "M") ||
			   check_feature(g_ptr->f, "NN"))) ||

			 ((check_feature(d_ptr->f, "DEC")) &&
			  (check_feature(g_ptr->f, "NN") ||
			   check_feature(g_ptr->f, "NR") ||
			   check_feature(g_ptr->f, "NT") ||
			   check_feature(g_ptr->f, "PN"))) ||

			 ((check_feature(d_ptr->f, "DEG")) &&
			  (check_feature(g_ptr->f, "DEG") ||
			   check_feature(g_ptr->f, "LC") ||
			   check_feature(g_ptr->f, "NN") ||
			   check_feature(g_ptr->f, "NR") ||
			   check_feature(g_ptr->f, "PN") ||
			   check_feature(g_ptr->f, "VE"))) ||

			 ((check_feature(d_ptr->f, "DER")) &&
			  (check_feature(g_ptr->f, "VA") ||
			   check_feature(g_ptr->f, "VV"))) ||

			 ((check_feature(d_ptr->f, "DEV")) &&
			  (check_feature(g_ptr->f, "BA") ||
			   check_feature(g_ptr->f, "LB") ||
			   check_feature(g_ptr->f, "VA") ||
			   check_feature(g_ptr->f, "VV"))) ||

			 ((check_feature(d_ptr->f, "DT")) &&
			  (check_feature(g_ptr->f, "BA") ||
			   check_feature(g_ptr->f, "DEG") ||
			   check_feature(g_ptr->f, "NN") ||
			   check_feature(g_ptr->f, "NR") ||
			   check_feature(g_ptr->f, "PN") ||
			   check_feature(g_ptr->f, "NT"))) ||

			 ((check_feature(d_ptr->f, "ETC")) &&
			  (check_feature(g_ptr->f, "VV"))) ||

			 ((check_feature(d_ptr->f, "FW")) &&
			  (check_feature(g_ptr->f, "CD") ||
			   check_feature(g_ptr->f, "FW") ||
			   check_feature(g_ptr->f, "M"))) ||

			 ((check_feature(d_ptr->f, "IJ")) &&
			  (check_feature(g_ptr->f, "VV"))) ||

			 ((check_feature(d_ptr->f, "JJ")) &&
			  (check_feature(g_ptr->f, "CD") ||
			   check_feature(g_ptr->f, "DEG") ||
			   check_feature(g_ptr->f, "DEV") ||
			   check_feature(g_ptr->f, "NN") ||
			   check_feature(g_ptr->f, "NR") ||
			   check_feature(g_ptr->f, "PN"))) ||

			 ((check_feature(d_ptr->f, "LC")) &&
			  (check_feature(g_ptr->f, "DEG") ||
			   check_feature(g_ptr->f, "NT"))) ||

			 ((check_feature(d_ptr->f, "MSP")) &&
			  (check_feature(g_ptr->f, "BA") ||
			   check_feature(g_ptr->f, "VA") ||
			   check_feature(g_ptr->f, "VE"))) ||

			 ((check_feature(d_ptr->f, "M")) &&
			  (check_feature(g_ptr->f, "DEC") ||
			   check_feature(g_ptr->f, "DEG") ||
			   check_feature(g_ptr->f, "DEV") ||
			   check_feature(g_ptr->f, "DT") ||
			   check_feature(g_ptr->f, "LC") ||
			   check_feature(g_ptr->f, "NN") ||
			   check_feature(g_ptr->f, "PN"))) ||

			 ((check_feature(d_ptr->f, "NN")) &&
			  (check_feature(g_ptr->f, "CS") ||
			   check_feature(g_ptr->f, "DEG") ||
			   check_feature(g_ptr->f, "DEV") ||
			   check_feature(g_ptr->f, "NP") ||
			   check_feature(g_ptr->f, "SB"))) ||

			 ((check_feature(d_ptr->f, "NR")) &&
			  (check_feature(g_ptr->f, "NN") ||
			   check_feature(g_ptr->f, "DEG") ||
			   check_feature(g_ptr->f, "DT") ||
			   check_feature(g_ptr->f, "NP") ||
			   check_feature(g_ptr->f, "PN"))) ||

			 ((check_feature(d_ptr->f, "NP")) &&
			  (check_feature(g_ptr->f, "NN"))) ||

			 ((check_feature(d_ptr->f, "NT")) &&
			  (check_feature(g_ptr->f, "BA") ||
			   check_feature(g_ptr->f, "DEG") ||
			   check_feature(g_ptr->f, "JJ") ||
			   check_feature(g_ptr->f, "VE"))) ||

			 ((check_feature(d_ptr->f, "OD")) &&
			  (check_feature(g_ptr->f, "DEC") ||
			   check_feature(g_ptr->f, "JJ") ||
			   check_feature(g_ptr->f, "M") ||
			   check_feature(g_ptr->f, "VA"))) ||

			 ((check_feature(d_ptr->f, "PN")) &&
			  (check_feature(g_ptr->f, "BA") ||
			   check_feature(g_ptr->f, "CD") ||
			   check_feature(g_ptr->f, "DEG") ||
			   check_feature(g_ptr->f, "DT") ||
			   check_feature(g_ptr->f, "ETC") ||
			   check_feature(g_ptr->f, "LB") ||
			   check_feature(g_ptr->f, "NN") ||
			   check_feature(g_ptr->f, "NR") ||
			   check_feature(g_ptr->f, "NT") ||
			   check_feature(g_ptr->f, "P") ||
			   check_feature(g_ptr->f, "PN"))) ||

			 ((check_feature(d_ptr->f, "SP")) &&
			  (check_feature(g_ptr->f, "DEC"))) ||

			 ((check_feature(d_ptr->f, "VA")) &&
			  (check_feature(g_ptr->f, "DEC") ||
			   check_feature(g_ptr->f, "DEV"))) ||

			 ((check_feature(d_ptr->f, "VE")) &&
			  (check_feature(g_ptr->f, "DEV"))) ||

			 ((check_feature(d_ptr->f, "VV")) &&
			  (check_feature(g_ptr->f, "DEC") ||
			   check_feature(g_ptr->f, "DEV"))))) {
			one_score += 10;
		    }
		}
		else if (cky_ptr->direction == RtoL) {
		    one_score += (Dpnd_matrix[g_ptr->num][d_ptr->num] == 'B') ?
			Dpnd_prob_matrix[d_ptr->num][g_ptr->num] : Dpnd_prob_matrix[g_ptr->num][d_ptr->num];
		    /* add score for stable dpnd */
		    if (g_ptr->num + 1 == d_ptr->num &&
			(((check_feature(d_ptr->f, "AS")) &&
			  (check_feature(g_ptr->f, "VE") ||
			   check_feature(g_ptr->f, "VV"))) ||

			 ((check_feature(d_ptr->f, "CD")) &&
			  (check_feature(g_ptr->f, "DT"))) ||

			 ((check_feature(d_ptr->f, "PN")) &&
			  (check_feature(g_ptr->f, "P"))) ||

			 ((check_feature(d_ptr->f, "DEC")) &&
			  (check_feature(g_ptr->f, "VC") ||
			   check_feature(g_ptr->f, "VA") ||
			   check_feature(g_ptr->f, "VE") ||
			   check_feature(g_ptr->f, "VV"))) ||

			 ((check_feature(d_ptr->f, "DER")) &&
			  (check_feature(g_ptr->f, "VV"))) ||

			 ((check_feature(d_ptr->f, "ETC")) &&
			  (check_feature(g_ptr->f, "NN") ||
			   check_feature(g_ptr->f, "NR"))))) {
			one_score += 10;
		    }
		    if (g_ptr->num < d_ptr->num &&
			(((check_feature(g_ptr->f, "AD")) &&
			  (check_feature(d_ptr->f, "CC")))||

			 ((check_feature(g_ptr->f, "CD")) &&
			  (check_feature(d_ptr->f, "CC")))||

			 ((check_feature(g_ptr->f, "VV") ||
			   check_feature(g_ptr->f, "VC") ||
			   check_feature(g_ptr->f, "VE")) &&
			  (check_feature(d_ptr->f, "VC") ||
			   check_feature(d_ptr->f, "VE") ||
			   check_feature(d_ptr->f, "VV")))||

			 ((check_feature(g_ptr->f, "DT")) &&
			  (check_feature(d_ptr->f, "CD")||
			   check_feature(d_ptr->f, "M")))||

			 ((check_feature(g_ptr->f, "LB")) &&
			  (check_feature(d_ptr->f, "CC")))||

			 ((check_feature(g_ptr->f, "NN")) &&
			  (check_feature(d_ptr->f, "X")))||

			 ((check_feature(g_ptr->f, "P")) &&
			  (check_feature(d_ptr->f, "CS") ||
			   check_feature(d_ptr->f, "DT") ||
			   check_feature(d_ptr->f, "JJ") ||
			   check_feature(d_ptr->f, "LC") ||
			   check_feature(d_ptr->f, "NR") ||
			   check_feature(d_ptr->f, "NN") ||
			   check_feature(d_ptr->f, "NT") ||
			   check_feature(d_ptr->f, "PN")))||

			 ((check_feature(g_ptr->f, "VA")) &&
			  (check_feature(d_ptr->f, "NR"))) ||

			 ((check_feature(g_ptr->f, "VC")) &&
			  (check_feature(d_ptr->f, "DT"))) ||

			 ((check_feature(g_ptr->f, "VE")) &&
			  (check_feature(d_ptr->f, "AS") ||
			   check_feature(d_ptr->f, "CC") ||
			   check_feature(d_ptr->f, "NT") ||
			   check_feature(d_ptr->f, "PN")))||

			 ((check_feature(g_ptr->f, "VV")) &&
			  (check_feature(d_ptr->f, "AS") ||
			   check_feature(d_ptr->f, "NN") ||
			   check_feature(d_ptr->f, "DER"))))) {
			one_score += 10;
		    }
		}
	    }
	}

	cky_ptr = cky_ptr->direction == LtoR ? cky_ptr->right : cky_ptr->left;
    }

    /* ÍÑ¸À¤Î¾ì¹ç¡¤ºÇ½ªÅª¤ËÌ¤³Ê,¥¬³Ê,¥ò³Ê,¥Ë³Ê,Ï¢ÂÎ½¤¾þ¤ËÂÐ¤·¤Æ
       ¥¬³Ê,¥ò³Ê,¥Ë³Ê¤Î¥¹¥í¥Ã¥ÈÊ¬¤À¤±ÅÀ¿ô¤òÍ¿¤¨¤ë */
    if (pred_p) {
	one_score += check_scase(g_ptr, scase_check, 0, *un_count);
    }

    if (OptDisplay == OPT_DEBUG) {
	printf("%.3f\n", one_score);
    }

    return one_score;
}

/* count dependency possibilities */
int count_distance(SENTENCE_DATA *sp, CKY *cky_ptr, BNST_DATA *g_ptr, int *pos) {
    int i, count = 0;
    *pos = 0;

    for (i = cky_ptr->left->b_ptr->num + 1; i < sp->Bnst_num; i++) {
	if (check_dpnd_possibility(sp, cky_ptr->left->b_ptr->num, i, cky_ptr->i, 
				   ((i == sp->Bnst_num - 1) && count == 0) ? TRUE : FALSE)) {
	    if (i == g_ptr->num) {
		*pos = count;
	    }
	    count++;
	}
    }

    return count;
}

/* scoring function based on case structure probabilities */
double calc_case_probability(SENTENCE_DATA *sp, CKY *cky_ptr, TOTAL_MGR *Best_mgr) {
    CKY *right_ptr = cky_ptr->right, *orig_cky_ptr = cky_ptr;
    BNST_DATA *g_ptr = cky_ptr->b_ptr, *d_ptr;
    TAG_DATA *t_ptr;
    CF_PRED_MGR *cpm_ptr, *pre_cpm_ptr;
    int i, pred_p = 0, count, pos, default_pos, child_num = 0;
    int renyou_modifying_num = 0, adverb_modifying_num = 0, noun_modifying_num = 0, flag;
    double one_score = 0, orig_score;
    char *para_key;

    /* ÂÐ¾Ý¤ÎÍÑ¸À°Ê³°¤Î¥¹¥³¥¢¤ò½¸¤á¤ë (right¤ò¤¿¤É¤ê¤Ê¤¬¤éleft¤Î¥¹¥³¥¢¤òÂ­¤¹) */
    while (cky_ptr) {
	if (cky_ptr->left) {
	    one_score += cky_ptr->left->score;
	}
	cky_ptr = cky_ptr->right;
    }
    if (OptDisplay == OPT_DEBUG) {
	printf("%.3f=>", one_score);
    }

    cky_ptr = orig_cky_ptr;

    if (check_feature(g_ptr->f, "¥¿¥°Ã±°Ì¼õ:-1") && g_ptr->tag_num > 1) { /* ¡Á¤Î¤Ï */
	t_ptr = g_ptr->tag_ptr + g_ptr->tag_num - 2;
    }
    else {
	t_ptr = g_ptr->tag_ptr + g_ptr->tag_num - 1;
    }

    if (t_ptr->cf_num > 0) { /* predicate or something which has case frames */
	pred_p = 1;
	cpm_ptr = cky_ptr->cpm_ptr;
	cpm_ptr->pred_b_ptr = t_ptr;
	cpm_ptr->score = -1;
	cpm_ptr->result_num = 0;
	cpm_ptr->tie_num = 0;
	cpm_ptr->cmm[0].cf_ptr = NULL;
	cpm_ptr->decided = CF_UNDECIDED;

	cpm_ptr->cf.pred_b_ptr = t_ptr;
	t_ptr->cpm_ptr = cpm_ptr;
	cpm_ptr->cf.element_num = 0;

	set_data_cf_type(cpm_ptr); /* set predicate type */
    }
    else {
	cky_ptr->cpm_ptr->pred_b_ptr = NULL;
    }

    /* check each child */
    while (cky_ptr) {
	if (cky_ptr->left && cky_ptr->para_flag == 0) {
	    d_ptr = cky_ptr->left->b_ptr;
	    flag = 0;

	    /* relax penalty */
	    if (cky_ptr->dpnd_type == 'R') {
		one_score += -1000;
	    }

	    /* coordination */
	    if (OptParaFix == 0) {
		if (d_ptr->para_num != -1 && (para_key = check_feature(d_ptr->f, "ÊÂ¥­"))) {
		    if (cky_ptr->para_score >= 0) {
			one_score += get_para_exist_probability(para_key, cky_ptr->para_score, TRUE);
			one_score += get_para_ex_probability(para_key, d_ptr->tag_ptr + d_ptr->tag_num - 1, t_ptr);
			flag++;
		    }
		    else {
			one_score += get_para_exist_probability(para_key, sp->para_data[d_ptr->para_num].max_score, FALSE);
		    }
		}
	    }

	    /* case component */
	    if (cky_ptr->dpnd_type != 'P' && pred_p) {
		make_work_mgr_dpnd_check(sp, cky_ptr, d_ptr);
		if (make_data_cframe_child(sp, cpm_ptr, d_ptr->tag_ptr + d_ptr->tag_num - 1, child_num, t_ptr->num == d_ptr->num + 1 ? TRUE : FALSE)) {
		    child_num++;
		    flag++;
		}

		if ((check_feature(d_ptr->f, "·¸:Ï¢ÍÑ") && 
		     (!check_feature(d_ptr->f, "ÍÑ¸À") || !check_feature(d_ptr->f, "Ê£¹ç¼­"))) || 
		    check_feature(d_ptr->f, "½¤¾þ")) {
		    flag++;
		}
	    }

	    /* clausal modifiee */
	    if (check_feature(d_ptr->f, "·¸:Ï¢³Ê") && 
		cky_ptr->left->cpm_ptr->pred_b_ptr) { /* ³Ê¥Õ¥ì¡¼¥à¤ò¤â¤Ã¤Æ¤¤¤ë¤Ù¤­ */
		pre_cpm_ptr = cky_ptr->left->cpm_ptr;
		pre_cpm_ptr->pred_b_ptr->cpm_ptr = pre_cpm_ptr;
		make_work_mgr_dpnd_check(sp, cky_ptr, d_ptr);
		make_data_cframe_rentai_simple(pre_cpm_ptr, pre_cpm_ptr->pred_b_ptr, t_ptr);

		orig_score = pre_cpm_ptr->score;
		one_score -= orig_score;
		one_score += find_best_cf(sp, pre_cpm_ptr, get_closest_case_component(sp, pre_cpm_ptr), 1);
		pre_cpm_ptr->score = orig_score;
		pre_cpm_ptr->cf.element_num--;
		flag++;
	    }

	    if (flag == 0) { /* Ì¾»ì³Ê¥Õ¥ì¡¼¥à¤Ø */
		make_work_mgr_dpnd_check(sp, cky_ptr, d_ptr);
		one_score += get_noun_co_ex_probability(d_ptr->tag_ptr + d_ptr->tag_num - 1, t_ptr);
		noun_modifying_num++;

		/* 
		one_score += FREQ0_ASSINED_SCORE;
		count = count_distance(sp, cky_ptr, g_ptr, &pos);
		default_pos = (d_ptr->dpnd_rule->preference == -1) ?
		    count : d_ptr->dpnd_rule->preference;
		one_score -= abs(default_pos - 1 - pos) * 5;
		*/
	    }

	    /* penalty of adverb etc. (tentative) */
	    if (check_feature(d_ptr->f, "·¸:Ï¢ÍÑ") && !check_feature(d_ptr->f, "ÍÑ¸À")) {
		count = count_distance(sp, cky_ptr, g_ptr, &pos);
		default_pos = (d_ptr->dpnd_rule->preference == -1) ?
		    count : d_ptr->dpnd_rule->preference;
		one_score -= abs(default_pos - 1 - pos) * 5;
	    }
	}
	cky_ptr = cky_ptr->right;
    }

    /* one_score += get_noun_co_num_probability(t_ptr, noun_modifying_num); */

    if (pred_p) {
	t_ptr->cpm_ptr = cpm_ptr;

	/* ÍÑ¸ÀÊ¸Àá¤¬¡Ö¡Ê¡Á¤ò¡Ë¡Á¤Ë¡×¤Î¤È¤­ 
	   ¡Ö¤¹¤ë¡×¤Î³Ê¥Õ¥ì¡¼¥à¤ËÂÐ¤·¤Æ¥Ë³Ê(Æ±Ê¸Àá)¤òÀßÄê
	   ¥ò³Ê¤Ï»Ò¶¡¤Î½èÍý¤Ç°·¤ï¤ì¤ë */
	if (check_feature(t_ptr->f, "£ÔÍÑ¸ÀÆ±Ê¸Àá")) {
	    if (_make_data_cframe_pp(cpm_ptr, t_ptr, TRUE)) {
		_make_data_cframe_sm(cpm_ptr, t_ptr);
		_make_data_cframe_ex(cpm_ptr, t_ptr);
		cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num] = t_ptr;
		cpm_ptr->elem_b_num[cpm_ptr->cf.element_num] = child_num;
		cpm_ptr->cf.weight[cpm_ptr->cf.element_num] = 0;
		cpm_ptr->cf.adjacent[cpm_ptr->cf.element_num] = TRUE;
		cpm_ptr->cf.element_num++;
	    }
	}

	/* call case structure analysis */
	one_score += find_best_cf(sp, cpm_ptr, get_closest_case_component(sp, cpm_ptr), 1);

	/* for each child */
	cky_ptr = orig_cky_ptr;
	while (cky_ptr) {
	    if (cky_ptr->left) {
		d_ptr = cky_ptr->left->b_ptr;
		if (cky_ptr->dpnd_type != 'P') {
		    /* modifying predicate */
		    if (check_feature(d_ptr->f, "·¸:Ï¢ÍÑ") && check_feature(d_ptr->f, "ÍÑ¸À") && 
			!check_feature(d_ptr->f, "Ê£¹ç¼­")) {
			make_work_mgr_dpnd_check(sp, cky_ptr, d_ptr);
			one_score += calc_vp_modifying_probability(t_ptr, cpm_ptr->cmm[0].cf_ptr, 
								   d_ptr->tag_ptr + d_ptr->tag_num - 1, 
								   cky_ptr->left->cpm_ptr->cmm[0].cf_ptr);
			renyou_modifying_num++;
		    }

		    /* modifying adverb */
		    if ((check_feature(d_ptr->f, "·¸:Ï¢ÍÑ") && !check_feature(d_ptr->f, "ÍÑ¸À")) || 
			check_feature(d_ptr->f, "½¤¾þ")) {
			make_work_mgr_dpnd_check(sp, cky_ptr, d_ptr);
			one_score += calc_adv_modifying_probability(t_ptr, cpm_ptr->cmm[0].cf_ptr, 
								    d_ptr->tag_ptr + d_ptr->tag_num - 1);
			adverb_modifying_num++;
		    }
		}
	    }
	    cky_ptr = cky_ptr->right;
	}

	one_score += calc_vp_modifying_num_probability(t_ptr, cpm_ptr->cmm[0].cf_ptr, renyou_modifying_num);
	one_score += calc_adv_modifying_num_probability(t_ptr, cpm_ptr->cmm[0].cf_ptr, adverb_modifying_num);
    }

    if (OptDisplay == OPT_DEBUG) {
	printf("%.3f\n", one_score);
    }

    return one_score;
}

int relax_barrier_for_P(CKY *cky_ptr, int dep, int gov, int *dep_check) {
    while (cky_ptr) {
	if (cky_ptr->left && 
	    cky_ptr->dpnd_type == 'P') {
	    if (*(dep_check + dep) >= cky_ptr->left->j) { /* ÊÂÎó¤Îº¸Â¦¤ËÊÉ¤¬¤¢¤ë¤Ê¤é¡¢±¦Â¦¤Þ¤ÇOK¤È¤¹¤ë */
		return TRUE;
	    }
	    else if (cky_ptr->para_flag) {
		if (relax_barrier_for_P(cky_ptr->left, dep, gov, dep_check)) {
		    return TRUE;
		}
	    }
	}
	cky_ptr = cky_ptr->right; /* go below */
    }

    return FALSE;
}

int relax_dpnd_for_P(CKY *cky_ptr, int dep, int gov) {
    int i;

    while (cky_ptr) {
	if (cky_ptr->left && 
	    cky_ptr->dpnd_type == 'P') {
	    for (i = cky_ptr->left->i; i <= cky_ptr->left->j; i++) {
		if (Dpnd_matrix[dep][i] && Quote_matrix[dep][i]) {
		    return TRUE;
		}
	    }
	}
	cky_ptr = cky_ptr->right;
    }

    return FALSE;
}

void fix_predicate_coordination(SENTENCE_DATA* sp) {
    int i, j, k;

    for (i = 0; i < sp->Para_num; i++) {
	if (sp->para_data[i].type == PARA_KEY_P) { /* predicate coordination */
	    if (sp->para_data[i].status == 'x') {
		sp->para_data[i].max_score = -1;
	    }

	    /* modify Para_matrix */
	    for (j = 0; j < sp->Bnst_num; j++) {
		for (k = 0; k < sp->Bnst_num; k++) {
		    /* preserve the best coordination */
		    if (sp->para_data[i].status == 'x' || 
			j != sp->para_data[i].max_path[0] || k != sp->para_data[i].jend_pos) { 
			Para_matrix[i][j][k] = -1;
		    }
		}
	    }

	    /* modify Dpnd_matrix */
	    if (sp->para_data[i].status != 'x') {
		for (j = sp->para_data[i].key_pos + 1; j < sp->Bnst_num; j++) {
		    if (j == sp->para_data[i].jend_pos) {
			Dpnd_matrix[sp->para_data[i].key_pos][j] = 'R';
		    }
		    else {
			Dpnd_matrix[sp->para_data[i].key_pos][j] = 0;
		    }
		}
	    }
	}
    }
}


void discard_bad_coordination(SENTENCE_DATA* sp) {
    int i, j, k;

    for (i = 0; i < sp->Para_num; i++) {
	if (sp->para_data[i].status == 'x') {
	    for (j = 0; j < sp->Bnst_num; j++) {
		for (k = 0; k < sp->Bnst_num; k++) {
		    Para_matrix[i][j][k] = -1;
		}
	    }
	}
    }
}

void handle_incomplete_coordination(SENTENCE_DATA* sp) {
    int i, j;

    for (i = 0; i < sp->Bnst_num; i++) {
	for (j = 0; j < sp->Bnst_num; j++) {
	    if (Mask_matrix[i][j] == 3 && 
		Dpnd_matrix[i][j] == 0) {
		Dpnd_matrix[i][j] = (int)'I';
	    }
	}
    }
}

void extend_para_matrix(SENTENCE_DATA* sp) {
    int i, j, k, l, flag, offset, max_pos;
    double max_score;

    for (i = 0; i < sp->Para_num; i++) {
	if (sp->para_data[i].max_score >= 0) {
	    if (sp->para_data[i].type == PARA_KEY_P) {
		offset = 0;
	    }
	    else { /* in case of noun coordination, only permit modifiers to the words before the para key */
		offset = 1;
	    }

	    /* for each endpos */
	    for (l = sp->para_data[i].key_pos + 1; l < sp->Bnst_num; l++) {
		max_score = -INT_MAX;
		for (j = sp->para_data[i].iend_pos; j >= 0; j--) {
		    if (max_score < Para_matrix[i][j][l]) {
			max_score = Para_matrix[i][j][l];
			max_pos = j;
		    }
		}
		if (max_score >= 0) {
		    /* go up to search modifiers */
		    for (j = max_pos - 1; j >= 0; j--) {
			if (check_stop_extend(sp, j)) { /* extention stop */
			    break;
			}

			/* check dependency to pre-conjunct */
			flag = 0;
			for (k = j + 1; k <= sp->para_data[i].key_pos - offset; k++) {
			    if (Dpnd_matrix[j][k] && Quote_matrix[j][k]) {
				Para_matrix[i][j][l] = max_score;
				flag = 1;
				if (OptDisplay == OPT_DEBUG) {
				    printf("Para Extension (%s-%s-%s) -> %s\n", 
					   (sp->bnst_data + max_pos)->head_ptr->Goi, 
					   (sp->bnst_data + sp->para_data[i].key_pos)->head_ptr->Goi, 
					   (sp->bnst_data + l)->head_ptr->Goi, 
					   (sp->bnst_data + j)->head_ptr->Goi);
				}
				break;
			    }
			}
			if (flag == 0) {
			    break;
			}
		    }
		}
	    }
	}
    }
}

void set_cky(SENTENCE_DATA *sp, CKY *cky_ptr, CKY *left_ptr, CKY *right_ptr, int i, int j, int k, 
	     char dpnd_type, int direction) {
    int l;

    cky_ptr->i = i;
    cky_ptr->j = j;
    cky_ptr->next = NULL;
    cky_ptr->left = left_ptr;
    cky_ptr->right = right_ptr;
    cky_ptr->direction = direction;
    cky_ptr->dpnd_type = dpnd_type;
    cky_ptr->cp = 'a' + j;
    if (cky_ptr->direction == RtoL) {
	cky_ptr->b_ptr = cky_ptr->left->b_ptr;
    }
    else {
	cky_ptr->b_ptr = cky_ptr->right ? cky_ptr->right->b_ptr : sp->bnst_data + j;
    }
    cky_ptr->un_count = 0;
    for (l = 0; l < SCASE_CODE_SIZE; l++) cky_ptr->scase_check[l] = 0;
    cky_ptr->para_flag = 0;
    cky_ptr->para_score = -1;
    cky_ptr->score = 0;
}

CKY *new_cky_data(int *cky_table_num) {
    CKY *cky_ptr;

    cky_ptr = &(cky_table[*cky_table_num]);
    if (OptAnalysis == OPT_CASE && *cky_table_num > cpm_allocated_cky_num) {
	cky_ptr->cpm_ptr = (CF_PRED_MGR *)malloc_data(sizeof(CF_PRED_MGR), "new_cky_data");
	init_case_frame(&(cky_ptr->cpm_ptr->cf));
	cpm_allocated_cky_num = *cky_table_num;
    }

    (*cky_table_num)++;
    if (*cky_table_num >= CKY_TABLE_MAX) {
	fprintf(stderr, ";;; cky_table_num exceeded maximum\n");
	return NULL;
    }

    return cky_ptr;
}

int cky (SENTENCE_DATA *sp, TOTAL_MGR *Best_mgr) {
    int i, j, k, l, sen_len, cky_table_num, dep_check[BNST_MAX];
    double best_score, para_score;
    char dpnd_type;
    CKY *cky_ptr, *left_ptr, *right_ptr, *best_ptr, *pre_ptr, *best_pre_ptr, *start_ptr;
    CKY **next_pp, **next_pp_for_ij;

    cky_table_num = 0;

    /* initialize */
    for (i = 0; i < sp->Bnst_num; i++) {
	dep_check[i] = -1;
	Best_mgr->dpnd.head[i] = -1;
	Best_mgr->dpnd.type[i] = 'D';
    }

    if (OptParaFix == 0) {
	discard_bad_coordination(sp);
	/* fix_predicate_coordination(sp); */
	/* extend_para_matrix(sp); */
	handle_incomplete_coordination(sp);
    }

    /* ¥ë¡¼¥×¤Ïº¸¤«¤é±¦,²¼¤«¤é¾å
       i¤«¤éj¤Þ¤Ç¤ÎÍ×ÁÇ¤ò¤Þ¤È¤á¤ë½èÍý */
    for (j = 0; j < sp->Bnst_num; j++) { /* left to right (º¸¤«¤é±¦) */
	for (i = j; i >= 0; i--) { /* bottom to top (²¼¤«¤é¾å) */
	    if (OptDisplay == OPT_DEBUG) {
		printf("(%d,%d)\n", i, j);
	    }
	    cky_matrix[i][j] = NULL;
	    if (i == j) {
		if ((cky_ptr = new_cky_data(&cky_table_num)) == NULL) {
		    return FALSE;
		}
		cky_matrix[i][j] = cky_ptr;

		set_cky(sp, cky_ptr, NULL, NULL, i, j, -1, 0, LtoR);
		cky_ptr->score = OptAnalysis == OPT_CASE ? 
		    calc_case_probability(sp, cky_ptr, Best_mgr) : calc_score(sp, cky_ptr);
	    }
	    else {
		next_pp_for_ij = NULL;	/* ¤½¤Î°ÌÃÖ¤Ë°ì¤Ä¤â¶ç¤¬¤Ç¤­¤Æ¤Ê¤¤°õ */

		/* merge (i .. i+k) and (i+k+1 .. j) */
		for (k = 0; k < j - i; k++) {
		    para_score = (sp->bnst_data + i + k)->para_num == -1 ? -1 : 
			Para_matrix[(sp->bnst_data + i + k)->para_num][i][j];
		    next_pp = NULL;
		    left_ptr = cky_matrix[i][i + k];
		    while (left_ptr) {
			right_ptr = cky_matrix[i + k + 1][j];
			while (right_ptr) {
			    /* make a phrase if condition is satisfied */
			    if ((dpnd_type = check_dpnd_possibility(sp, left_ptr->b_ptr->num, right_ptr->b_ptr->num, i, 
								    (j == sp->Bnst_num - 1) && dep_check[i + k] == -1 ? TRUE : FALSE)) && 
				(dpnd_type == 'P' || 
				 dep_check[i + k] <= 0 || /* no barrier */
				 dep_check[i + k] >= j || /* before barrier */
				 (OptParaFix == 0 && relax_barrier_for_P(right_ptr, i + k, j, dep_check)))) { /* barrier relaxation for P */
				if ((cky_ptr = new_cky_data(&cky_table_num)) == NULL) {
				    return FALSE;
				}
				if (next_pp == NULL) {
				    start_ptr = cky_ptr;
				}
				else {
				    *next_pp = cky_ptr;
				}

				set_cky(sp, cky_ptr, left_ptr, right_ptr, i, j, k, dpnd_type, 
					Dpnd_matrix[left_ptr->b_ptr->num][right_ptr->b_ptr->num] == 'L' ? RtoL : LtoR);
				next_pp = &(cky_ptr->next);

				if (OptDisplay == OPT_DEBUG) {
				    printf("   (%d,%d), (%d,%d) b=%d [%s%s%s], %c(para=%.3f), score=", 
					   i, i + k, i + k + 1, j, dep_check[i + k], 
					   left_ptr->b_ptr->head_ptr->Goi, 
					   cky_ptr->direction == RtoL ? "<-" : "->", 
					   right_ptr->b_ptr->head_ptr->Goi, 
					   dpnd_type, para_score);
				}

				cky_ptr->para_score = para_score;
				cky_ptr->score = OptAnalysis == OPT_CASE ? 
				    calc_case_probability(sp, cky_ptr, Best_mgr) : calc_score(sp, cky_ptr);
			    }

			    /* if dpnd direction is B, check RtoL again */
			    if (Dpnd_matrix[left_ptr->b_ptr->num][right_ptr->b_ptr->num] == 'B') {
				if ((cky_ptr = new_cky_data(&cky_table_num)) == NULL) {
				    return FALSE;
				}
				if (next_pp == NULL) {
				    start_ptr = cky_ptr;
				}
				else {
				    *next_pp = cky_ptr;
				}

				set_cky(sp, cky_ptr, left_ptr, right_ptr, i, j, k, 'L', RtoL);
				next_pp = &(cky_ptr->next);

				if (OptDisplay == OPT_DEBUG) {
				    printf("   (%d,%d), (%d,%d) [%s<-%s], score=", i, i + k, i + k + 1, j, 
					   left_ptr->b_ptr->head_ptr->Goi, 
					   right_ptr->b_ptr->head_ptr->Goi);
				}

				cky_ptr->para_score = para_score;
				cky_ptr->score = OptAnalysis == OPT_CASE ? 
				    calc_case_probability(sp, cky_ptr, Best_mgr) : calc_score(sp, cky_ptr);
			    }

			    if (Language != CHINESE && 
				!check_feature(right_ptr->b_ptr->f, "ÍÑ¸À")) { /* consider only the best one if noun */
				break;
			    }
			    right_ptr = right_ptr->next;
			}

			if (Language != CHINESE && 
			    (!check_feature(left_ptr->b_ptr->f, "ÍÑ¸À") || /* consider only the best one if noun or VP */
			     check_feature(left_ptr->b_ptr->f, "·¸:Ï¢ÍÑ"))) {
			    break;
			}
			left_ptr = left_ptr->next;
		    }

		    if (next_pp) {
			/* Ì¾»ì¤Î¾ì¹ç¤Ï¤³¤³¤Ç1¤Ä¤Ë¹Ê¤Ã¤Æ¤â¤è¤¤ */

			if (next_pp_for_ij == NULL) {
			    cky_matrix[i][j] = start_ptr;
			}
			else {
			    *next_pp_for_ij = start_ptr;
			}
			next_pp_for_ij = next_pp;

			/* barrier handling */
			if (j != sp->Bnst_num - 1) { /* don't check in case of relaxation */
			    if ((OptParaFix || Dpnd_matrix[i + k][j]) &&  /* don't set barrier in case of P */
				(sp->bnst_data + i + k)->dpnd_rule->barrier.fp[0] && 
				feature_pattern_match(&((sp->bnst_data + i + k)->dpnd_rule->barrier), 
						      (sp->bnst_data + j)->f, 
						      sp->bnst_data + i + k, sp->bnst_data + j) == TRUE) {
				dep_check[i + k] = j; /* set barrier */
			    }
			    else if (dep_check[i + k] == -1) {
				if (Language != CHINESE && 
				    (OptParaFix || Dpnd_matrix[i + k][j]) && /* don't set barrier in case of P */
				    (sp->bnst_data + i + k)->dpnd_rule->preference != -1 && 
				    (sp->bnst_data + i + k)->dpnd_rule->barrier.fp[0] == NULL) { /* no condition */
				    dep_check[i + k] = j; /* set barrier */
				}
				else {
				    dep_check[i + k] = 0; /* ·¸¤ê¼õ¤±¤¬¤¹¤¯¤Ê¤¯¤È¤â1¤Ä¤ÏÀ®Î©¤·¤¿¤³¤È¤ò¼¨¤¹ */
				}
			    }
			}
		    }
		}

		/* coordination that consists of more than 2 phrases */
		if (OptParaFix == 0) {
		    next_pp = NULL;
		    for (k = 0; k < j - i - 1; k++) {
			right_ptr = cky_matrix[i + k + 1][j];
			while (right_ptr) {
			    left_ptr = right_ptr;
			    while (left_ptr && (left_ptr->dpnd_type == 'P' || left_ptr->para_flag)) {
				left_ptr = left_ptr->left;
			    }
			    if (left_ptr && left_ptr != right_ptr) {
				left_ptr = cky_matrix[i][left_ptr->j]; /* ¾å(i¹Ô)¤Ë¤¢¤²¤ë */
				while (left_ptr) {
				    if (left_ptr->dpnd_type == 'P') {
					if ((cky_ptr = new_cky_data(&cky_table_num)) == NULL) {
					    return FALSE;
					}
					if (next_pp == NULL) {
					    start_ptr = cky_ptr;
					}
					else {
					    *next_pp = cky_ptr;
					}

					set_cky(sp, cky_ptr, left_ptr, right_ptr, i, j, k, 'P', LtoR);
					next_pp = &(cky_ptr->next);

					if (OptDisplay == OPT_DEBUG) {
					    printf("** (%d,%d), (%d,%d) b=%d [%s--%s], P(para=--), score=", 
						   i, left_ptr->j, i + k + 1, j, dep_check[i], 
						   (sp->bnst_data + i)->head_ptr->Goi, 
						   (sp->bnst_data + left_ptr->j)->head_ptr->Goi);
					}

					cky_ptr->para_flag = 1;
					cky_ptr->para_score = cky_ptr->left->para_score + cky_ptr->right->para_score;
					cky_ptr->score = OptAnalysis == OPT_CASE ? 
					    calc_case_probability(sp, cky_ptr, Best_mgr) : calc_score(sp, cky_ptr);
				    }
				    left_ptr = left_ptr->next;
				}
			    }
			    right_ptr = right_ptr->next;
			}
			/* if (next_pp) break; */
		    }

		    if (next_pp) {
			if (next_pp_for_ij == NULL) {
			    cky_matrix[i][j] = start_ptr;
			}
			else {
			    *next_pp_for_ij = start_ptr;
			}
			next_pp_for_ij = next_pp;
		    }
		}

		/* move the best one to the beginning of the list */
		if (next_pp_for_ij) {
		    cky_ptr = cky_matrix[i][j];
		    best_score = -INT_MAX;
		    pre_ptr = NULL;
		    while (cky_ptr) {
			if (cky_ptr->score > best_score) {
			    best_score = cky_ptr->score;
			    best_ptr = cky_ptr;
			    best_pre_ptr = pre_ptr;
			}
			pre_ptr = cky_ptr;
			cky_ptr = cky_ptr->next;
		    }
		    if (best_pre_ptr) { /* best¤¬ÀèÆ¬¤Ç¤Ï¤Ê¤¤¾ì¹ç */
			best_pre_ptr->next = best_ptr->next;
			best_ptr->next = cky_matrix[i][j];
			cky_matrix[i][j] = best_ptr;
		    }
		}
	    }
	}
    }

    if (OptDisplay == OPT_DEBUG) {
	printf(">>> n=%d\n", cky_table_num);
    }

    /* choose the best one */
    cky_ptr = cky_matrix[0][sp->Bnst_num - 1];
    if (!cky_ptr) {
	return FALSE;
    }

    if (cky_ptr->next) { /* if there are more than one possibility */
	best_score = -INT_MAX;
	pre_ptr = NULL;
	while (cky_ptr) {
	    if (cky_ptr->score > best_score) {
		best_score = cky_ptr->score;
		best_ptr = cky_ptr;
		best_pre_ptr = pre_ptr;
	    }
	    pre_ptr = cky_ptr;
	    cky_ptr = cky_ptr->next;
	}
	if (pre_ptr != best_ptr) {
	    if (best_pre_ptr) {
		best_pre_ptr->next = best_ptr->next;
	    }
	    else {
		cky_matrix[0][sp->Bnst_num - 1] = cky_matrix[0][sp->Bnst_num - 1]->next;
	    }
	    pre_ptr->next = best_ptr; /* move the best one to the end of the list */
	    best_ptr->next = NULL;
	}

	/* cky_ptr = cky_matrix[0][sp->Bnst_num - 1]; * when print all possible structures */
	cky_ptr = best_ptr;
    }

    /* count the number of predicates */
    Best_mgr->pred_num = 0;
    for (i = 0; i < sp->Tag_num; i++) {
	if ((sp->tag_data + i)->cf_num > 0 && 
	    ((sp->tag_data + i)->inum == 0 || /* the last basic phrase in a bunsetsu */
	     ((sp->tag_data + i)->inum == 1 && 
	      check_feature((sp->tag_data + i)->b_ptr->f, "¥¿¥°Ã±°Ì¼õ:-1")))) { 
	    (sp->tag_data + i)->pred_num = Best_mgr->pred_num;
	    Best_mgr->cpm[Best_mgr->pred_num].pred_b_ptr = NULL;
	    Best_mgr->pred_num++;
	}
    }

    while (cky_ptr) {
	if (OptDisplay == OPT_DEBUG) {
	    printf("---------------------\n");
	    printf("score=%.3f\n", cky_ptr->score);
	}

	Best_mgr->dpnd.head[cky_ptr->b_ptr->num] = -1;
	Best_mgr->score = cky_ptr->score;
	convert_to_dpnd(sp, Best_mgr, cky_ptr);
	cky_ptr = cky_ptr->next;
    }

    /* Ìµ³Ê½¾Â°: Á°¤ÎÊ¸Àá¤Î·¸¤ê¼õ¤±¤Ë½¾¤¦¾ì¹ç */
    for (i = 0; i < sp->Bnst_num - 1; i++) {
	if (Best_mgr->dpnd.head[i] < 0) {
	    /* ¤¢¤ê¤¨¤Ê¤¤·¸¤ê¼õ¤± */
	    if (i >= Best_mgr->dpnd.head[i + Best_mgr->dpnd.head[i]]) {
		if (Language != CHINESE) {
		    Best_mgr->dpnd.head[i] = sp->Bnst_num - 1; /* Ê¸Ëö¤Ë´ËÏÂ */
		}
		continue;
	    }
	    Best_mgr->dpnd.head[i] = Best_mgr->dpnd.head[i + Best_mgr->dpnd.head[i]];
	    /* Best_mgr->dpnd.check[i].pos[0] = Best_mgr->dpnd.head[i]; */
	}
    }

    /* record case analysis results */
    if (OptAnalysis == OPT_CASE) {
	for (i = 0; i < Best_mgr->pred_num; i++) {
	    if (Best_mgr->cpm[i].result_num != 0 && 
		Best_mgr->cpm[i].cmm[0].cf_ptr->cf_address != -1 && 
		Best_mgr->cpm[i].cmm[0].score != CASE_MATCH_FAILURE_PROB) {
		record_case_analysis(sp, &(Best_mgr->cpm[i]), NULL, FALSE);
	    }
	}
    }

    return TRUE;
}
