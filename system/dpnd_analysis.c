/*====================================================================

			     依存構造解析

                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

extern char CorpusComment[BNST_MAX][DATA_LEN];
int Possibility;	/* 依存構造の可能性の何番目か */
static int dpndID = 0;

extern FILE  *Infp;
extern FILE  *Outfp;

/*==================================================================*/
    void assign_bnst_feature(BnstRule *r_ptr, int size, int mode)
/*==================================================================*/
{
    int 	i, j;
    BnstRule	*loop_ptr; 
    BNST_DATA	*b_ptr;

    if (mode == LOOP_BREAK) {

	/* breakする場合 : 各文節に各規則を適用 */

	for (i = 0, b_ptr = bnst_data + Bnst_num - 1; i < Bnst_num;
						      i++, b_ptr--)
	    /* 文末の文節から順に処理．文頭からの場合は以下のループになる 
	       for (i = 0, b_ptr = bnst_data; i < Bnst_num; i++, b_ptr++)
	       */
	    for (j = 0, loop_ptr = r_ptr; j < size; j++, loop_ptr++)
		if (regexpbnstrule_match(loop_ptr, b_ptr) == TRUE ) {
		    assign_feature(&(b_ptr->f), &(loop_ptr->f), b_ptr);
		    break;
		}
    } else {

	/* breakしない場合 : 各規則を各文節に適用
	   (規則適用の結果が別の規則に副作用を与える) */

	for (j = 0, loop_ptr = r_ptr; j < size; j++, loop_ptr++)
	    for (i = 0, b_ptr = bnst_data + Bnst_num - 1; i < Bnst_num; 
							  i++, b_ptr--)
		/* 文末の文節から順に処理．文頭からの場合は以下のループになる 
		   for (i = 0, b_ptr = bnst_data; i < Bnst_num; i++, b_ptr++) 
		*/
		if (regexpbnstrule_match(loop_ptr, b_ptr) == TRUE ) {
		    assign_feature(&(b_ptr->f), &(loop_ptr->f), b_ptr);
		}
    }
}

/*==================================================================*/
		       void assign_dpnd_rule()
/*==================================================================*/
{
    int 	i, j;
    BNST_DATA	*b_ptr;
    DpndRule 	*r_ptr;

    for (i = 0, b_ptr = bnst_data; i < Bnst_num; i++, b_ptr++) {
	for (j = 0, r_ptr = DpndRuleArray; j < CurDpndRuleSize; j++, r_ptr++) {

	    if (feature_pattern_match(&(r_ptr->dependant), b_ptr->f, NULL, NULL) 
		== TRUE) {
		b_ptr->dpnd_rule = r_ptr; 
		break;
	    }
	}

	if (b_ptr->dpnd_rule == NULL) {
	    fprintf(stderr, "No DpndRule for %dth bnst (", i);
	    print_feature(b_ptr->f, stderr);
	    fprintf(stderr, ")\n");

	    /* DpndRuleArray[0] はマッチしない時用 */
	    b_ptr->dpnd_rule = DpndRuleArray;
	}
    }
}

/*==================================================================*/
		       void calc_dpnd_matrix()
/*==================================================================*/
{
    int i, j, k, value, first_uke_flag;
    BNST_DATA *k_ptr, *u_ptr;

    for (i = 0; i < Bnst_num; i++) {
	k_ptr = bnst_data + i;
	first_uke_flag = 1;
	for (j = i + 1; j < Bnst_num; j++) {
	    u_ptr = bnst_data + j;
	    Dpnd_matrix[i][j] = 0;
	    for (k = 0; k_ptr->dpnd_rule->dpnd_type[k]; k++) {
		value = feature_pattern_match(&(k_ptr->dpnd_rule->governor[k]),
					      u_ptr->f, k_ptr, u_ptr);
		if (value == TRUE) {
		    Dpnd_matrix[i][j] = (int)k_ptr->dpnd_rule->dpnd_type[k];
		    first_uke_flag = 0;
		    break;
		}
		/* コーパス中での係り受け頻度が一回のとき */
		else if (value == CORPUS_POSSIBILITY_1) {
		    /*
		      if (OptCheck == TRUE && first_uke_flag) {
		      Dpnd_matrix[i][j] = CORPUS_POSSIBILITY_1_FLAG;
		      first_uke_flag = 0;
		      }
		      else
		    */
		    Dpnd_matrix[i][j] = (int)k_ptr->dpnd_rule->dpnd_type[k];
		    break;
		}
	    }
	}
    }
}

