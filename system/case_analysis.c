/*====================================================================

			      格構造解析

                                               S.Kurohashi 91.10. 9
                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

extern int Possibility;

CF_MATCH_MGR	*Cf_match_mgr = NULL;	/* 作業領域 */
/* TOTAL_MGR	Dflt_mgr; */
TOTAL_MGR	Work_mgr;

/*==================================================================*/
		      void init_case_analysis()
/*==================================================================*/
{
    if (OptAnalysis == OPT_CASE || 
	OptAnalysis == OPT_CASE2 || 
	OptAnalysis == OPT_DISC) {
	int i, j;

	Cf_match_mgr = (CF_MATCH_MGR *)malloc_data(sizeof(CF_MATCH_MGR)*IPAL_FRAME_MAX*5, 
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
    {"にかぎる", "ニカギリル", 29},
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
      return -1;

    /* fprintf(stderr, "Invalid string (%s) in PP !\n", cp); */
    return -1;
}

int pp_hstr_to_code(char *cp)
{
    int i;
    for (i = 0; PP_str_to_code[i].hstr; i++)
      if (str_eq(PP_str_to_code[i].hstr, cp))
	return PP_str_to_code[i].code;
    return -1;
}

char *pp_code_to_kstr(int num)
{
    return PP_str_to_code[num].kstr;
}

char *pp_code_to_hstr(int num)
{
    return PP_str_to_code[num].hstr;
}

/*
int sm_zstr_to_code(char *cp)
{
    int i;
    for (i = 0; SM_str_to_code[i].zstr; i++)
      if (str_eq(SM_str_to_code[i].zstr, cp))
	return SM_str_to_code[i].code;
    * fprintf(stderr, "Invalid string (%s) in SM !\n", cp); *
    return -1;
}

int sm_str_to_code(char *cp)
{
    int i;
    
    for (i = 0; SM_str_to_code[i].str; i++)
      if (str_eq(SM_str_to_code[i].str, cp))
	return SM_str_to_code[i].code;
    * fprintf(stderr, "Invalid string (%s) in SM !\n", cp); *
    return -1;
}

char *sm_code_to_str(int code)
{
    int i;

    for (i = 0; SM_str_to_code[i].str; i++)
      if (SM_str_to_code[i].code == code)
	return SM_str_to_code[i].str;
    fprintf(stderr, "Invalid code (%d) in SM !\n", code);
    return NULL;
}
*/

/*==================================================================*/
     int case_analysis(CF_PRED_MGR *cpm_ptr, BNST_DATA *b_ptr)
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
    int i, frame_num, max_score, default_score;
    
    /* 初期化 */
    cpm_ptr->pred_b_ptr = b_ptr;
    cpm_ptr->score = -1;
    cpm_ptr->result_num = 0;
    cpm_ptr->cmm[0].cf_ptr = NULL;

    /* 入力文側の格要素設定 */
    cf_ptr->voice = b_ptr->voice;
    default_score = make_data_cframe(cpm_ptr);

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
	case_frame_match(cf_ptr, Cf_match_mgr, OptCFMode);
	cpm_ptr->score = Cf_match_mgr->score;
	cpm_ptr->cmm[0] = *Cf_match_mgr;
	cpm_ptr->result_num = 1;
	frame_num = 1;
    }
    else { /* このelseはtentative */
	for (i = 0; i < frame_num; i++) {

	    /* 選択可能
	       EXAMPLE
	       SEMANTIC_MARKER

	       ChangeLog:
	       意味マーカの利用に変更 (1998/10/02)
	       オプションで選択       (1999/06/15)
	       */

	    case_frame_match(cf_ptr, Cf_match_mgr+i, OptCFMode);

	    /* その格フレームとの対応付けがスコア最大であれば記憶 */

	    if ((Cf_match_mgr+i)->score > cpm_ptr->score) {
		cpm_ptr->score = (Cf_match_mgr+i)->score;
		cpm_ptr->cmm[0] = *(Cf_match_mgr+i);
		cpm_ptr->result_num = 1;
	    }

	    /* その格フレームとの対応付けがスコア最大と同点でも記憶 */

	    else if ((Cf_match_mgr+i)->score == cpm_ptr->score) {
		if (cpm_ptr->result_num >= CMM_MAX)
		    fprintf(stderr, "Not enough cmm.\n");
		else
		    cpm_ptr->cmm[cpm_ptr->result_num++] = *(Cf_match_mgr+i);
	    }
	}
    }

    /* corpus based case analysis 00/01/04

    for (i = 0; i < cpm_ptr->cf.element_num; i++) {
	if (cpm_ptr->elem_b_num[i] == -1) {
	    score -= cpm_ptr->cf.pred_b_ptr.dpnd_dflt * 2;
	} else {
	    score -= cpm_ptr->elem_b_ptr[i].dpnd_dflt * 2;
	}	
    }
    */

    /* 外の関係のスコアを足す */
    if (cpm_ptr->score > -1)  {
	cpm_ptr->score += default_score;
    }

    if (OptDisplay == OPT_DEBUG) {
	print_data_cframe(cf_ptr);
	print_good_crrspnds(cpm_ptr, Cf_match_mgr, frame_num);
    }

    return cpm_ptr->score;
}

