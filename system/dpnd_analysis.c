/*====================================================================

			     依存構造解析

                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

int Possibility;	/* 依存構造の可能性の何番目か */
static int dpndID = 0;

/*==================================================================*/
	       void assign_dpnd_rule(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int 	i, j;
    BNST_DATA	*b_ptr;
    DpndRule 	*r_ptr;

    for (i = 0, b_ptr = sp->bnst_data; i < sp->Bnst_num; i++, b_ptr++) {
	for (j = 0, r_ptr = DpndRuleArray; j < CurDpndRuleSize; j++, r_ptr++) {

	    if (feature_pattern_match(&(r_ptr->dependant), b_ptr->f, NULL, NULL) 
		== TRUE) {
		b_ptr->dpnd_rule = r_ptr; 
		break;
	    }
	}

	if (b_ptr->dpnd_rule == NULL) {
	    fprintf(stderr, ";; No DpndRule for %dth bnst (", i);
	    print_feature(b_ptr->f, stderr);
	    fprintf(stderr, ")\n");

	    /* DpndRuleArray[0] はマッチしない時用 */
	    b_ptr->dpnd_rule = DpndRuleArray;
	}
    }
}

/*==================================================================*/
	       void calc_dpnd_matrix(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, k, value, first_uke_flag;
    BNST_DATA *k_ptr, *u_ptr;

    for (i = 0; i < sp->Bnst_num; i++) {
	k_ptr = sp->bnst_data + i;
	first_uke_flag = 1;
	for (j = i + 1; j < sp->Bnst_num; j++) {
	    u_ptr = sp->bnst_data + j;
	    Dpnd_matrix[i][j] = 0;
	    for (k = 0; k_ptr->dpnd_rule->dpnd_type[k]; k++) {
		value = feature_pattern_match(&(k_ptr->dpnd_rule->governor[k]),
					      u_ptr->f, k_ptr, u_ptr);
		if (value == TRUE) {
		    Dpnd_matrix[i][j] = (int)k_ptr->dpnd_rule->dpnd_type[k];
		    first_uke_flag = 0;
		    break;
		}
	    }
	}
    }
}

/*==================================================================*/
	       int relax_dpnd_matrix(SENTENCE_DATA *sp)
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

    for (i = 0; i < sp->Bnst_num - 1  ; i++) {
	ok_flag = FALSE;
	last_possibility = i;
	for (j = i + 1; j < sp->Bnst_num ; j++) {
	    if (Quote_matrix[i][j]) {
		if (Dpnd_matrix[i][j] > 0) {
		    ok_flag = TRUE;
		    break;
		} else if (check_feature(sp->bnst_data[j].f, "係:文末")) {
		    last_possibility = j;
		    break;
		} else {
		    last_possibility = j;
		}
	    }
	}

	if (ok_flag == FALSE) {
	    if (check_feature(sp->bnst_data[last_possibility].f, "文末") ||
		check_feature(sp->bnst_data[last_possibility].f, "係:文末") ||
		check_feature(sp->bnst_data[last_possibility].f, "括弧終")) {
		Dpnd_matrix[i][last_possibility] = 'R';
		relax_flag = TRUE;
	    }
	}
    }

    return relax_flag;
}

