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
    int         index;          /* index of dpnd rule for Chinese */
    BNST_DATA	*b_ptr;
    int 	scase_check[SCASE_CODE_SIZE];
    int		un_count;
    CF_PRED_MGR *cpm_ptr;	/* case components */
    CKYptr	left;		/* pointer to the left child */
    CKYptr	right;		/* pointer to the right child */
    CKYptr	next;		/* pointer to the next CKY data at this point */
} CKY;

#define	CKY_TABLE_MAX	800000
CKY *cky_matrix[BNST_MAX][BNST_MAX];/* CKY行列の各位置の最初のCKYデータへのポインタ */
CKY cky_table[CKY_TABLE_MAX];	  /* an array of CKY data */
int cpm_allocated_cky_num = -1;

/* add a clausal modifiee to CPM */
void make_data_cframe_rentai_simple(CF_PRED_MGR *pre_cpm_ptr, TAG_DATA *d_ptr, TAG_DATA *t_ptr) {

    /* 外の関係以外のときは格要素に (外の関係でも形容詞のときは格要素にする) */
    if (!check_feature(t_ptr->f, "外の関係") || check_feature(d_ptr->f, "用言:形")) {
	_make_data_cframe_pp(pre_cpm_ptr, d_ptr, FALSE);
    }
    /* 一意に外の関係にする */
    else {
	pre_cpm_ptr->cf.pp[pre_cpm_ptr->cf.element_num][0] = pp_hstr_to_code("外の関係");
	pre_cpm_ptr->cf.pp[pre_cpm_ptr->cf.element_num][1] = END_M;
	pre_cpm_ptr->cf.oblig[pre_cpm_ptr->cf.element_num] = FALSE;
    }

    _make_data_cframe_sm(pre_cpm_ptr, t_ptr);
    _make_data_cframe_ex(pre_cpm_ptr, t_ptr);
    pre_cpm_ptr->elem_b_ptr[pre_cpm_ptr->cf.element_num] = t_ptr;
    pre_cpm_ptr->elem_b_ptr[pre_cpm_ptr->cf.element_num]->next = NULL;
    pre_cpm_ptr->elem_b_num[pre_cpm_ptr->cf.element_num] = -1;
    pre_cpm_ptr->cf.weight[pre_cpm_ptr->cf.element_num] = 0;
    pre_cpm_ptr->cf.adjacent[pre_cpm_ptr->cf.element_num] = FALSE;
    pre_cpm_ptr->cf.element_num++;
}

/* add coordinated case components to CPM */
TAG_DATA **add_coordinated_phrases(CF_PRED_MGR *cpm_ptr, CKY *cky_ptr, TAG_DATA **next) {
    /* while (cky_ptr) { * 修飾部分のスキップ *
	if (cky_ptr->para_flag || cky_ptr->dpnd_type == 'P') {
	    break;
	}
	cky_ptr = cky_ptr->right;
	} */

    if (!cky_ptr) {
	return NULL;
    }
    else if (cky_ptr->para_flag) { /* parent of <PARA> + <PARA> */
	return add_coordinated_phrases(cpm_ptr, cky_ptr->left, add_coordinated_phrases(cpm_ptr, cky_ptr->right, next));
    }
    else if (cky_ptr->dpnd_type == 'P') {
	*next = cky_ptr->left->b_ptr->tag_ptr + cky_ptr->left->b_ptr->tag_num - 1;
	(*next)->next = NULL;
	return &((*next)->next);
    }
    else {
	return NULL;
    }
}