/*==================================================================*/
      int all_case_analysis(BNST_DATA *b_ptr, TOTAL_MGR *t_ptr)
/*==================================================================*/
{
    CF_PRED_MGR *cpm_ptr;
    int i;
    int one_case_point;
    
    if (b_ptr->para_top_p != TRUE && 
	(check_feature(b_ptr->f, "用言") || 
	 check_feature(b_ptr->f, "準用言")) && 
	!check_feature(b_ptr->f, "複合辞")) {

	cpm_ptr = &(t_ptr->cpm[t_ptr->pred_num]);

	one_case_point = case_analysis(cpm_ptr, b_ptr);

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
	if (all_case_analysis(b_ptr->child[i], t_ptr) == FALSE)
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
	for (j = 0; j < PP_ELEMENT_MAX; j++) {
	    dst->pp[i][j] = src->pp[i][j];
	}
	for (j = 0; j < SM_ELEMENT_MAX*SM_CODE_SIZE; j++) {
	    dst->sm[i][j] = src->sm[i][j];
	}
	for (j = 0; j < SM_ELEMENT_MAX; j++) {
	    dst->sm_flag[i][j] = src->sm_flag[i][j];
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
		  void call_case_analysis(DPND dpnd)
/*==================================================================*/
{
    int i, j;
    int topic_score, topic_score_sum = 0, topic_slot[2], distance_cost = 0;
    char *cp;

    /* 格構造解析のメイン関数 */

    /* 依存構造木作成 */

    dpnd_info_to_bnst(&dpnd);
    make_dpnd_tree();
	
    if (OptDisplay == OPT_DEBUG)
	print_kakari();

    /* 格解析作業領域の初期化 */
	
    Work_mgr.pssb = Possibility;
    Work_mgr.dpnd = dpnd;
    Work_mgr.score = 0;
    Work_mgr.pred_num = 0;
    Work_mgr.dflt = 0;
    for (i = 0; i < sp->Bnst_num; i++)
	Work_mgr.dflt += dpnd.dflt[i];
    
    /* 格解析呼び出し */

    if (all_case_analysis(sp->bnst_data+sp->Bnst_num-1, &Work_mgr) == TRUE)
	Possibility++;
    else
	return;

    /* corpus based case analysis 00/01/04
       ここで default との距離のずれ, 提題を処理 */

    for (i = 0; i < sp->Bnst_num-1; i++) {
	if (dpnd.dflt[i] > 0) {
	    /* 提題 */
	    if (check_feature((sp->bnst_data+i)->f, "提題")) {
		distance_cost += dpnd.dflt[i];
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
			    distance_cost += 2;
			}
		    }
		}
		/* 自分に読点があり*/
		else {
		    /* 隣に係るとき */
		    if (dpnd.head[i] == i+1) {
			distance_cost += 2;
		    }
		}
	    }
	    distance_cost += dpnd.dflt[i]*2;
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

	    /* 係り側を探す */
	    for (j = 0; j < i; j++) {
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
		}
	    }

	    if ((topic_slot[0] == 1 || topic_slot[1] == 1) && 
		(topic_slot[0] < 2 && topic_slot[1] < 2)) {
		sscanf(cp, "%*[^:]:%d", &topic_score);
		/* とりあえず、時間は topic_score 点にしてみる */
		Work_mgr.score += topic_score*topic_slot[0]+topic_score*topic_slot[1];
		if (OptDisplay == OPT_DEBUG) {
		    topic_score_sum += topic_score*topic_slot[0]+topic_score*topic_slot[1];
		}
	    }

	}
    }

    if (OptDisplay == OPT_DEBUG) {
	fprintf(stdout, "■ %d点 (距離減点 %d点 (%d点) 提題スコア %d点)\n", 
		Work_mgr.score, distance_cost, Work_mgr.dflt*2, topic_score_sum);
    }
        
    /* 後処理 */

    if (Work_mgr.score > Best_mgr.score ||
	(Work_mgr.score == Best_mgr.score && 
	 compare_dpnd(&Work_mgr, &Best_mgr) == TRUE))
	copy_mgr(&Best_mgr, &Work_mgr);
    /* if (Work_mgr.dflt == 0) Dflt_mgr = Work_mgr; */
}

