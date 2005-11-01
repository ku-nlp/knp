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
		 void init_case_frame(CASE_FRAME *cf)
/*==================================================================*/
{
    int j;

    for (j = 0; j < CF_ELEMENT_MAX; j++) {
	if (Thesaurus == USE_BGH) {
	    cf->ex[j] = 
		(char *)malloc_data(sizeof(char)*EX_ELEMENT_MAX*BGH_CODE_SIZE, 
				    "init_case_frame");
	}
	else if (Thesaurus == USE_NTT) {
	    cf->ex[j] = 
		(char *)malloc_data(sizeof(char)*SM_ELEMENT_MAX*SM_CODE_SIZE, 
				    "init_case_frame");
	}
	cf->sm[j] = 
	    (char *)malloc_data(sizeof(char)*SM_ELEMENT_MAX*SM_CODE_SIZE, 
				"init_case_frame");
	cf->ex_list[j] = (char **)malloc_data(sizeof(char *), 
					      "init_case_frame");
	cf->ex_list[j][0] = (char *)malloc_data(sizeof(char)*WORD_LEN_MAX, 
						"init_case_frame");
	cf->ex_freq[j] = (int *)malloc_data(sizeof(int), 
					    "init_case_frame");
    }
}

/*==================================================================*/
		    void init_case_analysis_cmm()
/*==================================================================*/
{
    if (OptAnalysis == OPT_CASE || 
	OptAnalysis == OPT_CASE2) {

	/* 作業cmm領域確保 */
	Cf_match_mgr = 
	    (CF_MATCH_MGR *)malloc_data(sizeof(CF_MATCH_MGR)*ALL_CASE_FRAME_MAX, 
					"init_case_analysis_cmm");

	init_mgr_cf(&Work_mgr);
    }
}

/*==================================================================*/
		void clear_case_frame(CASE_FRAME *cf)