/*==================================================================*/
int check_uncertain_d_condition(SENTENCE_DATA *sp, DPND *dp, int gvnr)
/*==================================================================*/
{
    /* 後方チ(ェック)の d の係り受けを許す条件

       ・ 次の可能な係り先(D)が３つ以上後ろ ( d - - D など )
       ・ 係り元とdの後ろが同じ格	例) 日本で最初に京都で行われた
       ・ d(係り先)とdの後ろが同じ格	例) 東京で計画中に京都に変更された

       ※ 「dに読点がある」ことでdを係り先とするのは不適切
       例) 「うすい板を木目が直角になるように、何枚もはり合わせたもの。」
    */

    int i, next_D;
    char *dpnd_cp, *gvnr_cp, *next_cp;

    next_D = 0;
    for (i = gvnr + 1; i < sp->Bnst_num ; i++) {
	if (Mask_matrix[dp->pos][i] &&
	    Quote_matrix[dp->pos][i] &&
	    dp->mask[i] &&
	    Dpnd_matrix[dp->pos][i] == 'D') {
	    next_D = i;
	    break;
	}
    }
    dpnd_cp = check_feature(sp->bnst_data[dp->pos].f, "係");
    gvnr_cp = check_feature(sp->bnst_data[gvnr].f, "係");
    if (gvnr < sp->Bnst_num-1) {
	next_cp = check_feature(sp->bnst_data[gvnr+1].f, "係");	
    }
    else {
	next_cp = NULL;
    }

    if (next_D == 0 ||
	gvnr + 2 < next_D ||
	(gvnr + 2 == next_D && gvnr < sp->Bnst_num-1 &&
	 check_feature(sp->bnst_data[gvnr+1].f, "体言") &&
	 ((dpnd_cp && next_cp && !strcmp(dpnd_cp, next_cp)) ||
	  (gvnr_cp && next_cp && !strcmp(gvnr_cp, next_cp))))) {
	/* fprintf(stderr, "%d -> %d OK\n", i, j); */
	return 1;
    } else {
	return 0;
    }
}

/*==================================================================*/
int compare_dpnd(SENTENCE_DATA *sp, TOTAL_MGR *new_mgr, TOTAL_MGR *best_mgr)
/*==================================================================*/
{
    int i;

    return FALSE;

    if (Possibility == 1 || new_mgr->dflt < best_mgr->dflt) {
	return TRUE;
    } else {
	for (i = sp->Bnst_num - 2; i >= 0; i--) {
	    if (new_mgr->dpnd.dflt[i] < best_mgr->dpnd.dflt[i]) 
		return TRUE;
	    else if (new_mgr->dpnd.dflt[i] > best_mgr->dpnd.dflt[i]) 
		return FALSE;
	}
    }

    fprintf(stderr, ";; Error in compare_dpnd !!\n");
    exit(1);
}

/*==================================================================*/
	 void dpnd_info_to_bnst(SENTENCE_DATA *sp, DPND *dp)