/*==================================================================*/
		    int relax_dpnd_matrix(int num)
/*==================================================================*/
{
    /* 係り先がない場合の緩和

       括弧によるマスクは優先し，その制限内で末尾に係れるように変更

       ○ Ａ‥‥「‥‥‥‥‥」‥‥Ｂ (文末)
       ○ ‥‥‥「Ａ‥‥‥Ｂ」‥‥‥ (括弧終)
       ○ ‥‥‥「Ａ‥Ｂ．‥」‥‥‥ (係:文末)
       × Ａ‥‥‥‥Ｂ「‥‥‥‥Ｃ」 (Ｂに係り得るとはしない．
                                      Ｃとの関係は解析で対処)
    */

    int i, j, ok_flag, relax_flag, last_possibility;
  
    relax_flag = FALSE;

    for (i = 0; i < Bnst_num - 1  ; i++) {
	ok_flag = FALSE;
	last_possibility = i;
	for (j = i + 1; j < Bnst_num ; j++) {
	    if (Quote_matrix[i][j]) {
		if (Dpnd_matrix[i][j] > 0) {
		    ok_flag = TRUE;
		    break;
		} else if (check_feature(bnst_data[j].f, "係:文末")) {
		    last_possibility = j;
		    break;
		} else {
		    last_possibility = j;
		}
	    }
	}

	if (ok_flag == FALSE) {
	    if (check_feature(bnst_data[last_possibility].f, "文末") ||
		check_feature(bnst_data[last_possibility].f, "係:文末") ||
		check_feature(bnst_data[last_possibility].f, "括弧終")) {
		Dpnd_matrix[i][last_possibility] = 'R';
		relax_flag = TRUE;
	    }
	}
    }

    return relax_flag;
}

/*==================================================================*/
	 int check_uncertain_d_condition(DPND *dp, int gvnr)
/*==================================================================*/
{
    /* 用言:弱の d の係り受けを許す条件

       ・ 次の可能な係り先(D)が３つ以上後ろ ( d - - D など )
       ・ dに読点がある
       ・ 係り元とdの後ろが同じ格	例) 東京で最初に大学で行われた
       ・ d(係り先)とdの後ろが同じ格	例) 東京で計画中に京都に変更された
    */

    int i, next_D;
    char *dpnd_cp, *gvnr_cp, *next_cp;

    next_D = 0;
    for (i = gvnr + 1; i < Bnst_num ; i++) {
	if (Mask_matrix[dp->pos][i] &&
	    Quote_matrix[dp->pos][i] &&
	    dp->mask[i] &&
	    Dpnd_matrix[dp->pos][i] == 'D') {
	    next_D = i;
	    break;
	}
    }
    dpnd_cp = (char *)check_feature(bnst_data[dp->pos].f, "係");
    gvnr_cp = (char *)check_feature(bnst_data[gvnr].f, "係");
    next_cp = (char *)check_feature(bnst_data[gvnr+1].f, "係");

    if (next_D == 0 ||
	gvnr + 2 < next_D ||
	check_feature(bnst_data[gvnr].f, "読点") ||
	(gvnr + 2 == next_D &&
	 check_feature(bnst_data[gvnr+1].f, "体言") &&
	 ((dpnd_cp && next_cp && !strcmp(dpnd_cp, next_cp)) ||
	  (gvnr_cp && next_cp && !strcmp(gvnr_cp, next_cp))))) {
	/* fprintf(stderr, "%d -> %d OK\n", i, j); */
	return 1;
    } else {
	return 0;
    }
}

/*==================================================================*/
	  int compare_dpnd(TOTAL_MGR *new, TOTAL_MGR *best)