/*==================================================================*/
		     void record_case_analysis()
/*==================================================================*/
{
    int i, j, k, num;
    char feature_buffer[256];
    CF_PRED_MGR *cpm_ptr;
    CF_MATCH_MGR *cmm_ptr;
    CASE_FRAME *cf_ptr;
    BNST_DATA *pred_b_ptr;

    /* 格解析の結果(Best_mgrが管理)をfeatureとして用言文節に与える */

    for (j = 0; j < Best_mgr.pred_num; j++) {

	cpm_ptr = &(Best_mgr.cpm[j]);

	/* 格フレームがない場合 */
	if (cpm_ptr->result_num == 0) {
	    continue;
	}

	cmm_ptr = &(cpm_ptr->cmm[0]);
	cf_ptr = cmm_ptr->cf_ptr;
	pred_b_ptr = cpm_ptr->pred_b_ptr;

	sprintf(feature_buffer, "意味:%s", cf_ptr->imi);
	assign_cfeature(&(pred_b_ptr->f), feature_buffer);

	for (i = 0; i < cf_ptr->element_num; i++) {
	    num = cmm_ptr->result_lists_p[0].flag[i];

	    /* 対応関係 */

	    if (num == UNASSIGNED || cmm_ptr->score == -2) 
		sprintf(feature_buffer, "深層格N%d:NIL:", i+1);
	    else
		sprintf(feature_buffer, "深層格N%d:%d:", i+1, 
			cpm_ptr->elem_b_ptr[num]->num);
	    
	    /* 表層格 */

	    for (k = 0; cf_ptr->pp[i][k] != END_M; k++) {
		if (k != 0) 
		    sprintf(feature_buffer+strlen(feature_buffer), "/");
		sprintf(feature_buffer+strlen(feature_buffer), 
			"%s", pp_code_to_kstr(cf_ptr->pp[i][k]));
	    }
	    if (cf_ptr->oblig[i] == FALSE)
		sprintf(feature_buffer+strlen(feature_buffer), "*");

	    sprintf(feature_buffer+strlen(feature_buffer), ":");
	    
	    /* 意味素 */

	    for (k = 0; cf_ptr->sm[i][k]; k+=SM_CODE_SIZE) {
		if (k != 0) 
		    sprintf(feature_buffer+strlen(feature_buffer), "/");
		sprintf(feature_buffer+strlen(feature_buffer), 
			"%12.12s", &(cf_ptr->sm[i][k]));
	    }

	    /* feature付与 */

	    assign_cfeature(&(pred_b_ptr->f), feature_buffer);
	}
    }
}	    

/*====================================================================
                               END
====================================================================*/
