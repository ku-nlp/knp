/*====================================================================

			      格構造解析

                                               S.Kurohashi 91.10. 9
                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

extern int Possibility;
extern int MAX_Case_frame_num;

CF_MATCH_MGR	*Cf_match_mgr = NULL;	/* 作業領域 */
TOTAL_MGR	Work_mgr;

int	DISTANCE_STEP	= 5;
int	RENKAKU_STEP	= 2;
int	STRONG_V_COST	= 8;
int	ADJACENT_TOUTEN_COST	= 5;
int	LEVELA_COST	= 4;
int	TEIDAI_STEP	= 2;

/*==================================================================*/
			  void realloc_cmm()
/*==================================================================*/
{
    Cf_match_mgr = (CF_MATCH_MGR *)realloc_data(Cf_match_mgr, 
						sizeof(CF_MATCH_MGR)*(MAX_Case_frame_num), 
						"realloc_cmm");
}

/*==================================================================*/
		      void init_case_analysis()
/*==================================================================*/
{
    if (OptAnalysis == OPT_CASE || 
	OptAnalysis == OPT_CASE2 || 
	OptAnalysis == OPT_DISC) {
	int i, j;

	Cf_match_mgr = (CF_MATCH_MGR *)malloc_data(sizeof(CF_MATCH_MGR)*ALL_CASE_FRAME_MAX, 
						   "init_case_analysis");

	for (i = 0; i < CPM_MAX; i++) {
	    for (j = 0; j < CF_ELEMENT_MAX; j++) {
		if (Thesaurus == USE_BGH) {
		    Work_mgr.cpm[i].cf.ex[j] = 
			(char *)malloc_data(sizeof(char)*EX_ELEMENT_MAX*BGH_CODE_SIZE, "init_case_analysis");
		}
		else if (Thesaurus == USE_NTT) {
		    Work_mgr.cpm[i].cf.ex2[j] = 
			(char *)malloc_data(sizeof(char)*SM_ELEMENT_MAX*SM_CODE_SIZE, "init_case_analysis");
		}
		Work_mgr.cpm[i].cf.sm[j] = 
		    (char *)malloc_data(sizeof(char)*SM_ELEMENT_MAX*SM_CODE_SIZE, "init_case_analysis");
	    }
	}
    }
}

/*====================================================================
		       格助詞の文字−コード対応
====================================================================*/

struct PP_STR_TO_CODE {
    char *hstr;
    char *kstr;
    int  code;
} PP_str_to_code[] = {
    {"φ", "φ", 0},		/* 格助詞のないもの(数量表現等) */
    {"が", "ガ", 1},
    {"を", "ヲ", 2},
    {"に", "ニ", 3},
    {"から", "カラ", 4},
    {"へ", "ヘ", 5},
    {"より", "ヨリ", 6},
    {"と", "ト", 7},
    {"で", "デ", 8},
    {"によって", "ニヨッテ", 9},
    {"をめぐる", "ヲメグル", 10},	/* 複合辞関係 */
    {"をつうじる", "ヲツウジル", 11},
    {"をつうずる", "ヲツウズル", 12},
    {"をふくめる", "ヲフクメル", 13},
    {"をはじめる", "ヲハジメル", 14},
    {"にからむ", "ニカラム", 15},
    {"にそう", "ニソウ", 16},
    {"にむける", "ニムケル", 17},
    {"にともなう", "ニトモナウ", 18},
    {"にもとづく", "ニモトヅク", 19},
    {"をのぞく", "ヲノゾク", 20},
    {"による", "ニヨル", 21},
    {"にたいする", "ニタイスル", 22},
    {"にかんする", "ニカンスル", 23},
    {"にかわる", "ニカワル", 24},
    {"におく", "ニオク", 25},
    {"につく", "ニツク", 26},
    {"にとる", "ニトル", 27},
    {"にくわえる", "ニクワエル", 28},
    {"にかぎる", "ニカギル", 29},
    {"につづく", "ニツヅク", 30},
    {"にあわせる", "ニアワセル", 31},
    {"にくらべる", "ニクラベル", 32},
    {"にならぶ", "ニナラブ", 33},
    {"とする", "トスル", 34},
    {"によるぬ", "ニヨルヌ", 35},
    {"にかぎるぬ", "ニカギルヌ", 36},
    {"時間", "時間", 37},	/* ニ格, 無格で時間であるものを時間という格として扱う */
    {"まで", "マデ", 38},	/* 明示されない格であるが、辞書側の格として表示するために
				   書いておく */
    {"修飾", "修飾", 39},
    {"が２", "ガ２", 40},
    {"外の関係", "外ノ関係", 41},
    {"は", "ハ", 1},		/* NTT辞書では「ガガ」構文が「ハガ」
				   ※ NTT辞書の「ハ」は1(code)に変換されるが,
				      1は配列順だけで「ガ」に変換される */
    {"＊", "＊", -2},		/* 埋め込み文の被修飾詞 */
    {NULL, NULL, -1}		/* 格助詞の非明示のもの(提題助詞等) */
};