int check_chi_dpnd_possibility (int i, int j, int k, CKY *left, CKY *right, SENTENCE_DATA *sp, int direction) {
    int l;

    if (Language != CHINESE) {
	return 1;
    }
    else {
	if (Dpnd_matrix[left->b_ptr->num][right->b_ptr->num] > 0 && Dpnd_matrix[left->b_ptr->num][right->b_ptr->num] != 'O') {
	    if (direction != Dpnd_matrix[left->b_ptr->num][right->b_ptr->num] && direction != 'B') {
		return 0;
	    }
	}
        /* check if this cky corresponds with the grammar rules for Chinese */

	/* adj and verb cannot have dependency relation */
	if ((check_feature((sp->bnst_data + left->b_ptr->num)->f, "JJ") && 
	     (check_feature((sp->bnst_data + right->b_ptr->num)->f, "VV") ||
	      check_feature((sp->bnst_data + right->b_ptr->num)->f, "VA") ||
	      check_feature((sp->bnst_data + right->b_ptr->num)->f, "VC") ||
	      check_feature((sp->bnst_data + right->b_ptr->num)->f, "VE"))) ||
	    (check_feature((sp->bnst_data + right->b_ptr->num)->f, "JJ") && 
	     (check_feature((sp->bnst_data + left->b_ptr->num)->f, "VV") ||
	      check_feature((sp->bnst_data + left->b_ptr->num)->f, "VA") ||
	      check_feature((sp->bnst_data + left->b_ptr->num)->f, "VC") ||
	      check_feature((sp->bnst_data + left->b_ptr->num)->f, "VE")))) {
	    return 0;
	}

	/* PU cannot be head */
	if ((check_feature((sp->bnst_data + left->b_ptr->num)->f, "PU") && direction == 'L') || 
	    (check_feature((sp->bnst_data + right->b_ptr->num)->f, "PU") && direction == 'R')) { 
	    return 0;
	}

	/* for DEG, there should not be two modifiers */
	if (check_feature((sp->bnst_data + right->b_ptr->num)->f, "DEG") && (right->j - right->i > 0)) {
	    return 0;
	}

	/* for verb, there should be only one object afterword */
	if ((check_feature((sp->bnst_data + left->b_ptr->num)->f, "VV") ||
	     check_feature((sp->bnst_data + left->b_ptr->num)->f, "VC") ||
	     check_feature((sp->bnst_data + left->b_ptr->num)->f, "VE") ||
	     check_feature((sp->bnst_data + left->b_ptr->num)->f, "P") ||
	     check_feature((sp->bnst_data + left->b_ptr->num)->f, "VA")) &&
	    (check_feature((sp->bnst_data + right->b_ptr->num)->f, "NN") ||
	     check_feature((sp->bnst_data + right->b_ptr->num)->f, "NR") ||
	     check_feature((sp->bnst_data + right->b_ptr->num)->f, "PN")) &&
	    left->j - left->i > 0 &&
	    direction == 'L' &&
	    exist_chi(sp, left->b_ptr->num + 1, left->j, "noun") != -1) {
	    return 0;
	}

	/* for verb, there should be only one subject in front of it */
	if ((check_feature((sp->bnst_data + right->b_ptr->num)->f, "VV") ||
	     check_feature((sp->bnst_data + right->b_ptr->num)->f, "VC") ||
	     check_feature((sp->bnst_data + right->b_ptr->num)->f, "VE") ||
	     check_feature((sp->bnst_data + right->b_ptr->num)->f, "VA")) &&
	    (check_feature((sp->bnst_data + left->b_ptr->num)->f, "NN") ||
	     check_feature((sp->bnst_data + left->b_ptr->num)->f, "PN") ||
	     check_feature((sp->bnst_data + left->b_ptr->num)->f, "NR")) &&
	    (right->right != NULL && right->left != NULL &&
	     right->right->b_ptr->num == right->b_ptr->num &&
	     (check_feature((sp->bnst_data + right->left->b_ptr->num)->f, "NN") ||
	      check_feature((sp->bnst_data + right->left->b_ptr->num)->f, "PN") ||
	      check_feature((sp->bnst_data + right->left->b_ptr->num)->f, "NR")))) {
	    return 0;
	}

	/* for verb, if there exists noun before (without comma), it should have a subject */
	if ((check_feature((sp->bnst_data + right->b_ptr->num)->f, "VV") ||
	     check_feature((sp->bnst_data + right->b_ptr->num)->f, "VC") ||
	     check_feature((sp->bnst_data + right->b_ptr->num)->f, "VE") ||
	     check_feature((sp->bnst_data + right->b_ptr->num)->f, "VA")) &&
	    exist_chi(sp, left->i, left->b_ptr->num - 1, "noun") != -1 &&
	    !(check_feature((sp->bnst_data + left->b_ptr->num)->f, "NN")) &&
	    !(check_feature((sp->bnst_data + left->b_ptr->num)->f, "NR")) &&
	    direction == 'R') {
	    return 0;
	}
	
	/* for preposition, if there is LC in the following (no preposibion between them), the words between P and LC should depend on LC */
	if (check_feature((sp->bnst_data + left->b_ptr->num)->f, "P") &&
	    check_feature((sp->bnst_data + right->b_ptr->num)->f, "LC") &&
	    left->j - left->i > 0 &&
	    exist_chi(sp, left->b_ptr->num + 1, right->b_ptr->num - 1, "prep") == -1) {
	    return 0;
	}

	/* for preposition, if there is noun between it and following verb, the noun should depend on this preposition */
	if (check_feature((sp->bnst_data + left->b_ptr->num)->f, "P") &&
	    (check_feature((sp->bnst_data + right->b_ptr->num)->f, "VV") ||
	     check_feature((sp->bnst_data + right->b_ptr->num)->f, "VA") || 
	     check_feature((sp->bnst_data + right->b_ptr->num)->f, "VC") ||
	     check_feature((sp->bnst_data + right->b_ptr->num)->f, "VE")) &&
	    left->j - left->i == 0 &&
	    exist_chi(sp, right->i, right->b_ptr->num - 1, "noun") != -1) {
	    return 0;
	}

	/* check if this cky corresponds with the constraint of NP and quote */
	if ((Chi_np_end_matrix[i][i + k] != -1 && j > Chi_np_end_matrix[i][i + k]) ||
	    (Chi_np_start_matrix[i + k + 1][j] != -1 && i < Chi_np_start_matrix[i + k + 1][j])){
	    return 0;
	}
	if ((Chi_quote_end_matrix[i][i + k] != -1 && j > Chi_quote_end_matrix[i][i + k]) ||
	    (Chi_quote_start_matrix[i + k + 1][j] != -1 && i < Chi_quote_start_matrix[i + k + 1][j])){
	    return 0;
	}

	return 1;
    }
}

char check_dpnd_possibility (SENTENCE_DATA *sp, int dep, int gov, int begin, int relax_flag) {
    if ((OptParaFix == 0 && 
	 begin >= 0 && 
	 (sp->bnst_data + dep)->para_num != -1 && 
	 Para_matrix[(sp->bnst_data + dep)->para_num][begin][gov] >= 0) || /* para score is not minus */
	(OptParaFix == 1 && 
	 Mask_matrix[dep][gov] == 2)) {   /* 並列P */
	return 'P';
    }
    else if (OptParaFix == 1 && 
	     Mask_matrix[dep][gov] == 3) { /* 並列I */
	return 'I';
    }
    else if (Dpnd_matrix[dep][gov] && Quote_matrix[dep][gov] && 
	     (OptParaFix == 0 || Mask_matrix[dep][gov] == 1)) {
	return Dpnd_matrix[dep][gov];
    }
    else if ((Dpnd_matrix[dep][gov] == 'R' || relax_flag) && Language != CHINESE) { /* relax */
	return 'R';
    }
    else if (Language == CHINESE && (Mask_matrix[dep][gov] == 'N' || Mask_matrix[dep][gov] == 'G' || Mask_matrix[dep][gov] == 'V' || Mask_matrix[dep][gov] == 'E')) {
	return Dpnd_matrix[dep][gov];
    }

    return '\0';
}