/*==================================================================*/
{
    int j;

    for (j = 0; j < CF_ELEMENT_MAX; j++) {
	free(cf->ex[j]);
	free(cf->sm[j]);
	free(cf->ex_list[j][0]);
	free(cf->ex_list[j]);
	free(cf->ex_freq[j]);
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
    {"という", "トイウ", 37},	/* 〜というと? */
    {"時間", "時間", 38},	/* ニ格, 無格で時間であるものを時間という格として扱う */
    {"まで", "マデ", 39},	/* 明示されない格であるが、辞書側の格として表示するために
				   書いておく */
    {"修飾", "修飾", 40},
    {"の", "ノ", 41},		/* 格フレームのノ格 */
    {"が２", "ガ２", 42},
    {"外の関係", "外の関係", 43},
    {"がが", "ガガ", 42},
    {"外の関係", "外ノ関係", 43},	/* for backward compatibility */
    {"は", "ハ", 1},		/* NTT辞書では「ガガ」構文が「ハガ」
				   ※ NTT辞書の「ハ」は1(code)に変換されるが,
				      1は配列順だけで「ガ」に変換される */
    {"未", "未", -3},		/* 格フレームによって動的に割り当てる格を決定する */
    {"＊", "＊", -2},		/* 埋め込み文の被修飾詞 */
    {NULL, NULL, -1}		/* 格助詞の非明示のもの(提題助詞等) */
};

/* ※ 格の最大数を変えたら、PP_NUMBER(const.h)を変えること */

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

char *pp_code_to_kstr_in_context(CF_PRED_MGR *cpm_ptr, int num)
{
    if ((cpm_ptr->cf.type_flag && MatchPP(num, "φ")) || cpm_ptr->cf.type == CF_NOUN) {
	return "ノ";
    }   
    return pp_code_to_kstr(num);
}

/*==================================================================*/
		    int MatchPPn(int n, int *list)
/*==================================================================*/
{
    int i;

    if (n < 0) {
	return 0;
    }

    for (i = 0; list[i] != END_M; i++) {
	if (n == list[i]) {
	    return 1;
	}
    }
    return 0;
}

/*==================================================================*/
		     int MatchPP(int n, char *pp)
/*==================================================================*/
{
    if (n < 0) {
	return 0;
    }
    if (str_eq(pp_code_to_kstr(n), pp)) {
	return 1;
    }
    return 0;
}

/*==================================================================*/
		    int MatchPP2(int *n, char *pp)
/*==================================================================*/
{
    int i;

    /* 格の配列の中に調べたい格があるかどうか */

    if (n < 0) {
	return 0;
    }

    for (i = 0; *(n+i) != END_M; i++) {
	if (str_eq(pp_code_to_kstr(*(n+i)), pp)) {
	    return 1;
	}
    }
    return 0;
}

/*==================================================================*/
		 int CheckCfAdjacent(CASE_FRAME *cf)
/*==================================================================*/
{
    int i;
    for (i = 0; i < cf->element_num; i++) {
	if (cf->adjacent[i] && 
	    MatchPP(cf->pp[i][0], "修飾")) {
	    return FALSE;
	}
    }
    return TRUE;
}

/*==================================================================*/
	  int CheckCfClosest(CF_MATCH_MGR *cmm, int closest)
/*==================================================================*/
{
    return cmm->cf_ptr->adjacent[cmm->result_lists_d[0].flag[closest]];
}

/*==================================================================*/
double find_best_cf(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr, int closest, int decide)
/*==================================================================*/
{
    int i, j, frame_num = 0, pat_num;
    CASE_FRAME *cf_ptr = &(cpm_ptr->cf);
    TAG_DATA *b_ptr = cpm_ptr->pred_b_ptr;
    CF_MATCH_MGR tempcmm;

    /* 格要素なしの時の実験 */
    if (cf_ptr->element_num == 0) {
	/* この用言のすべての格フレームの OR、または
	   格フレームが 1 つのときはそれそのもの にする予定 */
	if (b_ptr->cf_num > 1) {
	    for (i = 0; i < b_ptr->cf_num; i++) {
		if ((b_ptr->cf_ptr+i)->etcflag & CF_SUM) {
		    (Cf_match_mgr + frame_num++)->cf_ptr = b_ptr->cf_ptr + i;
		    break;
		}
	    }
	    /* OR格フレームがないとき
	       「動,形,準」の指定がないことがあればこうなる */
	    if (frame_num == 0) {
		(Cf_match_mgr + frame_num++)->cf_ptr = b_ptr->cf_ptr;
	    }
	}
	else {
	    (Cf_match_mgr + frame_num++)->cf_ptr = b_ptr->cf_ptr;
	}
	case_frame_match(cpm_ptr, Cf_match_mgr, OptCFMode, -1);
	cpm_ptr->score = (int)Cf_match_mgr->score;
	cpm_ptr->cmm[0] = *Cf_match_mgr;
	cpm_ptr->result_num = 1;
    }
    else {
	int hiragana_prefer_flag = 0;

	/* 表記がひらがなの場合: 
	   格フレームの表記がひらがなの場合が多ければひらがなの格フレームのみを対象に、
	   ひらがな以外が多ければひらがな以外のみを対象にする */
	if (check_str_type(b_ptr->head_ptr->Goi) == TYPE_HIRAGANA) {
	    int hiragana_count = 0;
	    for (i = 0; i < b_ptr->cf_num; i++) {
		if (check_str_type((b_ptr->cf_ptr + i)->entry) == TYPE_HIRAGANA) {
		    hiragana_count++;
		}
	    }
	    if (2 * hiragana_count > b_ptr->cf_num) {
		hiragana_prefer_flag = 1;
	    }
	    else {
		hiragana_prefer_flag = -1;
	    }
	}

	/* 格フレーム設定 */
	for (i = 0; i < b_ptr->cf_num; i++) {
	    /* 格フレームが1個ではないとき */
	    if (b_ptr->cf_num != 1) {
		/* OR の格フレームを除く */
		if ((b_ptr->cf_ptr+i)->etcflag & CF_SUM) {
		    continue;
		}
		/* 直前格が修飾の場合などを除く */
		else if (CheckCfAdjacent(b_ptr->cf_ptr+i) == FALSE) {
		    continue;
		}
		else if ((hiragana_prefer_flag > 0 && check_str_type((b_ptr->cf_ptr + i)->entry) != TYPE_HIRAGANA) || 
			 (hiragana_prefer_flag < 0 && check_str_type((b_ptr->cf_ptr + i)->entry) == TYPE_HIRAGANA)) {
		    continue;
		}
	    }
	    (Cf_match_mgr + frame_num++)->cf_ptr = b_ptr->cf_ptr + i;
	}

	if (frame_num == 0) {
	    cpm_ptr->score = -2;
	    return -2;
	}

	cpm_ptr->result_num = 0;
	for (i = 0; i < frame_num; i++) {

	    /* 選択可能
	       EXAMPLE
	       SEMANTIC_MARKER */

	    /* closest があれば、直前格要素のみのスコアになる */
	    case_frame_match(cpm_ptr, Cf_match_mgr+i, OptCFMode, closest);

	    /* 結果を格納 */
	    cpm_ptr->cmm[cpm_ptr->result_num] = *(Cf_match_mgr+i);

	    /* DEBUG出力用: 下の print_good_crrspnds() で使う Cf_match_mgr のスコアを正規化 */
	    if (OptDisplay == OPT_DEBUG && closest > -1 && !OptEllipsis) {
		pat_num = count_pat_element((Cf_match_mgr+i)->cf_ptr, &((Cf_match_mgr+i)->result_lists_p[0]));
		if (!((Cf_match_mgr+i)->score < 0 || pat_num == 0)) {
		    (Cf_match_mgr+i)->score = (OptCaseFlag & OPT_CASE_USE_PROBABILITY) ? (Cf_match_mgr+i)->pure_score[0] : ((Cf_match_mgr+i)->pure_score[0] / sqrt((double)pat_num));
		}
	    }

	    /* スコア順にソート */
	    for (j = cpm_ptr->result_num - 1; j >= 0; j--) {
		if (cpm_ptr->cmm[j].score < cpm_ptr->cmm[j+1].score || 
		    (cpm_ptr->cmm[j].score != CASE_MATCH_FAILURE_PROB && 
		     cpm_ptr->cmm[j].score == cpm_ptr->cmm[j+1].score && (
			(closest > -1 && 
			 (CheckCfClosest(&(cpm_ptr->cmm[j+1]), closest) == TRUE && 
			  CheckCfClosest(&(cpm_ptr->cmm[j]), closest) == FALSE)) || 
			(closest < 0 && 
			 cpm_ptr->cmm[j].sufficiency < cpm_ptr->cmm[j+1].sufficiency)))) {
		    tempcmm = cpm_ptr->cmm[j];
		    cpm_ptr->cmm[j] = cpm_ptr->cmm[j+1];
		    cpm_ptr->cmm[j+1] = tempcmm;
		}
		else {
		    break;
		}
	    }
	    if (cpm_ptr->result_num < CMM_MAX - 1) {
		cpm_ptr->result_num++;
	    }
	}

	/* スコアが同点の格フレームの個数を設定 */
	if (cpm_ptr->result_num > 0) {
	    double top;
	    int cflag = 0;
	    cpm_ptr->tie_num = 1;
	    top = cpm_ptr->cmm[0].score;
	    if (closest > -1 && 
		CheckCfClosest(&(cpm_ptr->cmm[0]), closest) == TRUE) {
		cflag = 1;
	    }
	    for (i = 1; i < cpm_ptr->result_num; i++) {
		/* score が最高で、
		   直前格要素が格フレームの直前格にマッチしているものがあれば(0番目をチェック)
		   直前格要素が格フレームの直前格にマッチしていることが条件
		   ↓
		   score が最高であることだけにした
		*/
		if (cpm_ptr->cmm[i].score == top) {
/*		if (cpm_ptr->cmm[i].score == top && 
		    (cflag == 0 || CheckCfClosest(&(cpm_ptr->cmm[i]), closest) == TRUE)) { */
		    cpm_ptr->tie_num++;
		}
		else {
		    break;
		}
	    }
	}

	/* とりあえず設定
	   closest > -1: decided 決定用 */
	cpm_ptr->score = (int)cpm_ptr->cmm[0].score;
    }

    /* 文脈解析: 直前格要素のスコアが閾値以上なら格フレームを決定 */
    if (decide) {
	if (OptEllipsis) {
	    if (closest > -1 && cpm_ptr->score > CF_DECIDE_THRESHOLD) {
		if (cpm_ptr->tie_num > 1) {
		    cpm_ptr->decided = CF_CAND_DECIDED;
		}
		else {
		    cpm_ptr->decided = CF_DECIDED;
		    /* exact match して、最高点の格フレームがひとつなら、それだけを表示 */
		    if (cpm_ptr->score == EX_match_exact) {
			cpm_ptr->result_num = 1;
		    }
		}
	    }
	    else if (closest == -1 && cpm_ptr->cf.element_num > 0 && 
		     check_feature(cpm_ptr->pred_b_ptr->f, "用言:形")) {
		cpm_ptr->decided = CF_DECIDED;
	    }
	}
	else if (closest > -1) {
	    cpm_ptr->decided = CF_DECIDED;
	}
    }

    if (cf_ptr->element_num != 0) {
	/* 直前格があるときは直前格のスコアしか考慮されていないので、
	   すべての格のスコアを足して正規化したものにする */
	if (closest > -1) {
	    for (i = 0; i < cpm_ptr->result_num; i++) {
		/* 割り当て失敗のとき(score==-1)は、pure_score は定義されていない */
		/* 入力側に任意格しかなく割り当てがないとき(score==0)は、分子分母ともに0になる */
		pat_num = count_pat_element(cpm_ptr->cmm[i].cf_ptr, &(cpm_ptr->cmm[i].result_lists_p[0]));
		if (cpm_ptr->cmm[i].score < 0 || pat_num == 0) {
		    break;
		}
		cpm_ptr->cmm[i].score = (OptCaseFlag & OPT_CASE_USE_PROBABILITY) ? cpm_ptr->cmm[i].pure_score[0] : (cpm_ptr->cmm[i].pure_score[0] / sqrt((double)pat_num));
	    }
	    /* 直前格スコアが同点の格フレームを、すべてのスコアでsort */
	    for (i = cpm_ptr->tie_num - 1; i >= 1; i--) {
		for (j = i - 1; j >= 0; j--) {
		    if (cpm_ptr->cmm[i].score > cpm_ptr->cmm[j].score) {
			tempcmm = cpm_ptr->cmm[i];
			cpm_ptr->cmm[i] = cpm_ptr->cmm[j];
			cpm_ptr->cmm[j] = tempcmm;
		    }
		}
	    }
	}
	cpm_ptr->score = cpm_ptr->cmm[0].score;
    }

    if (OptDisplay == OPT_DEBUG) {
	print_data_cframe(cpm_ptr, Cf_match_mgr);
	/* print_good_crrspnds(cpm_ptr, Cf_match_mgr, frame_num); */
	for (i = 0; i < cpm_ptr->result_num; i++) {
	    print_crrspnd(cpm_ptr, &cpm_ptr->cmm[i]);
	}
    }

    return cpm_ptr->score;
}

/*==================================================================*/
int get_closest_case_component(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr)
/*==================================================================*/
{
    /* 用言より前にある格要素の中で
       もっとも用言に近いものを探す
       (内部文節は除く: num == -1) 
       対象格: ヲ格, ニ格 */

    int i, min = -1, elem_b_num;

    /* 直前格要素を走査 */
    for (i = 0; i < cpm_ptr->cf.element_num; i++) {
	/* 複合名詞の一部: 直前としない */
	if (cpm_ptr->elem_b_ptr[i]->inum > 0) {
	    return -1;
	}
	/* 「〜を〜に」 */
	else if (cpm_ptr->pred_b_ptr->num == cpm_ptr->elem_b_ptr[i]->num) {
	    return i;
	}
	/* 用言にもっとも近い格要素を探す 
	   <回数>:無格 以外 */
	else if (cpm_ptr->elem_b_num[i] > -2 && /* 省略の格要素じゃない */
		 cpm_ptr->elem_b_ptr[i]->num <= cpm_ptr->pred_b_ptr->num && 
		 min < cpm_ptr->elem_b_ptr[i]->num && 
		 !(MatchPP(cpm_ptr->cf.pp[i][0], "φ") && 
		   check_feature(cpm_ptr->elem_b_ptr[i]->f, "回数"))) {
	    min = cpm_ptr->elem_b_ptr[i]->num;
	    elem_b_num = i;
	}
    }

    /* 1. ヲ格, ニ格であるとき
       2. <主体>にマッチしない 1, 2 以外の格 (MatchPP(cpm_ptr->cf.pp[elem_b_num][0], "ガ"))
       3. 用言の直前の未格 (副詞がはさまってもよい)
       ★形容詞, 判定詞は?
       check_feature してもよい
       条件廃止: cpm_ptr->cf.pp[elem_b_num][1] == END_M */
    if (min != -1) {
	/* 決定しない:
	   1. 最近格要素が指示詞の場合 ★格だけマッチさせる?
	   2. ガ格で意味素がないとき */
	if (check_feature((sp->tag_data+min)->f, "指示詞") || 
	    (Thesaurus == USE_NTT && 
	     (sp->tag_data+min)->SM_code[0] == '\0' && 
	     MatchPP(cpm_ptr->cf.pp[elem_b_num][0], "ガ"))) {
	    return -2;
	}
	else if ((cpm_ptr->cf.pp[elem_b_num][0] == -1 && /* 未格 */
		  (cpm_ptr->pred_b_ptr->num == min + 1 || 
		   (cpm_ptr->pred_b_ptr->num == min + 2 && /* 副詞がはさまっている場合 */
		    (check_feature((sp->tag_data + min + 1)->f, "副詞") || 
		     check_feature((sp->tag_data + min + 1)->f, "係:連用"))))) || 
		 MatchPP(cpm_ptr->cf.pp[elem_b_num][0], "ヲ") || 
		 MatchPP(cpm_ptr->cf.pp[elem_b_num][0], "ニ") || 
		 (((cpm_ptr->cf.pp[elem_b_num][0] > 0 && 
		    cpm_ptr->cf.pp[elem_b_num][0] < 9) || /* 基本格 */
		   MatchPP(cpm_ptr->cf.pp[elem_b_num][0], "マデ")) && 
		   !cf_match_element(cpm_ptr->cf.sm[elem_b_num], "主体", FALSE))) {
	    cpm_ptr->cf.adjacent[elem_b_num] = TRUE;	/* 直前格のマーク */
	    return elem_b_num;
	}
    }
    return -1;
}

/*==================================================================*/
       static int number_compare(const void *i, const void *j)
/*==================================================================*/
{
    /* sort function */
    return *(const int *)i-*(const int *)j;
}

/*==================================================================*/
	       char *inputcc2num(CF_PRED_MGR *cpm_ptr)
/*==================================================================*/
{
    int i, numbers[CF_ELEMENT_MAX];
    char str[70], token[3], *key;

    for (i = 0; i < cpm_ptr->cf.element_num; i++) {
	numbers[i] = cpm_ptr->elem_b_ptr[i]->num;
    }

    qsort(numbers, cpm_ptr->cf.element_num, sizeof(int), number_compare);

    str[0] = '\0';
    for (i = 0; i < cpm_ptr->cf.element_num; i++) {
	if (i) {
	    sprintf(token, " %d", numbers[i]);
	}
	else {
	    sprintf(token, "%d", numbers[i]);
	}
	strcat(str, token);
    }
    sprintf(token, " %d", cpm_ptr->pred_b_ptr->num);
    strcat(str, token);

    key = strdup(str);
    return key;
}

typedef struct cpm_cache {
    char *key;
    CF_PRED_MGR *cpm;
    struct cpm_cache *next;
} CPM_CACHE;

CPM_CACHE *CPMcache[TBLSIZE];

/*==================================================================*/
			 void InitCPMcache()
/*==================================================================*/
{
    memset(CPMcache, 0, sizeof(CPM_CACHE *)*TBLSIZE);
}

/*==================================================================*/
			 void ClearCPMcache()
/*==================================================================*/
{
    int i;
    CPM_CACHE *ccp, *next;

    for (i = 0; i < TBLSIZE; i++) {
	if (CPMcache[i]) {
	    ccp = CPMcache[i];
	    while (ccp) {
		free(ccp->key);
		clear_case_frame(&(ccp->cpm->cf));
		free(ccp->cpm);
		next = ccp->next;
		free(ccp);
		ccp = next;
	    }
	    CPMcache[i] = NULL;
	}
    }
}

/*==================================================================*/
		void RegisterCPM(CF_PRED_MGR *cpm_ptr)
/*==================================================================*/
{
    int num;
    char *key;
    CPM_CACHE **ccpp;

    key = inputcc2num(cpm_ptr);
    if (key == NULL) {
	return;
    }
    num = hash(key, strlen(key));

    ccpp = &(CPMcache[num]);
    while (*ccpp) {
	ccpp = &((*ccpp)->next);
    }

    *ccpp = (CPM_CACHE *)malloc_data(sizeof(CPM_CACHE), "RegisterCPM");
    (*ccpp)->key = key;
    (*ccpp)->cpm = (CF_PRED_MGR *)malloc_data(sizeof(CF_PRED_MGR), "RegisterCPM");
    init_case_frame(&((*ccpp)->cpm->cf));
    copy_cpm((*ccpp)->cpm, cpm_ptr, 0);
    (*ccpp)->next = NULL;
}

/*==================================================================*/
	     CF_PRED_MGR *CheckCPM(CF_PRED_MGR *cpm_ptr)
/*==================================================================*/
{
    int num;
    char *key;
    CPM_CACHE *ccp;

    key = inputcc2num(cpm_ptr);
    if (key == NULL) {
	return NULL;
    }
    num = hash(key, strlen(key));

    ccp = CPMcache[num];
    while (ccp) {
	if (str_eq(key, ccp->key)) {
	    return ccp->cpm;
	}
	ccp = ccp->next;
    }
    return NULL;
}

/*==================================================================*/
double case_analysis(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr, TAG_DATA *t_ptr)
/*==================================================================*/
{
    /*
                                              戻値
      入力の格要素がない場合                    -3
      格フレームがない場合                      -2
      入力側に必須格が残る場合(解析不成功)      -1
      解析成功                               score (0以上)
    */

    int closest;
    CF_PRED_MGR *cache_ptr;

    /* 初期化 */
    cpm_ptr->pred_b_ptr = t_ptr;
    cpm_ptr->score = -1;
    cpm_ptr->result_num = 0;
    cpm_ptr->tie_num = 0;
    cpm_ptr->cmm[0].cf_ptr = NULL;
    cpm_ptr->decided = CF_UNDECIDED;

    /* 入力文側の格要素設定 */
    set_data_cf_type(cpm_ptr);
    closest = make_data_cframe(sp, cpm_ptr);

    /* 格フレーム解析スキップ
    if (cpm_ptr->cf.element_num == 0) {
	cpm_ptr->cmm[0].cf_ptr = NULL;
	return -3;
    }
    */

    /* cache */
    if (OptAnalysis == OPT_CASE && 
	(cache_ptr = CheckCPM(cpm_ptr))) {
	copy_cpm(cpm_ptr, cache_ptr, 0);
	return cpm_ptr->score;
    }

    /* もっともスコアのよい格フレームを決定する
       文脈解析: 直前格要素がなければ格フレームを決定しない */

    /* 直前格要素がある場合 (closest > -1) のときは格フレームを決定する */
    find_best_cf(sp, cpm_ptr, closest, 1);

    if (OptAnalysis == OPT_CASE) {
	RegisterCPM(cpm_ptr);
    }

    return cpm_ptr->score;
}

/*==================================================================*/
int all_case_analysis(SENTENCE_DATA *sp, TAG_DATA *t_ptr, TOTAL_MGR *t_mgr)
/*==================================================================*/
{
    CF_PRED_MGR *cpm_ptr;
    int i;
    double one_case_point;

    /* 格フレームの有無をチェック: set_pred_caseframe()の条件に従う */
    if (t_ptr->para_top_p != TRUE && 
	t_ptr->cf_num > 0) {

	if (t_mgr->pred_num >= CPM_MAX) {
	    fprintf(stderr, ";; too many predicates in a sentence. (> %d)\n", CPM_MAX);
	    exit(1);
	}

	cpm_ptr = &(t_mgr->cpm[t_mgr->pred_num]);

	one_case_point = case_analysis(sp, cpm_ptr, t_ptr);

	/* 解析不成功(入力側に必須格が残る)場合にその依存構造の解析を
	   やめる場合
	if (one_case_point == -1) return FALSE;
	*/

	t_mgr->score += one_case_point;
	t_mgr->pred_num++;
    }

    for (i = 0; t_ptr->child[i]; i++) {
	if (all_case_analysis(sp, t_ptr->child[i], t_mgr) == FALSE) {
	    return FALSE;
	}
    }

    return TRUE;
}

/*==================================================================*/
      void copy_cf_with_alloc(CASE_FRAME *dst, CASE_FRAME *src)
/*==================================================================*/
{
    int i, j;

    dst->type = src->type;
    dst->element_num = src->element_num;
    for (i = 0; i < src->element_num; i++) {
	dst->oblig[i] = src->oblig[i];
	dst->adjacent[i] = src->adjacent[i];
	for (j = 0; j < PP_ELEMENT_MAX; j++) {
	    dst->pp[i][j] = src->pp[i][j];
	}
	if (src->pp_str[i]) {
	    dst->pp_str[i] = strdup(src->pp_str[i]);
	}
	else {
	    dst->pp_str[i] = NULL;
	}
	if (src->sm[i]) {
	    dst->sm[i] = strdup(src->sm[i]);
	}
	else {
	    dst->sm[i] = NULL;
	}
	if (src->ex[i]) {
	    dst->ex[i] = strdup(src->ex[i]);
	}
	else {
	    dst->ex[i] = NULL;
	}
	if (src->ex_list[i]) {
	    dst->ex_list[i] = (char **)malloc_data(sizeof(char *)*src->ex_size[i], 
						   "copy_cf_with_alloc");
	    dst->ex_freq[i] = (int *)malloc_data(sizeof(int)*src->ex_size[i], 
						 "copy_cf_with_alloc");
	    for (j = 0; j < src->ex_num[i]; j++) {
		dst->ex_list[i][j] = strdup(src->ex_list[i][j]);
		dst->ex_freq[i][j] = src->ex_freq[i][j];
	    }
	}
	else {
	    dst->ex_list[i] = NULL;
	    dst->ex_freq[i] = NULL;
	}
	dst->ex_size[i] = src->ex_size[i];
	dst->ex_num[i] = src->ex_num[i];
	if (src->semantics[i]) {
	    dst->semantics[i] = strdup(src->semantics[i]);
	}
	else {
	    dst->semantics[i] = NULL;
	}
    }
    dst->voice = src->voice;
    dst->cf_address = src->cf_address;
    dst->cf_size = src->cf_size;
    strcpy(dst->cf_id, src->cf_id);
    strcpy(dst->pred_type, src->pred_type);
    strcpy(dst->imi, src->imi);
    dst->etcflag = src->etcflag;
    if (src->feature) {
	dst->feature = strdup(src->feature);
    }
    else {
	dst->feature = NULL;
    }
    if (src->entry) {
	dst->entry = strdup(src->entry);
    }
    else {
	dst->entry = NULL;
    }
    for (i = 0; i < CF_ELEMENT_MAX; i++) {
	dst->samecase[i][0] = src->samecase[i][0];
	dst->samecase[i][1] = src->samecase[i][1];
    }
    dst->cf_similarity = src->cf_similarity;
    /* weight, pred_b_ptr は未設定 */
}

/*==================================================================*/
	    void copy_cf(CASE_FRAME *dst, CASE_FRAME *src)
/*==================================================================*/
{
    int i, j;

    dst->type = src->type;
    dst->type_flag = src->type_flag;
    dst->element_num = src->element_num;
/*    for (i = 0; i < CF_ELEMENT_MAX; i++) { */
    for (i = 0; i < src->element_num; i++) {
	dst->oblig[i] = src->oblig[i];
	dst->adjacent[i] = src->adjacent[i];
	for (j = 0; j < PP_ELEMENT_MAX; j++) {
	    dst->pp[i][j] = src->pp[i][j];
	}
	dst->pp_str[i] = src->pp_str[i];	/* これを使う場合問題あり */
	/* for (j = 0; j < SM_ELEMENT_MAX*SM_CODE_SIZE; j++) {
	    dst->sm[i][j] = src->sm[i][j];
	} */
	if (src->sm[i]) strcpy(dst->sm[i], src->sm[i]);
	if (src->ex[i]) strcpy(dst->ex[i], src->ex[i]);
	strcpy(dst->ex_list[i][0], src->ex_list[i][0]);
	for (j = 0; j < src->ex_num[i]; j++) {
	    dst->ex_freq[i][j] = src->ex_freq[i][j];
	}
	dst->ex_size[i] = src->ex_size[i];
	dst->ex_num[i] = src->ex_num[i];
    }
    dst->voice = src->voice;
    dst->cf_address = src->cf_address;
    dst->cf_size = src->cf_size;
    strcpy(dst->cf_id, src->cf_id);
    strcpy(dst->pred_type, src->pred_type);
    strcpy(dst->imi, src->imi);
    dst->etcflag = src->etcflag;
    dst->feature = src->feature;
    dst->entry = src->entry;
    for (i = 0; i < CF_ELEMENT_MAX; i++) {
	dst->samecase[i][0] = src->samecase[i][0];
	dst->samecase[i][1] = src->samecase[i][1];
    }
    dst->pred_b_ptr = src->pred_b_ptr;
    dst->cf_similarity = src->cf_similarity;
}

/*==================================================================*/
     void copy_cpm(CF_PRED_MGR *dst, CF_PRED_MGR *src, int flag)
/*==================================================================*/
{
    int i;

    if (flag) {
	copy_cf_with_alloc(&dst->cf, &src->cf);
    }
    else {
	copy_cf(&dst->cf, &src->cf);
    }
    dst->pred_b_ptr = src->pred_b_ptr;
    for (i = 0; i < CF_ELEMENT_MAX; i++) {
	dst->elem_b_ptr[i] = src->elem_b_ptr[i];
	dst->elem_b_num[i] = src->elem_b_num[i];
	dst->elem_s_ptr[i] = src->elem_s_ptr[i];
    }
    dst->score = src->score;
    dst->result_num = src->result_num;
    dst->tie_num = src->tie_num;
    for (i = 0; i < CMM_MAX; i++) {
	dst->cmm[i] = src->cmm[i];
    }
    dst->decided = src->decided;
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
	copy_cpm(&dst->cpm[i], &src->cpm[i], 0);
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
    dpnd_info_to_tag(sp, &dpnd);
    make_dpnd_tree(sp);
    bnst_to_tag_tree(sp);
	
    if (OptDisplay == OPT_DEBUG)
	print_kakari(sp, OPT_TREE);

    /* 格解析作業領域の初期化 */
	
    Work_mgr.pssb = Possibility;
    Work_mgr.dpnd = dpnd;
    Work_mgr.score = 0;
    Work_mgr.pred_num = 0;
    Work_mgr.dflt = 0;
    for (i = 0; i < sp->Bnst_num; i++)
	Work_mgr.dflt += dpnd.dflt[i];
    
    /* 格解析呼び出し */

    if (all_case_analysis(sp, sp->tag_data + sp->Tag_num - 1, &Work_mgr) == TRUE)
	Possibility++;
    else
	return;

    /* ここで default との距離のずれ, 提題を処理 */

    for (i = 0; i < sp->Bnst_num - 1; i++) {
	/* ガ格 -> レベル:A (ルールでこの係り受けを許した場合は、
	   ここでコストを与える) */
	if (!(OptCaseFlag & OPT_CASE_USE_PROBABILITY) && 
	    check_feature((sp->bnst_data + i)->f, "係:ガ格") && 
	    check_feature((sp->bnst_data + dpnd.head[i])->f, "レベル:A")) {
	    distance_cost += LEVELA_COST;
	}

	if (dpnd.dflt[i] > 0) {
	    /* 提題 */
	    if (!(OptCaseFlag & OPT_CASE_USE_PROBABILITY) && 
		check_feature((sp->bnst_data + i)->f, "提題")) {
		distance_cost += dpnd.dflt[i];

		/* 提題につられて遠くに係ってしまった文節の距離コスト */
		for (j = 0; j < i - 1; j++) {
		    if (dpnd.head[i] == dpnd.head[j]) {
			for (k = j + 1; k < i; k++) {
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
	    if (!(OptCaseFlag & OPT_CASE_USE_PROBABILITY) && 
		!check_feature((sp->bnst_data + i)->f, "係:連用")) {
		/* 自分に読点がなく、隣の強い用言 (連体以外) を越えているとき */
		if (!check_feature((sp->bnst_data + i)->f, "読点")) {
		    if (dpnd.head[i] > i + 1 && 
			subordinate_level_check("B", sp->bnst_data + i + 1) && 
			(cp = (char *)check_feature((sp->bnst_data + i + 1)->f, "係"))) {
			if (strcmp(cp+3, "連体") && strcmp(cp+3, "連格")) {
			    distance_cost += STRONG_V_COST;
			}
		    }
		}
		/* 自分に読点があり*/
		else {
		    /* 隣に係るとき */
		    if (dpnd.head[i] == i + 1) {
			distance_cost += ADJACENT_TOUTEN_COST;
		    }
		}
	    }

	    /* 確率的: 副詞などのコスト (tentative) */
	    if (OptCaseFlag & OPT_CASE_USE_PROBABILITY) {
		if (check_feature((sp->bnst_data + i)->f, "係:連用") && 
		    !check_feature((sp->bnst_data + i)->f, "用言")) {
		    distance_cost += dpnd.dflt[i]*DISTANCE_STEP;
		}
	    }
	    /* デフォルトとの差 x 2 を距離のコストとする
	       ただし、形容詞を除く連格の場合は x 1 */
	    else {
		if (!check_feature((sp->bnst_data + i)->f, "係:連格") || 
		    check_feature((sp->bnst_data + i)->f, "用言:形")) {
		    distance_cost += dpnd.dflt[i]*DISTANCE_STEP;
		}
		else {
		    distance_cost += dpnd.dflt[i]*RENKAKU_STEP;
		}
	    }
	}		    
    }

    Work_mgr.score -= distance_cost;

    if (!(OptCaseFlag & OPT_CASE_USE_PROBABILITY)) {
	for (i = sp->Bnst_num - 1; i > 0; i--) {
	    /* 文末から用言ごとに提題を処理する */
	    if ((cp = (char *)check_feature((sp->bnst_data + i)->f, "提題受"))) {

		/* topic_slot[0]	時間以外のハ格のスロット
		   topic_slot[1]	「<<時間>>は」のスロット
		   両方とも 1 以下しか許可しない
		*/

		topic_slot[0] = 0;
		topic_slot[1] = 0;
		one_topic_score = 0;

		/* 係り側を探す */
		for (j = i - 1; j >= 0; j--) {
		    if (dpnd.head[j] != i) {
			continue;
		    }
		    if (check_feature((sp->bnst_data + j)->f, "提題")) {
			if (check_feature((sp->bnst_data + j)->f, "時間")) {
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
    }

    if (OptDisplay == OPT_DEBUG) {
	if (OptCaseFlag & OPT_CASE_USE_PROBABILITY) {
	    fprintf(stdout, "■ %d点 (距離減点 %d点)\n", 
		    Work_mgr.score, distance_cost);
	}
	else {
	    fprintf(stdout, "■ %d点 (距離減点 %d点 (%d点) 提題スコア %d点)\n", 
		    Work_mgr.score, distance_cost, Work_mgr.dflt*2, topic_score_sum);
	}
    }
        
    /* 後処理 */

    if (Work_mgr.score > sp->Best_mgr->score ||
	(Work_mgr.score == sp->Best_mgr->score && 
	 compare_dpnd(sp, &Work_mgr, sp->Best_mgr) == TRUE))
	copy_mgr(sp->Best_mgr, &Work_mgr);
}

/*==================================================================*/
      int add_cf_slot(CF_PRED_MGR *cpm_ptr, char *cstr, int num)
/*==================================================================*/
{
    if (cpm_ptr->cmm[0].cf_ptr->element_num >= CF_ELEMENT_MAX) {
	return FALSE;
    }

    _make_ipal_cframe_pp(cpm_ptr->cmm[0].cf_ptr, cstr, cpm_ptr->cmm[0].cf_ptr->element_num, CF_PRED);
    cpm_ptr->cmm[0].result_lists_d[0].flag[num] = cpm_ptr->cmm[0].cf_ptr->element_num;
    cpm_ptr->cmm[0].result_lists_d[0].score[num] = 0;
    cpm_ptr->cmm[0].result_lists_p[0].flag[cpm_ptr->cmm[0].cf_ptr->element_num] = num;
    cpm_ptr->cmm[0].result_lists_p[0].score[cpm_ptr->cmm[0].cf_ptr->element_num] = 0;
    cpm_ptr->cmm[0].cf_ptr->element_num++;

    return TRUE;
}

/*==================================================================*/
    int assign_cf_slot(CF_PRED_MGR *cpm_ptr, int cnum, int num)
/*==================================================================*/
{
    /* 格フレームのその格にすでに対応付けがあれば */
    if (cpm_ptr->cmm[0].result_lists_p[0].flag[cnum] != UNASSIGNED) {
	return FALSE;
    }

    cpm_ptr->cmm[0].result_lists_d[0].flag[num] = cnum;
    cpm_ptr->cmm[0].result_lists_d[0].score[num] = 0;
    cpm_ptr->cmm[0].result_lists_p[0].flag[cnum] = num;
    cpm_ptr->cmm[0].result_lists_p[0].score[cnum] = 0;

    return TRUE;
}

/*==================================================================*/
		int check_ga2_ok(CF_PRED_MGR *cpm_ptr)
/*==================================================================*/
{
    int i;
    for (i = 0; i < cpm_ptr->cmm[0].cf_ptr->element_num; i++) {
	/* 割り当てなしのガ格, ヲ格, ニ格が存在するならば、ガ２不可 */
	if (cpm_ptr->cmm[0].result_lists_p[0].flag[i] == UNASSIGNED && 
	    (MatchPP(cpm_ptr->cmm[0].cf_ptr->pp[i][0], "ガ") || 
	     MatchPP(cpm_ptr->cmm[0].cf_ptr->pp[i][0], "ヲ") || 
	     MatchPP(cpm_ptr->cmm[0].cf_ptr->pp[i][0], "ニ"))) {
	    return 0;
	}
    }
    return 1;
}

/*==================================================================*/
      void decide_voice(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr)
/*==================================================================*/
{
    TAG_DATA *check_b_ptr;

    if (cpm_ptr->cmm[0].cf_ptr->voice == FRAME_ACTIVE) {
	cpm_ptr->pred_b_ptr->voice = 0;
    }
    else {
	cpm_ptr->pred_b_ptr->voice = VOICE_UKEMI;
    }

    /* なくならないように */
    check_b_ptr = cpm_ptr->pred_b_ptr;
    while (check_b_ptr->parent && check_b_ptr->parent->para_top_p == TRUE) {
	check_b_ptr->parent->voice = cpm_ptr->pred_b_ptr->voice;
	check_b_ptr = check_b_ptr->parent;
    }
}

/*==================================================================*/
	   char *make_print_string(TAG_DATA *bp, int flag)
/*==================================================================*/
{
    int i, start = 0, end = 0, length = 0;
    char *ret;

    /*
       flag == 1: 自立語列
       flag == 0: 最後の自立語
    */

    if (flag) {
	/* 先頭をみる */
	for (i = 0; i < bp->mrph_num; i++) {
	    /* 付属の特殊を除く */
	    if (strcmp(Class[(bp->mrph_ptr + i)->Hinshi][0].id, "特殊") || 
		check_feature((bp->mrph_ptr + i)->f, "自立")) {
		start = i;
		break;
	    }
	}

	/* 末尾をみる */
	for (i = bp->mrph_num - 1; i >= start; i--) {
	    /* 特殊, 助詞, 助動詞, 判定詞を除く */
	    if ((strcmp(Class[(bp->mrph_ptr + i)->Hinshi][0].id, "特殊") || 
		 check_feature((bp->mrph_ptr + i)->f, "自立")) && 
		strcmp(Class[(bp->mrph_ptr + i)->Hinshi][0].id, "助詞") && 
		strcmp(Class[(bp->mrph_ptr + i)->Hinshi][0].id, "助動詞") && 
		strcmp(Class[(bp->mrph_ptr + i)->Hinshi][0].id, "判定詞")) {
		end = i;
		break;
	    }
	}

	if (start > end) {
	    start = bp->jiritu_ptr-bp->mrph_ptr;
	    end = bp->settou_num+bp->jiritu_num - 1;
	}

	for (i = start; i <= end; i++) {
	    length += strlen((bp->mrph_ptr + i)->Goi2);
	}
	if (length == 0) {
	    return NULL;
	}
	ret = (char *)malloc_data(length + 1, "make_print_string");
	*ret = '\0';
	for (i = start; i <= end; i++) {
	    strcat(ret, (bp->mrph_ptr + i)->Goi2);
	}
    }
    else {
	ret = strdup(bp->head_ptr->Goi2);
    }
    return ret;
}

/*==================================================================*/
    void record_match_ex(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr)
/*==================================================================*/
{
    int i, num, pos;
    char feature_buffer[DATA_LEN];

    for (i = 0; i < cpm_ptr->cf.element_num; i++) {
	num = cpm_ptr->cmm[0].result_lists_d[0].flag[i];
	if (num != NIL_ASSIGNED && /* 割り当てがある */
	    cpm_ptr->elem_b_ptr[i] && 
	    cpm_ptr->elem_b_num[i] < 0) { /* 省略, 〜は, 連体修飾 */
	    pos = cpm_ptr->cmm[0].result_lists_p[0].pos[num];
	    if (pos == MATCH_NONE || pos == MATCH_SUBJECT) {
		sprintf(feature_buffer, "マッチ用例;%s:%s-%s", 
			pp_code_to_kstr_in_context(cpm_ptr, cpm_ptr->cmm[0].cf_ptr->pp[num][0]), 
			cpm_ptr->elem_b_ptr[i]->head_ptr->Goi, 
			pos == MATCH_NONE ? "NONE" : "SUBJECT");
	    }
	    else {
		sprintf(feature_buffer, "マッチ用例;%s:%s-%s:%d", 
			pp_code_to_kstr_in_context(cpm_ptr, cpm_ptr->cmm[0].cf_ptr->pp[num][0]), 
			cpm_ptr->elem_b_ptr[i]->head_ptr->Goi, 
			cpm_ptr->cmm[0].cf_ptr->ex_list[num][pos], 
			cpm_ptr->cmm[0].result_lists_p[0].score[num]);
	    }
	    assign_cfeature(&(cpm_ptr->pred_b_ptr->f), feature_buffer);
	}
    }
}

/*==================================================================*/
  void after_case_analysis(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr)
/*==================================================================*/
{
    int i, c;

    if (cpm_ptr->score < 0) {
	return;
    }

    /* 未対応の格要素の処理 */

    for (i = 0; i < cpm_ptr->cf.element_num; i++) {
	if (cpm_ptr->cmm[0].result_lists_d[0].flag[i] == NIL_ASSIGNED) {
	    /* 未格, 連格 */
	    if (cpm_ptr->elem_b_num[i] == -1) {
		/* <時間> => 時間 */
		if (check_feature(cpm_ptr->elem_b_ptr[i]->f, "時間")) {
		    if (check_cf_case(cpm_ptr->cmm[0].cf_ptr, "時間") < 0) {
			add_cf_slot(cpm_ptr, "時間", i);
		    }
		}
		/* 二重主語構文の外のガ格 */
		else if (cpm_ptr->elem_b_ptr[i]->num < cpm_ptr->pred_b_ptr->num && 
			 check_feature(cpm_ptr->elem_b_ptr[i]->f, "係:未格") && 
			 cpm_ptr->pred_b_ptr->num != cpm_ptr->elem_b_ptr[i]->num+1 && /* 用言の直前ではない (実は、もうひとつのガ格よりも前にあることを条件にしたい) */
			 check_ga2_ok(cpm_ptr)) {
		    if (check_cf_case(cpm_ptr->cmm[0].cf_ptr, "ガ２") < 0) {
			add_cf_slot(cpm_ptr, "ガ２", i);
		    }
		}
		/* その他 => 外の関係
		   複合名詞の前側: 保留
		   用言直前のノ格: 保留 */
		else if (cpm_ptr->cf.type != CF_NOUN && 
			 !(cpm_ptr->elem_b_ptr[i]->inum > 0 && 
			   cpm_ptr->elem_b_ptr[i]->parent == cpm_ptr->pred_b_ptr) && 
			 cpm_ptr->cf.pp[i][0] != pp_kstr_to_code("未") && 
			 MatchPP2(cpm_ptr->cf.pp[i], "外の関係")) { /* 「外の関係」の可能性あるもの */
		    if ((c = check_cf_case(cpm_ptr->cmm[0].cf_ptr, "外の関係")) < 0) {
			add_cf_slot(cpm_ptr, "外の関係", i);
		    }
		    else {
			assign_cf_slot(cpm_ptr, c, i);
		    }
		}
	    }
	    /* 格は明示されているが、格フレーム側にその格がなかった場合 */
	    /* ★ とりうる格が複数あるとき: ヘ格 */
	    else {
		if (check_cf_case(cpm_ptr->cmm[0].cf_ptr, pp_code_to_kstr(cpm_ptr->cf.pp[i][0])) < 0) {
		    add_cf_slot(cpm_ptr, pp_code_to_kstr(cpm_ptr->cf.pp[i][0]), i);
		}
	    }
	}
    }
}

/*==================================================================*/
 char *make_cc_string(char *word, int tag_n, char *pp_str, int cc_type,
		      int dist, char *sid)
/*==================================================================*/
{
    char *buf;

    buf = (char *)malloc_data(strlen(pp_str) + strlen(word) + strlen(sid) + (dist ? log(dist) : 0) + 11, 
			      "make_cc_string");

    sprintf(buf, "%s/%c/%s/%d/%d/%s", 
	    pp_str, 
	    cc_type == -2 ? 'O' : 	/* 省略 */
	    cc_type == -3 ? 'D' : 	/* 照応 */
	    cc_type == -1 ? 'N' : 'C', 
	    word, 
	    tag_n, 
	    dist, 
	    sid);
    return buf;
}

/*==================================================================*/
void record_case_analysis(SENTENCE_DATA *sp, CF_PRED_MGR *cpm_ptr, 
			  ELLIPSIS_MGR *em_ptr, int lastflag)
/*==================================================================*/
{
    int i, j, num, sent_n, tag_n, dist_n;
    char feature_buffer[DATA_LEN], relation[DATA_LEN], buffer[DATA_LEN], *word, *sid, *cp;
    ELLIPSIS_COMPONENT *ccp;

    /* voice 決定 */
    if (cpm_ptr->pred_b_ptr->voice == VOICE_UNKNOWN) {
	decide_voice(sp, cpm_ptr);
    }

    /* 「格フレーム変化」フラグがついている格フレームを使用した場合 */
    if (cpm_ptr->cmm[0].cf_ptr->etcflag & CF_CHANGE) {
	assign_cfeature(&(cpm_ptr->pred_b_ptr->f), "格フレーム変化");
    }

    /* 入力側の各格要素の記述 */
    for (i = 0; i < cpm_ptr->cf.element_num; i++) {
	/* 省略解析の結果は除く
	   指示詞の解析をする場合は、指示詞を除く */
	if (cpm_ptr->elem_b_num[i] <= -2) {
	    continue;
	}

	num = cpm_ptr->cmm[0].result_lists_d[0].flag[i];

	/* 割り当てなし */
	if (num == NIL_ASSIGNED) {
	    strcpy(relation, "--");
	}
	/* 割り当てられている格 */
	else if (num >= 0) {
	    /* 格フレームに割りあててあるガ２格 */
	    if (MatchPP(cpm_ptr->cmm[0].cf_ptr->pp[num][0], "ガ２")) {
		strcpy(relation, "ガガ");
		/* sprintf(feature_buffer, "%s判定", relation);
		assign_cfeature(&(cpm_ptr->elem_b_ptr[i]->f), feature_buffer); */
	    }
	    else {
		strcpy(relation, 
		       pp_code_to_kstr_in_context(cpm_ptr, cpm_ptr->cmm[0].cf_ptr->pp[num][0]));
	    }
	}
	/* else: UNASSIGNED はないはず */


	/* feature を用言文節に与える */
	word = make_print_string(cpm_ptr->elem_b_ptr[i], 0);
	if (word) {
	    if (cpm_ptr->elem_b_ptr[i]->num >= 0) {
		sprintf(feature_buffer, "格関係%d:%s:%s", 
			cpm_ptr->elem_b_ptr[i]->num, 
			relation, word);
	    }
	    /* 文節内部の要素の場合 */
	    else {
		sprintf(feature_buffer, "格関係%d:%s:%s", 
			cpm_ptr->elem_b_ptr[i]->parent->num, 
			relation, word);
	    }
	    assign_cfeature(&(cpm_ptr->pred_b_ptr->f), feature_buffer);
	    free(word);
	}
    }

    /* 格解析結果 buffer溢れ注意 */
    sprintf(feature_buffer, "格解析結果:%s:", cpm_ptr->cmm[0].cf_ptr->cf_id);
    for (i = 0; i < cpm_ptr->cmm[0].cf_ptr->element_num; i++) {
	num = cpm_ptr->cmm[0].result_lists_p[0].flag[i];
	ccp = em_ptr ? CheckEllipsisComponent(&(em_ptr->cc[cpm_ptr->cmm[0].cf_ptr->pp[i][0]]), 
					      cpm_ptr->cmm[0].cf_ptr->pp_str[i]) : NULL;

	if (i != 0) {
	    strcat(feature_buffer, ";");
	}

	/* 割り当てなし */
	if (num == UNASSIGNED) {
	    /* 割り当てなし */
	    sprintf(buffer, "%s/U/-/-/-/-", 
		    pp_code_to_kstr_in_context(cpm_ptr, cpm_ptr->cmm[0].cf_ptr->pp[i][0]));
	    strcat(feature_buffer, buffer);
	}
	/* 割り当てあり */
	else {
	    /* 例外タグ */
	    if (ccp && ccp->bnst < 0) {
		sprintf(buffer, "%s/E/%s/-/-/-", 
			pp_code_to_kstr_in_context(cpm_ptr, cpm_ptr->cmm[0].cf_ptr->pp[i][0]), 
			ETAG_name[abs(ccp->bnst)]);
		strcat(feature_buffer, buffer);
	    }
	    else {
		/* 省略の場合 (特殊タグ以外) */
		if (ccp && cpm_ptr->elem_b_num[num] <= -2) {
		    sid = ccp->s->KNPSID ? ccp->s->KNPSID + 5 : NULL;
		    dist_n = ccp->dist;
		    sent_n = ccp->s->Sen_num;
		}
		/* 同文内 */
		else {
		    sid = sp->KNPSID ? sp->KNPSID + 5 : NULL;
		    dist_n = 0;
		    sent_n = sp->Sen_num;
		}

		/* 並列の子供 
		   省略時: OK 
		   直接の係り受け時: 未実装(elem_b_ptrが para_top_p) */
		if (cpm_ptr->elem_b_ptr[num]->para_type == PARA_NORMAL && 
		    cpm_ptr->elem_b_ptr[num]->parent && 
		    cpm_ptr->elem_b_ptr[num]->parent->para_top_p) {
		    for (j = 0; cpm_ptr->elem_b_ptr[num]->parent->child[j]; j++) {
			if (cpm_ptr->elem_b_ptr[num] == cpm_ptr->elem_b_ptr[num]->parent->child[j] || /* target */
			    cpm_ptr->elem_b_ptr[num]->parent->child[j]->para_type != PARA_NORMAL || /* 並列ではない */
			    (cpm_ptr->pred_b_ptr->num < cpm_ptr->elem_b_ptr[num]->num && /* 連体修飾の場合は、 */
			     (cpm_ptr->elem_b_ptr[num]->parent->child[j]->num < cpm_ptr->pred_b_ptr->num || /* 用言より前はいけない */
			      cpm_ptr->elem_b_ptr[num]->num < cpm_ptr->elem_b_ptr[num]->parent->child[j]->num))) { /* 新たな並列の子が元の子より後はいけない */
			    continue;
			}
			word = make_print_string(cpm_ptr->elem_b_ptr[num]->parent->child[j], 0);
			cp = make_cc_string(word ? word : "(null)", cpm_ptr->elem_b_ptr[num]->parent->child[j]->num, 
					    pp_code_to_kstr_in_context(cpm_ptr, cpm_ptr->cmm[0].cf_ptr->pp[i][0]), 
					    cpm_ptr->elem_b_num[num], dist_n, sid ? sid : "?");
			strcat(feature_buffer, cp);
			strcat(feature_buffer, ";");
			free(cp);
			if (word) free(word);
		    }
		}

		word = make_print_string(cpm_ptr->elem_b_ptr[num], 0);
		tag_n = cpm_ptr->elem_b_ptr[num]->num;
		cp = make_cc_string(word ? word : "(null)", tag_n, 
				    pp_code_to_kstr_in_context(cpm_ptr, cpm_ptr->cmm[0].cf_ptr->pp[i][0]), 
				    cpm_ptr->elem_b_num[num], dist_n, sid ? sid : "?");
		strcat(feature_buffer, cp);
		free(cp);

		/* 格・省略関係の保存 (文脈解析用) */
		if (OptEllipsis) {
		    RegisterTagTarget(cpm_ptr->pred_b_ptr->head_ptr->Goi, 
				      cpm_ptr->pred_b_ptr->voice, 
				      cpm_ptr->cmm[0].cf_ptr->cf_address, 
				      cpm_ptr->cmm[0].cf_ptr->pp[i][0], 
				      cpm_ptr->cmm[0].cf_ptr->type == CF_NOUN ? cpm_ptr->cmm[0].cf_ptr->pp_str[i] : NULL, 
				      word, sent_n, tag_n, CREL);
		}
		if (word) free(word);
	    }
	}
    }

    assign_cfeature(&(cpm_ptr->pred_b_ptr->f), feature_buffer);
}

/*==================================================================*/
		 char *get_mrph_rep(MRPH_DATA *m_ptr)
/*==================================================================*/
{
    char *cp;

    if (cp = strstr(m_ptr->Imi, "代表表記:")) {
	return cp + 9;
    }
    return NULL;
}

/*==================================================================*/
  void lexical_disambiguation_by_case_analysis(CF_PRED_MGR *cpm_ptr)
/*==================================================================*/
{
    /* 格解析結果から形態素の曖昧性解消を行う */

    char *rep_cp = get_mrph_rep(cpm_ptr->pred_b_ptr->head_ptr);

    /* 現在の形態素代表表記と格フレームの表記が異なる場合のみ変更 */
    if (rep_cp && 
	strncmp(rep_cp, cpm_ptr->cmm[0].cf_ptr->entry, strlen(cpm_ptr->cmm[0].cf_ptr->entry)) && 
	(check_feature(cpm_ptr->pred_b_ptr->head_ptr->f, "原形曖昧") || /* 原形が曖昧な用言 */
	 (check_str_type(cpm_ptr->pred_b_ptr->head_ptr->Goi) == TYPE_HIRAGANA && 
	  check_feature(cpm_ptr->pred_b_ptr->head_ptr->f, "品曖")))) { /* 品曖なひらがな */
	FEATURE *fp;
	MRPH_DATA m;

	fp = cpm_ptr->pred_b_ptr->head_ptr->f;
	while (fp) {
	    if (!strncmp(fp->cp, "ALT-", 4)) {
		sscanf(fp->cp + 4, "%[^-]-%[^-]-%[^-]-%d-%d-%d-%d-%[^\n]", 
		       m.Goi2, m.Yomi, m.Goi, 
		       &m.Hinshi, &m.Bunrui, 
		       &m.Katuyou_Kata, &m.Katuyou_Kei, m.Imi);
		rep_cp = get_mrph_rep(&m);
		/* 選択した格フレームの表記と一致する代表表記をもつ形態素を選択 */
		if (rep_cp && 
		    !strncmp(rep_cp, cpm_ptr->cmm[0].cf_ptr->entry, 
			     strlen(cpm_ptr->cmm[0].cf_ptr->entry))) {
		    char *imip, *cp;

		    /* 現在の形態素をALTに保存 */
		    assign_feature_alt_mrph(&(cpm_ptr->pred_b_ptr->head_ptr->f), 
					    cpm_ptr->pred_b_ptr->head_ptr);

		    strcpy(cpm_ptr->pred_b_ptr->head_ptr->Goi, m.Goi);
		    strcpy(cpm_ptr->pred_b_ptr->head_ptr->Yomi, m.Yomi);
		    cpm_ptr->pred_b_ptr->head_ptr->Hinshi = m.Hinshi;
		    cpm_ptr->pred_b_ptr->head_ptr->Bunrui = m.Bunrui;
		    cpm_ptr->pred_b_ptr->head_ptr->Katuyou_Kata = m.Katuyou_Kata;
		    cpm_ptr->pred_b_ptr->head_ptr->Katuyou_Kei = m.Katuyou_Kei;
		    strcpy(cpm_ptr->pred_b_ptr->head_ptr->Imi, m.Imi);
		    delete_cfeature(&(cpm_ptr->pred_b_ptr->head_ptr->f), fp->cp);

		    /* 意味情報をfeatureへ */
		    if (m.Imi[0] == '\"') { /* 通常 "" で括られている */
			imip = &m.Imi[1];
			if (cp = strchr(imip, '\"')) {
			    *cp = '\0';
			}
		    }
		    else {
			imip = m.Imi;
		    }
		    imi2feature(imip, cpm_ptr->pred_b_ptr->head_ptr);
		    assign_cfeature(&(cpm_ptr->pred_b_ptr->head_ptr->f), "形態素曖昧性解消");
		    break;
		}
	    }
	    fp = fp->next;
	}
    }
}

/*==================================================================*/
       int get_dist_from_work_mgr(BNST_DATA *bp, BNST_DATA *hp)
/*==================================================================*/
{
    int i, dist = 0;

    /* 候補チェック */
    if (Work_mgr.dpnd.check[bp->num].num == -1) {
	return -1;
    }
    for (i = 0; i < Work_mgr.dpnd.check[bp->num].num; i++) {
	if (Work_mgr.dpnd.check[bp->num].pos[i] == hp->num) {
	    dist = ++i;
	    break;
	}
    }
    if (dist == 0) {
	return -1;
    }
    else if (dist > 1) {
	dist = 2;
    }
    return dist;
}

/*====================================================================
                               END
====================================================================*/