/*====================================================================
			 文字−コード対応関数
====================================================================*/
int pp_kstr_to_code(char *cp)
{
    int i;
    for (i = 0; PP_str_to_code[i].kstr; i++)
	if (str_eq(PP_str_to_code[i].kstr, cp))
	    return PP_str_to_code[i].code;
    
    if (str_eq(cp, "ニトッテ"))		/* 「待つ」 IPALのバグ ?? */
	return pp_kstr_to_code("ニヨッテ");
    else if (str_eq(cp, "ノ"))		/* 格要素でなくなる場合 */
	return END_M;

    /* fprintf(stderr, "Invalid string (%s) in PP !\n", cp); */
    return END_M;
}

int pp_hstr_to_code(char *cp)
{
    int i;
    for (i = 0; PP_str_to_code[i].hstr; i++)
	if (str_eq(PP_str_to_code[i].hstr, cp))
	    return PP_str_to_code[i].code;
    return END_M;
}

char *pp_code_to_kstr(int num)
{
    return PP_str_to_code[num].kstr;
}

char *pp_code_to_hstr(int num)
{
    return PP_str_to_code[num].hstr;
}

/*==================================================================*/
int case_analysis(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr, BNST_DATA *b_ptr)
/*==================================================================*/
{
    /*
                                              戻値
      入力の格要素がない場合                    -3
      格フレームがない場合                      -2
      入力側に必須格が残る場合(解析不成功)      -1
      解析成功                               score (0以上)
    */

    CASE_FRAME *cf_ptr = &(cpm_ptr->cf);
    int i, j, frame_num, default_score;
    CF_MATCH_MGR tempcmm;
    
    /* 初期化 */
    cpm_ptr->pred_b_ptr = b_ptr;
    cpm_ptr->score = -1;
    cpm_ptr->result_num = 0;
    cpm_ptr->cmm[0].cf_ptr = NULL;

    /* 入力文側の格要素設定 */
    cf_ptr->voice = b_ptr->voice;
    default_score = make_data_cframe(sp, cpm_ptr);

    /* 格フレーム解析スキップ
    if (cf_ptr->element_num == 0) {
	cpm_ptr->cmm[0].cf_ptr = NULL;
	return -3;
    }
    */

    /* 格フレーム設定 */
    frame_num = 0;
    for (i = 0; i < b_ptr->cf_num; i++) {
	/* 前隣を結合したフレームなのにもかかわらず、
	   前隣が自分に係らない構造の場合はスキップ */
  	if ((b_ptr->cf_ptr + i)->concatenated_flag == 1 && 
  	    b_ptr->num > 0 && (b_ptr-1)->dpnd_head != b_ptr->num) {
  	    continue;
  	}
	(Cf_match_mgr + frame_num++)->cf_ptr = b_ptr->cf_ptr + i;
    }

    if (frame_num == 0) {
	return -2;
    }

    /* 格要素なしの時の実験 98/12/16 */
    if (cf_ptr->element_num == 0) {
	case_frame_match(cpm_ptr, Cf_match_mgr, OptCFMode);
	cpm_ptr->score = Cf_match_mgr->score;
	cpm_ptr->cmm[0] = *Cf_match_mgr;
	cpm_ptr->result_num = 1;
	frame_num = 1;
    }
    else { /* このelseはtentative */
	cpm_ptr->result_num = 0;
	for (i = 0; i < frame_num; i++) {

	    /* 選択可能
	       EXAMPLE
	       SEMANTIC_MARKER

	       ChangeLog:
	       意味マーカの利用に変更 (1998/10/02)
	       オプションで選択       (1999/06/15)
	       */

	    case_frame_match(cpm_ptr, Cf_match_mgr+i, OptCFMode);

	    cpm_ptr->cmm[cpm_ptr->result_num] = *(Cf_match_mgr+i);
	    for (j = cpm_ptr->result_num-1; j >= 0; j--) {
		if (cpm_ptr->cmm[j].score < cpm_ptr->cmm[j+1].score) {
		    tempcmm = cpm_ptr->cmm[j];
		    cpm_ptr->cmm[j] = cpm_ptr->cmm[j+1];
		    cpm_ptr->cmm[j+1] = tempcmm;
		}
		else {
		    break;
		}
	    }
	    if (cpm_ptr->result_num < CMM_MAX-1) {
		cpm_ptr->result_num++;
	    }
	}
	cpm_ptr->score = cpm_ptr->cmm[0].score;
    }

    /* 外の関係のスコアを足す */
    if (cpm_ptr->score > -1)  {
	cpm_ptr->score += default_score;
    }

    if (OptDisplay == OPT_DEBUG) {
	print_data_cframe(cpm_ptr);
	print_good_crrspnds(cpm_ptr, Cf_match_mgr, frame_num);
    }

    return cpm_ptr->score;
}