/*==================================================================*/
{
    int i;

    if (Possibility == 1 || new->dflt < best->dflt) {
	return TRUE;
    } else {
	for (i = Bnst_num - 2; i >= 0; i--) {
	    if (new->dpnd.dflt[i] < best->dpnd.dflt[i]) 
	      return TRUE;
	    else if (new->dpnd.dflt[i] > best->dpnd.dflt[i]) 
	      return FALSE;
	}
    }

    fprintf(stderr, "Error in compare_dpnd !!\n");
    exit(1);
}

/*==================================================================*/
		   void dpnd_info_to_bnst(DPND *dp)
/*==================================================================*/
{
    /* 係り受けに関する種々の情報を bnst 構造体に記憶 */

    int		i;
    BNST_DATA	*b_ptr;

    for (i = 0, b_ptr = bnst_data; i < Bnst_num; i++, b_ptr++) {

	if (i == Bnst_num - 1){		/* 最後の文節 */
	    b_ptr->dpnd_head = -1;
	    b_ptr->dpnd_type = 'D';
	} else if (dp->type[i] == 'd' || dp->type[i] == 'R') {
	    b_ptr->dpnd_head = dp->head[i];
	    b_ptr->dpnd_type = 'D';	/* relaxした場合もDに */
	} else {
	    b_ptr->dpnd_head = dp->head[i];
	    b_ptr->dpnd_type = dp->type[i];
	}
	b_ptr->dpnd_dflt = dp->dflt[i];

	strcpy(b_ptr->dpnd_int, "0");	/* 文節係り先内部の位置，今は未処理 */
	strcpy(b_ptr->dpnd_ext, "");	/* 他の文節係り先可能性，今は未処理 */
    }
}

/*==================================================================*/
		       void para_postprocess()
/*==================================================================*/
{
    int i;

    for (i = 0; i < Bnst_num; i++) {
	if ((check_feature((bnst_data + i)->f, "用言:強") ||
	     check_feature((bnst_data + i)->f, "用言:弱")) &&
	    (bnst_data + i)->para_num != -1 &&
	    para_data[(bnst_data + i)->para_num].status != 'x') {
	    
	    assign_cfeature(&((bnst_data + i)->f), "提題受:30");
	}
    }
}

/*==================================================================*/
		    void dpnd_evaluation(DPND dpnd)