/*==================================================================*/
{
    /* 係り受けに関する種々の情報を DPND から BNST_DATA にコピー */

    int		i;
    BNST_DATA	*b_ptr;

    for (i = 0, b_ptr = sp->bnst_data; i < sp->Bnst_num; i++, b_ptr++) {

	if (i == sp->Bnst_num - 1) {		/* 最後の文節 */
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
    }
}

/*==================================================================*/
	void dpnd_info_to_tag_raw(SENTENCE_DATA *sp, DPND *dp)
/*==================================================================*/
{
    /* 係り受けに関する種々の情報を DPND から TAG_DATA にコピー */

    int		i, last_b, offset;
    char	*cp;
    TAG_DATA	*t_ptr;

    for (i = 0, t_ptr = sp->tag_data; i < sp->Tag_num; i++, t_ptr++) {
	/* もっとも近い文節行を記憶 */
	if (t_ptr->bnum >= 0) {
	    last_b = t_ptr->bnum;
	}

	/* 文末 */
	if (i == sp->Tag_num - 1) {
	    t_ptr->dpnd_head = -1;
	    t_ptr->dpnd_type = 'D';
	}
	/* 隣にかける */
	else if (t_ptr->inum != 0) {
	    t_ptr->dpnd_head = t_ptr->num + 1;
	    t_ptr->dpnd_type = 'D';
	}
	/* 文節内最後のタグ単位 (inum == 0) */
	else {
	    if ((!check_feature((sp->bnst_data + last_b)->f, "タグ単位受無視")) &&
		(cp = check_feature((sp->bnst_data + dp->head[last_b])->f, "タグ単位受"))) {
		offset = atoi(cp + 11);
		if (offset > 0 || (sp->bnst_data + dp->head[last_b])->tag_num <= -1 * offset) {
		    offset = 0;
		}
	    }
	    else {
		offset = 0;
	    }

	    t_ptr->dpnd_head = ((sp->bnst_data + dp->head[last_b])->tag_ptr + 
				(sp->bnst_data + dp->head[last_b])->tag_num - 1 + offset)->num;

	    if (dp->type[last_b] == 'd' || dp->type[last_b] == 'R') {
		t_ptr->dpnd_type = 'D';
	    }
	    else {
		t_ptr->dpnd_type = dp->type[last_b];
	    }
	}
    }
}

/*==================================================================*/
	  void dpnd_info_to_tag(SENTENCE_DATA *sp, DPND *dp)
/*==================================================================*/
{
    if (OptInput == OPT_RAW || 
	(OptInput & OPT_INPUT_BNST)) {
	dpnd_info_to_tag_raw(sp, dp);
    }
    else {
	dpnd_info_to_tag_pm(sp);
    }
}

/*==================================================================*/
	       void para_postprocess(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;

    for (i = 0; i < sp->Bnst_num; i++) {
	if (check_feature((sp->bnst_data + i)->f, "用言") &&
	    !check_feature((sp->bnst_data + i)->f, "〜とみられる") &&
	    (sp->bnst_data + i)->para_num != -1 &&
	    sp->para_data[(sp->bnst_data + i)->para_num].status != 'x') {
	    
	    assign_cfeature(&((sp->bnst_data + i)->f), "提題受:30");
	    assign_cfeature(&(((sp->bnst_data + i)->tag_ptr + 
			       (sp->bnst_data + i)->tag_num - 1)->f), "提題受:30");
	}
    }
}

/*==================================================================*/
	  void dpnd_evaluation(SENTENCE_DATA *sp, DPND dpnd)
/*==================================================================*/
{
    int i, j, k, one_score, score, rentai, vacant_slot_num;
    int topic_score;
    int scase_check[SCASE_CODE_SIZE], ha_check, un_count, pred_p;
    char *cp, *cp2, *buffer;
    BNST_DATA *g_ptr, *d_ptr;

    /* 依存構造だけを評価する場合の関数
       (各文節について，そこに係っている文節の評価点を計算)

       評価基準
       ========
       0. 係り先のdefault位置との差をペナルティに(kakari_uke.rule)

       1. 「〜は」(提題,係:未格)の係り先は優先されるものがある
       		(bnst_etc.ruleで指定，並列のキーは並列解析後プログラムで指定)

       2. 「〜は」は一述語に一つ係ることを優先(時間,数量は別)

       3. すべての格要素は同一表層格が一述語に一つ係ることを優先(ガガは別)

       4. 未格，連体修飾先はガ,ヲ,ニ格の余っているスロット数だけ点数付与
    */

    score = 0;
    for (i = 1; i < sp->Bnst_num; i++) {
	g_ptr = sp->bnst_data + i;

	one_score = 0;
	for (k = 0; k < SCASE_CODE_SIZE; k++) scase_check[k] = 0;
	ha_check = 0;
	un_count = 0;

	if (check_feature(g_ptr->f, "用言") ||
	    check_feature(g_ptr->f, "準用言")) {
	    pred_p = 1;
	} else {
	    pred_p = 0;
	}

	for (j = i-1; j >= 0; j--) {
	    d_ptr = sp->bnst_data + j;

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

		if (pred_p &&
		    (cp = check_feature(d_ptr->f, "係")) != NULL) {
		    
		    /* 未格 提題(「〜は」)の扱い */

		    if (check_feature(d_ptr->f, "提題") &&
			!strcmp(cp, "係:未格")) {

			/* 文末, 「〜が」など, 並列末, C, B'に係ることを優先 */

			if ((cp2 = check_feature(g_ptr->f, "提題受")) 
			    != NULL) {
			    sscanf(cp2, "%*[^:]:%d", &topic_score);
			    one_score += topic_score;
			}
			/* else {one_score -= 15;} */

			/* 一つめの提題にだけ点を与える (時間,数量は別)
			     → 複数の提題が同一述語に係ることを防ぐ */

			if (check_feature(d_ptr->f, "時間") ||
			    check_feature(d_ptr->f, "数量")) {
			    one_score += 10;
			} else if (ha_check == 0){
			    one_score += 10;
			    ha_check = 1;
			}
		    }

		    k = case2num(cp+3);

		    /* 格要素一般の扱い */

		    /* 未格 : 数えておき，後で空スロットを調べる (時間,数量は別) */

		    if (!strcmp(cp, "係:未格")) {
			if (check_feature(d_ptr->f, "時間") ||
			    check_feature(d_ptr->f, "数量")) {
			    one_score += 10;
			} else {
			    un_count++;
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
		if (check_feature(sp->bnst_data[dpnd.head[i]].f, "外の関係") || 
		    check_feature(sp->bnst_data[dpnd.head[i]].f, "ルール外の関係")) {
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
		check_feature(g_ptr->f, "用言:動")) {
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
	fprintf(Outfp, "=%d\n", score);
    }

    if (OptDisplay == OPT_DEBUG) {
	dpnd_info_to_bnst(sp, &dpnd);
	if (!(OptExpress & OPT_NOTAG)) {
	    dpnd_info_to_tag(sp, &dpnd); 
	}
	make_dpnd_tree(sp);
	if (!(OptExpress & OPT_NOTAG)) {
	    bnst_to_tag_tree(sp); /* タグ単位の木へ */
	}
	print_kakari(sp, OptExpress & OPT_NOTAG ? OPT_NOTAGTREE : OPT_TREE);
    }

    if (score > sp->Best_mgr->score) {
	sp->Best_mgr->dpnd = dpnd;
	sp->Best_mgr->score = score;
	sp->Best_mgr->ID = dpndID;
	Possibility++;
    }

    dpndID++;
}

/*==================================================================*/
	    void decide_dpnd(SENTENCE_DATA *sp, DPND dpnd)
/*==================================================================*/
{
    int i, count, possibilities[BNST_MAX], default_pos, d_possibility;
    int MaskFlag = 0;
    char *cp;
    BNST_DATA *b_ptr;
    
    if (OptDisplay == OPT_DEBUG) {
	if (dpnd.pos == sp->Bnst_num - 1) {
	    fprintf(Outfp, "------");
	    for (i = 0; i < sp->Bnst_num; i++)
	      fprintf(Outfp, "-%02d", i);
	    fputc('\n', Outfp);
	}
	fprintf(Outfp, "In %2d:", dpnd.pos);
	for (i = 0; i < sp->Bnst_num; i++)
	    fprintf(Outfp, " %2d", dpnd.head[i]);
	fputc('\n', Outfp);
    }

    dpnd.pos --;

    /* 文頭まで解析が終わったら評価関数をよぶ */

    if (dpnd.pos == -1) {
	/* 無格従属: 前の文節の係り受けに従う場合 */
	for (i = 0; i < sp->Bnst_num -1; i++)
	    if (dpnd.head[i] < 0) {
		/* ありえない係り受け */
		if (i >= dpnd.head[i + dpnd.head[i]]) {
		    return;
		}
		dpnd.head[i] = dpnd.head[i + dpnd.head[i]];
		dpnd.check[i].pos[0] = dpnd.head[i];
	    }

	if (OptAnalysis == OPT_DPND ||
	    OptAnalysis == OPT_CASE2) {
	    dpnd_evaluation(sp, dpnd);
	} 
	else if (OptAnalysis == OPT_CASE) {
	    call_case_analysis(sp, dpnd);
	}
	return;
    }

    b_ptr = sp->bnst_data + dpnd.pos;
    dpnd.f[dpnd.pos] = b_ptr->f;

    /* (前の係りによる)非交差条件の設定 (dpnd.mask が 0 なら係れない) */

    if (dpnd.pos < sp->Bnst_num - 2)
	for (i = dpnd.pos + 2; i < dpnd.head[dpnd.pos + 1]; i++)
	    dpnd.mask[i] = 0;
    
    /* 並列構造のキー文節, 部分並列の文節<I>
       (すでに行われた並列構造解析の結果をマークするだけ) */

    for (i = dpnd.pos + 1; i < sp->Bnst_num; i++) {
	if (Mask_matrix[dpnd.pos][i] == 2) {
	    dpnd.head[dpnd.pos] = i;
	    dpnd.type[dpnd.pos] = 'P';
	    /* チェック用 */
	    /* 並列の場合は一意に決まっているので、候補を挙げるのは意味がない */
	    dpnd.check[dpnd.pos].num = 1;
	    dpnd.check[dpnd.pos].pos[0] = i;
	    decide_dpnd(sp, dpnd);
	    return;
	} else if (Mask_matrix[dpnd.pos][i] == 3) {
	    dpnd.head[dpnd.pos] = i;
	    dpnd.type[dpnd.pos] = 'I';

	    dpnd.check[dpnd.pos].num = 1;
	    dpnd.check[dpnd.pos].pos[0] = i;
	    decide_dpnd(sp, dpnd);
	    return;
	}
    }

    /* 前の文節の係り受けに従う場合  例) 「〜大統領は一日，〜」 */

    if ((cp = check_feature(b_ptr->f, "係:無格従属")) != NULL) {
        sscanf(cp, "%*[^:]:%*[^:]:%d", &(dpnd.head[dpnd.pos]));
        dpnd.type[dpnd.pos] = 'D';
        dpnd.dflt[dpnd.pos] = 0;
	dpnd.check[dpnd.pos].num = 1;
        decide_dpnd(sp, dpnd);
        return;
    }

    /* 通常の係り受け解析 */

    /* 係り先の候補を調べる */
    
    count = 0;
    d_possibility = 1;
    for (i = dpnd.pos + 1; i < sp->Bnst_num; i++) {
	if (Mask_matrix[dpnd.pos][i] &&
	    Quote_matrix[dpnd.pos][i] &&
	    dpnd.mask[i]) {

	    if (d_possibility && Dpnd_matrix[dpnd.pos][i] == 'd') {
		if (check_uncertain_d_condition(sp, &dpnd, i)) {
		    possibilities[count] = i;
		    count++;
		}
		d_possibility = 0;
	    }
	    else if (Dpnd_matrix[dpnd.pos][i] && 
		     Dpnd_matrix[dpnd.pos][i] != 'd') {
		possibilities[count] = i;
		count++;
		d_possibility = 0;
	    }

	    /* バリアのチェック */
	    if (count &&
		b_ptr->dpnd_rule->barrier.fp[0] &&
		feature_pattern_match(&(b_ptr->dpnd_rule->barrier), 
				      sp->bnst_data[i].f,
				      b_ptr, sp->bnst_data + i) == TRUE)
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

	dpnd.check[dpnd.pos].num = count;	/* 候補数 */
	dpnd.check[dpnd.pos].def = default_pos;	/* デフォルトの位置 */
	for (i = 0; i < count; i++) {
	    dpnd.check[dpnd.pos].pos[i] = possibilities[i];
	}

	/* 一意に決定する場合 */

	if (b_ptr->dpnd_rule->barrier.fp[0] == NULL || 
	    b_ptr->dpnd_rule->decide) {
	    if (default_pos <= count) {
		dpnd.head[dpnd.pos] = possibilities[default_pos - 1];
	    } else {
		dpnd.head[dpnd.pos] = possibilities[count - 1];
		/* default_pos が 2 なのに，countが 1 しかない場合 */
	    }
	    dpnd.type[dpnd.pos] = Dpnd_matrix[dpnd.pos][dpnd.head[dpnd.pos]];
	    dpnd.dflt[dpnd.pos] = 0;
	    decide_dpnd(sp, dpnd);
	} 

	/* すべての可能性をつくり出す場合 */
	/* 節間の係り受けの場合は一意に決めるべき */

	else {
	    for (i = 0; i < count; i++) {
		dpnd.head[dpnd.pos] = possibilities[i];
		dpnd.type[dpnd.pos] = Dpnd_matrix[dpnd.pos][dpnd.head[dpnd.pos]];
		dpnd.dflt[dpnd.pos] = abs(default_pos - 1 - i);
		decide_dpnd(sp, dpnd);
	    }
	}
    } 

    /* 係り先がない場合
       文末が並列にマスクされていなければ，文末に係るとする */

    else {
	if (Mask_matrix[dpnd.pos][sp->Bnst_num - 1]) {
	    dpnd.head[dpnd.pos] = sp->Bnst_num - 1;
	    dpnd.type[dpnd.pos] = 'D';
	    dpnd.dflt[dpnd.pos] = 10;
	    dpnd.check[dpnd.pos].num = 1;
	    dpnd.check[dpnd.pos].pos[0] = sp->Bnst_num - 1;
	    decide_dpnd(sp, dpnd);
	}
    }
}

/*==================================================================*/
	     void when_no_dpnd_struct(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;

    sp->Best_mgr->dpnd.head[sp->Bnst_num - 1] = -1;

    for (i = sp->Bnst_num - 2; i >= 0; i--) {
	sp->Best_mgr->dpnd.head[i] = i + 1;
	sp->Best_mgr->dpnd.type[i] = 'D';
	sp->Best_mgr->dpnd.check[i].num = 1;
	sp->Best_mgr->dpnd.check[i].pos[0] = i + 1;
    }

    sp->Best_mgr->score = 0;
}

/*==================================================================*/
	       int after_decide_dpnd(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j;
    TAG_DATA *check_b_ptr;
    
    /* 解析済: 構造は与えられたもの1つのみ */
    if (OptInput & OPT_PARSED) {
	Possibility = 1;
    }

    if (Possibility != 0) {
	/* 依存構造決定後 格解析を行う場合 */
	if (OptAnalysis == OPT_CASE2) {
	    sp->Best_mgr->score = -10000;
	    call_case_analysis(sp, sp->Best_mgr->dpnd);
	}

	if (OptAnalysis == OPT_CASE ||
	    OptAnalysis == OPT_CASE2) {
	    /* 格解析の結果を用言文節へ */
	    for (i = 0; i < sp->Best_mgr->pred_num; i++) {
		sp->Best_mgr->cpm[i].pred_b_ptr->cpm_ptr = &(sp->Best_mgr->cpm[i]);
		/* ※ 暫定的
		   並列のときに make_dpnd_tree() を呼び出すと cpm_ptr がなくなるので、
		   ここでコピーしておく */
		check_b_ptr = sp->Best_mgr->cpm[i].pred_b_ptr;
		while (check_b_ptr->parent && check_b_ptr->parent->para_top_p == TRUE && 
		       check_b_ptr->parent->cpm_ptr == NULL) {
		    check_b_ptr->parent->cpm_ptr = &(sp->Best_mgr->cpm[i]);
		    check_b_ptr = check_b_ptr->parent;
		}

		/* 各格要素の親用言を設定
		   ※ 文脈解析のときに格フレームを決定してなくても格解析は行っているので
		      これは成功する */
		for (j = 0; j < sp->Best_mgr->cpm[i].cf.element_num; j++) {
		    /* 省略解析の結果 or 連体修飾は除く */
		    if (sp->Best_mgr->cpm[i].elem_b_num[j] <= -2 || 
			sp->Best_mgr->cpm[i].elem_b_ptr[j]->num > sp->Best_mgr->cpm[i].pred_b_ptr->num) {
			continue;
		    }
		    sp->Best_mgr->cpm[i].elem_b_ptr[j]->pred_b_ptr = sp->Best_mgr->cpm[i].pred_b_ptr;
		}

		/* 格フレームがある場合 */
		if (sp->Best_mgr->cpm[i].result_num != 0 && 
		    sp->Best_mgr->cpm[i].cmm[0].cf_ptr->cf_address != -1 && 
		    (((OptCaseFlag & OPT_CASE_USE_PROBABILITY) && 
		      sp->Best_mgr->cpm[i].cmm[0].score != CASE_MATCH_FAILURE_PROB) || 
		     (!(OptCaseFlag & OPT_CASE_USE_PROBABILITY) && 
		      sp->Best_mgr->cpm[i].cmm[0].score != CASE_MATCH_FAILURE_SCORE))) {
		    /* 文脈解析のときは格フレーム決定している用言についてのみ */
		    if (!OptEllipsis || sp->Best_mgr->cpm[i].decided == CF_DECIDED) {
			if (OptCaseFlag & OPT_CASE_ASSIGN_GA_SUBJ) {
			    assign_ga_subject(sp, &(sp->Best_mgr->cpm[i]));
			}
			after_case_analysis(sp, &(sp->Best_mgr->cpm[i]));
			fix_sm_place(sp, &(sp->Best_mgr->cpm[i]));

			if (OptUseSmfix == TRUE) {
			    specify_sm_from_cf(sp, &(sp->Best_mgr->cpm[i]));
			}

			/* record_match_ex(sp, &(sp->Best_mgr->cpm[i])); 類似度最大マッチの用例を記録 */

			/* 格解析の結果を featureへ */
			record_case_analysis(sp, &(sp->Best_mgr->cpm[i]), NULL, 
					     check_feature(sp->Best_mgr->cpm[i].pred_b_ptr->f, "主節") ? 1 : 0);

			/* 格解析の結果を用いて形態素曖昧性を解消 */
			verb_lexical_disambiguation_by_case_analysis(&(sp->Best_mgr->cpm[i]));
			noun_lexical_disambiguation_by_case_analysis(&(sp->Best_mgr->cpm[i]));
		    }
		    else if (sp->Best_mgr->cpm[i].decided == CF_CAND_DECIDED) {
			if (OptCaseFlag & OPT_CASE_ASSIGN_GA_SUBJ) {
			    assign_ga_subject(sp, &(sp->Best_mgr->cpm[i]));
			}
		    }

		    if (sp->Best_mgr->cpm[i].decided == CF_DECIDED) {
			assign_cfeature(&(sp->Best_mgr->cpm[i].pred_b_ptr->f), "格フレーム決定");
		    }
		}
		/* 格フレームない場合も格解析結果を書く */
		else if (sp->Best_mgr->cpm[i].result_num == 0 || 
			 sp->Best_mgr->cpm[i].cmm[0].cf_ptr->cf_address == -1) {
		    /* 格解析の結果を featureへ */
		    record_case_analysis(sp, &(sp->Best_mgr->cpm[i]), NULL, 
					 check_feature(sp->Best_mgr->cpm[i].pred_b_ptr->f, "主節") ? 1 : 0);
		}
	    }
	}
	return TRUE;
    }
    else { 
	return FALSE;
    }
}

/*==================================================================*/
	    int detect_dpnd_case_struct(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;
    DPND dpnd;

    sp->Best_mgr->score = -10000; /* スコアは「より大きい」時に入れ換えるので，
				    初期値は十分小さくしておく */
    sp->Best_mgr->dflt = 0;
    sp->Best_mgr->ID = -1;
    Possibility = 0;
    dpndID = 0;

    /* 係り状態の初期化 */

    for (i = 0; i < sp->Bnst_num; i++) {
	dpnd.head[i] = -1;
	dpnd.dflt[i] = 0;
	dpnd.mask[i] = 1;
	memset(&(dpnd.check[i]), 0, sizeof(CHECK_DATA));
	dpnd.check[i].num = -1;
	dpnd.f[i] = NULL;
    }
    dpnd.pos = sp->Bnst_num - 1;

    /* 格解析キャッシュの初期化 */
    if (OptAnalysis == OPT_CASE) {
	InitCPMcache();
    }

    /* 依存構造解析 --> 格構造解析 */

    decide_dpnd(sp, dpnd);

    /* 格解析キャッシュの初期化 */
    if (OptAnalysis == OPT_CASE) {
	ClearCPMcache();
    }

    /* 構造決定後の処理 */

    return after_decide_dpnd(sp);
}

/*==================================================================*/
	       void check_candidates(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j;
    TOTAL_MGR *tm = sp->Best_mgr;
    char buffer[DATA_LEN], buffer2[SMALL_DATA_LEN], *cp;

    /* 各文節ごとにチェック用の feature を与える */
    for (i = 0; i < sp->Bnst_num; i++)
	if (tm->dpnd.check[i].num != -1) {
	    /* 係り側 -> 係り先 */
	    sprintf(buffer, "候補");
	    for (j = 0; j < tm->dpnd.check[i].num; j++) {
		/* 候補たち */
		sprintf(buffer2, ":%d", tm->dpnd.check[i].pos[j]);
		if (strlen(buffer)+strlen(buffer2) >= DATA_LEN) {
		    fprintf(stderr, ";; Too long string <%s> (%d) in check_candidates. (%s)\n", 
			    buffer, tm->dpnd.check[i].num, sp->KNPSID ? sp->KNPSID+5 : "?");
		    return;
		}
		strcat(buffer, buffer2);
	    }
	    assign_cfeature(&(sp->bnst_data[i].f), buffer);
	}
}

/*==================================================================*/
	       void memo_by_program(SENTENCE_DATA *sp)
/*==================================================================*/
{
    /*
     *  プログラムによるメモへの書き込み
     */

    /* 緩和をメモに記録する場合
    int i;

    for (i = 0; i < sp->Bnst_num - 1; i++) {
	if (sp->Best_mgr->dpnd.type[i] == 'd') {
	    strcat(PM_Memo, " 緩和d");
	    sprintf(PM_Memo+strlen(PM_Memo), "(%d)", i);
	} else if (sp->Best_mgr->dpnd.type[i] == 'R') {
	    strcat(PM_Memo, " 緩和R");
	    sprintf(PM_Memo+strlen(PM_Memo), "(%d)", i);
	}
    }
    */

    /* 遠い係り受けをメモに記録する場合

    for (i = 0; i < sp->Bnst_num - 1; i++) {
	if (sp->Best_mgr->dpnd.head[i] > i + 3 &&
	    !check_feature(sp->bnst_data[i].f, "ハ") &&
	    !check_feature(sp->bnst_data[i].f, "読点") &&
	    !check_feature(sp->bnst_data[i].f, "用言") &&
	    !check_feature(sp->bnst_data[i].f, "係:ガ格") &&
	    !check_feature(sp->bnst_data[i].f, "用言:無") &&
	    !check_feature(sp->bnst_data[i].f, "並キ") &&
	    !check_feature(sp->bnst_data[i+1].f, "括弧始")) {
	    strcat(PM_Memo, " 遠係");
	    sprintf(PM_Memo+strlen(PM_Memo), "(%d)", i);
	}
    }
    */
}

/*====================================================================
                               END
====================================================================*/