/*==================================================================*/
int all_case_analysis(SENTENCE_DATA *sp, BNST_DATA *b_ptr, TOTAL_MGR *t_ptr)
/*==================================================================*/
{
    CF_PRED_MGR *cpm_ptr;
    int i;
    int one_case_point;

    if (b_ptr->para_top_p != TRUE && 
	((check_feature(b_ptr->f, "用言") && !check_feature(b_ptr->f, "ID:（弱連体）")) || 
	 check_feature(b_ptr->f, "準用言") || 
	 check_feature(L_Jiritu_M(b_ptr)->f, "サ変")) && 
	!check_feature(b_ptr->f, "複合辞")) {

	cpm_ptr = &(t_ptr->cpm[t_ptr->pred_num]);

	one_case_point = case_analysis(sp, cpm_ptr, b_ptr);

	/* 解析不成功(入力側に必須格が残る)場合にその依存構造の解析を
	   やめる場合
	if (one_case_point == -1) return FALSE;
	*/

	t_ptr->score += one_case_point;
	
	if (t_ptr->pred_num++ >= CPM_MAX) {
	    fprintf(stderr, "too many predicates in a sentence.\n");
	    exit(1);
	}
    }

    for (i = 0; b_ptr->child[i]; i++)
	if (all_case_analysis(sp, b_ptr->child[i], t_ptr) == FALSE)
	    return FALSE;

    return TRUE;
}

/*==================================================================*/
	    void copy_cf(CASE_FRAME *dst, CASE_FRAME *src)
/*==================================================================*/
{
    int i, j;

    dst->element_num = src->element_num;
    for (i = 0; i < CF_ELEMENT_MAX; i++) {
	dst->oblig[i] = src->oblig[i];
	dst->adjacent[i] = src->adjacent[i];
	for (j = 0; j < PP_ELEMENT_MAX; j++) {
	    dst->pp[i][j] = src->pp[i][j];
	}
	for (j = 0; j < SM_ELEMENT_MAX*SM_CODE_SIZE; j++) {
	    dst->sm[i][j] = src->sm[i][j];
	}
	if (Thesaurus == USE_BGH) {
	    strcpy(dst->ex[i], src->ex[i]);
	}
	else if (Thesaurus == USE_NTT) {
	    strcpy(dst->ex2[i], src->ex2[i]);
	}
	dst->examples[i] = src->examples[i];	/* これを使う場合問題あり */
    }
    dst->voice = src->voice;
    dst->ipal_address = src->ipal_address;
    dst->ipal_size = src->ipal_size;
    strcpy(dst->ipal_id, src->ipal_id);
    strcpy(dst->imi, src->imi);
    dst->concatenated_flag = src->concatenated_flag;
}

/*==================================================================*/
	  void copy_cpm(CF_PRED_MGR *dst, CF_PRED_MGR *src)
/*==================================================================*/
{
    int i;

    copy_cf(&dst->cf, &src->cf);
    dst->pred_b_ptr = src->pred_b_ptr;
    for (i = 0; i < CF_ELEMENT_MAX; i++) {
	dst->elem_b_ptr[i] = src->elem_b_ptr[i];
	dst->elem_b_num[i] = src->elem_b_num[i];
    }
    dst->score = src->score;
    dst->result_num = src->result_num;
    for (i = 0; i < CMM_MAX; i++) {
	dst->cmm[i] = src->cmm[i];
    }
}