/*==================================================================*/
{
    int i, j, k, one_score, score, rentai, vacant_slot_num;
    int topic_score, optional_flag = 0;
    int optional_score = 0, total_optional_score = 0;
    int scase_check[11], ha_check, un_count, pred_p;
    char *cp, *cp2, *buffer;
    BNST_DATA *g_ptr, *d_ptr;

    /* 依存構造だけを評価する場合の関数
       (各文節について，そこに係っている文節の評価点を計算)

       評価基準
       ========
       0. 係り先のdefault位置との差をペナルティに(kakari_uke.rule)

       1. 「〜は」(提題,係:未格)の係り先は優先されるものがある
       		(bnst_etc.ruleで指定，並列のキーは並列解析後プログラムで指定)

       2. 「〜は」は一述語に一つ係ることを優先(時間は別)

       3. すべての格要素は同一表層格が一述語に一つ係ることを優先(ガガは別)

       4. 未格，連体修飾先はガ,ヲ,ニ格の余っているスロット数だけ点数付与
    */

    score = 0;
    for (i = 1; i < Bnst_num; i++) {
	g_ptr = bnst_data + i;

	one_score = 0;
	for (k = 0; k < 11; k++) scase_check[k] = 0;
	ha_check = 0;
	un_count = 0;

	if (check_feature(g_ptr->f, "用言:強") ||
	    check_feature(g_ptr->f, "用言:弱")) {
	    pred_p = 1;
	} else {
	    pred_p = 0;
	}

	for (j = i-1; j >= 0; j--) {
	    d_ptr = bnst_data + j;

	    if (dpnd.head[j] == i) {

		/* 係り先のDEFAULTの位置との差をペナルティに
		     ※ 提題はC,B'を求めて遠くに係ることがあるが，それが
		        他の係り先に影響しないよう,ペナルティに差をつける */

		if (check_feature(d_ptr->f, "提題")) {
		    one_score -= dpnd.dflt[j];
		} else {
		    one_score -= dpnd.dflt[j] * 2;
		}
	    
		/* 読点をもつものが隣にかかることを防ぐ */

		if (j + 1 == i && check_feature(d_ptr->f, "読点")) {
		    one_score -= 5;
		}

		/* 任意格の係り受けがコーパス中に存在するかどうか */
		if ((cp = (char *)check_feature(d_ptr->f, "係")) != NULL) {
		    if (!(OptInhibit & OPT_INHIBIT_OPTIONAL_CASE) && 
			check_optional_case(cp+3) == TRUE) {

			/* 共起頻度による重み付け */
			optional_score = CorpusExampleDependencyCalculation(d_ptr, cp+3, i, &(dpnd.check[j]));

			/* optional_score = corpus_optional_case_comp(d_ptr, cp+3, g_ptr); */

			/* one_score += optional_score*10; */
			/* 距離重み */ /* j が i に係っている */
			/* optional_score += corpus_optional_case_comp(d_ptr, cp+3, g_ptr)*10*(Bnst_num-1-i)/(Bnst_num-1-j); */
			if (optional_score > 0) {
			    dpnd.op[j].flag = TRUE;
			    dpnd.op[j].weight = optional_score;
			    dpnd.op[j].type = cp+3;
			    if (dpnd.comment) {
				buffer = dpnd.comment;
				dpnd.comment = (char *)malloc(strlen(buffer)+strlen(CorpusComment[j])+2);
				strcpy(dpnd.comment, buffer);
				strcat(dpnd.comment, " ");
				strcat(dpnd.comment, CorpusComment[j]);
			    }
			    else {
				dpnd.comment = strdup(CorpusComment[j]);
			    }
			    optional_flag = 1;
			    total_optional_score += optional_score;
			    /* total_optional_score += optional_score*5; */
			}
		    }
		}

		if (pred_p &&
		    (cp = (char *)check_feature(d_ptr->f, "係")) != NULL) {
		    
		    /* 未格 提題(「〜は」)の扱い */

		    if (check_feature(d_ptr->f, "提題") &&
			!strcmp(cp, "係:未格")) {

			/* 文末, 「〜が」など, 並列末, C, B'に係ることを優先 */

			if ((cp2 = (char *)check_feature(g_ptr->f, "提題受")) 
			    != NULL) {
			    sscanf(cp2, "%*[^:]:%d", &topic_score);
			    one_score += topic_score;
			}
			/* else {one_score -= 15;} */

			/* 一つめの提題にだけ点を与える (時間は別)
			     → 複数の提題が同一述語に係ることを防ぐ */

			if (check_feature(d_ptr->f, "時間")) {
			    one_score += 10;
			} else if (ha_check == 0){
			    one_score += 10;
			    ha_check = 1;
			}
		    }

		    k = case2num(cp+3);

		    /* 格要素一般の扱い */

		    /* 未格 : 数えておき，後で空スロットを調べる (時間は別) */

		    if (!strcmp(cp, "係:未格")) {
			if (check_feature(d_ptr->f, "時間")) {
			    one_score += 10;
			} else {
			    un_count++;
			}
		    }

		    /* ノ格 : 体言以外なら break 
		       	      → それより前の格要素には点を与えない．
			      → ノ格がかかればそれより前の格はかからない

			      ※ 「体言」というのは判定詞のこと，ただし
			         文末などでは用言:強:動となっていることも
				 あるので，「体言」でチェック */

		    else if (!strcmp(cp, "係:ノ格")) {
			if (!check_feature(g_ptr->f, "体言")) {
			    one_score += 10;
			    break;
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
		}
	    }
	}

	/* 用言の場合，最終的に未格,ガ格,ヲ格,ニ格,連体修飾に対して
	   ガ格,ヲ格,ニ格のスロット分だけ点数を与える */

	if (pred_p) {

	    /* 連体修飾の場合，係先が
	       ・形式名詞,副詞的名詞
	       ・「予定」,「見込み」など
	       でなければ一つの格要素と考える */

	    if (check_feature(g_ptr->f, "係:連格")) {
		if (check_feature(bnst_data[dpnd.head[i]].f, "外の関係")) {
		    rentai = 0;
		    one_score += 10;	/* 外の関係ならここで加点 */
		} else {
		    rentai = 1;	/* それ以外なら後で空きスロットをチェック */
		}
	    } else {
		rentai = 0;
	    }

	    /* 空いているガ格,ヲ格,ニ格,ガ２ */

	    vacant_slot_num = 0;
	    if ((g_ptr->SCASE_code[case2num("ガ格")]
		 - scase_check[case2num("ガ格")]) == 1) {
		vacant_slot_num ++;
	    }
	    if ((g_ptr->SCASE_code[case2num("ヲ格")]
		 - scase_check[case2num("ヲ格")]) == 1) {
		vacant_slot_num ++;
	    }
	    if ((g_ptr->SCASE_code[case2num("ニ格")]
		 - scase_check[case2num("ニ格")]) == 1 &&
		rentai == 1 &&
		check_feature(g_ptr->f, "用言:強:動")) {
		vacant_slot_num ++;
		/* ニ格は動詞で連体修飾の場合だけ考慮，つまり連体
		   修飾に割り当てるだけで，未格のスロットとはしない */
	    }
	    if ((g_ptr->SCASE_code[case2num("ガ２")]
		 - scase_check[case2num("ガ２")]) == 1) {
		vacant_slot_num ++;
	    }

	    /* 空きスロット分だけ連体修飾，未格にスコアを与える */

	    if ((rentai + un_count) <= vacant_slot_num) 
		one_score += (rentai + un_count) * 10;
	    else 
		one_score += vacant_slot_num * 10;
	}

	score += one_score;

	if (OptDisplay == OPT_DEBUG) {
	    if (i == 1) 
		fprintf(Outfp, "Score:    ");
	    if (pred_p) {
		fprintf(Outfp, "%2d*", one_score);
	    } else {
		fprintf(Outfp, "%2d ", one_score);
	    }
	}
    }

    if (OptDisplay == OPT_DEBUG) {
	fprintf(Outfp, "=%d", score);
	if (optional_flag)
	    fprintf(Outfp, "+%d=%d", total_optional_score, score+total_optional_score);
	fprintf(Outfp, "\n");
    }

    if (OptDisplay == OPT_DEBUG) {
	dpnd_info_to_bnst(&dpnd);
	make_dpnd_tree();
	print_kakari();
    }

    if (score > Best_mgr.score) {
	Best_mgr.dpnd = dpnd;
	Best_mgr.score = score;
	Best_mgr.ID = dpndID;
	Possibility++;
    }

    /* 事例情報を使ったとき */
    if (optional_flag) {
	if (!OptLearn) {
	    score += total_optional_score;
	    if (score > Op_Best_mgr.score) {
		Op_Best_mgr.dpnd = dpnd;
		Op_Best_mgr.score = score;
		Op_Best_mgr.ID = dpndID;
	    }
	}
	else {
	    fprintf(Outfp, ";;;OK 候補 %d %s %d\n", dpndID, KNPSID, score);
	    for (i = 0;i < Bnst_num; i++) {
		if (dpnd.op[i].flag) {
		    fprintf(Outfp, ";;;OK * %d %d %d %s\n", i, dpnd.head[i], dpnd.op[i].weight, dpnd.op[i].type);
		}
	    }
	}
    }
    dpndID++;
}

/*==================================================================*/
		     void decide_dpnd(DPND dpnd)
/*==================================================================*/
{
    int i, count, possibilities[32], default_pos, d_possibility;
    int corpus_possibilities_flag[32], MaskFlag = 0;
    char *cp;
    BNST_DATA *b_ptr;
    
    if (OptDisplay == OPT_DEBUG) {
	if (dpnd.pos == Bnst_num - 1) {
	    fprintf(Outfp, "------");
	    for (i = 0; i < Bnst_num; i++)
	      fprintf(Outfp, "-%02d", i);
	    fputc('\n', Outfp);
	}
	fprintf(Outfp, "In %2d:", dpnd.pos);
	for (i = 0; i < Bnst_num; i++)
	    fprintf(Outfp, " %2d", dpnd.head[i]);
	fputc('\n', Outfp);
    }

    dpnd.pos --;

    /* 文頭まで解析が終わったら評価関数をよぶ */

    if (dpnd.pos == -1) {
	/* 前の文節の係り受けに従う場合 */
	for (i = 0; i < Bnst_num -1; i++)
	    if (dpnd.head[i] < 0) {
		dpnd.head[i] = dpnd.head[i+dpnd.head[i]];
		dpnd.check[i].pos[0] = dpnd.head[i];
	    }

	if (OptAnalysis == OPT_DPND ||
	    OptAnalysis == OPT_CASE2 ||
	    OptAnalysis == OPT_DISC) {
	    dpnd_evaluation(dpnd);
	} 
	else if (OptAnalysis == OPT_CASE) {
	    call_case_analysis(dpnd);
	}
	return;
    }

    b_ptr = bnst_data + dpnd.pos;
    dpnd.f[dpnd.pos] = b_ptr->f;

    /* (前の係りによる)非交差条件の設定 (dpnd.mask が 0 なら係れない) */

    if (dpnd.pos < Bnst_num -2)
	for (i = dpnd.pos + 2; i < dpnd.head[dpnd.pos+1]; i++)
	    dpnd.mask[i] = 0;
    
    /* 並列構造のキー文節, 部分並列の文節<I>
       (すでに行われた並列構造解析の結果をマークするだけ) */

    for (i = dpnd.pos + 1; i < Bnst_num; i++) {
	if (Mask_matrix[dpnd.pos][i] == 2) {
	    dpnd.head[dpnd.pos] = i;
	    dpnd.type[dpnd.pos] = 'P';
	    /* チェック用 */
	    /* 並列の場合は一意に決まっているので、候補を挙げるのは意味がない */
	    dpnd.check[dpnd.pos].num = 1;
	    dpnd.check[dpnd.pos].pos[0] = i;

	    if (OptCheck == TRUE)
		assign_cfeature(&(bnst_data[dpnd.pos].f), "候補:PARA");
	    decide_dpnd(dpnd);
	    return;
	} else if (Mask_matrix[dpnd.pos][i] == 3) {
	    dpnd.head[dpnd.pos] = i;
	    dpnd.type[dpnd.pos] = 'I';

	    dpnd.check[dpnd.pos].num = 1;
	    dpnd.check[dpnd.pos].pos[0] = i;

	    if (OptCheck == TRUE)
		assign_cfeature(&(bnst_data[dpnd.pos].f), "候補:PARA");
	    decide_dpnd(dpnd);
	    return;
	}
    }

    /* 前の文節の係り受けに従う場合  例) 「〜大統領は一日，〜」 */

    if ((cp = (char *)check_feature(b_ptr->f, "係:無格従属")) != NULL) {
        sscanf(cp, "%*[^:]:%*[^:]:%d", &(dpnd.head[dpnd.pos]));
        dpnd.type[dpnd.pos] = 'D';
        dpnd.dflt[dpnd.pos] = 0;
	dpnd.check[dpnd.pos].num = 1;
	if (OptCheck == TRUE)
	    assign_cfeature(&(bnst_data[dpnd.pos].f), "候補:無格従属");
        decide_dpnd(dpnd);
        return;
    }

    /* 通常の係り受け解析 */

    /* 係り先の候補を調べる */
    
    count = 0;
    d_possibility = 1;
    for (i = dpnd.pos + 1; i < Bnst_num; i++) {
	if (Mask_matrix[dpnd.pos][i] &&
	    Quote_matrix[dpnd.pos][i] &&
	    dpnd.mask[i]) {

	    if (d_possibility && Dpnd_matrix[dpnd.pos][i] == 'd') {
		if (check_uncertain_d_condition(&dpnd, i)) {
		    possibilities[count] = i;
		    count++;
		}
		d_possibility = 0;
	    }
	    else if (Dpnd_matrix[dpnd.pos][i] && 
		     Dpnd_matrix[dpnd.pos][i] != 'd') {
		if (Dpnd_matrix[dpnd.pos][i] == CORPUS_POSSIBILITY_1_FLAG)
		    corpus_possibilities_flag[count] = TRUE;
		else
		    corpus_possibilities_flag[count] = FALSE;
		possibilities[count] = i;
		count++;
		d_possibility = 0;
	    }

	    /* バリアのチェック */
	    if (count &&
		b_ptr->dpnd_rule->barrier.fp[0] &&
		feature_pattern_match(&(b_ptr->dpnd_rule->barrier), 
				      bnst_data[i].f,
				      b_ptr, bnst_data + i) == TRUE)
		break;
	}
	else {
	    MaskFlag = 1;
	}
    }

    /* 実際に候補をつくっていく(この関数の再帰的呼び出し) */

    if (count) {

	/* preference は一番近く:1, 二番目:2, 最後:-1
	   default_pos は一番近く:1, 二番目:2, 最後:count に変更 */

	default_pos = (b_ptr->dpnd_rule->preference == -1) ?
	    count: b_ptr->dpnd_rule->preference;

	/* チェック用 */
	if (OptCheck == TRUE) {
	    if (!MaskFlag)
		assign_cfeature(&(bnst_data[dpnd.pos].f), "候補:EXIST");
	    else
		assign_cfeature(&(bnst_data[dpnd.pos].f), "候補:MASK");
	}

	dpnd.check[dpnd.pos].num = count;	/* 候補数 */
	dpnd.check[dpnd.pos].def = default_pos;	/* デフォルトの位置 */
	for (i = 0; i < count; i++) {
	    if (i < BNST_MAX)
		dpnd.check[dpnd.pos].pos[i] = possibilities[i];
	    else {
#ifdef DEBUG
		fprintf(stderr, ";; MAX checks overflowed.\n");
#endif
		break;
	    }
	}

	/* 一意に決定する場合 */

	if (b_ptr->dpnd_rule->barrier.fp[0] == NULL) {
	    if (default_pos <= count) {
		dpnd.head[dpnd.pos] = possibilities[default_pos - 1];
		if (corpus_possibilities_flag[0] == TRUE) {
		    dpnd.flag = CORPUS_POSSIBILITY_1;
		    /*
		      if (dpnd.comment) fprintf(stderr, "Event CORPUS_POSSIBILITY_1 appears over two times\n");
		      dpnd.comment = CorpusComment[dpnd.pos];
		      */
		}
	    } else {
		dpnd.head[dpnd.pos] = possibilities[count - 1];
		/* default_pos が 2 なのに，countが 1 しかない場合 */
		if (corpus_possibilities_flag[count -1] == TRUE) {
		    dpnd.flag = CORPUS_POSSIBILITY_1;
		    /*
		      if (dpnd.comment) fprintf(stderr, "Event CORPUS_POSSIBILITY_1 appears over two times\n");
		      dpnd.comment = CorpusComment[dpnd.pos];
		      */
		}
	    }
	    dpnd.type[dpnd.pos] = Dpnd_matrix[dpnd.pos][dpnd.head[dpnd.pos]];
	    dpnd.dflt[dpnd.pos] = 0;
	    decide_dpnd(dpnd);
	} 

	/* すべての可能性をつくり出す場合 */
	/* 節間の係り受けの場合は一意に決めるべき */

	else {
	    for (i = 0; i < count; i++) {
		dpnd.head[dpnd.pos] = possibilities[i];
		if (corpus_possibilities_flag[i] == TRUE) {
		    dpnd.flag = CORPUS_POSSIBILITY_1;
		    /* 
		       if (dpnd.comment) fprintf(stderr, "Event CORPUS_POSSIBILITY_1 appears over two times\n");
		       dpnd.comment = CorpusComment[dpnd.pos];
		       */
		}
		dpnd.type[dpnd.pos] = Dpnd_matrix[dpnd.pos][dpnd.head[dpnd.pos]];
		dpnd.dflt[dpnd.pos] = abs(default_pos - 1 - i);
		decide_dpnd(dpnd);
	    }
	}
    } 

    /* 係り先がない場合
       文末が並列にマスクされていなければ，文末に係るとする */

    else {
	if (Mask_matrix[dpnd.pos][Bnst_num - 1]) {
	    dpnd.head[dpnd.pos] = Bnst_num - 1;
	    dpnd.type[dpnd.pos] = 'D';
	    dpnd.dflt[dpnd.pos] = 10;
	    dpnd.check[dpnd.pos].num = 1;
	    dpnd.check[dpnd.pos].pos[0] = Bnst_num - 1;
	    if (OptCheck == TRUE)
		assign_cfeature(&(bnst_data[dpnd.pos].f), "候補:NONE");
	    decide_dpnd(dpnd);
	}
    }
}

/*==================================================================*/
		      void when_no_dpnd_struct()
/*==================================================================*/
{
    int i;

    Best_mgr.dpnd.head[Bnst_num - 1] = -1;

    for (i = Bnst_num - 2; i >= 0; i--) {
	Best_mgr.dpnd.head[i] = i + 1;
	Best_mgr.dpnd.type[i] = 'D';
    }

    Best_mgr.score = 0;
}

/*==================================================================*/
		    int detect_dpnd_case_struct()
/*==================================================================*/
{
    int i;
    DPND dpnd;
    
    Best_mgr.score = -10000; /* スコアは「より大きい」時に入れ換えるので，
				初期値は十分小さくしておく */
    Best_mgr.dflt = 0;
    Best_mgr.ID = -1;
    Possibility = 0;
    dpndID = 0;

    Op_Best_mgr.score = -10000;
    Op_Best_mgr.ID = -1;

    /* 係り状態の初期化 */

    for (i = 0; i < Bnst_num; i++) {
	dpnd.head[i] = -1;
	dpnd.dflt[i] = 0;
	dpnd.mask[i] = 1;
	dpnd.check[i].num = -1;
	memset(&(dpnd.op[i]), 0, sizeof(struct _optionalcase));
	dpnd.f[i] = NULL;
    }
    dpnd.pos = Bnst_num - 1;
    dpnd.flag = 0;
    dpnd.comment = NULL;

    /* 依存構造解析 --> 格構造解析 */
    
    decide_dpnd(dpnd);

    /* 依存構造決定後 格解析を行う場合 */

    if (OptAnalysis == OPT_CASE2 ||
	OptAnalysis == OPT_DISC) {	
	Best_mgr.score = -10000;
	call_case_analysis(Best_mgr.dpnd);
    }

    if (Possibility != 0) {
	if (OptAnalysis == OPT_CASE ||
	    OptAnalysis == OPT_CASE2 ||
	    OptAnalysis == OPT_DISC) {
	    /* 格解析の結果を用言文節へ */
	    for (i = 0; i < Best_mgr.pred_num; i++)
		Best_mgr.cpm[i].pred_b_ptr->cpm_ptr = &(Best_mgr.cpm[i]);
	    /* 格解析の結果をfeatureへ */
	    record_case_analysis();
	    /* 主格を feature へ(固有名詞認識処理用)
	    assign_agent();
	    */
	}
	return TRUE;
    } else { 
	return FALSE;
    }
}

/*==================================================================*/
		      void memo_by_program(void)
/*==================================================================*/
{
    /*
     *  プログラムによるメモへの書き込み
     */

    int i;

    /* 緩和をメモに記録する場合

    for (i = 0; i < Bnst_num - 1; i++) {
	if (Best_mgr.dpnd.type[i] == 'd') {
	    strcat(PM_Memo, " 緩和d");
	    sprintf(PM_Memo+strlen(PM_Memo), "(%d)", i);
	} else if (Best_mgr.dpnd.type[i] == 'R') {
	    strcat(PM_Memo, " 緩和R");
	    sprintf(PM_Memo+strlen(PM_Memo), "(%d)", i);
	}
    }
    */

    /* 遠い係り受けをメモに記録する場合

    for (i = 0; i < Bnst_num - 1; i++) {
	if (Best_mgr.dpnd.head[i] > i + 3 &&
	    !check_feature(bnst_data[i].f, "ハ") &&
	    !check_feature(bnst_data[i].f, "読点") &&
	    !check_feature(bnst_data[i].f, "用言:強") &&
	    !check_feature(bnst_data[i].f, "係:ガ格") &&
	    !check_feature(bnst_data[i].f, "用言:無") &&
	    !check_feature(bnst_data[i].f, "並キ") &&
	    !check_feature(bnst_data[i+1].f, "括弧始")) {
	    strcat(PM_Memo, " 遠係");
	    sprintf(PM_Memo+strlen(PM_Memo), "(%d)", i);
	}
    }
    */
}

/*====================================================================
                               END
====================================================================*/
