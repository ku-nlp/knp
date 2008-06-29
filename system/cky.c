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
    double      chicase_score;
    double      chicase_lex_score;
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

  int        left_pos_index;  /* pos index for Chinese */
  int        right_pos_index;  /* pos index for Chinese */
} CKY;

#define DOUBLE_MINUS    -999.0
#define PARA_THRESHOLD	0

#ifdef LANGUAGE_CHINESE
#define	CKY_TABLE_MAX	12900000 
#else
#define	CKY_TABLE_MAX	1000000 
#endif

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
TAG_DATA **add_coordinated_phrases(CKY *cky_ptr, TAG_DATA **next) {
    while (cky_ptr) { /* 修飾部分のスキップ */
	if (cky_ptr->para_flag || cky_ptr->dpnd_type == 'P') {
	    break;
	}
	cky_ptr = cky_ptr->right;
    }

    if (!cky_ptr) {
	return NULL;
    }
    else if (cky_ptr->para_flag) { /* parent of <PARA> + <PARA> */
	return add_coordinated_phrases(cky_ptr->left, add_coordinated_phrases(cky_ptr->right, next));
    }
    else if (cky_ptr->dpnd_type == 'P') {
	TAG_DATA **next_pp;

	*next = cky_ptr->left->b_ptr->tag_ptr + cky_ptr->left->b_ptr->tag_num - 1;
	(*next)->next = NULL;
	next_pp = add_coordinated_phrases(cky_ptr->right, &((*next)->next));

	if (next_pp) {
	    return next_pp;
	}
	else {
	    return &((*next)->next);
	}
    }
    else {
	return NULL;
    }
}