/*==================================================================*/
	    void copy_mgr(TOTAL_MGR *dst, TOTAL_MGR *src)
/*==================================================================*/
{
    int i;

    dst->dpnd = src->dpnd;
    dst->pssb = src->pssb;
    dst->dflt = src->dflt;
    dst->score = src->score;
    dst->pred_num = src->pred_num;
    for (i = 0; i < CPM_MAX; i++) {
	copy_cpm(&dst->cpm[i], &src->cpm[i]);
    }
    dst->ID = src->ID;
}

/*==================================================================*/
	void call_case_analysis(SENTENCE_DATA *sp, DPND dpnd)
/*==================================================================*/
{
    int i, j, k;
    int one_topic_score, topic_score, topic_score_sum = 0, topic_slot[2], distance_cost = 0;
    char *cp;

    /* 格構造解析のメイン関数 */

    /* 依存構造木作成 */

    dpnd_info_to_bnst(sp, &dpnd);
    make_dpnd_tree(sp);
	
    if (OptDisplay == OPT_DEBUG)
	print_kakari(sp);

    /* 格解析作業領域の初期化 */
	
    Work_mgr.pssb = Possibility;
    Work_mgr.dpnd = dpnd;
    Work_mgr.score = 0;
    Work_mgr.pred_num = 0;
    Work_mgr.dflt = 0;
    for (i = 0; i < sp->Bnst_num; i++)
	Work_mgr.dflt += dpnd.dflt[i];
    
    /* 格解析呼び出し */

    if (all_case_analysis(sp, sp->bnst_data+sp->Bnst_num-1, &Work_mgr) == TRUE)
	Possibility++;
    else
	return;

    /* ここで default との距離のずれ, 提題を処理 */

    for (i = 0; i < sp->Bnst_num-1; i++) {
	/* ガ格 -> レベル:A (ルールでこの係り受けを許した場合は、
	   このでコストを与える) */
	if (check_feature((sp->bnst_data+i)->f, "係:ガ格") && 
	    check_feature((sp->bnst_data+dpnd.head[i])->f, "レベル:A")) {
	    distance_cost += LEVELA_COST;
	}

	if (dpnd.dflt[i] > 0) {
	    /* 提題 */
	    if (check_feature((sp->bnst_data+i)->f, "提題")) {
		distance_cost += dpnd.dflt[i];

		/* 提題につられて遠くに係ってしまった文節の距離コスト */
		for (j = 0; j < i-1; j++) {
		    if (dpnd.head[i] == dpnd.head[j]) {
			for (k = j+1; k < i; k++) {
			    if (Mask_matrix[j][k] && Quote_matrix[j][k] && Dpnd_matrix[j][k] && Dpnd_matrix[j][k] != 'd') {
				distance_cost += dpnd.dflt[i]*TEIDAI_STEP;
			    }
			}
		    }
		}
		continue;
	    }
	    /* 提題以外 */
	    /* 係り側が連用でないとき */
	    if (!check_feature((sp->bnst_data+i)->f, "係:連用")) {
		/* 自分に読点がなく、隣の強い用言 (連体以外) を越えているとき */
		if (!check_feature((sp->bnst_data+i)->f, "読点")) {
		    if (dpnd.head[i] > i+1 && 
			subordinate_level_check("B", sp->bnst_data+i+1) && 
			(cp = (char *)check_feature((sp->bnst_data+i+1)->f, "係"))) {
			if (strcmp(cp+3, "連体") && strcmp(cp+3, "連格")) {
			    distance_cost += STRONG_V_COST;
			}
		    }
		}
		/* 自分に読点があり*/
		else {
		    /* 隣に係るとき */
		    if (dpnd.head[i] == i+1) {
			distance_cost += ADJACENT_TOUTEN_COST;
		    }
		}
	    }

	    /* デフォルトとの差 x 2 を距離のコストとする
	       ただし、形容詞を除く連格の場合は x 1 */
	    if (!check_feature((sp->bnst_data+i)->f, "係:連格") || 
		check_feature((sp->bnst_data+i)->f, "用言:形")) {
		distance_cost += dpnd.dflt[i]*DISTANCE_STEP;
	    }
	    else {
		distance_cost += dpnd.dflt[i]*RENKAKU_STEP;
	    }
	}		    
    }

    Work_mgr.score -= distance_cost;

    for (i = sp->Bnst_num-1; i > 0; i--) {
	/* 文末から用言ごとに提題を処理する */
	if (cp = (char *)check_feature((sp->bnst_data+i)->f, "提題受")) {

	    /* topic_slot[0]	時間以外のハ格のスロット
	       topic_slot[1]	「<<時間>>は」のスロット
	       両方とも 1 以下しか許可しない
	    */

	    topic_slot[0] = 0;
	    topic_slot[1] = 0;
	    one_topic_score = 0;

	    /* 係り側を探す */
	    for (j = i-1; j >= 0; j--) {
		if (dpnd.head[j] != i) {
		    continue;
		}
		if (check_feature((sp->bnst_data+j)->f, "提題")) {
		    if (check_feature((sp->bnst_data+j)->f, "時間")) {
			topic_slot[1]++;
		    }
		    else {
			topic_slot[0]++;
		    }
		    sscanf(cp, "%*[^:]:%d", &topic_score);
		    one_topic_score += topic_score;
		}
	    }

	    if (topic_slot[0] > 0 || topic_slot[1] > 0) {
		one_topic_score += 20;
	    }
	    Work_mgr.score += one_topic_score;
	    if (OptDisplay == OPT_DEBUG) {
		topic_score_sum += one_topic_score;
	    }
	}
    }

    if (OptDisplay == OPT_DEBUG) {
	fprintf(stdout, "■ %d点 (距離減点 %d点 (%d点) 提題スコア %d点)\n", 
		Work_mgr.score, distance_cost, Work_mgr.dflt*2, topic_score_sum);
    }
        
    /* 後処理 */

    if (Work_mgr.score > sp->Best_mgr->score ||
	(Work_mgr.score == sp->Best_mgr->score && 
	 compare_dpnd(sp, &Work_mgr, sp->Best_mgr) == TRUE))
	copy_mgr(sp->Best_mgr, &Work_mgr);
}