/* make an array of dependency possibilities */
void make_work_mgr_dpnd_check(SENTENCE_DATA *sp, CKY *cky_ptr, BNST_DATA *d_ptr) {
    int i, count = 0, start;

    /* 隣にある並列構造(1+1)に係る場合は距離1とする */
    if (cky_ptr->right && cky_ptr->right->dpnd_type == 'P' && cky_ptr->right->j < d_ptr->num + 3)
	start = cky_ptr->right->j;
    else
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
	    check_feature(cky_ptr->left->cpm_ptr->pred_b_ptr->f, "係:連格")) { /* clausal modifiee */
	    CF_PRED_MGR *cpm_ptr = &(Best_mgr->cpm[cky_ptr->left->cpm_ptr->pred_b_ptr->pred_num]);

	    if (cpm_ptr->pred_b_ptr == NULL) { /* まだBest_mgrにコピーしていないとき */
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
		(cp = check_feature(cky_ptr->left->b_ptr->f, "係:無格従属")) != NULL) {
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

    /* 空いているガ格,ヲ格,ニ格,ガ２ */
    if ((g_ptr->SCASE_code[case2num("ガ格")]
	 - scase_check[case2num("ガ格")]) == 1) {
	vacant_slot_num++;
    }
    if ((g_ptr->SCASE_code[case2num("ヲ格")]
	 - scase_check[case2num("ヲ格")]) == 1) {
	vacant_slot_num++;
    }
    if ((g_ptr->SCASE_code[case2num("ニ格")]
	 - scase_check[case2num("ニ格")]) == 1 &&
	rentai == 1 &&
	check_feature(g_ptr->f, "用言:動")) {
	vacant_slot_num++;
	/* ニ格は動詞で連体修飾の場合だけ考慮，つまり連体
	   修飾に割り当てるだけで，未格のスロットとはしない */
    }
    if ((g_ptr->SCASE_code[case2num("ガ２")]
	 - scase_check[case2num("ガ２")]) == 1) {
	vacant_slot_num++;
    }

    /* 空きスロット分だけ連体修飾，未格にスコアを与える */
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

    /* 対象の用言以外のスコアを集める (rightをたどりながらleftのスコアを足す) */
    while (tmp_cky_ptr) {
	if (tmp_cky_ptr->direction == LtoR ? tmp_cky_ptr->left : tmp_cky_ptr->right) {
	    one_score += tmp_cky_ptr->direction == LtoR ? tmp_cky_ptr->left->score : tmp_cky_ptr->right->score;
	}
	tmp_cky_ptr = tmp_cky_ptr->direction == LtoR ? tmp_cky_ptr->right : tmp_cky_ptr->left;
    }
    if (OptDisplay == OPT_DEBUG) {
	printf("%.3f=>", one_score);
    }

    if (check_feature(g_ptr->f, "用言") ||
	check_feature(g_ptr->f, "準用言")) {
	pred_p = 1;
	for (k = 0; k < SCASE_CODE_SIZE; k++) cky_ptr->scase_check[k] = 0;
	scase_check = &(cky_ptr->scase_check[0]);
	cky_ptr->un_count = 0;
	un_count = &(cky_ptr->un_count);
    }

    /* すべての子供について */
    while (cky_ptr) {
	if (cky_ptr->direction == LtoR ? cky_ptr->left : cky_ptr->right) {
	    d_ptr = cky_ptr->direction == LtoR ? cky_ptr->left->b_ptr : cky_ptr->right->b_ptr;

	    if ((d_ptr->num < g_ptr->num &&
		 (Mask_matrix[d_ptr->num][g_ptr->num] == 2 || /* 並列P */
		  Mask_matrix[d_ptr->num][g_ptr->num] == 3)) || /* 並列I */
		(g_ptr->num < d_ptr->num &&
		 (Mask_matrix[g_ptr->num][d_ptr->num] == 2 || /* 並列P */
		  Mask_matrix[g_ptr->num][d_ptr->num] == 3))) { /* 並列I */
		;
	    }
	    else {
		/* 距離コストを計算するために、まず係り先の候補を調べる */
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
			     !strcmp((sp->bnst_data+i)->head_ptr->Goi, "：") ||
			     !strcmp((sp->bnst_data+i)->head_ptr->Goi, ":") ||
			     !strcmp((sp->bnst_data+i)->head_ptr->Goi, "；") ||
			     !strcmp((sp->bnst_data+i)->head_ptr->Goi, "，"))) {
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
			     !strcmp((sp->bnst_data+i)->head_ptr->Goi, "：") ||
			     !strcmp((sp->bnst_data+i)->head_ptr->Goi, ":") ||
			     !strcmp((sp->bnst_data+i)->head_ptr->Goi, "；") ||
			     !strcmp((sp->bnst_data+i)->head_ptr->Goi, "，"))) {
			    comma++;
			}
		    }
		}

		if (Language == CHINESE) {
		    one_score -= 8 * verb;
		    one_score -= 15 * comma;
		}

		default_pos = (d_ptr->dpnd_rule->preference == -1) ?
		    count : d_ptr->dpnd_rule->preference;
		

		/* 係り先のDEFAULTの位置との差をペナルティに
		   ※ 提題はC,B'を求めて遠くに係ることがあるが，それが
		   他の係り先に影響しないよう,ペナルティに差をつける */
		if (check_feature(d_ptr->f, "提題")) {
		    one_score -= abs(default_pos - 1 - pos);
		}
		else if (Language != CHINESE){
		    one_score -= abs(default_pos - 1 - pos) * 2;
		}