char check_dpnd_possibility (SENTENCE_DATA *sp, int dep, int gov, int begin, int relax_flag) {
    if ((OptParaFix == 0 && 
	 begin >= 0 && 
	 (sp->bnst_data + dep)->para_num != -1 && 
	 Para_matrix[(sp->bnst_data + dep)->para_num][begin][gov] >= PARA_THRESHOLD) || /* para score is more than threshold */
	(OptParaFix == 1 && 
	 Mask_matrix[dep][gov] == 2)) {   /* 並列P */
	return 'P';
    }
    else if (OptParaFix == 1 && 
	     Mask_matrix[dep][gov] == 3) { /* 並列I */
	return 'I';
    }
    else if (Dpnd_matrix[dep][gov] && Quote_matrix[dep][gov] && 
	     ((Language != CHINESE && (OptParaFix == 0 || Mask_matrix[dep][gov] == 1)) ||
	      (Language == CHINESE && Mask_matrix[dep][gov] != 0))) {
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
    BNST_DATA *tmp_d_ptr;
    TAG_DATA *tmp_t_ptr = d_ptr->tag_ptr + d_ptr->tag_num - 1;

    while (tmp_t_ptr) {
	tmp_d_ptr = tmp_t_ptr->b_ptr;
	/* 隣にある並列構造(1+1)に係る場合は距離1とする */
	if (cky_ptr->right && cky_ptr->right->dpnd_type == 'P' && cky_ptr->right->j < tmp_d_ptr->num + 3)
	    start = cky_ptr->right->j;
	else
	    start = tmp_d_ptr->num + 1;

	for (i = start; i < sp->Bnst_num; i++) {
	    if (check_dpnd_possibility(sp, tmp_d_ptr->num, i, -1, ((i == sp->Bnst_num - 1) && count == 0) ? TRUE : FALSE)) {
		Work_mgr.dpnd.check[tmp_d_ptr->num].pos[count] = i;
		count++;
	    }
	}

	Work_mgr.dpnd.check[tmp_d_ptr->num].num = count;
	tmp_t_ptr = tmp_t_ptr->next;
    }
}

void make_work_mgr_dpnd_check_for_noun(SENTENCE_DATA *sp, BNST_DATA *d_ptr) {
    int i, count = 0;

    for (i = d_ptr->num + 1; i < sp->Bnst_num; i++) {
	if (check_feature((sp->bnst_data + i)->f, "体言")) {
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

	    if (Language == CHINESE && cky_ptr->dpnd_type != 'P') {
		if (cky_ptr->para_score > PARA_THRESHOLD) {
		    if (cky_ptr->direction == RtoL) { /* <- */
			Best_mgr->dpnd.head[cky_ptr->right->b_ptr->num] = cky_ptr->left->b_ptr->num;
			Best_mgr->dpnd.type[cky_ptr->right->b_ptr->num] = cky_ptr->dpnd_type;
			(sp->bnst_data + cky_ptr->right->b_ptr->num)->is_para = 1;
			(sp->bnst_data + cky_ptr->left->b_ptr->num)->is_para = 2;
		    }
		    else { /* -> */
			Best_mgr->dpnd.head[cky_ptr->left->b_ptr->num] = cky_ptr->right->b_ptr->num;
			Best_mgr->dpnd.type[cky_ptr->left->b_ptr->num] = cky_ptr->dpnd_type;
			(sp->bnst_data + cky_ptr->left->b_ptr->num)->is_para = 1;
			(sp->bnst_data + cky_ptr->right->b_ptr->num)->is_para = 2;
		    }
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

    call_count_dpnd_candidates(sp, &(Best_mgr->dpnd));
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
    CKY *right_ptr = cky_ptr->right, *tmp_cky_ptr = cky_ptr, *tmp_child_ptr;
    BNST_DATA *g_ptr = cky_ptr->b_ptr, *d_ptr;
    int i, k, pred_p = 0, topic_score = 0;
    int ha_check = 0, *un_count;
    int rentai, vacant_slot_num, *scase_check;
    int count, pos, default_pos;
    int verb, comma;
    double one_score = 0;
    double prob;
    char *cp, *cp2;

    double chi_pa_thre;
    double weight_dpnd, weight_pos, weight_comma, weight_root, weight_pa;
    double pos_prob_thre_high, pos_prob_thre_low;
    int pos_occur_thre_high, pos_occur_thre_low;

    int left_arg_num;
    int right_arg_num;
    int ptr_num;
    double chicase_prob;

    int pre_pos_index;

    chi_pa_thre = 0.00005;

    weight_dpnd = 1.0;
    weight_pos = 0.5;
    weight_comma = 1.0;
    weight_root = 0.5;
    weight_pa = 1.0;

    pos_prob_thre_high = 0.95;
    pos_prob_thre_low = 0.05;

    pos_occur_thre_high = 100;
    pos_occur_thre_low = 50;

    /* 対象の用言以外のスコアを集める (rightをたどりながらleftのスコアを足す) */
    while (tmp_cky_ptr) {
	if (tmp_cky_ptr->direction == LtoR ? tmp_cky_ptr->left : tmp_cky_ptr->right) {
	    one_score += tmp_cky_ptr->direction == LtoR ? tmp_cky_ptr->left->score : tmp_cky_ptr->right->score;
	}
	tmp_cky_ptr = tmp_cky_ptr->direction == LtoR ? tmp_cky_ptr->right : tmp_cky_ptr->left;
    }
    if (OptDisplay == OPT_DEBUG) {
	if (Language == CHINESE) {
	    printf("%.6f=>", one_score);
	}
	else {
	    printf("%.3f=>", one_score);
	}
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
			if (Language == CHINESE && !OptChiPos &&
			    (check_feature((sp->bnst_data+i)->f, "VV") ||
			     check_feature((sp->bnst_data+i)->f, "VA") ||
			     check_feature((sp->bnst_data+i)->f, "VC") ||
			     check_feature((sp->bnst_data+i)->f, "VE"))) {
			    verb++;
			}
			if (Language == CHINESE && !OptChiPos &&
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
			if (Language == CHINESE && !OptChiPos &&
			    (check_feature((sp->bnst_data+i)->f, "VV") ||
			     check_feature((sp->bnst_data+i)->f, "VA") ||
			     check_feature((sp->bnst_data+i)->f, "VC") ||
			     check_feature((sp->bnst_data+i)->f, "VE"))) {
			    verb++;
			}
			if (Language == CHINESE && !OptChiPos &&
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
		if (OptChiGenerative) {
		    prob = 0;
		    chicase_prob = 0;
		   
		    /* initialization */
		    for (i = 0; i < CHI_ARG_NUM_MAX + 1; i++) {
		      left_arg[i] = -1;
		      right_arg[i] = -1;
		    }
		    ptr_num = -1;
		    left_arg_num = 0;
		    right_arg_num = 0;

		    /* add penalty from deterministic parsing */
		    char *det_head = malloc(12);
		    if (cky_ptr->i == 0 && cky_ptr->j == sp->Bnst_num) {
		      sprintf(det_head, "DETHEAD_-1");
		    }
		    else {
		      sprintf(det_head, "DETHEAD_%i", g_ptr->num);
		    }
		    if (!check_feature(d_ptr->f, det_head)) {
		      prob += log(CHI_DET_PENALTY);
		    }
		    if (det_head) {
		      free(det_head);
		    }

		    if (!OptChiPos) { // only parsing
		      if (cky_ptr->direction == LtoR) {
			prob += log(Chi_dpnd_matrix[d_ptr->num][g_ptr->num].prob_LtoR[0]);
			prob += log(Chi_dpnd_matrix[d_ptr->num][g_ptr->num].prob_dis_comma_LtoR[0]);
			if (OptDisplay == OPT_DEBUG) {
			  printf("(dpnd:%d,%d prob:%f dis_comma:%f)%.6f=>", d_ptr->num, g_ptr->num, Chi_dpnd_matrix[d_ptr->num][g_ptr->num].prob_LtoR[0], Chi_dpnd_matrix[d_ptr->num][g_ptr->num].prob_dis_comma_LtoR[0], prob);
			}

			if ((cky_ptr->chicase_score + 1) > -DOUBLE_MIN && (cky_ptr->chicase_score + 1) < DOUBLE_MIN) {
			  /* get probability of case frame */
			  /* get left arguments */
			  if (cky_ptr->left) {
			    ptr_num = cky_ptr->left->b_ptr->num;
			    if (strcmp((sp->bnst_data+ptr_num)->head_ptr->Type, "") != 0) {
			      // merge consequent adv before verb
			      if (strcmp((sp->bnst_data+g_ptr->num)->head_ptr->Type, "verb") != 0 ||
				  left_arg_num <= 0 ||
				  strcmp((sp->bnst_data+left_arg[left_arg_num - 1])->head_ptr->Type, "adv") != 0 ||
				  strcmp((sp->bnst_data+ptr_num)->head_ptr->Type, "adv") != 0) {
				left_arg[left_arg_num] = ptr_num;
				left_arg_num++;
				if (left_arg_num > CHI_ARG_NUM_MAX) {
				  fprintf(stderr, ";;; number of arguments exceeded maximum\n");
				  return DOUBLE_MINUS;
				}
			      }
			    }
			  }

			  tmp_cky_ptr = cky_ptr->right;
			  while (tmp_cky_ptr) {
			    if (tmp_cky_ptr->direction == RtoL) {
			      tmp_cky_ptr = tmp_cky_ptr->left;
			    }
			    else {
			      if (tmp_cky_ptr->left) {
				ptr_num = tmp_cky_ptr->left->b_ptr->num;
				if (strcmp((sp->bnst_data+ptr_num)->head_ptr->Type, "") != 0) {
				  if (strcmp((sp->bnst_data+ptr_num)->head_ptr->Type, "") != 0) {
				    // merge consequent adv before verb
				    if (strcmp((sp->bnst_data+g_ptr->num)->head_ptr->Type, "verb") != 0 ||
					left_arg_num <= 0 ||
					strcmp((sp->bnst_data+left_arg[left_arg_num - 1])->head_ptr->Type, "adv") != 0 ||
					strcmp((sp->bnst_data+ptr_num)->head_ptr->Type, "adv") != 0) {
				      left_arg[left_arg_num] = ptr_num;
				      left_arg_num++;
				      if (left_arg_num > CHI_ARG_NUM_MAX) {
					fprintf(stderr, ";;; number of arguments exceeded maximum\n");
					return DOUBLE_MINUS;
				      }
				    }
				  }
				}
			      }
			      tmp_cky_ptr = tmp_cky_ptr->right;
			    }
			  }
			  /* get right arguments */
			  tmp_cky_ptr = cky_ptr;
			  while (tmp_cky_ptr) {
			    if (tmp_cky_ptr->direction == RtoL) {
			      if (tmp_cky_ptr->right) {
				ptr_num = tmp_cky_ptr->right->b_ptr->num;
				if (strcmp((sp->bnst_data+ptr_num)->head_ptr->Type, "") != 0) {
				  right_arg[right_arg_num] = ptr_num;
				  right_arg_num++;
				  if (right_arg_num > CHI_ARG_NUM_MAX) {
				    fprintf(stderr, ";;; number of arguments exceeded maximum\n");
				    return DOUBLE_MINUS;
				  }
				}
			      }
			      tmp_cky_ptr = tmp_cky_ptr->left;
			    }
			    else {
			      tmp_cky_ptr = tmp_cky_ptr->right;
			    }
			  }
			  // do not calculate case score for null arguments
			  if ((left_arg_num > 0 || right_arg_num > 0)) {
			    cky_ptr->chicase_score = get_case_prob(sp, g_ptr->num, left_arg_num, right_arg_num);
			  }
			  else {
			    cky_ptr->chicase_score = 1.0;
			  }
			}
			if (cky_ptr->chicase_score > DOUBLE_MIN) {
			  prob += log(cky_ptr->chicase_score);
			}
			else {
			    prob += DOUBLE_MINUS;
			}
			if (OptDisplay == OPT_DEBUG) {
			  printf("(dpnd:%d,%d chicase:%.6f)%.6f=>", g_ptr->num, d_ptr->num, cky_ptr->chicase_score, prob);
			}
		      }
		      else if (cky_ptr->direction == RtoL) {
			prob += log(Chi_dpnd_matrix[g_ptr->num][d_ptr->num].prob_RtoL[0]);
			prob += log(Chi_dpnd_matrix[g_ptr->num][d_ptr->num].prob_dis_comma_RtoL[0]);
			if (OptDisplay == OPT_DEBUG) {
			  printf("(dpnd:%d,%d prob:%f dis_comma:%f)%.6f=>", g_ptr->num, d_ptr->num, Chi_dpnd_matrix[g_ptr->num][d_ptr->num].prob_RtoL[0], Chi_dpnd_matrix[g_ptr->num][d_ptr->num].prob_dis_comma_RtoL[0], prob);
			}

			if ((cky_ptr->chicase_score + 1) > -DOUBLE_MIN && (cky_ptr->chicase_score + 1) < DOUBLE_MIN) {
			  /* get probability of case frame */
			  /* get right arguments */
			  if (cky_ptr->right) {
			    ptr_num = cky_ptr->right->b_ptr->num;
			    if (strcmp((sp->bnst_data+ptr_num)->head_ptr->Type, "") != 0) {
			      right_arg[right_arg_num] = ptr_num;
			      right_arg_num++;
			      if (right_arg_num > CHI_ARG_NUM_MAX) {
				fprintf(stderr, ";;; number of arguments exceeded maximum\n");
				return DOUBLE_MINUS;
			      }
			    }
			  }

			  tmp_cky_ptr = cky_ptr->left;
			  while (tmp_cky_ptr) {
			    if (tmp_cky_ptr->direction == LtoR) {
			      tmp_cky_ptr = tmp_cky_ptr->right;
			    }
			    else {
			      if (tmp_cky_ptr->right) {
				ptr_num = tmp_cky_ptr->right->b_ptr->num;
				if (strcmp((sp->bnst_data+ptr_num)->head_ptr->Type, "") != 0) {
				  right_arg[right_arg_num] = ptr_num;
				  right_arg_num++;
				  if (right_arg_num > CHI_ARG_NUM_MAX) {
				    fprintf(stderr, ";;; number of arguments exceeded maximum\n");
				    return DOUBLE_MINUS;
				  }
				}
			      }
			      tmp_cky_ptr = tmp_cky_ptr->left;
			    }
			  }

			  /* get left arguments */
			  tmp_cky_ptr = cky_ptr->left;
			  while (tmp_cky_ptr) {
			    if (tmp_cky_ptr->direction == LtoR) {
			      if (tmp_cky_ptr->left) {
				ptr_num = tmp_cky_ptr->left->b_ptr->num;
				if (strcmp((sp->bnst_data+ptr_num)->head_ptr->Type, "") != 0) {
				  // merge consequent adv before verb
				  if (strcmp((sp->bnst_data+g_ptr->num)->head_ptr->Type, "verb") != 0 ||
				      left_arg_num <= 0 ||
				      strcmp((sp->bnst_data+left_arg[left_arg_num - 1])->head_ptr->Type, "adv") != 0 ||
				      strcmp((sp->bnst_data+ptr_num)->head_ptr->Type, "adv") != 0) {
				    left_arg[left_arg_num] = ptr_num;
				    left_arg_num++;
				    if (left_arg_num > CHI_ARG_NUM_MAX) {
				      fprintf(stderr, ";;; number of arguments exceeded maximum\n");
				      return DOUBLE_MINUS;
				    }
				  }
				}
			      }
			      tmp_cky_ptr = tmp_cky_ptr->right;
			    }
			    else {
			      tmp_cky_ptr = tmp_cky_ptr->left;
			    }
			  }
			  // do not calculate case score for null arguments
			  if ((left_arg_num > 0 || right_arg_num > 0)) {
			    cky_ptr->chicase_score = get_case_prob(sp, g_ptr->num, left_arg_num, right_arg_num);
			  }
			  else {
			    cky_ptr->chicase_score = 1.0;
			  }
			}
			if (cky_ptr->chicase_score > DOUBLE_MIN) {
			  prob += log(cky_ptr->chicase_score);
			}
			else {
			    prob += DOUBLE_MINUS;
			}
			if (OptDisplay == OPT_DEBUG) {
			  printf("(dpnd:%d,%d chicase:%.6f)%.6f=>", g_ptr->num, d_ptr->num, cky_ptr->chicase_score, prob);
			}
		      }

		      if (cky_ptr->i == 0 && cky_ptr->j == sp->Bnst_num - 1) {
			prob += log(Chi_root_prob_matrix[g_ptr->num].prob[0]);
			if (OptDisplay == OPT_DEBUG) {
			  printf("(root:%.16f)%.16f=>", Chi_root_prob_matrix[g_ptr->num].prob[0], prob);
			}
		      }

		      one_score += prob;
		    }
		    else { // parsing with pos-tagging
		      if (cky_ptr->direction == LtoR) {
			prob += log(Chi_dpnd_matrix[d_ptr->num][g_ptr->num].prob_LtoR[cky_ptr->index]);
			prob += log(Chi_dpnd_matrix[d_ptr->num][g_ptr->num].prob_dis_comma_LtoR[cky_ptr->index]);
			prob += log(Chi_pos_matrix[d_ptr->num].prob_pos_index[cky_ptr->left_pos_index]);
			prob += log(Chi_pos_matrix[g_ptr->num].prob_pos_index[cky_ptr->right_pos_index]);
			if (OptDisplay == OPT_DEBUG) {
			  printf("(dpnd:%d,%d (%s,%s) prob:%f dis_comma:%f)%.6f=>", d_ptr->num, g_ptr->num, Chi_word_pos[cky_ptr->left_pos_index], Chi_word_pos[cky_ptr->right_pos_index], Chi_dpnd_matrix[d_ptr->num][g_ptr->num].prob_LtoR[cky_ptr->index], Chi_dpnd_matrix[d_ptr->num][g_ptr->num].prob_dis_comma_LtoR[cky_ptr->index], prob);
			}

			if ((cky_ptr->chicase_score + 1) > -DOUBLE_MIN && (cky_ptr->chicase_score + 1) < DOUBLE_MIN) {
			  /* get probability of case frame */
			  /* get left arguments */
			  if (cky_ptr->left) {
			    ptr_num = cky_ptr->left_pos_index;
			    if (strcmp(Chi_word_type[cky_ptr->left_pos_index], "") != 0) {
			      // merge consequent adv before verb
			      if (strcmp(Chi_word_type[cky_ptr->left_pos_index], "verb") != 0 ||
				  left_arg_num <= 0 ||
				  strcmp(Chi_word_type[pre_pos_index], "adv") != 0 ||
				  strcmp(Chi_word_type[cky_ptr->left_pos_index], "adv") != 0) {
				left_arg[left_arg_num] = ptr_num;
				pre_pos_index = cky_ptr->left_pos_index;
				left_arg_num++;
				if (left_arg_num > CHI_ARG_NUM_MAX) {
				  fprintf(stderr, ";;; number of arguments exceeded maximum\n");
				  return DOUBLE_MINUS;
				}
			      }
			    }
			  }

			  tmp_cky_ptr = cky_ptr->right;
			  while (tmp_cky_ptr) {
			    if (tmp_cky_ptr->direction == RtoL) {
			      tmp_cky_ptr = tmp_cky_ptr->left;
			    }
			    else {
			      if (tmp_cky_ptr->left) {
				ptr_num = tmp_cky_ptr->left_pos_index;
				if (strcmp(Chi_word_type[cky_ptr->left_pos_index], "") != 0) {
				  if (strcmp(Chi_word_type[cky_ptr->left_pos_index], "") != 0) {
				    // merge consequent adv before verb
				    if (strcmp(Chi_word_type[cky_ptr->left_pos_index], "verb") != 0 ||
					left_arg_num <= 0 ||
					strcmp(Chi_word_type[pre_pos_index], "adv") != 0 ||
					strcmp(Chi_word_type[cky_ptr->left_pos_index], "adv") != 0) {
				      left_arg[left_arg_num] = ptr_num;
				      pre_pos_index = cky_ptr->left_pos_index;
				      left_arg_num++;
				      if (left_arg_num > CHI_ARG_NUM_MAX) {
					fprintf(stderr, ";;; number of arguments exceeded maximum\n");
					return DOUBLE_MINUS;
				      }
				    }
				  }
				}
			      }
			      tmp_cky_ptr = tmp_cky_ptr->right;
			    }
			  }
			  /* get right arguments */
			  tmp_cky_ptr = cky_ptr;
			  while (tmp_cky_ptr) {
			    if (tmp_cky_ptr->direction == RtoL) {
			      if (tmp_cky_ptr->right) {
				ptr_num = tmp_cky_ptr->right_pos_index;
				if (strcmp(Chi_word_type[tmp_cky_ptr->right_pos_index], "") != 0) {
				  right_arg[right_arg_num] = ptr_num;
				  right_arg_num++;
				  if (right_arg_num > CHI_ARG_NUM_MAX) {
				    fprintf(stderr, ";;; number of arguments exceeded maximum\n");
				    return DOUBLE_MINUS;
				  }
				}
			      }
			      tmp_cky_ptr = tmp_cky_ptr->left;
			    }
			    else {
			      tmp_cky_ptr = tmp_cky_ptr->right;
			    }
			  }
			  // do not calculate case score for null arguments
			  if ((left_arg_num > 0 || right_arg_num > 0)) {
			    cky_ptr->chicase_score = get_case_prob_wpos(sp, g_ptr->num, left_arg_num, right_arg_num, cky_ptr->right_pos_index);
			  }
			  else {
			    cky_ptr->chicase_score = 1.0;
			  }
			}
			if (cky_ptr->chicase_score > DOUBLE_MIN) {
			  prob += log(cky_ptr->chicase_score);
			}
			else {
			  prob += DOUBLE_MINUS;
			}

			prob += log(Chi_pos_matrix[g_ptr->num].prob_pos_index[cky_ptr->right_pos_index]);

			if (OptDisplay == OPT_DEBUG) {
			  printf("(dpnd:%d,%d chicase:%.6f)%.6f=>", g_ptr->num, d_ptr->num, cky_ptr->chicase_score, prob);
			}

			if (cky_ptr->i == 0 && cky_ptr->j == sp->Bnst_num - 1) {
			  prob += log(Chi_root_prob_matrix[g_ptr->num].prob[cky_ptr->index]);
			  prob += log(Chi_pos_matrix[g_ptr->num].prob_pos_index[cky_ptr->right_pos_index]);
			  if (OptDisplay == OPT_DEBUG) {
			    printf("(root:%.16f)%.16f=>", Chi_root_prob_matrix[g_ptr->num].prob[cky_ptr->index], prob);
			  }
			}
		      }
		      else if (cky_ptr->direction == RtoL) {
			prob += log(Chi_dpnd_matrix[g_ptr->num][d_ptr->num].prob_RtoL[cky_ptr->index]);
			prob += log(Chi_dpnd_matrix[g_ptr->num][d_ptr->num].prob_dis_comma_RtoL[cky_ptr->index]);
			prob += log(Chi_pos_matrix[d_ptr->num].prob_pos_index[cky_ptr->right_pos_index]);
			prob += log(Chi_pos_matrix[g_ptr->num].prob_pos_index[cky_ptr->left_pos_index]);
			if (OptDisplay == OPT_DEBUG) {
			  printf("(dpnd:%d,%d (%s,%s) prob:%f dis_comma:%f)%.6f=>", g_ptr->num, d_ptr->num, Chi_word_pos[cky_ptr->left_pos_index], Chi_word_pos[cky_ptr->right_pos_index], Chi_dpnd_matrix[g_ptr->num][d_ptr->num].prob_RtoL[cky_ptr->index], Chi_dpnd_matrix[g_ptr->num][d_ptr->num].prob_dis_comma_RtoL[cky_ptr->index], prob);
			}

			if ((cky_ptr->chicase_score + 1) > -DOUBLE_MIN && (cky_ptr->chicase_score + 1) < DOUBLE_MIN) {
			  /* get probability of case frame */
			  /* get right arguments */
			  if (cky_ptr->right) {
			    ptr_num = cky_ptr->right_pos_index;
			    if (strcmp(Chi_word_type[cky_ptr->right_pos_index], "") != 0) {
			      right_arg[right_arg_num] = ptr_num;
			      right_arg_num++;
			      if (right_arg_num > CHI_ARG_NUM_MAX) {
				fprintf(stderr, ";;; number of arguments exceeded maximum\n");
				return DOUBLE_MINUS;
			      }
			    }
			  }

			  tmp_cky_ptr = cky_ptr->left;
			  while (tmp_cky_ptr) {
			    if (tmp_cky_ptr->direction == LtoR) {
			      tmp_cky_ptr = tmp_cky_ptr->right;
			    }
			    else {
			      if (tmp_cky_ptr->right) {
				ptr_num = tmp_cky_ptr->right_pos_index;
				if (strcmp(Chi_word_type[tmp_cky_ptr->left_pos_index], "") != 0) {
				  right_arg[right_arg_num] = ptr_num;
				  right_arg_num++;
				  if (right_arg_num > CHI_ARG_NUM_MAX) {
				    fprintf(stderr, ";;; number of arguments exceeded maximum\n");
				    return DOUBLE_MINUS;
				  }
				}
			      }
			      tmp_cky_ptr = tmp_cky_ptr->left;
			    }
			  }

			  /* get left arguments */
			  tmp_cky_ptr = cky_ptr->left;
			  while (tmp_cky_ptr) {
			    if (tmp_cky_ptr->direction == LtoR) {
			      if (tmp_cky_ptr->left) {
				ptr_num = tmp_cky_ptr->left_pos_index;
				if (strcmp(Chi_word_type[tmp_cky_ptr->left_pos_index], "") != 0) {
				  // merge consequent adv before verb
				  if (strcmp(Chi_word_type[cky_ptr->right_pos_index], "verb") != 0 ||
				      left_arg_num <= 0 ||
				      strcmp(Chi_word_type[pre_pos_index], "adv") != 0 ||
				      strcmp(Chi_word_type[tmp_cky_ptr->left_pos_index], "adv") != 0) {
				    left_arg[left_arg_num] = ptr_num;
				    pre_pos_index = tmp_cky_ptr->left_pos_index;
				    left_arg_num++;
				    if (left_arg_num > CHI_ARG_NUM_MAX) {
				      fprintf(stderr, ";;; number of arguments exceeded maximum\n");
				      return DOUBLE_MINUS;
				    }
				  }
				}
			      }
			      tmp_cky_ptr = tmp_cky_ptr->right;
			    }
			    else {
			      tmp_cky_ptr = tmp_cky_ptr->left;
			    }
			  }
			  // do not calculate case score for null arguments
			  if ((left_arg_num > 0 || right_arg_num > 0)) {
			    cky_ptr->chicase_score = get_case_prob_wpos(sp, g_ptr->num, left_arg_num, right_arg_num, cky_ptr->left_pos_index);
			  }
			  else {
			    cky_ptr->chicase_score = 1.0;
			  }
			}

			if (cky_ptr->chicase_score > DOUBLE_MIN) {
			  prob += log(cky_ptr->chicase_score);
			}
			else {
			  prob += DOUBLE_MINUS;
			}

			prob += log(Chi_pos_matrix[g_ptr->num].prob_pos_index[cky_ptr->left_pos_index]);

			if (OptDisplay == OPT_DEBUG) {
			  printf("(dpnd:%d,%d chicase:%.6f)%.6f=>", g_ptr->num, d_ptr->num, cky_ptr->chicase_score, prob);
			}

			if (cky_ptr->i == 0 && cky_ptr->j == sp->Bnst_num - 1) {
			  prob += log(Chi_root_prob_matrix[g_ptr->num].prob[cky_ptr->index]);
			  prob += log(Chi_pos_matrix[g_ptr->num].prob_pos_index[cky_ptr->left_pos_index]);
			  if (OptDisplay == OPT_DEBUG) {
			    printf("(root:%.16f)%.16f=>", Chi_root_prob_matrix[g_ptr->num].prob[cky_ptr->index], prob);
			  }
			}
		      }

		      one_score += prob;
		    }
		}
		else {
		    if (cky_ptr->direction == LtoR) {
			one_score += weight_dpnd * Chi_dpnd_matrix[d_ptr->num][g_ptr->num].prob_LtoR[cky_ptr->index];

			if (OptDisplay == OPT_DEBUG) {
			    printf("(dpnd:%d,%d %f)%.6f=>", d_ptr->num, g_ptr->num, Chi_dpnd_matrix[d_ptr->num][g_ptr->num].prob_LtoR[cky_ptr->index], one_score);
			}

			if (Chi_dpnd_matrix[d_ptr->num][g_ptr->num].prob_pos_LtoR >= pos_prob_thre_high && 
			    Chi_dpnd_matrix[d_ptr->num][g_ptr->num].occur_pos >= pos_occur_thre_high) {
			    one_score += weight_pos * Chi_dpnd_matrix[d_ptr->num][g_ptr->num].prob_pos_LtoR;
			}
			else if (Chi_dpnd_matrix[d_ptr->num][g_ptr->num].occur_pos <= pos_occur_thre_low) {
			    one_score -= weight_pos * Chi_dpnd_matrix[d_ptr->num][g_ptr->num].prob_pos_LtoR;
			}
			else if (Chi_dpnd_matrix[d_ptr->num][g_ptr->num].prob_pos_LtoR <= pos_prob_thre_low) {
			    one_score -= weight_pos * (1 - Chi_dpnd_matrix[d_ptr->num][g_ptr->num].prob_pos_LtoR);
			}

			if (OptDisplay == OPT_DEBUG) {
			    printf("(pos:%f)%.6f=>", Chi_dpnd_matrix[d_ptr->num][g_ptr->num].prob_pos_LtoR, one_score);
			}

			/* punish from comma */
			one_score -= weight_comma * (1.0 * comma) / (comma + 1);

			if (OptDisplay == OPT_DEBUG) {
			    printf("(comma:%d)%.6f=>", comma, one_score);
			}

			/* punish from root */
			if (d_ptr->num == Chi_root) {
			    one_score -= weight_root;
			}

			if (OptDisplay == OPT_DEBUG) {
			    printf("(root)%.6f=>", one_score);
			}

			/* add bonus from gigaword pa */
			if (Chi_pa_matrix[d_ptr->num][g_ptr->num] >= chi_pa_thre) {
			    one_score += weight_pa * Chi_pa_matrix[d_ptr->num][g_ptr->num];
			}

			if (OptDisplay == OPT_DEBUG) {
			    printf("(pa:%f)%.6f=>", Chi_pa_matrix[d_ptr->num][g_ptr->num], one_score);
			}
		    }
		    else if (cky_ptr->direction == RtoL) {
			one_score += weight_dpnd * Chi_dpnd_matrix[g_ptr->num][d_ptr->num].prob_RtoL[cky_ptr->index];

			if (OptDisplay == OPT_DEBUG) {
			    printf("(dpnd:%d,%d %f)%.6f=>", d_ptr->num, g_ptr->num, Chi_dpnd_matrix[g_ptr->num][d_ptr->num].prob_RtoL[cky_ptr->index], one_score);
			}

			if (Chi_dpnd_matrix[g_ptr->num][d_ptr->num].prob_pos_RtoL >= pos_prob_thre_high &&
			    Chi_dpnd_matrix[g_ptr->num][d_ptr->num].occur_pos >= pos_occur_thre_high) {
			    one_score += weight_pos * Chi_dpnd_matrix[g_ptr->num][d_ptr->num].prob_pos_RtoL;
			}
			else if (Chi_dpnd_matrix[g_ptr->num][d_ptr->num].occur_pos <= pos_occur_thre_low) {
			    one_score -= weight_pos * Chi_dpnd_matrix[g_ptr->num][d_ptr->num].prob_pos_RtoL;
			}
			else if (Chi_dpnd_matrix[g_ptr->num][d_ptr->num].prob_pos_RtoL <= pos_prob_thre_low) {
			    one_score -= weight_pos * (1 - Chi_dpnd_matrix[g_ptr->num][d_ptr->num].prob_pos_RtoL);
			}

			if (OptDisplay == OPT_DEBUG) {
			    printf("(pos: %f)%.6f=>", Chi_dpnd_matrix[g_ptr->num][d_ptr->num].prob_pos_RtoL, one_score);
			}

			/* punish from comma */
			one_score -= weight_comma * (1.0 * comma) / (comma + 1);

			if (OptDisplay == OPT_DEBUG) {
			    printf("(comma: %d)%.6f=>", comma, one_score);
			}

			/* punish from root */
			if (d_ptr->num == Chi_root) {
			    one_score -= weight_root;
			}

			if (OptDisplay == OPT_DEBUG) {
			    printf("(root)%.6f=>", one_score);
			}

			/* add bonus from gigaword pa */
			if (Chi_pa_matrix[d_ptr->num][g_ptr->num] >= chi_pa_thre) {
			    one_score += weight_pa * Chi_pa_matrix[d_ptr->num][g_ptr->num];
			}

			if (OptDisplay == OPT_DEBUG) {
			    printf("(pa:%f)%.6f=>", Chi_pa_matrix[d_ptr->num][g_ptr->num], one_score);
			}
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
	if (Language == CHINESE) {
	    printf("%.6f\n", one_score);
	}
	else {
	    printf("%.3f\n", one_score);
	}
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
    TAG_DATA *t_ptr, *dt_ptr;
    CF_PRED_MGR *cpm_ptr, *pre_cpm_ptr;
    int i, pred_p = 0, child_num = 0, wo_ni_overwritten_flag = 0;
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
	cky_ptr->cpm_ptr->pred_b_ptr = t_ptr;
	set_data_cf_type(cky_ptr->cpm_ptr); /* set predicate type */
	if (cky_ptr->cpm_ptr->cf.type == CF_PRED && /* currently, restrict to predicates */
	    !(cky_ptr->i == cky_ptr->j && check_feature(g_ptr->f, "ID:（〜を）〜に"))) {
	    pred_p = 1;
	    cpm_ptr = cky_ptr->cpm_ptr;
	    cpm_ptr->score = -1;
	    cpm_ptr->result_num = 0;
	    cpm_ptr->tie_num = 0;
	    cpm_ptr->cmm[0].cf_ptr = NULL;
	    cpm_ptr->decided = CF_UNDECIDED;

	    cpm_ptr->cf.pred_b_ptr = t_ptr;
	    t_ptr->cpm_ptr = cpm_ptr;
	    cpm_ptr->cf.element_num = 0;
	}
	else {
	    cky_ptr->cpm_ptr->pred_b_ptr = NULL;
	    cky_ptr->cpm_ptr->cmm[0].cf_ptr = NULL;
	}
    }
    else {
	cky_ptr->cpm_ptr->pred_b_ptr = NULL;
	cky_ptr->cpm_ptr->cmm[0].cf_ptr = NULL;
    }

    /* check each child */
    while (cky_ptr) {
	if (cky_ptr->left && cky_ptr->para_flag == 0) {
	    d_ptr = cky_ptr->left->b_ptr;
	    dt_ptr = d_ptr->tag_ptr + d_ptr->tag_num - 1;
	    flag = 0;

	    /* left_ptrが「（〜を）〜に」で、1つの句からなるときは、係:ニ格 に変更 */
	    if (cky_ptr->left->i == cky_ptr->left->j && 
		check_feature(d_ptr->f, "ID:（〜を）〜に")) {
		assign_cfeature(&(d_ptr->f), "係:ニ格", FALSE);
		assign_cfeature(&(dt_ptr->f), "係:ニ格", FALSE);
		assign_cfeature(&(dt_ptr->f), "Ｔ解析格-ニ", FALSE);
		delete_cfeature(&(dt_ptr->f), "Ｔ用言同文節");
		wo_ni_overwritten_flag = 1;
	    }
	    else {
		wo_ni_overwritten_flag = 0;
	    }

	    /* relax penalty */
	    if (cky_ptr->dpnd_type == 'R') {
		one_score += -1000;
	    }

	    /* coordination */
	    if (OptParaFix == 0) {
		if (d_ptr->para_num != -1 && (para_key = check_feature(d_ptr->f, "並キ"))) {
		    make_work_mgr_dpnd_check_for_noun(sp, d_ptr);
		    if (cky_ptr->dpnd_type == 'P') {
			one_score += get_para_exist_probability(para_key, cky_ptr->para_score, TRUE, d_ptr->tag_ptr + d_ptr->tag_num - 1, t_ptr);
			if ((OptParaNoFixFlag & OPT_PARA_MULTIPLY_ALL_EX) || check_feature(d_ptr->f, "係:連用")) {
			    one_score += get_para_ex_probability(para_key, cky_ptr->para_score, d_ptr->tag_ptr + d_ptr->tag_num - 1, t_ptr);
			}
			flag++;
		    }
		    else {
			one_score += get_para_exist_probability(para_key, sp->para_data[d_ptr->para_num].max_score, FALSE, d_ptr->tag_ptr + d_ptr->tag_num - 1, t_ptr);
		    }
		}
	    }

	    /* case component */
	    if (cky_ptr->dpnd_type != 'P' && pred_p) {
		make_work_mgr_dpnd_check(sp, cky_ptr, d_ptr);
		if (make_data_cframe_child(sp, cpm_ptr, d_ptr->tag_ptr + d_ptr->tag_num - 1, child_num, t_ptr->num == d_ptr->num + 1 ? TRUE : FALSE)) {
		    add_coordinated_phrases(cky_ptr->left, &(cpm_ptr->elem_b_ptr[cpm_ptr->cf.element_num - 1]->next));
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
		add_coordinated_phrases(cky_ptr->right, &(pre_cpm_ptr->elem_b_ptr[pre_cpm_ptr->cf.element_num - 1]->next));

		orig_score = pre_cpm_ptr->score;
		one_score -= orig_score;
		one_score += find_best_cf(sp, pre_cpm_ptr, get_closest_case_component(sp, pre_cpm_ptr), 1);
		pre_cpm_ptr->score = orig_score;
		pre_cpm_ptr->cf.element_num--;
		flag++;
	    }

	    if (OptParaFix == 0 && flag == 0 && /* 名詞格フレームへ */
		check_feature(g_ptr->f, "体言") && /* 複合辞などを飛ばす */
		check_feature(d_ptr->f, "体言")) {
		(d_ptr->tag_ptr + d_ptr->tag_num - 1)->next = NULL; /* 並列要素格納用(係り側) */
		t_ptr->next = NULL; /* 並列要素格納用(受け側) */
		add_coordinated_phrases(cky_ptr->left, &((d_ptr->tag_ptr + d_ptr->tag_num - 1)->next));
		add_coordinated_phrases(cky_ptr->right, &(t_ptr->next));
		make_work_mgr_dpnd_check(sp, cky_ptr, d_ptr);
		one_score += get_noun_co_ex_probability(d_ptr->tag_ptr + d_ptr->tag_num - 1, t_ptr);
		noun_modifying_num++;
	    }

	    /* 係:ニ格に変更したものを元にもどす */
	    if (wo_ni_overwritten_flag) {
		assign_cfeature(&(d_ptr->f), "係:連用", FALSE);
		assign_cfeature(&(dt_ptr->f), "係:連用", FALSE);
		assign_cfeature(&(dt_ptr->f), "Ｔ用言同文節", FALSE);
		delete_cfeature(&(dt_ptr->f), "Ｔ解析格-ニ");
	    }
	}
	cky_ptr = cky_ptr->right;
    }

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
		if (cky_ptr->dpnd_type != 'P' && 
		    !(cky_ptr->left->i == cky_ptr->left->j && /* 「〜に」が格要素なので除外 */
		      check_feature(d_ptr->f, "ID:（〜を）〜に"))) {
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

    /* 名詞修飾個数生成 */
    if (OptParaFix == 0 && !pred_p || check_feature(t_ptr->f, "用言:判")) {
	one_score += get_noun_co_num_probability(t_ptr, noun_modifying_num);
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


void restrict_parenthetic_coordination(SENTENCE_DATA* sp) {
    int i, j, count;

    for (i = 0; i < sp->Bnst_num - 1; i++) {
	if (check_feature((sp->bnst_data + i)->f, "係:括弧並列")) {
	    count = 0;
	    for (j = i + 1; j < sp->Bnst_num; j++) {
		if (Dpnd_matrix[i][j]) {
		    if (count > 0) {
			/* only permit the first head */
			Dpnd_matrix[i][j] = 0;
		    }
		    count++;
		}
	    }
	}
    }
}

void restrict_end_prefer_dependency(SENTENCE_DATA* sp) {
    int i, j, count;

    for (i = 0; i < sp->Bnst_num - 1; i++) {
	if ((sp->bnst_data + i)->dpnd_rule->preference == -1) {
	    count = 0;
	    for (j = sp->Bnst_num - 1; j > i; j--) {
		if (Dpnd_matrix[i][j]) {
		    if (count > 0) {
			/* only permit the last head */
			Dpnd_matrix[i][j] = 0;
		    }
		    count++;
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
    cky_ptr->chicase_score = -1;
    cky_ptr->chicase_lex_score = -1;
    cky_ptr->score = 0;

    if (Language == CHINESE) {
      if (left_ptr != NULL && right_ptr != NULL) {
	cky_ptr->left_pos_index = Chi_dpnd_matrix[left_ptr->b_ptr->num][right_ptr->b_ptr->num].left_pos_index[index];
	cky_ptr->right_pos_index = Chi_dpnd_matrix[left_ptr->b_ptr->num][right_ptr->b_ptr->num].right_pos_index[index];
      }
      else {
	if (cky_ptr->i == cky_ptr->j && cky_ptr->i != -1) {
	  cky_ptr->left_pos_index = Chi_dpnd_matrix[cky_ptr->i][cky_ptr->j].left_pos_index[0];
	  cky_ptr->right_pos_index = Chi_dpnd_matrix[cky_ptr->i][cky_ptr->j].right_pos_index[0];
	}
	else {
	  cky_ptr->left_pos_index = -1;
	  cky_ptr->right_pos_index = -1;
	}
      }
    }
}

CKY *new_cky_data(int *cky_table_num) {
    CKY *cky_ptr;

    cky_ptr = &(cky_table[*cky_table_num]);
    if (OptAnalysis == OPT_CASE && *cky_table_num > cpm_allocated_cky_num) {
	cky_ptr->cpm_ptr = (CF_PRED_MGR *)malloc_data(sizeof(CF_PRED_MGR), "new_cky_data");
	init_case_frame(&(cky_ptr->cpm_ptr->cf));
	cky_ptr->cpm_ptr->cf.type = 0;
	cpm_allocated_cky_num = *cky_table_num;
    }

    (*cky_table_num)++;
    if (*cky_table_num >= CKY_TABLE_MAX) {
	fprintf(stderr, ";;; cky_table_num exceeded maximum\n");
	return NULL;
    }

    return cky_ptr;
}

void copy_cky_data(CKY *dest, CKY *src) {
    int l;

    if (dest == src) {
	return;
    }

    dest->index = src->index;
    dest->i = src->i;
    dest->j = src->j;
    dest->next = src->next;
    dest->left = src->left;
    dest->right = src->right;
    dest->direction = src->direction;
    dest->dpnd_type = src->dpnd_type;
    dest->cp = src->cp;
    dest->direction = src->direction;
    dest->b_ptr = src->b_ptr;
    dest->un_count = src->un_count;
    for (l = 0; l < SCASE_CODE_SIZE; l++) dest->scase_check[l] = src->scase_check[l];
    dest->para_flag = src->para_flag;
    dest->para_score = src->para_score;
    dest->score = src->score;
    dest->cpm_ptr = src->cpm_ptr;
}

int after_cky(SENTENCE_DATA *sp, TOTAL_MGR *Best_mgr, CKY *cky_ptr) {
    int i, j;
    CKY *tmp_cky_ptr;
    CKY *node_stack[BNST_MAX];
    int tail_index;

    /* count the number of predicates */
    Best_mgr->pred_num = 0;
    for (i = 0; i < sp->Tag_num; i++) {
	if ((sp->tag_data + i)->cf_num > 0 && 
	    (sp->tag_data + i)->cpm_ptr && (sp->tag_data + i)->cpm_ptr->cf.type == CF_PRED && 
	    (((sp->tag_data + i)->inum == 0 && /* the last basic phrase in a bunsetsu */
	      !check_feature((sp->tag_data + i)->b_ptr->f, "タグ単位受:-1")) || 
	     ((sp->tag_data + i)->inum == 1 && 
	      check_feature((sp->tag_data + i)->b_ptr->f, "タグ単位受:-1")))) { 
	    (sp->tag_data + i)->pred_num = Best_mgr->pred_num;
	    Best_mgr->pred_num++;
	}
    }

    // assign the best pos-tag for Chinese words
    if (Language == CHINESE && OptChiPos) {
      tail_index = 0;
      node_stack[tail_index] = cky_ptr;
      while (tail_index >= 0) {
	tmp_cky_ptr = node_stack[tail_index];
	tail_index--;
	if (tmp_cky_ptr) {
	    if (tmp_cky_ptr->direction == LtoR) {
	      strcpy((sp->bnst_data + tmp_cky_ptr->b_ptr->num)->head_ptr->Pos, Chi_word_pos[tmp_cky_ptr->right_pos_index]);
	    }
	    else if (tmp_cky_ptr->direction == RtoL) {
	      strcpy((sp->bnst_data + tmp_cky_ptr->b_ptr->num)->head_ptr->Pos, Chi_word_pos[tmp_cky_ptr->left_pos_index]);
	    }
	    if (tmp_cky_ptr->right) {
	      tail_index++;
	      node_stack[tail_index] = tmp_cky_ptr->right;
	    }
	    if (tmp_cky_ptr->left) {
	      tail_index++;
	      node_stack[tail_index] = tmp_cky_ptr->left;
	    }
	}
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
	sp->score = Best_mgr->score;
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
		if (Best_mgr->cpm[i].pred_b_ptr == NULL) { /* 述語ではないと判断したものはスキップ */
		    continue;
		}
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

	    /* disambiguation by case analysis */
	    if (OptAnalysis == OPT_CASE) {
		for (i = 0; i < Best_mgr->pred_num; i++) {
		    if (Best_mgr->cpm[i].pred_b_ptr == NULL) { /* 述語ではないと判断したものはスキップ */
			continue;
		    }
		    if (Best_mgr->cpm[i].result_num != 0 && 
			Best_mgr->cpm[i].cmm[0].cf_ptr->cf_address != -1 && 
			Best_mgr->cpm[i].cmm[0].score != CASE_MATCH_FAILURE_PROB) {
			/* 格解析の結果を用いて形態素曖昧性を解消 */
			verb_lexical_disambiguation_by_case_analysis(&(sp->Best_mgr->cpm[i]));
			noun_lexical_disambiguation_by_case_analysis(&(sp->Best_mgr->cpm[i]));
		    }
		}
	    }

	    /* print for debug or nbest */
	    if (OptNbest == TRUE) {
		/* 構造決定後のルール適用 */
		assign_general_feature(sp->bnst_data, sp->Bnst_num, AfterDpndBnstRuleType, FALSE, TRUE);
		assign_general_feature(sp->tag_data, sp->Tag_num, AfterDpndTagRuleType, FALSE, TRUE);

		if (OptAnalysis == OPT_CASE) { /* preserve case analysis result for n-best */
		    record_all_case_analisys(sp, TRUE);
		}
		print_result(sp, 0);

		if (OptAnalysis == OPT_CASE && OptDisplay == OPT_DEBUG) { /* case analysis results */
		    for (i = 0; i < Best_mgr->pred_num; i++) {
			if (Best_mgr->cpm[i].pred_b_ptr == NULL) { /* 述語ではないと判断したものはスキップ */
			    continue;
			}
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

void sort_cky_ptrs(CKY **orig_cky_ptr_ptr, int beam) {
    CKY *cky_ptr = *orig_cky_ptr_ptr, **start_cky_ptr_ptr = orig_cky_ptr_ptr, *pre_ptr, *best_ptr, *best_pre_ptr;
    double best_score;
    int i;

    for (i = 0; i < beam && cky_ptr; i++) {
	best_score = -INT_MAX;
	best_pre_ptr = NULL;
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

	if (best_pre_ptr) { /* best_ptr is not at the beginning */
	    best_pre_ptr->next = best_ptr->next;
	    best_ptr->next = *start_cky_ptr_ptr;
	    *start_cky_ptr_ptr = best_ptr;
	}

	start_cky_ptr_ptr = &(best_ptr->next);
	cky_ptr = best_ptr->next;
    }

//  best_ptr->next = NULL; /* do not consider more candidates than beam */
}

int cky (SENTENCE_DATA *sp, TOTAL_MGR *Best_mgr) {
    int i, j, k, l, m, sort_flag, sen_len, cky_table_num, pre_cky_table_num, dep_check[BNST_MAX];
    double best_score, para_score;
    char dpnd_type;
    CKY *cky_ptr, *left_ptr, *right_ptr, *best_ptr, *pre_ptr, *best_pre_ptr, *start_ptr, *sort_pre_ptr, *tmp_ptr;
    CKY **next_pp, **next_pp_for_ij;

    cky_table_num = 0;

    /* initialize */
    for (i = 0; i < sp->Bnst_num; i++) {
	dep_check[i] = -1;
	Best_mgr->dpnd.head[i] = -1;
	Best_mgr->dpnd.type[i] = 'D';
    }

    /* set barrier for parenthetic coordinations */
    restrict_parenthetic_coordination(sp);

    /* restrict the possible heads of end-prefer dependents */
    restrict_end_prefer_dependency(sp);

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
		pre_cky_table_num = cky_table_num;

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
					if (Chi_dpnd_matrix[left_ptr->b_ptr->num][right_ptr->b_ptr->num].direction[l] == 'B') {
					    /* check R first */
					    if (check_chi_dpnd_possibility(i, j, k, left_ptr, right_ptr, sp, 'R', l)) {
					      if ((i == 0 && j == sp->Bnst_num - 1 && Chi_root_prob_matrix[right_ptr->b_ptr->num].prob[0] <= DOUBLE_MIN)) {
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

						set_cky(sp, cky_ptr, left_ptr, right_ptr, i, j, k, 'R', LtoR, l);

						next_pp = &(cky_ptr->next);
					
						if (OptDisplay == OPT_DEBUG) {
						    printf("   (%d,%d), (%d,%d) b=%d [%s%s%s], %c(para=%.3f), score=", 
							   i, i + k, i + k + 1, j, dep_check[i + k], 
							   left_ptr->b_ptr->head_ptr->Goi, 
							   "->", 
							   right_ptr->b_ptr->head_ptr->Goi, 
							   'R', para_score);
						}

						cky_ptr->para_score = para_score;
						cky_ptr->score = OptAnalysis == OPT_CASE ? 
						    calc_case_probability(sp, cky_ptr, Best_mgr) : calc_score(sp, cky_ptr);

						if (OptParaFix) {
						    if (Mask_matrix[i][i + k] == 'N' && Mask_matrix[i + k + 1][j] == 'N') {
							cky_ptr->score += 50;
							if (OptDisplay == OPT_DEBUG) {
							    printf("=>%.3f\n", cky_ptr->score);
							} 
						    }
						}
						if (!OptParaFix && !OptChiPos) {
						    /* add similarity of coordination */
						    if (cky_ptr->para_score > PARA_THRESHOLD && 
							(Mask_matrix[i][i + k] == 'N' || Mask_matrix[i + k + 1][j] == 'N')) {
							cky_ptr->score += log(cky_ptr->para_score + 1);
						    }
						    else if ((sp->bnst_data + i + k)->para_num != -1 && cky_ptr->right && 
							     Para_matrix[(sp->bnst_data + i + k)->para_num][i][cky_ptr->right->b_ptr->num] > PARA_THRESHOLD &&
							     exist_chi(sp, cky_ptr->right->b_ptr->num + 1, j, "pu") == -1 &&
							     (Mask_matrix[i][i + k] == 'V' || Mask_matrix[i + k + 1][cky_ptr->right->b_ptr->num] == 'V')) {
							cky_ptr->score += log(Para_matrix[(sp->bnst_data + i + k)->para_num][i][cky_ptr->right->b_ptr->num] + 1);
						    }
						    if (OptDisplay == OPT_DEBUG) {
							printf("(para)=>%.3f\n", cky_ptr->score);
						    } 
						}
					    }

					    /* then check L */
					    if (check_chi_dpnd_possibility(i, j, k, left_ptr, right_ptr, sp, 'L', l)) {
					      if ((i == 0 && j == sp->Bnst_num - 1 && Chi_root_prob_matrix[left_ptr->b_ptr->num].prob[0] <= DOUBLE_MIN)) {
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

						set_cky(sp, cky_ptr, left_ptr, right_ptr, i, j, k, 'L', RtoL, l);

						next_pp = &(cky_ptr->next);
					
						if (OptDisplay == OPT_DEBUG) {
						    printf("   (%d,%d), (%d,%d) b=%d [%s%s%s], %c(para=%.3f), score=", 
							   i, i + k, i + k + 1, j, dep_check[i + k], 
							   left_ptr->b_ptr->head_ptr->Goi, 
							   "<-", 
							   right_ptr->b_ptr->head_ptr->Goi, 
							   'L', para_score);
						}

						cky_ptr->para_score = para_score;
						cky_ptr->score = OptAnalysis == OPT_CASE ? 
						    calc_case_probability(sp, cky_ptr, Best_mgr) : calc_score(sp, cky_ptr);

						if (OptParaFix) {
						    if (Mask_matrix[i][i + k] == 'V' && Mask_matrix[i + k + 1][j] == 'V') {
							cky_ptr->score += 50;
							if (OptDisplay == OPT_DEBUG) {
							    printf("=>%.3f\n", cky_ptr->score);
							} 
						    }
						}
						if (!OptParaFix && !OptChiPos) {
						    /* add similarity of coordination */
						    if (cky_ptr->para_score > PARA_THRESHOLD && 
							(Mask_matrix[i][i + k] == 'N' && Mask_matrix[i + k + 1][j] == 'N')) {
							cky_ptr->score += log(cky_ptr->para_score + 1);
						    }
						    else if ((sp->bnst_data + i + k)->para_num != -1 && cky_ptr->right && 
							     Para_matrix[(sp->bnst_data + i + k)->para_num][i][cky_ptr->right->b_ptr->num] > PARA_THRESHOLD &&
							     exist_chi(sp, cky_ptr->right->b_ptr->num + 1, j, "pu") == -1 &&
							     (Mask_matrix[i][i + k] == 'V' && Mask_matrix[i + k + 1][cky_ptr->right->b_ptr->num] == 'V')) {
							cky_ptr->score += log(Para_matrix[(sp->bnst_data + i + k)->para_num][i][cky_ptr->right->b_ptr->num] + 1);
						    }
						    if (OptDisplay == OPT_DEBUG) {
							printf("(para)=>%.3f\n", cky_ptr->score);
						    } 
						}
					    }
					}
					else {
					    if (!(check_chi_dpnd_possibility(i, j, k, left_ptr, right_ptr, sp, Chi_dpnd_matrix[left_ptr->b_ptr->num][right_ptr->b_ptr->num].direction[l])), l) {
						continue;
					    }
					    if ((i == 0 && j == sp->Bnst_num - 1 && 
						 (Chi_dpnd_matrix[left_ptr->b_ptr->num][right_ptr->b_ptr->num].direction[l] == 'R' ? Chi_root_prob_matrix[right_ptr->b_ptr->num].prob[0] : Chi_root_prob_matrix[left_ptr->b_ptr->num].prob[0]) <= DOUBLE_MIN)) {
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

					    if (OptParaFix) {
					      if (Mask_matrix[i][i + k] == 'N' && Mask_matrix[i + k + 1][j] == 'N') {
						set_cky(sp, cky_ptr, left_ptr, right_ptr, i, j, k, 'R', LtoR, l); 
					      }
					      else if (Mask_matrix[i][i + k] == 'V' && Mask_matrix[i + k + 1][j] == 'V') {
						set_cky(sp, cky_ptr, left_ptr, right_ptr, i, j, k, 'L', RtoL, l); 
					      }
					      else {
						set_cky(sp, cky_ptr, left_ptr, right_ptr, i, j, k, 
							Chi_dpnd_matrix[left_ptr->b_ptr->num][right_ptr->b_ptr->num].direction[l], 
							Chi_dpnd_matrix[left_ptr->b_ptr->num][right_ptr->b_ptr->num].direction[l] == 'L' ? RtoL : LtoR, l);
					      }
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

					    if (OptParaFix) {
						if (Mask_matrix[i][i + k] == 'N' && Mask_matrix[i + k + 1][j] == 'N') {
						    cky_ptr->score += 50;
						    if (OptDisplay == OPT_DEBUG) {
							printf("=>%.3f\n", cky_ptr->score);
						    } 
						}
						else if (Mask_matrix[i][i + k] == 'V' && Mask_matrix[i + k + 1][j] == 'V') {
						    cky_ptr->score += 50;
						    if (OptDisplay == OPT_DEBUG) {
							printf("=>%.3f\n", cky_ptr->score);
						    } 
						}
					    }
					    if (!OptParaFix && !OptChiPos) {
						/* add similarity of coordination */
						if (cky_ptr->para_score > PARA_THRESHOLD && 
						    (Mask_matrix[i][i + k] == 'N' || Mask_matrix[i + k + 1][j] == 'N')) {
						    cky_ptr->score += log(cky_ptr->para_score + 1);
						}
						else if ((sp->bnst_data + i + k)->para_num != -1 && cky_ptr->right && 
							 Para_matrix[(sp->bnst_data + i + k)->para_num][i][cky_ptr->right->b_ptr->num] > PARA_THRESHOLD &&
							 exist_chi(sp, cky_ptr->right->b_ptr->num + 1, j, "pu") == -1 &&
							 (Mask_matrix[i][i + k] == 'V' || Mask_matrix[i + k + 1][cky_ptr->right->b_ptr->num] == 'V')) {
						    cky_ptr->score += log(Para_matrix[(sp->bnst_data + i + k)->para_num][i][cky_ptr->right->b_ptr->num] + 1);
						}
						if (OptDisplay == OPT_DEBUG) {
						    printf("(para)=>%.3f\n", cky_ptr->score);
						} 
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
				OptParaFix && 
				!check_feature(right_ptr->b_ptr->f, "用言")) { /* consider only the best one if noun */
				break;
			    }
			    right_ptr = right_ptr->next;
			}

			if (Language != CHINESE && 
			    OptNbest == FALSE && 
			    OptParaFix && /* 並列の曖昧性を許す場合、名詞が並列かどうかで、並列名詞の平均をとる箇所でスコアに変化が起こる */
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

		if (next_pp_for_ij) {
		    /* leave the best candidates within the beam */
		    if (OptBeam) {
			sort_cky_ptrs(&(cky_matrix[i][j]), OptBeam);
			next_pp = &(cky_matrix[i][j]);
			for (l = 0; l < OptBeam && *next_pp; l++) {
			    /* swap_cky_data(&(cky_table[pre_cky_table_num + l]), *next_pp); */
			    /* *next_pp = &(cky_table[pre_cky_table_num + l]); */
			    next_pp = &((*next_pp)->next);
			}
			/* cky_table_num = pre_cky_table_num + l; */
			*next_pp = NULL;
		    }
		    /* move the best one to the beginning of the list for the next step */
		    else {
			sort_cky_ptrs(&(cky_matrix[i][j]), 1);
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
				// if the score of two ptrs are the same, only keep one
				tmp_ptr = cky_matrix[i][j];
				while (tmp_ptr && tmp_ptr != sort_pre_ptr->next && sort_pre_ptr && best_ptr) {
				    if ((tmp_ptr->score - best_ptr->score) < DOUBLE_MIN && (tmp_ptr->score - best_ptr->score) > -DOUBLE_MIN &&
					tmp_ptr->direction == best_ptr->direction &&
					tmp_ptr->b_ptr == best_ptr->b_ptr &&
					tmp_ptr->left->b_ptr == best_ptr->left->b_ptr &&
					tmp_ptr->right->b_ptr == best_ptr->right->b_ptr) {
					sort_pre_ptr->next = best_ptr->next;
					best_ptr = NULL;
					m--;
					break;
				    }
				    tmp_ptr = tmp_ptr->next;
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
		(!strcmp((sp->bnst_data+k)->head_ptr->Goi, ",") ||
		 !strcmp((sp->bnst_data+k)->head_ptr->Goi, "：") ||
		 !strcmp((sp->bnst_data+k)->head_ptr->Goi, ":") ||
		 !strcmp((sp->bnst_data+k)->head_ptr->Goi, "；") ||
		 !strcmp((sp->bnst_data+k)->head_ptr->Goi, "，"))) {
		break;
	    }
	    if (check_feature((sp->bnst_data + k)->f, "NN") ||
		check_feature((sp->bnst_data + k)->f, "NT") ||
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
    else if (!strcmp(type, "CC")) {
	for (k = i; k <= j; k++) {
	    if (check_feature((sp->bnst_data + k)->f, "CC")) {
		return k;
	    }
	}
    }
    else if (!strcmp(type, "pu")) {
	for (k = i; k <= j; k++) {
	    if (check_feature((sp->bnst_data + k)->f, "PU") && 
		(!strcmp((sp->bnst_data+k)->head_ptr->Goi, ",") ||
		 !strcmp((sp->bnst_data+k)->head_ptr->Goi, "：") ||
		 !strcmp((sp->bnst_data+k)->head_ptr->Goi, ":") ||
		 !strcmp((sp->bnst_data+k)->head_ptr->Goi, "；") ||
		 !strcmp((sp->bnst_data+k)->head_ptr->Goi, "，"))) {
		return k;
	    }
	}
    }
    else if (!strcmp(type, "dunhao")) {
	for (k = i; k <= j; k++) {
	    if (check_feature((sp->bnst_data + k)->f, "PU") && 
		!strcmp((sp->bnst_data+k)->head_ptr->Goi, "、")) {
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

/* check if this node has special child, direction = 0 means check in the left side, direction = 1 means check in the right side */
int has_child_chi(SENTENCE_DATA *sp, CKY *cky_ptr, char *pos, int direction) {
    CKY *ptr = cky_ptr;
    if (ptr->direction == LtoR) {
	if (direction == 0) {
	    if (ptr->left && check_feature((sp->bnst_data + ptr->left->b_ptr->num)->f, pos)) {
		return 1;
	    }
	    if (ptr->right) {
		ptr = ptr->right;
		while (ptr) {
		    if (ptr->direction == LtoR) {
			if (ptr->left && check_feature((sp->bnst_data + ptr->left->b_ptr->num)->f, pos)) {
			    return 1;
			}
			else {
			    ptr = ptr->right;
			}
		    }
		    else {
			ptr = ptr->left;
		    }
		}
	    }
	}
	else {
	    if (ptr->right) {
		ptr = ptr->right;
		while (ptr) {
		    if (ptr->direction == RtoL) {
			if (ptr->right && check_feature((sp->bnst_data + ptr->right->b_ptr->num)->f, pos)) {
			    return 1;
			}
			else {
			    ptr = ptr->left;
			}
		    }
		    else {
			ptr = ptr->right;
		    }
		}
	    }
	}
    }
    else {
	if (direction == 1) {
	    if (ptr->right && check_feature((sp->bnst_data + ptr->right->b_ptr->num)->f, pos)) {
		return 1;
	    }
	    if (ptr->left) {
		ptr = ptr->left;
		while (ptr) {
		    if (ptr->direction == RtoL) {
			if (ptr->right && check_feature((sp->bnst_data + ptr->right->b_ptr->num)->f, pos)) {
			    return 1;
			}
			else {
			    ptr = ptr->left;
			}
		    }
		    else {
			ptr = ptr->right;
		    }
		}
	    }
	}
	else {
	    if (ptr->left) {
		ptr = ptr->left;
		while (ptr) {
		    if (ptr->direction == LtoR) {
			if (ptr->left && check_feature((sp->bnst_data + ptr->left->b_ptr->num)->f, pos)) {
			    return 1;
			}
			else {
			    ptr = ptr->right;
			}
		    }
		    else {
			ptr = ptr->left;
		    }
		}
	    }
	}
    }	
    return 0;
}

int check_chi_dpnd_possibility (int i, int j, int k, CKY *left, CKY *right, SENTENCE_DATA *sp, int direction, int index) {
    int l;
    char *left_pos, *right_pos;

    if (Language != CHINESE) {
	return 1;
    }
    else {
	if (Dpnd_matrix[left->b_ptr->num][right->b_ptr->num] > 0 && Dpnd_matrix[left->b_ptr->num][right->b_ptr->num] != 'O') {
	    if (direction != Dpnd_matrix[left->b_ptr->num][right->b_ptr->num]) {
		return 0;
	    }
	}

	if (!OptChiPos) {
	  left_pos = (sp->bnst_data + left->b_ptr->num)->head_ptr->Pos;
	  right_pos = (sp->bnst_data + right->b_ptr->num)->head_ptr->Pos;
	}
	else {
	  left_pos = Chi_word_pos[Chi_dpnd_matrix[left->b_ptr->num][right->b_ptr->num].left_pos_index[index]];
	  right_pos = Chi_word_pos[Chi_dpnd_matrix[left->b_ptr->num][right->b_ptr->num].right_pos_index[index]];
	}

	/* check if this cky corresponds with the grammar rules for Chinese */
	/* LC cannot depend on noun */
	if (!strcmp(left_pos, "LC") &&
	    (!strcmp(right_pos, "NN") ||
	     !strcmp(right_pos, "NR") ||
	     !strcmp(right_pos, "PN") ||
	     !strcmp(right_pos, "NT")) &&
	    direction == 'R') {
	  return 0;
	}

	/* sp and main verb */
	if (!strcmp(right_pos, "SP") &&
	    strcmp(left_pos, "VV") != 0 &&
	    strcmp(left_pos, "VA") != 0 &&
	    strcmp(left_pos, "VC") != 0 &&
	    strcmp(left_pos, "VE") != 0 &&
	    direction == 'L') {
	  return 0;
	}

	/* verb cannot depend on SP */
	if ((!strcmp(right_pos, "SP") &&
	     (!strcmp(left_pos, "VV") ||
	      !strcmp(left_pos, "VA") ||
	      !strcmp(left_pos, "VC") ||
	      !strcmp(left_pos, "VE")) &&
	     direction == 'R') ||
	    (!strcmp(left_pos, "SP") &&
	     (!strcmp(right_pos, "VV") ||
	      !strcmp(right_pos, "VA") ||
	      !strcmp(right_pos, "VC") ||
	      !strcmp(right_pos, "VE")) &&
	     direction == 'L')) {
	  return 0;
	}

	/* adj and verb cannot have dependency relation */
	if ((!strcmp(left_pos, "JJ") &&
	     (!strcmp(right_pos, "VV") ||
	      !strcmp(right_pos, "VA") ||
	      !strcmp(right_pos, "VC") ||
	      !strcmp(right_pos, "VE"))) ||
	    (!strcmp(right_pos, "JJ") &&
	     (!strcmp(left_pos, "VV") ||
	      !strcmp(left_pos, "VA") ||
	      !strcmp(left_pos, "VC") ||
	      !strcmp(left_pos, "VE")))) {
	  return 0;
	}

	/* only the quote PU can be head */
	if ((direction == 'R' && 
	     !strcmp(right_pos, "PU") &&
	     Chi_quote_end_matrix[right->b_ptr->num][right->b_ptr->num] != right->b_ptr->num) ||
	    (direction == 'L' && 
	     !strcmp(left_pos, "PU") &&
	     Chi_quote_start_matrix[left->b_ptr->num][left->b_ptr->num] != left->b_ptr->num)) {
	  return 0;
	}

	/* AD cannot be head except for AD */
	if ((!strcmp(left_pos, "AD") &&
	     strcmp(right_pos, "AD") != 0 &&
	     direction == 'L') || 
	    (!strcmp(right_pos, "AD") &&
	     strcmp(left_pos, "AD") != 0 &&
	     direction == 'R')) {
	  return 0;
	}

	/* DEC cannot depend on VV before */
	if (!strcmp(left_pos, "VV") &&
	    direction == 'L' && 
	    !strcmp(right_pos, "DEC")) {
	  return 0;
	}

	/* for DEG , DEV, DEC and LC, there should not be two modifiers */
	if ((!strcmp(right_pos, "DEG") ||
	     !strcmp(right_pos, "DEV") ||
	     !strcmp(right_pos, "LC")) &&
	    right->b_ptr->num - right->i > 0 && 
	    direction == 'R') {
	  return 0;
	}

	/* DEC cannot have two verb modifiers */
	if (!strcmp(right_pos, "DEC") &&
	    direction == 'R' &&
	    (!strcmp(left_pos, "VV") || 
	     !strcmp(left_pos, "VA")) &&
	    (has_child_chi(sp, right, "VV", 0) ||
	     has_child_chi(sp, right, "VA", 0))) {
	  return 0;
	}

	/* for DEC, if there exists noun between it and previous verb, the noun should depend on verb */
	if (!strcmp(right_pos, "DEC") &&
	    (!strcmp(left_pos, "VV") ||
	     !strcmp(left_pos, "VA")) &&
	    exist_chi(sp, right->i, right->b_ptr->num - 1, "noun") != -1 &&
	    direction == 'R') {
	  return 0;
	}

	/* for DEG, its right head should be noun afterwords */
	if (!strcmp(left_pos, "DEG") &&
	    strcmp(right_pos, "NN") != 0 && 
	    strcmp(right_pos, "NT") != 0 && 
	    strcmp(right_pos, "NR") != 0 && 
	    strcmp(right_pos, "PN") != 0 && 
	    strcmp(right_pos, "M") != 0 && 
	    direction == 'R') {
	  return 0;
	}

	/* for DEG and DEC, it must have some word before modifying it */
	if (((!strcmp(left_pos, "DEG") || 
	      !strcmp(left_pos, "DEC")) &&
	     left->i == left->b_ptr->num &&
	     direction == 'R') ||
	    ((!strcmp(right_pos, "DEG") ||
	      !strcmp(right_pos, "DEC")) &&
	     right->i == right->b_ptr->num &&
	     direction == 'L')) {
	  return 0;
	}

	/* for DEC, it must have some verb before modifying it */
	if (strcmp(left_pos, "VV") != 0 &&
	    strcmp(left_pos, "VA") != 0 &&
	    strcmp(left_pos, "VC") != 0 &&
	    strcmp(left_pos, "VE") != 0 &&
	    !strcmp(right_pos, "DEC") &&
	    right->i == right->b_ptr->num &&
	    direction == 'R') {
	  return 0;
	}

	/* LC must have modifier before */
	if (!strcmp(left_pos, "LC") &&
	    left->i == left->b_ptr->num) {
	  return 0;
	}

	/* VC and VE must have modifier behind */
	if ((!strcmp(left_pos, "VC") ||
	     !strcmp(left_pos, "VE")) &&
	    ((direction == 'R' &&
	      left->j == left->b_ptr->num))) {
	  return 0;
	}

	/* for verb, there should be only one object afterword */
	if ((!strcmp(left_pos, "VV") ||
	     !strcmp(left_pos, "VC") ||
	     !strcmp(left_pos, "VE") ||
	     !strcmp(left_pos, "P") ||
	     !strcmp(left_pos, "VA")) &&
	    (!strcmp(right_pos, "NN") ||
	     !strcmp(right_pos, "NR") ||
	     !strcmp(right_pos, "PN") ||
	     !strcmp(right_pos, "M") ||
	     !strcmp(right_pos, "DEG")) &&
	    direction == 'L' &&
	    left->j != left->i &&
	    (has_child_chi(sp, left, "NN", 1)||
	     has_child_chi(sp, left, "NR", 1)||
	     has_child_chi(sp, left, "M", 1)||
	     has_child_chi(sp, left, "DEG", 1)||
	     has_child_chi(sp, left, "PN", 1))) {
	  return 0;
	}

	/* if a verb has object, then between the verb and its object, there should not be another verb depend on the first verb */
	if ((!strcmp(left_pos, "VV") ||
	     !strcmp(left_pos, "VC") ||
	     !strcmp(left_pos, "VE") ||
	     !strcmp(left_pos, "VA")) &&
	    (!strcmp(right_pos, "NN") ||
	     !strcmp(right_pos, "PN") ||
	     !strcmp(right_pos, "NR")) &&
	    (has_child_chi(sp, left, "VV", 1) ||
	     has_child_chi(sp, left, "VA", 1) ||
	     has_child_chi(sp, left, "VC", 1) ||
	     has_child_chi(sp, left, "VE", 1))) {
	  return 0;
	}    

	/* for verb, there should be only one subject in front of it */
	if ((!strcmp(right_pos, "VV") ||
	     !strcmp(right_pos, "VC") ||
	     !strcmp(right_pos, "VE") ||
	     !strcmp(right_pos, "VA")) &&
	    (!strcmp(left_pos, "NN") ||
	     !strcmp(left_pos, "PN") ||
	     !strcmp(left_pos, "NR")) &&
	    direction == 'R' &&
	    right->j != right->i &&
	    (has_child_chi(sp, right, "NN", 0)||
	     has_child_chi(sp, right, "NR", 0)||
	     has_child_chi(sp, right, "PN", 0))) {
	  return 0;
	}

	/* for preposition, it must have non-pu modifier */
	if ((!strcmp(left_pos, "P") &&
	     direction == 'R' &&
	     left->j - left->i == 0) || 
	    (!strcmp(right_pos, "P") &&
	     direction == 'L' &&
	     right->j - right->i == 0)) {
	  return 0;
	}

	/* for preposition, it cannot depend on a preposition */
	if (!strcmp(right_pos, "P") &&
	    !strcmp(left_pos, "P")) {
	  return 0;
	}

	/* for preposition, it cannot depend on a noun before */
	if (!strcmp(right_pos, "P") &&
	    (!strcmp(left_pos, "NN") ||
	     !strcmp(left_pos, "NR") ||
	     !strcmp(left_pos, "PN"))) {
	  return 0;
	}

	/* for preposition, it cannot depend on a CD or AD */
	if (!strcmp(left_pos, "P") &&
	    !strcmp(right_pos, "CD") &&
	    direction == 'R') {
	  return 0;
	}

	/* a noun cannot depend on its following preposition */
	if (!strcmp(right_pos, "P") &&
	    (!strcmp(left_pos, "NN") || 
	     !strcmp(left_pos, "NR") || 
	     !strcmp(left_pos, "PN")) && 
	    direction == 'R') {
	  return 0;
	}

	/* for preposition, if it depend on verb, it should have modifier */
	if ((!strcmp(left_pos, "P") &&
	     (!strcmp(right_pos, "VA") || 
	      !strcmp(right_pos, "VC") || 
	      !strcmp(right_pos, "VE") || 
	      !strcmp(right_pos, "VV")) &&
	     direction == 'R' &&
	     left->j == left->b_ptr->num) ||
	    (!strcmp(right_pos, "P") &&
	     (!strcmp(left_pos, "VA") || 
	      !strcmp(left_pos, "VC") || 
	      !strcmp(left_pos, "VE") || 
	      !strcmp(left_pos, "VV")) &&
	     direction == 'L' &&
	     right->j == right->b_ptr->num)) {
	  return 0;
	}

	/* for preposition, it cannot have two modifiers after it */
	if (!strcmp(left_pos, "P") && 
	    left->right &&
	    !check_feature((sp->bnst_data + left->right->b_ptr->num)->f, "PU") &&
	    direction == 'L') {
	  return 0;
	}

	/* for preposition, if it depend on verb before, the verb should have object */
	if (!strcmp(right_pos, "P") &&
	    (!strcmp(left_pos, "VC") || 
	     !strcmp(left_pos, "VE") || 
	     !strcmp(left_pos, "VV")) &&
	    direction == 'L' &&
	    right->j == sp->Bnst_num - 1) {
	  return 0;
	}

	/* for preposition, if there is LC in the following (no preposibion between them), the words between P and LC should depend on LC */
	if (!strcmp(left_pos, "P") &&
	    !strcmp(right_pos, "LC") &&
	    left->j - left->i > 0 &&
	    exist_chi(sp, left->b_ptr->num + 1, right->b_ptr->num - 1, "prep") == -1) {
	  return 0;
	}

	/* for preposition, if there is noun between it and following verb, if preposition is head of the verb, all the noun should depend on verb, if verb is head of preposition, all the noun should depend on preposition */
	if (!strcmp(left_pos, "P") &&
	    (!strcmp(right_pos, "VV") ||
	     !strcmp(right_pos, "VA") || 
	     !strcmp(right_pos, "VC") ||
	     !strcmp(right_pos, "VE")) &&
	    (direction == 'L' && /* preposition is head */
	     left->j != left->i &&
	     (has_child_chi(sp, left, "NN", 1)||
	      has_child_chi(sp, left, "NR", 1)||
	      has_child_chi(sp, left, "PN", 1)))) {
	  return 0;
	}

	if (!OptChiPos) {
	  /* the word before dunhao cannot have left dependency */
	  if (direction == 'L' &&
	      check_feature((sp->bnst_data + right->b_ptr->num + 1)->f, "PU") &&
	      !check_feature((sp->bnst_data + right->b_ptr->num + 2)->f, "VV") &&
	      !check_feature((sp->bnst_data + right->b_ptr->num + 2)->f, "VC") &&
	      !check_feature((sp->bnst_data + right->b_ptr->num + 2)->f, "VE") &&
	      !check_feature((sp->bnst_data + right->b_ptr->num + 2)->f, "VA") &&
	      !strcmp((sp->bnst_data + right->b_ptr->num + 1)->head_ptr->Goi, "、")) {
	    return 0;
	  }

	  /* if a SB is followed by VV, then this SB cannot have modifier */
	  if ((check_feature((sp->bnst_data + left->b_ptr->num)->f, "SB") &&
	       check_feature((sp->bnst_data + left->b_ptr->num + 1)->f, "VV") &&
	       left->i != left->b_ptr->num) ||
	      (check_feature((sp->bnst_data + right->b_ptr->num)->f, "SB") &&
	       check_feature((sp->bnst_data + right->b_ptr->num + 1)->f, "VV") &&
	       (right->i != right->b_ptr->num || direction == 'R'))) {
	    return 0;
	  }	    

	  /* the noun before dunhao should depend on noun after it */
	  if (direction == 'R' &&
	      (check_feature((sp->bnst_data + left->b_ptr->num)->f, "NN") ||
	       check_feature((sp->bnst_data + left->b_ptr->num)->f, "NR") ||
	       check_feature((sp->bnst_data + left->b_ptr->num)->f, "PN") ||
	       check_feature((sp->bnst_data + left->b_ptr->num)->f, "JJ") ||
	       check_feature((sp->bnst_data + left->b_ptr->num)->f, "NT") ||
	       check_feature((sp->bnst_data + left->b_ptr->num)->f, "M") ||
	       check_feature((sp->bnst_data + left->b_ptr->num)->f, "DEG")) &&
	      (!check_feature((sp->bnst_data + right->b_ptr->num)->f, "NN") &&
	       !check_feature((sp->bnst_data + right->b_ptr->num)->f, "NT") &&
	       !check_feature((sp->bnst_data + right->b_ptr->num)->f, "JJ") &&
	       !check_feature((sp->bnst_data + right->b_ptr->num)->f, "NR") &&
	       !check_feature((sp->bnst_data + right->b_ptr->num)->f, "PN") &&
	       !check_feature((sp->bnst_data + right->b_ptr->num)->f, "DEG") &&
	       !check_feature((sp->bnst_data + right->b_ptr->num)->f, "M")) &&
	      check_feature((sp->bnst_data + left->b_ptr->num + 1)->f, "PU") &&
	      !check_feature((sp->bnst_data + left->b_ptr->num + 2)->f, "VV") &&
	      !check_feature((sp->bnst_data + left->b_ptr->num + 2)->f, "VC") &&
	      !check_feature((sp->bnst_data + left->b_ptr->num + 2)->f, "VE") &&
	      !check_feature((sp->bnst_data + left->b_ptr->num + 2)->f, "VA") &&
	      !strcmp((sp->bnst_data + left->b_ptr->num + 1)->head_ptr->Goi, "、")) {
	    return 0;
	  }

	  /* for preposition, if it has a VV modifier after it, this VV should have object or subject */
	  if (check_feature((sp->bnst_data + left->b_ptr->num)->f, "P") &&
	      check_feature((sp->bnst_data + right->b_ptr->num)->f, "VV") &&
	      direction == 'L' &&
	      !check_feature((sp->bnst_data + right->b_ptr->num + 1)->f, "CC") &&
	      (!has_child_chi(sp, right, "NN", 0) &&
	       !has_child_chi(sp, right, "NR", 0) &&
	       !has_child_chi(sp, right, "PN", 0)) &&
	      (!has_child_chi(sp, right, "NN", 1) &&
	       !has_child_chi(sp, right, "NR", 1) &&
	       !has_child_chi(sp, right, "PN", 1))) {
	    return 0;
	  }

	  /* VC and VE must have modifier before */
	  if ((check_feature((sp->bnst_data + left->j)->f, "VC") &&
	       left->j != left->b_ptr->num) ||
	      (check_feature((sp->bnst_data + right->i)->f, "VC") &&
	       right->i != right->b_ptr->num)) {
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
	}

	return 1;
    }
}