/*==================================================================*/
	     void record_case_analysis(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, num;
    char feature_buffer[DATA_LEN];
    CF_PRED_MGR *cpm_ptr;

    /* 格解析の結果(Best_mgrが管理)をfeatureとして格要素文節に与える */

    for (j = 0; j < sp->Best_mgr->pred_num; j++) {

	cpm_ptr = &(sp->Best_mgr->cpm[j]);

	/* 格フレームがない場合 */
	if (cpm_ptr->result_num == 0 || 
	    cpm_ptr->cmm[0].cf_ptr->ipal_address == -1 || 
	    cpm_ptr->cmm[0].score == -2) {
	    continue;
	}

	for (i = 0; i < cpm_ptr->cf.element_num; i++) {
	    num = cpm_ptr->cmm[0].result_lists_d[0].flag[i];
	    if (num == NIL_ASSIGNED) {
		if (check_feature(cpm_ptr->pred_b_ptr->f, "係:連格")) {
		    sprintf(feature_buffer, "格関係%d:外の関係", cpm_ptr->pred_b_ptr->num);
		}
		else {
		    sprintf(feature_buffer, "格関係%d:×", cpm_ptr->pred_b_ptr->num);
		}
	    }
	    else if (num >= 0) {
		sprintf(feature_buffer, "格関係%d:%s", cpm_ptr->pred_b_ptr->num, 
			pp_code_to_kstr(cpm_ptr->cmm[0].cf_ptr->pp[num][0]));
	    }
	    /* else: UNASSIGNED はないはず */

	    sprintf(feature_buffer, "%s:%s", feature_buffer, cpm_ptr->elem_b_ptr[i]->Jiritu_Go);

	    /* feature を格要素文節に与える */
	    if (cpm_ptr->elem_b_ptr[i]->num >= 0) {
		assign_cfeature(&(cpm_ptr->elem_b_ptr[i]->f), feature_buffer);
	    }
	    /* 文節内部の要素の場合 */
	    else {
		assign_cfeature(&(cpm_ptr->elem_b_ptr[i]->parent->f), feature_buffer);
	    }
	}
    }
}

/*====================================================================
                               END
====================================================================*/