/* 		else if (Language == CHINESE) { */
/* 		    if (abs(default_pos - 1 - pos) > 20) { */
/* 			one_score -= 20; */
/* 		    } */
/* 		} */

		/* 読点をもつものが隣にかかることを防ぐ */
		if (d_ptr->num + 1 == g_ptr->num && 
		    abs(default_pos - 1 - pos) > 0 && 
		    (check_feature(d_ptr->f, "読点"))) {
		    one_score -= 5;
		}
	    }
	    
	    if (pred_p && (cp = check_feature(d_ptr->f, "係")) != NULL) {
		    
		/* 未格 提題(「〜は」)の扱い */
		if (check_feature(d_ptr->f, "提題") && !strcmp(cp, "係:未格")) {

		    /* 文末, 「〜が」など, 並列末, C, B'に係ることを優先 */
		    if ((cp2 = check_feature(g_ptr->f, "提題受")) != NULL) {
			sscanf(cp2, "%*[^:]:%d", &topic_score);
			one_score += topic_score;
		    }

		    /* 一つめの提題にだけ点を与える (時間,数量は別)
		       → 複数の提題が同一述語に係ることを防ぐ */
		    if (check_feature(d_ptr->f, "時間") ||
			check_feature(d_ptr->f, "数量")) {
			one_score += 10;
		    }
		    else if (ha_check == 0){
			one_score += 10;
			ha_check = 1;
		    }
		}

		k = case2num(cp + 3);

		/* 格要素一般の扱い */

		/* 未格 : 数えておき，後で空スロットを調べる (時間,数量は別) */
		if (!strcmp(cp, "係:未格")) {
		    if (check_feature(d_ptr->f, "時間") ||
			check_feature(d_ptr->f, "数量")) {
			one_score += 10;
		    }
		    else {
			(*un_count)++;
		    }
		}

		/* ノ格 : 体言以外なら break 
		   → それより前の格要素には点を与えない．
		   → ノ格がかかればそれより前の格はかからない

		   ※ 「体言」というのは判定詞のこと，ただし
		   文末などでは用言:動となっていることも
		   あるので，「体言」でチェック */
		else if (!strcmp(cp, "係:ノ格")) {
		    if (!check_feature(g_ptr->f, "体言")) {
			/* one_score += 10;
			   break; */
			if (g_ptr->SCASE_code[case2num("ガ格")] &&
			    scase_check[case2num("ガ格")] == 0) {
			    one_score += 10;
			    scase_check[case2num("ガ格")] = 1;
			}
		    }
		} 

		/* ガ格 : ガガ構文があるので少し複雑 */
		else if (!strcmp(cp, "係:ガ格")) {
		    if (g_ptr->SCASE_code[case2num("ガ格")] &&
			scase_check[case2num("ガ格")] == 0) {
			one_score += 10;
			scase_check[case2num("ガ格")] = 1;
		    }
		    else if (g_ptr->SCASE_code[case2num("ガ２")] &&
			     scase_check[case2num("ガ２")] == 0) {
			one_score += 10;
			scase_check[case2num("ガ格")] = 1;
		    }
		}

		/* 他の格 : 各格1つは点数をあたえる
		   ※ ニ格の場合，時間とそれ以外は区別する方がいいかも？ */
		else if (k != -1) {
		    if (scase_check[k] == 0) {
			scase_check[k] = 1;
			one_score += 10;
		    } 
		}

		/* 「〜するのは〜だ」にボーナス 01/01/11
		   ほとんどの場合改善．

		   改善例)
		   「抗議したのも 任官を 拒否される 理由の 一つらしい」

		   「使うのは 恐ろしい ことだ。」
		   「円満決着に なるかどうかは 微妙な ところだ。」
		   ※ これらの例は「こと/ところだ」に係ると扱う

		   「他人に 教えるのが 好きになる やり方です」
		   ※ この例は曖昧だが，文脈上正しい

		   副作用例)
		   「だれが ＭＶＰか 分からない 試合でしょう」
		   「〜 殴るなど した 疑い。」
		   「ビザを 取るのも 大変な 時代。」
		   「波が 高まるのは 避けられそうにない 雲行きだ。」
		   「あまり 役立つとは 思われない 論理だ。」
		   「どう 折り合うかが 問題視されてきた 法だ。」
		   「認められるかどうかが 争われた 裁判で」

		   ※問題※
		   「あの戦争」が〜 のような場合も用言とみなされるのが問題
		*/

		if (check_feature(d_ptr->f, "用言") &&
		    (check_feature(d_ptr->f, "係:未格") ||
		     check_feature(d_ptr->f, "係:ガ格")) &&
		    check_feature(g_ptr->f, "用言:判")) {
		    one_score += 3;
		}
	    }

	    /* 連体修飾の場合，係先が
	       ・形式名詞,副詞的名詞
	       ・「予定」,「見込み」など
	       でなければ一つの格要素と考える */
	    if (check_feature(d_ptr->f, "係:連格")) {
		if (check_feature(g_ptr->f, "外の関係") || 
		    check_feature(g_ptr->f, "ルール外の関係")) {
		    one_score += 10;	/* 外の関係ならここで加点 */
		}
		else {
		    /* それ以外なら空きスロットをチェック (連体修飾を付けたときのスコアの差分) */
		    one_score += check_scase(d_ptr, &(cky_ptr->left->scase_check[0]), 1, cky_ptr->left->un_count) - 
			check_scase(d_ptr, &(cky_ptr->left->scase_check[0]), 0, cky_ptr->left->un_count);
		}
	    }

	    /* calc score for Chinese */
	    if (Language == CHINESE) {
		/* add score from verb case frame */
		if ((check_feature(g_ptr->f, "VV") ||
		     check_feature(g_ptr->f, "VA") ||
		     check_feature(g_ptr->f, "VC") ||
		     check_feature(g_ptr->f, "VE") ||
		     (check_feature(g_ptr->f, "P") && g_ptr->num < d_ptr->num)) &&
		    (check_feature(d_ptr->f, "NN") ||
		     check_feature(d_ptr->f, "M") ||
		     check_feature(d_ptr->f, "NT") ||
		     check_feature(d_ptr->f, "PN"))) {
		    /* calc case frame score for Chinese */
		    if (Chi_case_prob_matrix[g_ptr->num][d_ptr->num] >= 0.01) {
			one_score += Chi_case_prob_matrix[g_ptr->num][d_ptr->num] * 20;
		    }
		    else if (Chi_case_prob_matrix[g_ptr->num][d_ptr->num] >= 0.001) {
			one_score += Chi_case_prob_matrix[g_ptr->num][d_ptr->num] * 1500;
		    }
		    else if (Chi_case_prob_matrix[g_ptr->num][d_ptr->num] >= 0.0001) {
			one_score += Chi_case_prob_matrix[g_ptr->num][d_ptr->num] * 10000;
		    }
		    else if (Chi_case_prob_matrix[g_ptr->num][d_ptr->num] >= 0.00001) {
			one_score += Chi_case_prob_matrix[g_ptr->num][d_ptr->num] * 50000;
		    }
		    else if (Chi_case_prob_matrix[g_ptr->num][d_ptr->num] >= 0.000001) {
			one_score += Chi_case_prob_matrix[g_ptr->num][d_ptr->num] * 200000;
		    }
		}

		/* add score from nominal case frame */
		if ((check_feature(g_ptr->f, "NN") ||
		     check_feature(g_ptr->f, "NT") ||
		     check_feature(g_ptr->f, "PN") ||
		     check_feature(g_ptr->f, "M")) &&
		    (check_feature(d_ptr->f, "NN") ||
		     check_feature(d_ptr->f, "NR") ||
		     check_feature(d_ptr->f, "M") ||
		     check_feature(d_ptr->f, "NT") ||
		     check_feature(d_ptr->f, "PN"))) {
		    if (check_feature((sp->bnst_data+d_ptr->num+1)->f, "DEG") ||
			(check_feature(g_ptr->f, "NN") &&
			 (check_feature(d_ptr->f, "NT") ||
			  check_feature(d_ptr->f, "M"))) ||
			(check_feature(g_ptr->f, "NN") &&
			 check_feature(d_ptr->f, "NN") &&
			 d_ptr->num - g_ptr->num == 1)) {}
		    else {
			/* calc case frame score for Chinese */
			if (Chi_case_nominal_prob_matrix[g_ptr->num][d_ptr->num] >= 0.01) {
			    one_score += Chi_case_nominal_prob_matrix[g_ptr->num][d_ptr->num] * 20;
			}
			else if (Chi_case_nominal_prob_matrix[g_ptr->num][d_ptr->num] >= 0.001) {
			    one_score += Chi_case_nominal_prob_matrix[g_ptr->num][d_ptr->num] * 1500;
			}
			else if (Chi_case_nominal_prob_matrix[g_ptr->num][d_ptr->num] >= 0.0001) {
			    one_score += Chi_case_nominal_prob_matrix[g_ptr->num][d_ptr->num] * 10000;
			}
			else if (Chi_case_nominal_prob_matrix[g_ptr->num][d_ptr->num] >= 0.00001) {
			    one_score += Chi_case_nominal_prob_matrix[g_ptr->num][d_ptr->num] * 20000;
			}
			else if (Chi_case_nominal_prob_matrix[g_ptr->num][d_ptr->num] >= 0.000001) {
			    one_score += Chi_case_nominal_prob_matrix[g_ptr->num][d_ptr->num] * 100000;
			}
		    }
		}

		if (cky_ptr->direction == LtoR) {
		    one_score += Chi_dpnd_matrix[d_ptr->num][g_ptr->num].prob_LtoR[cky_ptr->index] * TIME_PROB;

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
			   check_feature(g_ptr->f, "VV") ||
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
		    if ((check_feature(d_ptr->f, "VA") ||
			 check_feature(d_ptr->f, "VV")) &&
			check_feature(g_ptr->f, "DEC") &&
			exist_chi(sp, d_ptr->num+ 1, g_ptr->num - 1, "verb") == -1) {
			one_score += 5;
		    }
		}
		else if (cky_ptr->direction == RtoL) {
		    one_score += Chi_dpnd_matrix[g_ptr->num][d_ptr->num].prob_RtoL[cky_ptr->index] * TIME_PROB;

		    /* add score for stable dpnd */
		    if (g_ptr->num + 1 == d_ptr->num &&
			(((check_feature(d_ptr->f, "AS")) &&
			  (check_feature(g_ptr->f, "VE") ||
			   check_feature(g_ptr->f, "VV"))) ||

			 ((check_feature(d_ptr->f, "CD")) &&
			  (check_feature(g_ptr->f, "DT"))) ||

			 ((check_feature(d_ptr->f, "PN")) &&
			  (check_feature(g_ptr->f, "P"))) ||

			 ((check_feature(d_ptr->f, "NT")) &&
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
			one_score += 5;
		    }
		    if (check_feature(g_ptr->f, "P") && check_feature(d_ptr->f, "CS")) {
			one_score += 10;
		    }
		}
	    }
	}

	cky_ptr = cky_ptr->direction == LtoR ? cky_ptr->right : cky_ptr->left;
    }

    /* 用言の場合，最終的に未格,ガ格,ヲ格,ニ格,連体修飾に対して
       ガ格,ヲ格,ニ格のスロット分だけ点数を与える */
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

    /* 対象の用言以外のスコアを集める (rightをたどりながらleftのスコアを足す) */
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

    if (check_feature(g_ptr->f, "タグ単位受:-1") && g_ptr->tag_num > 1) { /* 〜のは */
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
		if (d_ptr->para_num != -1 && (para_key = check_feature(d_ptr->f, "並キ"))) {
		    if (cky_ptr->para_score >= 0) {
			one_score += get_para_exist_probability(para_key, cky_ptr->para_score, TRUE);
			one_score += get_para_ex_probability(para_key, cky_ptr->para_score, d_ptr->tag_ptr + d_ptr->tag_num - 1, t_ptr);
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
		    add_coordinated_phrases(cpm_ptr, cky_ptr->left, &(cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num - 1]->next));
		    child_num++;
		    flag++;
		}

		if ((check_feature(d_ptr->f, "係:連用") && 
		     (!check_feature(d_ptr->f, "用言") || !check_feature(d_ptr->f, "複合辞"))) || 
		    check_feature(d_ptr->f, "修飾")) {
		    flag++;
		}
	    }

	    /* clausal modifiee */
	    if (check_feature(d_ptr->f, "係:連格") && 
		cky_ptr->left->cpm_ptr->pred_b_ptr) { /* 格フレームをもっているべき */
		pre_cpm_ptr = cky_ptr->left->cpm_ptr;
		pre_cpm_ptr->pred_b_ptr->cpm_ptr = pre_cpm_ptr;
		make_work_mgr_dpnd_check(sp, cky_ptr, d_ptr);
		make_data_cframe_rentai_simple(pre_cpm_ptr, pre_cpm_ptr->pred_b_ptr, t_ptr);
		add_coordinated_phrases(pre_cpm_ptr, cky_ptr->right, &(pre_cpm_ptr->elem_b_ptr[pre_cpm_ptr->cf.element_num - 1]->next));

		orig_score = pre_cpm_ptr->score;
		one_score -= orig_score;
		one_score += find_best_cf(sp, pre_cpm_ptr, get_closest_case_component(sp, pre_cpm_ptr), 1);
		pre_cpm_ptr->score = orig_score;
		pre_cpm_ptr->cf.element_num--;
		flag++;
	    }

	    if (flag == 0) { /* 名詞格フレームへ */
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
	    if (check_feature(d_ptr->f, "係:連用") && !check_feature(d_ptr->f, "用言")) {
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

	/* 用言文節が「（〜を）〜に」のとき 
	   「する」の格フレームに対してニ格(同文節)を設定
	   ヲ格は子供の処理で扱われる */
	if (check_feature(t_ptr->f, "Ｔ用言同文節")) {
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
		    if (check_feature(d_ptr->f, "係:連用") && check_feature(d_ptr->f, "用言") && 
			!check_feature(d_ptr->f, "複合辞")) {
			make_work_mgr_dpnd_check(sp, cky_ptr, d_ptr);
			one_score += calc_vp_modifying_probability(t_ptr, cpm_ptr->cmm[0].cf_ptr, 
								   d_ptr->tag_ptr + d_ptr->tag_num - 1, 
								   cky_ptr->left->cpm_ptr->cmm[0].cf_ptr);
			renyou_modifying_num++;
		    }

		    /* modifying adverb */
		    if ((check_feature(d_ptr->f, "係:連用") && !check_feature(d_ptr->f, "用言")) || 
			check_feature(d_ptr->f, "修飾")) {
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
	    if (*(dep_check + dep) >= cky_ptr->left->j) { /* 並列の左側に壁があるなら、右側までOKとする */
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
	     char dpnd_type, int direction, int index) {
    int l;

    cky_ptr->index = index;
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

int after_cky(SENTENCE_DATA *sp, TOTAL_MGR *Best_mgr, CKY *cky_ptr) {
    int i, j;

    /* count the number of predicates */
    Best_mgr->pred_num = 0;
    for (i = 0; i < sp->Tag_num; i++) {
	if ((sp->tag_data + i)->cf_num > 0 && 
	    (((sp->tag_data + i)->inum == 0 && /* the last basic phrase in a bunsetsu */
	      !check_feature((sp->tag_data + i)->b_ptr->f, "タグ単位受:-1")) || 
	     ((sp->tag_data + i)->inum == 1 && 
	      check_feature((sp->tag_data + i)->b_ptr->f, "タグ単位受:-1")))) { 
	    (sp->tag_data + i)->pred_num = Best_mgr->pred_num;
	    Best_mgr->pred_num++;
	}
    }

    /* for all possible structures */
    while (cky_ptr) {
	for (i = 0; i < Best_mgr->pred_num; i++) {
	    Best_mgr->cpm[i].pred_b_ptr = NULL;
	}

	if (OptDisplay == OPT_DEBUG) {
	    printf("---------------------\n");
	    printf("score=%.3f\n", cky_ptr->score);
	}

	Best_mgr->dpnd.head[cky_ptr->b_ptr->num] = -1;
	Best_mgr->score = cky_ptr->score;
	convert_to_dpnd(sp, Best_mgr, cky_ptr);

	/* 無格従属: 前の文節の係り受けに従う場合 */
	for (i = 0; i < sp->Bnst_num - 1; i++) {
	    if (Best_mgr->dpnd.head[i] < 0) {
		/* ありえない係り受け */
		if (i >= Best_mgr->dpnd.head[i + Best_mgr->dpnd.head[i]]) {
		    if (Language != CHINESE) {
			Best_mgr->dpnd.head[i] = sp->Bnst_num - 1; /* 文末に緩和 */
		    }
		    continue;
		}
		Best_mgr->dpnd.head[i] = Best_mgr->dpnd.head[i + Best_mgr->dpnd.head[i]];
		/* Best_mgr->dpnd.check[i].pos[0] = Best_mgr->dpnd.head[i]; */
	    }
	}

	/* 格解析結果の情報をfeatureへ */
	if (OptAnalysis == OPT_CASE) {
	    /* 格解析結果を用言基本句featureへ */
	    for (i = 0; i < sp->Best_mgr->pred_num; i++) {
		assign_nil_assigned_components(sp, &(sp->Best_mgr->cpm[i])); /* 未対応格要素の処理 */

		assign_case_component_feature(sp, &(sp->Best_mgr->cpm[i]), FALSE);

		/* 格フレームの意味情報を用言基本句featureへ */
		for (j = 0; j < sp->Best_mgr->cpm[i].cmm[0].cf_ptr->element_num; j++) {
		    append_cf_feature(&(sp->Best_mgr->cpm[i].pred_b_ptr->f), 
				      &(sp->Best_mgr->cpm[i]), sp->Best_mgr->cpm[i].cmm[0].cf_ptr, j);
		}
	    }
	}

	/* to tree structure */
	dpnd_info_to_bnst(sp, &(Best_mgr->dpnd));
	para_recovery(sp);
	if (!(OptExpress & OPT_NOTAG)) {
	    dpnd_info_to_tag(sp, &(Best_mgr->dpnd));
	}
	if (make_dpnd_tree(sp)) {
	    bnst_to_tag_tree(sp); /* タグ単位の木へ */

	    /* 構造決定後のルール適用 */
	    assign_general_feature(sp->tag_data, sp->Tag_num, AfterDpndTagRuleType, FALSE, FALSE);

	    /* record case analysis results */
	    if (OptAnalysis == OPT_CASE) {
		for (i = 0; i < Best_mgr->pred_num; i++) {
		    if (Best_mgr->cpm[i].result_num != 0 && 
			Best_mgr->cpm[i].cmm[0].cf_ptr->cf_address != -1 && 
			Best_mgr->cpm[i].cmm[0].score != CASE_MATCH_FAILURE_PROB) {
			record_case_analysis(sp, &(Best_mgr->cpm[i]), NULL, FALSE);

			/* 格解析の結果を用いて形態素曖昧性を解消 */
			verb_lexical_disambiguation_by_case_analysis(&(sp->Best_mgr->cpm[i]));
			noun_lexical_disambiguation_by_case_analysis(&(sp->Best_mgr->cpm[i]));
		    }
		}
	    }

	    /* print for debug or nbest */
	    if (OptNbest == TRUE) {
		sp->score = Best_mgr->score;
		print_result(sp, 0);

		if (OptDisplay == OPT_DEBUG) { /* case analysis results */
		    for (i = 0; i < Best_mgr->pred_num; i++) {
			print_data_cframe(&(Best_mgr->cpm[i]), &(Best_mgr->cpm[i].cmm[0]));
			for (j = 0; j < Best_mgr->cpm[i].result_num; j++) {
			    print_crrspnd(&(Best_mgr->cpm[i]), &(Best_mgr->cpm[i].cmm[j]));
			}
		    }
		}
	    }
	    else if (OptDisplay == OPT_DEBUG) {
		print_kakari(sp, OptExpress & OPT_NOTAG ? OPT_NOTAGTREE : OPT_TREE);
	    }
	}

	cky_ptr = cky_ptr->next;
    }

    return TRUE;
}

int cky (SENTENCE_DATA *sp, TOTAL_MGR *Best_mgr) {
    int i, j, k, l, m, sort_flag, sen_len, cky_table_num, dep_check[BNST_MAX];
    double best_score, para_score;
    char dpnd_type;
    CKY *cky_ptr, *left_ptr, *right_ptr, *best_ptr, *pre_ptr, *best_pre_ptr, *start_ptr, *sort_pre_ptr;
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

    /* ループは左から右,下から上
       iからjまでの要素をまとめる処理 */
    for (j = 0; j < sp->Bnst_num; j++) { /* left to right (左から右) */
	for (i = j; i >= 0; i--) { /* bottom to top (下から上) */
	    if (OptDisplay == OPT_DEBUG) {
		printf("(%d,%d)\n", i, j);
	    }
	    cky_matrix[i][j] = NULL;
	    if (i == j) {
		if ((cky_ptr = new_cky_data(&cky_table_num)) == NULL) {
		    return FALSE;
		}
		cky_matrix[i][j] = cky_ptr;

		set_cky(sp, cky_ptr, NULL, NULL, i, j, -1, 0, LtoR, -1);
		cky_ptr->score = OptAnalysis == OPT_CASE ? 
		    calc_case_probability(sp, cky_ptr, Best_mgr) : calc_score(sp, cky_ptr);
	    }
	    else {
		next_pp_for_ij = NULL;	/* その位置に一つも句ができてない印 */

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

				if (Language == CHINESE) {
				    for (l = 0; l < Chi_dpnd_matrix[left_ptr->b_ptr->num][right_ptr->b_ptr->num].count; l++) {
					if (!(check_chi_dpnd_possibility(i, j, k, left_ptr, right_ptr, sp, Chi_dpnd_matrix[left_ptr->b_ptr->num][right_ptr->b_ptr->num].direction[l]))) {
					    continue;
					}
					if ((cky_ptr = new_cky_data(&cky_table_num)) == NULL) {
					    return FALSE;
					}
					if (next_pp == NULL) {
					    start_ptr = cky_ptr;
					}
					else {
					    *next_pp = cky_ptr;
					}

					if (Mask_matrix[i][i + k] == 'N' && Mask_matrix[i + k + 1][j] == 'N') {
					    set_cky(sp, cky_ptr, left_ptr, right_ptr, i, j, k, 'R', LtoR, l); 
					}
					else if (Mask_matrix[i][i + k] == 'G' && Mask_matrix[i + k + 1][j] == 'G') {
					    set_cky(sp, cky_ptr, left_ptr, right_ptr, i, j, k, 'R', LtoR, l); 
					}
					else if (Mask_matrix[i][i + k] == 'V' && Mask_matrix[i + k + 1][j] == 'V') {
					    set_cky(sp, cky_ptr, left_ptr, right_ptr, i, j, k, 'L', RtoL, l); 
					}
					else if (Mask_matrix[i][i + k] == 'E' && Mask_matrix[i + k + 1][j] == 'E') {
					    set_cky(sp, cky_ptr, left_ptr, right_ptr, i, j, k, 'L', RtoL, l); 
					}
					else {
					    set_cky(sp, cky_ptr, left_ptr, right_ptr, i, j, k, 
						    Chi_dpnd_matrix[left_ptr->b_ptr->num][right_ptr->b_ptr->num].direction[l], 
						    Chi_dpnd_matrix[left_ptr->b_ptr->num][right_ptr->b_ptr->num].direction[l] == 'L' ? RtoL : LtoR, l);
					}

					next_pp = &(cky_ptr->next);
					
					if (OptDisplay == OPT_DEBUG) {
					    printf("   (%d,%d), (%d,%d) b=%d [%s%s%s], %c(para=%.3f), score=", 
						   i, i + k, i + k + 1, j, dep_check[i + k], 
						   left_ptr->b_ptr->head_ptr->Goi, 
						   cky_ptr->direction == RtoL ? "<-" : "->", 
						   right_ptr->b_ptr->head_ptr->Goi, 
						   Chi_dpnd_matrix[left_ptr->b_ptr->num][right_ptr->b_ptr->num].direction[l], para_score);
					}

					cky_ptr->para_score = para_score;
					cky_ptr->score = OptAnalysis == OPT_CASE ? 
					    calc_case_probability(sp, cky_ptr, Best_mgr) : calc_score(sp, cky_ptr);

					if (Mask_matrix[i][i + k] == 'N' && Mask_matrix[i + k + 1][j] == 'N') {
					    cky_ptr->score += 20;
					}
					else if (Mask_matrix[i][i + k] == 'G' && Mask_matrix[i + k + 1][j] == 'G') {
					    cky_ptr->score += 20;
					}
					else if (Mask_matrix[i][i + k] == 'V' && Mask_matrix[i + k + 1][j] == 'V') {
					    cky_ptr->score += 20;
					}
					else if (Mask_matrix[i][i + k] == 'E' && Mask_matrix[i + k + 1][j] == 'E') {
					    cky_ptr->score += 20;
					}

					/* if dpnd direction is B, check RtoL again */
					if (Chi_dpnd_matrix[left_ptr->b_ptr->num][right_ptr->b_ptr->num].direction[l] == 'B') {
					    if ((cky_ptr = new_cky_data(&cky_table_num)) == NULL) {
						return FALSE;
					    }
					    if (next_pp == NULL) {
						start_ptr = cky_ptr;
					    }
					    else {
						*next_pp = cky_ptr;
					    }
					    
					    if (Mask_matrix[i][i + k] == 'V' && Mask_matrix[i + k + 1][j] == 'V') {
						set_cky(sp, cky_ptr, left_ptr, right_ptr, i, j, k, 'L', RtoL, l); 
					    }
					    else if (Mask_matrix[i][i + k] == 'E' && Mask_matrix[i + k + 1][j] == 'E') {
						set_cky(sp, cky_ptr, left_ptr, right_ptr, i, j, k, 'L', RtoL, l); 
					    }
					    else {
						set_cky(sp, cky_ptr, left_ptr, right_ptr, i, j, k, 'L', RtoL, l); 
					    }

					    next_pp = &(cky_ptr->next);
					    
					    if (OptDisplay == OPT_DEBUG) {
						printf("   (%d,%d), (%d,%d) [%s<-%s], score=", i, i + k, i + k + 1, j, 
						       left_ptr->b_ptr->head_ptr->Goi, 
						       right_ptr->b_ptr->head_ptr->Goi);
					    }

					    cky_ptr->para_score = para_score;
					    cky_ptr->score = OptAnalysis == OPT_CASE ? 
						calc_case_probability(sp, cky_ptr, Best_mgr) : calc_score(sp, cky_ptr);

					    if (Mask_matrix[i][i + k] == 'V' && Mask_matrix[i + k + 1][j] == 'V') {
						cky_ptr->score += 20;
					    }
					    else if (Mask_matrix[i][i + k] == 'E' && Mask_matrix[i + k + 1][j] == 'E') {
						cky_ptr->score += 20;
					    }
					}
				    }
				}
				else {
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
					    Dpnd_matrix[left_ptr->b_ptr->num][right_ptr->b_ptr->num] == 'L' ? RtoL : LtoR, -1);
				    
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
			    }

			    if (Language != CHINESE && 
				OptNbest == FALSE && 
				!check_feature(right_ptr->b_ptr->f, "用言")) { /* consider only the best one if noun */
				break;
			    }
			    right_ptr = right_ptr->next;
			}

			if (Language != CHINESE && 
			    OptNbest == FALSE && 
			    (!check_feature(left_ptr->b_ptr->f, "用言") || /* consider only the best one if noun or VP */
			     check_feature(left_ptr->b_ptr->f, "係:連用"))) {
			    break;
			}
			left_ptr = left_ptr->next;
		    }

		    if (next_pp) {
			/* 名詞の場合はここで1つに絞ってもよい */

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
				    dep_check[i + k] = 0; /* 係り受けがすくなくとも1つは成立したことを示す */
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
				left_ptr = cky_matrix[i][left_ptr->j]; /* 上(i行)にあげる */
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

					set_cky(sp, cky_ptr, left_ptr, right_ptr, i, j, k, 'P', LtoR, -1);
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
		    if (best_pre_ptr) { /* bestが先頭ではない場合 */
			best_pre_ptr->next = best_ptr->next;
			best_ptr->next = cky_matrix[i][j];
			cky_matrix[i][j] = best_ptr;
		    }

		    if (Language == CHINESE && cky_matrix[i][j]->next && cky_matrix[i][j]->next->next) {
			/* only keep CHI_CKY_MAX probabilities for word pair */
			m = 1;
			sort_flag = 1;
			while (m < CHI_CKY_MAX && sort_flag) {
			    cky_ptr = cky_matrix[i][j];			    
			    sort_pre_ptr = NULL;
			    for (l = 0; l < m; l++) {
				if (cky_ptr->next) {
				    sort_pre_ptr = cky_ptr;
				    cky_ptr = cky_ptr->next;
				}
				else {
				    break;
				}
			    }
			    if (cky_ptr->next) {
				sort_flag = 1;
				best_score = -INT_MAX;
				pre_ptr = NULL;
				best_pre_ptr = NULL;
				while (cky_ptr) {
				    if (cky_ptr->score > best_score) {
					best_score = cky_ptr->score;
					best_ptr = cky_ptr;
					best_pre_ptr = pre_ptr;
				    }
				    pre_ptr = cky_ptr;
				    cky_ptr = cky_ptr->next;
				}
				if (best_pre_ptr) { /* bestが先頭ではない場合 */
				    best_pre_ptr->next = best_ptr->next;
				    best_ptr->next = sort_pre_ptr->next;
				    sort_pre_ptr->next = best_ptr;
				}
			    }
			    else {
				sort_flag = 0;
			    }
			    m++;
			}
			cky_ptr = cky_matrix[i][j];
			for (l = 0; l < m; l++) {
			    if (cky_ptr->next) {
				cky_ptr = cky_ptr->next;
			    }
			}
			cky_ptr->next = NULL;
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

	if (OptNbest == TRUE) {
	    cky_ptr = cky_matrix[0][sp->Bnst_num - 1]; /* when print all possible structures */
	}
	else {
	    cky_ptr = best_ptr;
	}
    }

    return after_cky(sp, Best_mgr, cky_ptr);
}

/* check if there exists special word in one region */
int exist_chi(SENTENCE_DATA *sp, int i, int j, char *type) {
    int k;

    if (!strcmp(type, "noun")) {
	for (k = i; k <= j; k++) {
	    if (check_feature((sp->bnst_data + k)->f, "PU") && 
		(!strcmp((sp->bnst_data+i)->head_ptr->Goi, ",") ||
		 !strcmp((sp->bnst_data+i)->head_ptr->Goi, "：") ||
		 !strcmp((sp->bnst_data+i)->head_ptr->Goi, ":") ||
		 !strcmp((sp->bnst_data+i)->head_ptr->Goi, "；") ||
		 !strcmp((sp->bnst_data+i)->head_ptr->Goi, "，"))) {
		break;
	    }
	    if (check_feature((sp->bnst_data + k)->f, "NN") ||
		check_feature((sp->bnst_data + k)->f, "NR")){
		return k;
	    }
	}
    }
    else if (!strcmp(type, "DEC")) {
	for (k = i; k <= j; k++) {
	    if (check_feature((sp->bnst_data + k)->f, "DEC")) {
		return k;
	    }
	}
    }
    else if (!strcmp(type, "verb")) {
	for (k = i; k <= j; k++) {
	    if (check_feature((sp->bnst_data + k)->f, "VV") ||
		check_feature((sp->bnst_data + k)->f, "VA")) {
		return k;
	    }
	}
    }
    else if (!strcmp(type, "prep")) {
	for (k = i; k <= j; k++) {
	    if (check_feature((sp->bnst_data + k)->f, "P")) {
		return k;
	    }
	}
    }
    
    return -1;
}

/* check the number of special pos-tag in a sentence */
int check_pos_num_chi(SENTENCE_DATA *sp, char *type) {
    int k;
    int num = 0;

    if (!strcmp(type, "verb")) {
	for (k = 0; k < sp->Bnst_num; k++) {
	    if (check_feature((sp->bnst_data + k)->f, "VV") ||
		check_feature((sp->bnst_data + k)->f, "VA") ||
		check_feature((sp->bnst_data + k)->f, "VC") ||
		check_feature((sp->bnst_data + k)->f, "VE")) {
		num++;
	    }
	}
    }
    else if (!strcmp(type, "DEC")) {
	for (k = 0; k < sp->Bnst_num; k++) {
	    if (check_feature((sp->bnst_data + k)->f, "DEC")) {
		num++;
	    }
	}
    }

    return num;
}
