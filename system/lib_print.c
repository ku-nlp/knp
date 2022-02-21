/*====================================================================

			     出力ルーチン

                                               S.Kurohashi 91. 6.25
                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"
#include "version.h"

char mrph_buffer[SMALL_DATA_LEN];
int Sen_Num = 1; /* -table のときのみ使用する */
int Tag_Num = 1; /* -table のときのみ使用する */

/* for printing Chinese parse tree */
int         bnst_dpnd[BNST_MAX];
int         bnst_level[BNST_MAX];
char*       bnst_word[BNST_MAX];
char*       bnst_pos[BNST_MAX];
char*       bnst_tree[BNST_MAX][TREE_WIDTH_MAX];
char*       bnst_inverse_tree[TREE_WIDTH_MAX][BNST_MAX];

/*==================================================================*/
		 char *pp2mrph(char *pp, int pp_len)
/*==================================================================*/
{
    char *hira_pp;
    int hinsi_id;

    if (pp_len == strlen("ガ２") && !strncmp(pp, "ガ２", pp_len)) {
	pp_len -= strlen("２"); /* ガ２ -> ガ */
    }
    sprintf(mrph_buffer, "%.*s", pp_len, pp);
    hira_pp = katakana2hiragana(mrph_buffer);
    hinsi_id = get_hinsi_id("助詞");

    sprintf(mrph_buffer, "%s %s %s 助詞 %d 格助詞 %d * 0 * 0 NIL", 
	    hira_pp, hira_pp, hira_pp, 
	    hinsi_id, 
	    get_bunrui_id("格助詞", hinsi_id));

    free(hira_pp);
    return mrph_buffer;
}

/*==================================================================*/
	     char pos2symbol(char *hinshi, char *bunrui)
/*==================================================================*/
{
    if (!strcmp(hinshi, "特殊")) return ' ';
    else if (!strcmp(hinshi, "動詞")) return 'v';
    else if (!strcmp(hinshi, "形容詞")) return 'j';
    else if (!strcmp(hinshi, "判定詞")) return 'c';
    else if (!strcmp(hinshi, "助動詞")) return 'x';
    else if (!strcmp(hinshi, "名詞") &&
	     !strcmp(bunrui, "固有名詞")) return 'N';
    else if (!strcmp(hinshi, "名詞") &&
	     !strcmp(bunrui, "人名")) return 'J';
    else if (!strcmp(hinshi, "名詞") &&
	     !strcmp(bunrui, "地名")) return 'C';
    else if (!strcmp(hinshi, "名詞")) return 'n';
    else if (!strcmp(hinshi, "指示詞")) return 'd';
    else if (!strcmp(hinshi, "副詞")) return 'a';
    else if (!strcmp(hinshi, "助詞")) return 'p';
    else if (!strcmp(hinshi, "接続詞")) return 'c';
    else if (!strcmp(hinshi, "連体詞")) return 'm';
    else if (!strcmp(hinshi, "感動詞")) return '!';
    else if (!strcmp(hinshi, "接頭辞")) return 'p';
    else if (!strcmp(hinshi, "接尾辞")) return 's';

    return '?';
}

/*==================================================================*/
		  void print_mrph(MRPH_DATA *m_ptr)
/*==================================================================*/
{
    fprintf(Outfp, "%s %s %s ", m_ptr->Goi2, m_ptr->Yomi, m_ptr->Goi);

    if (Language == JAPANESE) {
	if (m_ptr->Hinshi >= CLASS_num) {
	    fputc('\n', Outfp);
	    fprintf(stderr, ";; Hinshi number is invalid. (%d)\n", m_ptr->Hinshi);
	    exit(1);
	}
	fprintf(Outfp, "%s ", Class[m_ptr->Hinshi][0].id);
    }
    else {
	fprintf(Outfp, "* ");
    }
    fprintf(Outfp, "%d ", m_ptr->Hinshi);
	
    if (Language == JAPANESE && m_ptr->Bunrui) 
	fprintf(Outfp, "%s ", Class[m_ptr->Hinshi][m_ptr->Bunrui].id);
    else
	fprintf(Outfp, "* ");
    fprintf(Outfp, "%d ", m_ptr->Bunrui);
	
    if (Language == JAPANESE && m_ptr->Katuyou_Kata) 
	fprintf(Outfp, "%s ", Type[m_ptr->Katuyou_Kata].name);
    else                    
	fprintf(Outfp, "* ");
    fprintf(Outfp, "%d ", m_ptr->Katuyou_Kata);
    
    if (Language == JAPANESE && m_ptr->Katuyou_Kei) 
	fprintf(Outfp, "%s ", 
		Form[m_ptr->Katuyou_Kata][m_ptr->Katuyou_Kei].name);
    else 
	fprintf(Outfp, "* ");
    fprintf(Outfp, "%d ", m_ptr->Katuyou_Kei);

    fprintf(Outfp, "%s", m_ptr->Imi);
}

/*==================================================================*/
		  void print_mrph_f(MRPH_DATA *m_ptr)
/*==================================================================*/
{
    char yomi_buffer[SMALL_DATA_LEN];

    sprintf(yomi_buffer, "(%s)", m_ptr->Yomi);
    fprintf(Outfp, "%-16.16s%-18.18s %-14.14s",
	    m_ptr->Goi2, yomi_buffer, 
	    Class[m_ptr->Hinshi][m_ptr->Bunrui].id);
    if (m_ptr->Katuyou_Kata)
	fprintf(Outfp, " %-14.14s %-12.12s",
		Type[m_ptr->Katuyou_Kata].name,
		Form[m_ptr->Katuyou_Kata][m_ptr->Katuyou_Kei].name);
}

/*==================================================================*/
	  TAG_DATA *search_nearest_para_child(TAG_DATA *bp)
/*==================================================================*/
{
    int i;

    /* 並列のときに、最後から2番目の要素をかえす */
    if (bp->para_top_p) {
	for (i = 1; bp->child[i]; i++) { /* 0は最後の要素 */
	    if (bp->child[i]->para_type != PARA_NIL) {
		return bp->child[i];
	    }
	}
    }
    return NULL;
}

/*==================================================================*/
		     void print_eos(int eos_flag)
/*==================================================================*/
{
    if (eos_flag) {
	fputs("EOS\n", Outfp);
    }
    else {
	fputs("EOP\n", Outfp);
    }
}

/*==================================================================*/
      void print_tags(SENTENCE_DATA *sp, int flag, int eos_flag)
/*==================================================================*/
{
    /* 現在は常に flag == 1 (0は旧形式出力) */

    int		i, j, count = 0, b_count = 0, case_len, bp_independent_offset = 0, dpnd_head;
    int		t_table[TAG_MAX], b_table[BNST_MAX], t_proj_table[TAG_MAX], t_copula_table[TAG_MAX];
    char	*cp;
    FEATURE	*fp;
    BNST_DATA	*pre_bp = NULL;
    MRPH_DATA	*m_ptr;
    TAG_DATA	*t_ptr, *bp;

    /* ノードの挿入を考慮しながら、基本句、文節の変換テーブルを作成 */
    for (i = 0, t_ptr = sp->tag_data; i < sp->Tag_num; i++, t_ptr++) {
	if (t_ptr->num == -1) {
	    continue; /* 後処理でマージされたタグ */
	}

	/* 追加ノード */
	if (OptRecoverPerson && pre_bp != t_ptr->b_ptr) { /* 文節の切れ目ごとにチェック */
	    fp = (t_ptr->b_ptr->tag_ptr + t_ptr->b_ptr->tag_num - 1)->f; /* headの基本句 */
	    while (fp) { /* featureのloop: featureをチェック */
		if (!strncmp(fp->cp, "格要素-", strlen("格要素-")) && 
		    strstr(fp->cp, ":＃")) { /* tag_after_dpnd_and_case.ruleで使われている */
		    t_copula_table[count] = 0;
		    count++;
		    b_count++;
		}
		fp = fp->next;
	    }
	    pre_bp = t_ptr->b_ptr;
	}
	/* 判定詞(-copula)の基本句を分解するとき */
	if (check_feature(t_ptr->f, "Ｔ基本句分解")) { 
	    t_copula_table[count] = 0;
	    count++;
	    t_copula_table[count] = 1; /* 新しいテーブルにおける基本句分解の位置を記録 */
	}
	else {
	    t_copula_table[count] = 0;
	}

	t_table[t_ptr->num] = count++; /* numを更新しているので使える */

	if (t_ptr->bnum >= 0) { /* 文節行 (bnumを更新しているので使える) */
	    b_table[t_ptr->bnum] = b_count++;
	}
    }

    for (i = 0; i < count; i++) { /* 非交差条件チェック用 */
	t_proj_table[i] = 0;
    }

    count = 0;
    pre_bp = NULL;
    for (i = 0, t_ptr = sp->tag_data; i < sp->Tag_num; i++, t_ptr++) {
	if (t_ptr->num == -1) {
	    continue; /* 後処理でマージされたタグ */
	}
	if (flag == 1) {
	    bp_independent_offset = 0;

	    if (OptExpress == OPT_TABLE)
		fprintf(Outfp, "%%%% LABEL=%d_%db\n", Sen_Num - 1, i + 1);

	    /* 追加ノード */
	    if (OptRecoverPerson && pre_bp != t_ptr->b_ptr) {
		fp = (t_ptr->b_ptr->tag_ptr + t_ptr->b_ptr->tag_num - 1)->f; /* headの基本句 */
		while (fp) { /* featureのloop: featureをチェック */
		    if (!strncmp(fp->cp, "格要素-", strlen("格要素-")) && 
			(cp = strstr(fp->cp, ":＃"))) {
			case_len = cp - fp->cp - strlen("格要素-"); /* 格の部分の長さ */
			cp++; /* ＃の頭 */

			dpnd_head = t_table[(t_ptr->b_ptr->tag_ptr + t_ptr->b_ptr->tag_num - 1)->num]; /* 係り先はhead */
			if (t_proj_table[count] && dpnd_head > t_proj_table[count]) { /* 非交差条件 */
			    dpnd_head = t_proj_table[count];
			}

			if (!strncmp(fp->cp + strlen("格要素-"), "＃", case_len)) {
			    fprintf(Outfp, "* %dD <ノード挿入>\n", b_table[t_ptr->b_ptr->num]);
			    fprintf(Outfp, "+ %dD <ノード挿入>\n", dpnd_head);
			    fprintf(Outfp, "%s %s %s 名詞 6 普通名詞 1 * 0 * 0 NIL\n", cp, cp, cp);
			}
			else {
			    fprintf(Outfp, "* %dD <ノード挿入><係:%.*s格>\n", b_table[t_ptr->b_ptr->num], case_len, fp->cp + strlen("格要素-"));
			    fprintf(Outfp, "+ %dD <ノード挿入><係:%.*s格><解析格:%.*s>\n", dpnd_head, 
				    case_len, fp->cp + strlen("格要素-"), case_len, fp->cp + strlen("格要素-"));
			    fprintf(Outfp, "%s %s %s 名詞 6 普通名詞 1 * 0 * 0 NIL\n", cp, cp, cp);
			    fprintf(Outfp, "%s\n", pp2mrph(fp->cp + strlen("格要素-"), case_len));
			}
			count++;
		    }
		    fp = fp->next;
		}
		pre_bp = t_ptr->b_ptr;
	    }

	    /* 判定詞(-copula)の基本句を分解するとき */
	    if (check_feature(t_ptr->f, "Ｔ基本句分解")) {
		if (t_ptr->bnum >= 0) { /* 文節行 */
		    fprintf(Outfp, "* %d%c", 
			    t_ptr->b_ptr->dpnd_head == -1 ? -1 : b_table[t_ptr->b_ptr->dpnd_head], 
			    t_ptr->b_ptr->dpnd_type);
		    if (t_ptr->b_ptr->f) {
			fputc(' ', Outfp);
			print_feature(t_ptr->b_ptr->f, Outfp);
		    }
		    fputc('\n', Outfp);
		}
		fprintf(Outfp, "+ %dD <判定詞基本句分解><係:隣>\n", t_table[t_ptr->num]);

		for (j = 0, m_ptr = t_ptr->mrph_ptr; j < t_ptr->mrph_num; j++, m_ptr++) {
		    if (check_feature(m_ptr->f, "後処理-基本句始")) {
			break;
		    }
		    print_mrph(m_ptr);
		    if (m_ptr->f) {
			fputc(' ', Outfp);
			print_feature(m_ptr->f, Outfp);
		    }
		    fputc('\n', Outfp);
		    bp_independent_offset++;
		}
		count++;
	    }

	    /* 文節行 */
	    if (bp_independent_offset == 0 && t_ptr->bnum >= 0) {
		if (PrintNum) {
		    fprintf(Outfp, "* %d %d%c", 
			    t_ptr->bnum,
			    t_ptr->b_ptr->dpnd_head == -1 ? -1 : b_table[t_ptr->b_ptr->dpnd_head], 
			    t_ptr->b_ptr->dpnd_type);
		}
		else {
		    fprintf(Outfp, "* %d%c", 
			    t_ptr->b_ptr->dpnd_head == -1 ? -1 : b_table[t_ptr->b_ptr->dpnd_head], 
			    t_ptr->b_ptr->dpnd_type);
		}
		    
		if (t_ptr->b_ptr->f) {
		    fputc(' ', Outfp);
		    print_feature(t_ptr->b_ptr->f, Outfp);
		}
		if (OptExpress == OPT_TABLE)
		    fprintf(Outfp, "<BR><BR>");
		fputc('\n', Outfp);
	    }

	    /* 判定詞分解時: 連体修飾は判定詞の前の名詞に係るように修正 */
	    dpnd_head = t_ptr->dpnd_head == -1 ? -1 : t_table[t_ptr->dpnd_head];
	    if (OptCopula && 
		dpnd_head != -1 && 
		t_copula_table[dpnd_head]) { /* 係り先が判定詞分解 */
		if (t_table[t_ptr->num] < dpnd_head - 1 && 
		    (((check_feature(t_ptr->f, "連体修飾") || 
		       check_feature(t_ptr->f, "係:隣") || 
		       check_feature(t_ptr->f, "係:文節内")) && 
		      (t_ptr->para_type == PARA_NIL || /* 並列のときは最後から2番目の要素のみ修正 */
		       ((bp = search_nearest_para_child(t_ptr->parent)) && t_ptr->num == bp->num))) || 
		     (t_proj_table[t_table[t_ptr->num]] && dpnd_head > t_proj_table[t_table[t_ptr->num]]))) { /* 非交差条件 */
		    dpnd_head--;
		}
	    }
	    if (PrintNum) {
		fprintf(Outfp, "+ %d %d%c", t_ptr->num, dpnd_head, t_ptr->dpnd_type);
	    } 
	    else {
		fprintf(Outfp, "+ %d%c", dpnd_head, t_ptr->dpnd_type);
	    }
	    if (t_ptr->f) {
		fputc(' ', Outfp);
		print_feature(t_ptr->f, Outfp);
	    }
	    if (OptExpress == OPT_TABLE)
		fprintf(Outfp, "<BR><BR>");
	    fputc('\n', Outfp);

	    for (j = t_table[t_ptr->num]; j < dpnd_head; j++) {
		if (!t_proj_table[j] || t_proj_table[j] > dpnd_head) {
		    t_proj_table[j] = dpnd_head;
		}
	    }
	}
	else {
	    fprintf(Outfp, "%c\n", t_ptr->bnum < 0 ? '+' : '*');
	}

	for (j = bp_independent_offset, m_ptr = t_ptr->mrph_ptr + bp_independent_offset; j < t_ptr->mrph_num; j++, m_ptr++) {
	    print_mrph(m_ptr);
	    if (m_ptr->f) {
		fputc(' ', Outfp);
		print_feature(m_ptr->f, Outfp);
	    }
	    if (OptExpress == OPT_TABLE)
		fprintf(Outfp, "<BR><BR>");
	    fputc('\n', Outfp);
	}
	count++;
    }
    print_eos(eos_flag);
}

/*==================================================================*/
	  void print_mrphs(SENTENCE_DATA *sp, int eos_flag)
/*==================================================================*/
{
    int i;
    MRPH_DATA *m_ptr;
    TAG_DATA *t_ptr;
    FEATURE *bp_f = NULL, *bp_copied_f;

    for (i = 0, m_ptr = sp->mrph_data; i < sp->Mrph_num; i++, m_ptr++) {
	/* 基本句行 */
	if (m_ptr->tnum >= 0) {
	    t_ptr = sp->tag_data + m_ptr->tnum;
	    /* 文節行 */
	    if (t_ptr->bnum >= 0) {
		if (PrintNum) {
		    fprintf(Outfp, "* %d %d%c", 
			    t_ptr->bnum,
			    t_ptr->b_ptr->dpnd_head == -1 ? -1 : t_ptr->b_ptr->dpnd_head, 
			    t_ptr->b_ptr->dpnd_type);
		}
		else {
		    fprintf(Outfp, "* %d%c", 
			    t_ptr->b_ptr->dpnd_head == -1 ? -1 : t_ptr->b_ptr->dpnd_head, 
			    t_ptr->b_ptr->dpnd_type);
		}
		    
		if (t_ptr->b_ptr->f) {
		    fputc(' ', Outfp);
		    print_feature(t_ptr->b_ptr->f, Outfp);
		}
		fputc('\n', Outfp);
	    }

	    if (PrintNum) {
		fprintf(Outfp, "+ %d %d%c", 
			m_ptr->tnum, t_ptr->dpnd_head, t_ptr->dpnd_type);
	    }
	    else {
		fprintf(Outfp, "+ %d%c", 
			t_ptr->dpnd_head, t_ptr->dpnd_type);
	    }
		    
	    if (t_ptr->f) {
		fputc(' ', Outfp);
		print_feature(t_ptr->f, Outfp);
		bp_f = t_ptr->f;
	    }
	    fputc('\n', Outfp);
	}

	/* 形態素係り受け行 */
	if (PrintNum) {
	    fprintf(Outfp, "- %d %d%c", m_ptr->num, m_ptr->dpnd_head, m_ptr->dpnd_type);
	} 
	else {
	    fprintf(Outfp, "- %d%c", m_ptr->dpnd_head, m_ptr->dpnd_type);
	}
	/* 形態素featureは以下の形態素行に出力するので省略 */
	fputc('\n', Outfp);

	/* 形態素情報 */
	print_mrph(m_ptr);
	/* 基本句headの形態素に基本句のfeatureを付与 */
	if (m_ptr->out_head_flag) {
	    if (bp_f) {
		fputc(' ', Outfp);
		if (m_ptr->f) { /* 形態素自身のfeature -> bp_fとマージ */
		    bp_copied_f = NULL;
		    copy_feature(&bp_copied_f, bp_f);
		    copy_feature(&bp_copied_f, m_ptr->f); /* bp_f中の正規化代表表記などを上書き */
		    print_feature(bp_copied_f, Outfp);
		    clear_feature(&bp_copied_f);
		}
		else {
		    print_feature(bp_f, Outfp);
		}
		bp_f = NULL;
	    }
	}
	else {
	    fputs(" <係:基本句内>", Outfp);
	    if (m_ptr->f) { /* 形態素自身のfeature */
		print_feature(m_ptr->f, Outfp);
	    }
	}
	fputc('\n', Outfp);
    }
    print_eos(eos_flag);
}

/*==================================================================*/
	void print_mrphs_only(SENTENCE_DATA *sp, int eos_flag)
/*==================================================================*/
{
    int i;

    for (i = 0; i < sp->Mrph_num; i++) {
	print_mrph(sp->mrph_data + i);
	if ((sp->mrph_data + i)->f) {
	    fprintf(Outfp, " ");
	    print_feature((sp->mrph_data + i)->f, Outfp);
	}
	fprintf(Outfp, "\n");
    }
    print_eos(eos_flag);
}

/*==================================================================*/
void print_bnst_with_mrphs(SENTENCE_DATA *sp, int have_dpnd_flag, int eos_flag)
/*==================================================================*/
{
    int		i, j;
    char *cp;
    MRPH_DATA	*m_ptr;
    BNST_DATA	*b_ptr;

    for (i = 0, b_ptr = sp->bnst_data; i < sp->Bnst_num; i++, b_ptr++) {
	if (b_ptr->num == -1) {
	    continue; /* 後処理でマージされた文節 */
	}
	if (have_dpnd_flag == 1) {
	    if (Language == CHINESE && (b_ptr->is_para == 1 || b_ptr->is_para ==2)) {
		fprintf(Outfp, "* %dP", b_ptr->dpnd_head);
	    }
	    else {
		fprintf(Outfp, "* %d%c", b_ptr->dpnd_head, b_ptr->dpnd_type);		
	    }
	    if (b_ptr->f) {
		fprintf(Outfp, " ");
		print_feature(b_ptr->f, Outfp);
	    }
	    fprintf(Outfp, "\n");
	}
	else {
	    fprintf(Outfp, "*\n");
	}

	for (j = 0, m_ptr = b_ptr->mrph_ptr; j < b_ptr->mrph_num; j++, m_ptr++) {
	    print_mrph(m_ptr);
	    if (m_ptr->f) {
		fprintf(Outfp, " ");
		print_feature(m_ptr->f, Outfp);
	    }
	    /* print_mrph_f(m_ptr); */
	    fprintf(Outfp, "\n");
	}
    }
    print_eos(eos_flag);
}

/*==================================================================*/
	void print_all_result(SENTENCE_DATA *sp, int eos_flag)
/*==================================================================*/
{
    if (OptAnalysis == OPT_FILTER) {
	print_mrphs_only(sp, eos_flag);
    }
    else if (OptAnalysis == OPT_BNST) {
	print_bnst_with_mrphs(sp, 0, eos_flag);
    }
    else if (OptNbest == FALSE && !(OptArticle && OptEllipsis)) {
	print_result(sp, 1, eos_flag);
    }
    if (Language == CHINESE) {
	print_tree_for_chinese(sp);
    }
    fflush(Outfp);
}

/*==================================================================*/
		   void _print_bnst(TAG_DATA *ptr)
/*==================================================================*/
{
    int i;

    if (ptr) {
	for (i = 0; i < ptr->mrph_num; i++)
	    fprintf(Outfp, "%s", (ptr->mrph_ptr + i)->Goi2);
    }
    else {
	fprintf(Outfp, "不特定:人");
    }
}

/*==================================================================*/
	 void print_mrph_with_para(MRPH_DATA *ptr, char *cp)
/*==================================================================*/
{
    int i;

    if (cp && ptr) {
	if (OptExpress == OPT_TABLE) {
	    if ( ptr->para_type == PARA_NORMAL ) strcpy(cp, "＜P＞");
	    else if ( ptr->para_type == PARA_INCOMP ) strcpy(cp, "&lt;I&gt;");
	    else			     cp[0] = '\0';
	}
	else {
	    if ( ptr->para_type == PARA_NORMAL ) strcpy(cp, "<P>");
	    else if ( ptr->para_type == PARA_INCOMP ) strcpy(cp, "<I>");
	    else			     cp[0] = '\0';
	}
	if ( ptr->para_top_p == TRUE )
	    strcat(cp, "PARA");
	else {
	    strcpy(cp, ptr->Goi2);
	}
    } else if (cp == NULL && ptr) {
	if ( ptr->para_top_p == TRUE ) {
	    fprintf(Outfp, "PARA");
	} else {
	    if (OptExpress == OPT_TABLE)
		fprintf(Outfp, "%%%% %d %d 1 LABEL=%d_%db align=right style=white-space:nowrap\n", 
			Sen_Num, Tag_Num++, Sen_Num, Tag_Num - 1);
	    fprintf(Outfp, "%s", ptr->Goi2);
	    if (Language == JAPANESE && OptDisplay != OPT_NORMAL && OptDisplay != OPT_SIMPLE) {
		fprintf(Outfp, "%c", 
			pos2symbol(Class[ptr->Hinshi][0].id,
				   Class[ptr->Hinshi][ptr->Bunrui].id));
	    }
	}

	if (OptExpress == OPT_TABLE) {
	    if ( ptr->para_type == PARA_NORMAL ) fprintf(Outfp, "＜P＞");
	    else if ( ptr->para_type == PARA_INCOMP ) fprintf(Outfp, "&lt;I&gt;");
	}
	else {
	    if ( ptr->para_type == PARA_NORMAL ) fprintf(Outfp, "<P>");
	    else if ( ptr->para_type == PARA_INCOMP ) fprintf(Outfp, "<I>");
	}
	if ( ptr->to_para_p == TRUE ) fprintf(Outfp, "(D)");   
    }
}

/*==================================================================*/
		void print_bnst(BNST_DATA *ptr, char *cp)
/*==================================================================*/
{
    int i;

    if (cp && ptr) {
	if (OptExpress == OPT_TABLE) {
	    if ( ptr->para_type == PARA_NORMAL ) strcpy(cp, "＜P＞");
	    else if ( ptr->para_type == PARA_INCOMP ) strcpy(cp, "&lt;I&gt;");
	    else			     cp[0] = '\0';
	}
	else {
	    if ( ptr->para_type == PARA_NORMAL ) strcpy(cp, "<P>");
	    else if ( ptr->para_type == PARA_INCOMP ) strcpy(cp, "<I>");
	    else			     cp[0] = '\0';
	}
	if ( ptr->para_top_p == TRUE )
	  strcat(cp, "PARA");
	else {
	    strcpy(cp, ptr->mrph_ptr->Goi2);
	    for (i = 1; i < ptr->mrph_num; i++) 
	      strcat(cp, (ptr->mrph_ptr + i)->Goi2);
	}
    } else if (cp == NULL && ptr) {
	if ( ptr->para_top_p == TRUE ) {
	    fprintf(Outfp, "PARA");
	} else {
	    if (OptExpress == OPT_TABLE)
		fprintf(Outfp, "%%%% %d %d 1 LABEL=%d_%db align=right style=white-space:nowrap\n", 
			Sen_Num, Tag_Num++, Sen_Num, Tag_Num - 1);
	    for (i = 0; i < ptr->mrph_num; i++) {
		fprintf(Outfp, "%s", (ptr->mrph_ptr + i)->Goi2);
		if (Language == JAPANESE && OptDisplay != OPT_NORMAL && OptDisplay != OPT_SIMPLE) {
		    fprintf(Outfp, "%c", 
			    pos2symbol(Class[(ptr->mrph_ptr + i)->Hinshi]
				       [0].id,
				       Class[(ptr->mrph_ptr + i)->Hinshi]
				       [(ptr->mrph_ptr + i)->Bunrui].id));
		}
	    }
	}

	if (OptExpress == OPT_TABLE) {
	    if ( ptr->para_type == PARA_NORMAL ) fprintf(Outfp, "＜P＞");
	    else if ( ptr->para_type == PARA_INCOMP ) fprintf(Outfp, "&lt;I&gt;");
	}
	else {
	    if ( ptr->para_type == PARA_NORMAL ) fprintf(Outfp, "<P>");
	    else if ( ptr->para_type == PARA_INCOMP ) fprintf(Outfp, "<I>");
	}
	if ( ptr->to_para_p == TRUE ) fprintf(Outfp, "(D)");   
    }
}

/*==================================================================*/
  void print_data2ipal_corr(BNST_DATA *b_ptr, CF_PRED_MGR *cpm_ptr)
/*==================================================================*/
{
    int i, j, elem_num = 0;
    int offset;
    int flag;

    switch (cpm_ptr->cmm[0].cf_ptr->voice) {
      case FRAME_PASSIVE_I:
      case FRAME_CAUSATIVE_WO_NI:
      case FRAME_CAUSATIVE_WO:
      case FRAME_CAUSATIVE_NI:
	offset = 0;
	break;
      default:
	offset = 1;
	break;
    }

    flag = FALSE;
    if (cpm_ptr->elem_b_num[0] == -1) {
	elem_num = 0;
	flag = TRUE;
    }
    if (flag == TRUE) {
	flag = FALSE;
	for (j = 0; j < cpm_ptr->cmm[0].cf_ptr->element_num; j++)
	  if (cpm_ptr->cmm[0].result_lists_p[0].flag[j] == elem_num) {
	      fprintf(Outfp, " N%d", offset + j);
	      flag = TRUE;
	  }
    }
    if (flag == FALSE)
      fprintf(Outfp, " *");

    for (i = 0; b_ptr->child[i]; i++) {
	flag = FALSE;
	for (j = 0; j < cpm_ptr->cf.element_num; j++)
	  if (cpm_ptr->elem_b_num[j] == i) {
	      elem_num = j;
	      flag = TRUE;
	      break;
	  }
	if (flag == TRUE) {
	    flag = FALSE;
	    for (j = 0; j < cpm_ptr->cmm[0].cf_ptr->element_num; j++)
	      if (cpm_ptr->cmm[0].result_lists_p[0].flag[j] == elem_num) {
		  fprintf(Outfp, " N%d", offset + j);
		  flag = TRUE;
	      }
	}
	if (flag == FALSE)
	  fprintf(Outfp, " *");
    }
}

/*==================================================================*/
		void print_bnst_detail(BNST_DATA *ptr)
/*==================================================================*/
{
    int i;
    MRPH_DATA *m_ptr;
     
    fputc('(', Outfp);	/* 文節始り */

    if ( ptr->para_top_p == TRUE ) {
	if (ptr->child[1] && 
	    ptr->child[1]->para_key_type == PARA_KEY_N)
	  fprintf(Outfp, "noun_para"); 
	else
	  fprintf(Outfp, "pred_para");
    }
    else {
	fprintf(Outfp, "%d ", ptr->num);

	/* 係り受け情報の表示 (追加:97/10/29) */

	fprintf(Outfp, "(type:%c) ", ptr->dpnd_type);

	fputc('(', Outfp);
	for (i=0, m_ptr=ptr->mrph_ptr; i < ptr->mrph_num; i++, m_ptr++) {
	    fputc('(', Outfp);
	    print_mrph(m_ptr);
	    fprintf(Outfp, " ");
	    print_feature2(m_ptr->f, Outfp);
	    fputc(')', Outfp);
	}
	fputc(')', Outfp);

	fprintf(Outfp, " ");
	print_feature2(ptr->f, Outfp);

	if (OptAnalysis == OPT_DPND ||
	    !check_feature(ptr->f, "用言") ||	/* 用言でない場合 */
	    ptr->cpm_ptr == NULL) { 		/* 解析前 */
	    fprintf(Outfp, " NIL");
	}
	else {
	    fprintf(Outfp, " (");
	    
	    if (ptr->cpm_ptr->cmm[0].cf_ptr == NULL)
		fprintf(Outfp, "-2");	/* 格フレームにENTRYなし */
	    else if ((ptr->cpm_ptr->cmm[0].cf_ptr)->cf_address == -1)
		fprintf(Outfp, "-1");	/* 格要素なし */
	    else {
		fprintf(Outfp, "%s", 
			(ptr->cpm_ptr->cmm[0].cf_ptr)->cf_id);
		switch (ptr->cpm_ptr->cmm[0].cf_ptr->voice) {
		case FRAME_ACTIVE:
		    fprintf(Outfp, " 能動"); break;
		case FRAME_PASSIVE_I:
		    fprintf(Outfp, " 間受"); break;
		case FRAME_PASSIVE_1:
		    fprintf(Outfp, " 直受１"); break;
		case FRAME_PASSIVE_2:
		    fprintf(Outfp, " 直受２"); break;
		case FRAME_CAUSATIVE_WO_NI:
		    fprintf(Outfp, " 使役ヲニ"); break;
		case FRAME_CAUSATIVE_WO:
		    fprintf(Outfp, " 使役ヲ"); break;
		case FRAME_CAUSATIVE_NI:
		    fprintf(Outfp, " 使役ニ"); break;
		case FRAME_CAUSATIVE_PASSIVE:
		    fprintf(Outfp, " 使役&受身"); break;
		case FRAME_POSSIBLE:
		    fprintf(Outfp, " 可能"); break;
		case FRAME_POLITE:
		    fprintf(Outfp, " 尊敬"); break;
		case FRAME_SPONTANE:
		    fprintf(Outfp, " 自発"); break;
		default: break;
		}
		fprintf(Outfp, " (");
		print_data2ipal_corr(ptr, ptr->cpm_ptr);
		fprintf(Outfp, ")");
	    }
	    fprintf(Outfp, ")");

	    /* ------------変更:述語素, 格形式を出力-----------------
	    if (ptr->cpm_ptr != NULL &&
		ptr->cpm_ptr->cmm[0].cf_ptr != NULL &&
		(ptr->cpm_ptr->cmm[0].cf_ptr)->cf_address != -1) {
		get_ipal_frame(i_ptr, 
			       (ptr->cpm_ptr->cmm[0].cf_ptr)->cf_address);
		if (i_ptr->DATA[i_ptr->jyutugoso]) {
		    fprintf(Outfp, " 述語素 %s", 
			    i_ptr->DATA+i_ptr->jyutugoso);
		} else {
		    fprintf(Outfp, " 述語素 nil");
		}
		fprintf(Outfp, " 格形式 (");
		for (j=0; *((i_ptr->DATA)+(i_ptr->kaku_keishiki[j])) 
			       != NULL; j++){
		    fprintf(Outfp, " %s", 
			    i_ptr->DATA+i_ptr->kaku_keishiki[j]);
		}
		fprintf(Outfp, ")");
	    }
	    ------------------------------------------------------- */
	}
    }
    fputc(')', Outfp);	/* 文節終わり */
}

/*==================================================================*/
	     void print_sentence_slim(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;

    init_bnst_tree_property(sp);

    fputc('(', Outfp);
    for ( i=0; i<sp->Bnst_num; i++ )
	print_bnst(&(sp->bnst_data[i]), NULL);
    fputc(')', Outfp);
    fputc('\n', Outfp);
}

/*====================================================================
			       行列表示
====================================================================*/

/*==================================================================*/
void print_M_bnst(SENTENCE_DATA *sp, int b_num, int max_length, int *para_char)
/*==================================================================*/
{
    BNST_DATA *ptr = &(sp->bnst_data[b_num]);
    int i, len, space, comma_p;
    char tmp[BNST_LENGTH_MAX], *cp = tmp;

    if ( ptr->mrph_num == 1 ) {
	strcpy(tmp, ptr->mrph_ptr->Goi2);
	comma_p = FALSE;
    } else {
	strcpy(tmp, ptr->mrph_ptr->Goi2);
	for (i = 1; i < (ptr->mrph_num - 1); i++) 
	  strcat(tmp, (ptr->mrph_ptr + i)->Goi2);

	if (!strcmp(Class[(ptr->mrph_ptr + ptr->mrph_num - 1)->Hinshi][0].id,
		    "特殊") &&
	    !strcmp(Class[(ptr->mrph_ptr + ptr->mrph_num - 1)->Hinshi]
		    [(ptr->mrph_ptr + ptr->mrph_num - 1)->Bunrui].id, 
		    "読点")) {
	    strcat(tmp, ",");
	    comma_p = TRUE;
	} else {
	    strcat(tmp, (ptr->mrph_ptr + ptr->mrph_num - 1)->Goi2);
	    comma_p = FALSE;	      
	}
    }

    space = ptr->para_key_type ?
      max_length-(sp->Bnst_num-b_num-1)*3-2 : max_length-(sp->Bnst_num-b_num-1)*3;
    len = comma_p ? 
      ptr->length - 1 : ptr->length;

    if ( len > space ) {
	if ( (space%2) != (len%2) ) {
	    cp += len + 1 - space;
	    fputc(' ', Outfp);
	} else
	  cp += len - space;
    } else
	for ( i=0; i<space-len; i++ ) fputc(' ', Outfp);

    if ( ptr->para_key_type ) {
	fprintf(Outfp, "%c>", 'a'+ (*para_char));
	(*para_char)++;
    }
    fprintf(Outfp, "%s", cp);
}

/*==================================================================*/
		void print_line(int length, int flag)
/*==================================================================*/
{
    int i;
    for ( i=0; i<(length-1); i++ ) fputc('-', Outfp);
    flag ? fputc(')', Outfp) : fputc('-', Outfp);
    fputc('\n', Outfp);
}

/*==================================================================*/
     void print_matrix(SENTENCE_DATA *sp, int type, int key_pos)
/*==================================================================*/
{
    int i, j, length;
    int over_flag = 0;
    int max_length = 0;
    int para_char = 0;     /* para_key の表示用 */
    PARA_DATA *ptr;

    for ( i=0; i<sp->Bnst_num; i++ )
	for ( j=0; j<sp->Bnst_num; j++ )
	    path_matrix[i][j] = 0;
    
    /* パスのマーク付け(PARA) */

    if (type == PRINT_PARA) {
	for ( i=0; i<sp->Para_num; i++ ) {
	    ptr = &sp->para_data[i];

	    if (ptr->max_score < 0.0) continue;
	    /* statusがxでもスコアがあれば参考のため表示 */

	    for ( j=ptr->key_pos+1; j<=ptr->jend_pos; j++ ) {
		if (Language != CHINESE) {
		    path_matrix[ptr->max_path[j-ptr->key_pos-1]][j] =
			path_matrix[ptr->max_path[j-ptr->key_pos-1]][j] ?
			-1 : 'a' + i;
		}
		else {
		    if (check_feature((sp->bnst_data + j)->f, "CC") || check_feature((sp->bnst_data + j)->f, "PU")) {
			path_matrix[ptr->max_path[j-ptr->key_pos]][j] =
			    path_matrix[ptr->max_path[j-ptr->key_pos]][j] ?
			    -1 : 'a' + i;
		    }
		    else {
			path_matrix[ptr->max_path[j-ptr->key_pos-1]][j] =
			    path_matrix[ptr->max_path[j-ptr->key_pos-1]][j] ?
			    -1 : 'a' + i;
		    }
		}
	    }
	}
    }

    /* 長さの計算 */

    for ( i=0; i<sp->Bnst_num; i++ ) {
	length = sp->bnst_data[i].length + (sp->Bnst_num-i-1)*3;
        if ( sp->bnst_data[i].para_key_type ) length += 2;
	if ( max_length < length )    max_length = length;
    }
    
    /* 印刷用の処理 */

    if ( 0 ) {
	if ( PRINT_WIDTH < sp->Bnst_num*3 ) {
	    over_flag = 1;
	    sp->Bnst_num = PRINT_WIDTH/3;
	    max_length = PRINT_WIDTH;
	} else if ( PRINT_WIDTH < max_length ) {
	    max_length = PRINT_WIDTH;
	}
    }

    if (type == PRINT_PARA)
	fprintf(Outfp, "<< PARA MATRIX >>\n");
    else if (type == PRINT_DPND)
	fprintf(Outfp, "<< DPND MATRIX >>\n");
    else if (type == PRINT_MASK)
	fprintf(Outfp, "<< MASK MATRIX >>\n");
    else if (type == PRINT_QUOTE)
	fprintf(Outfp, "<< QUOTE MATRIX >>\n");
    else if (type == PRINT_RSTR)
	fprintf(Outfp, "<< RESTRICT MATRIX for PARA RELATION>>\n");
    else if (type == PRINT_RSTD)
	fprintf(Outfp, "<< RESTRICT MATRIX for DEPENDENCY STRUCTURE>>\n");
    else if (type == PRINT_RSTQ)
	fprintf(Outfp, "<< RESTRICT MATRIX for QUOTE SCOPE>>\n");

    print_line(max_length, over_flag);
    for ( i=0; i<(max_length-sp->Bnst_num*3); i++ ) fputc(' ', Outfp);
    for ( i=0; i<sp->Bnst_num; i++ ) fprintf(Outfp, "%2d ", i);
    fputc('\n', Outfp);
    print_line(max_length, over_flag);

    for ( i=0; i<sp->Bnst_num; i++ ) {
	print_M_bnst(sp, i, max_length, &para_char);
	for ( j=i+1; j<sp->Bnst_num; j++ ) {

	    if (type == PRINT_PARA) {
		fprintf(Outfp, "%2d", match_matrix[i][j]);
	    } else if (type == PRINT_DPND) {
		if (Dpnd_matrix[i][j] == 0)
		    fprintf(Outfp, " -");
		else
		    fprintf(Outfp, " %c", (char)Dpnd_matrix[i][j]);
		
	    } else if (type == PRINT_MASK) {
		fprintf(Outfp, "%2d", Mask_matrix[i][j]);

	    } else if (type == PRINT_QUOTE) {
		fprintf(Outfp, "%2d", Quote_matrix[i][j]);

	    } else if (type == PRINT_RSTR || 
		       type == PRINT_RSTD ||
		       type == PRINT_RSTQ) {
		if (j <= key_pos) 
		    fprintf(Outfp, "--");
		else if (key_pos < i)
		    fprintf(Outfp, " |");
		else
		    fprintf(Outfp, "%2d", restrict_matrix[i][j]);
	    }

	    switch(path_matrix[i][j]) {
	      case  0:	fputc(' ', Outfp); break;
	      case -1:	fputc('*', Outfp); break;
	      default:	fputc(path_matrix[i][j], Outfp); break;
	    }
	}
	fputc('\n', Outfp);
    }

    print_line(max_length, over_flag);
    
    if (type == PRINT_PARA) {
	for (i = 0; i < sp->Para_num; i++) {
	    fprintf(Outfp, "%c(%c):%4.1f(%4.1f) ", 
		    sp->para_data[i].para_char, 
		    sp->para_data[i].status, 
		    sp->para_data[i].max_score,
		    sp->para_data[i].pure_score);
	}
	fputc('\n', Outfp);
    }
}

/*==================================================================*/
	void assign_para_similarity_feature(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;
    char buffer[DATA_LEN];

    for (i = 0; i < sp->Para_num; i++) {
	sprintf(buffer, "並列類似度:%.3f", sp->para_data[i].max_score);
	assign_cfeature(&(sp->bnst_data[sp->para_data[i].key_pos].f), buffer, FALSE);
    }
}

/*====================================================================
	                並列構造間の関係表示
====================================================================*/

/*==================================================================*/
void print_para_manager(SENTENCE_DATA *sp, PARA_MANAGER *m_ptr, int level)
/*==================================================================*/
{
    int i;
    
    for (i = 0; i < level * 5; i++)
	fputc(' ', Outfp);

    for (i = 0; i < m_ptr->para_num; i++)
	fprintf(Outfp, " %c", sp->para_data[m_ptr->para_data_num[i]].para_char);
    fputc(':', Outfp);

    for (i = 0; i < m_ptr->part_num; i++) {
	if (m_ptr->start[i] == m_ptr->end[i]) {
	    fputc('(', Outfp);
	    print_bnst(&sp->bnst_data[m_ptr->start[i]], NULL);
	    fputc(')', Outfp);
	} else {
	    fputc('(', Outfp);
	    print_bnst(&sp->bnst_data[m_ptr->start[i]], NULL);
	    fputc('-', Outfp);
	    print_bnst(&sp->bnst_data[m_ptr->end[i]], NULL);
	    fputc(')', Outfp);
	}
    }
    fputc('\n', Outfp);

    for (i = 0; i < m_ptr->child_num; i++)
	print_para_manager(sp, m_ptr->child[i], level+1);
}

/*==================================================================*/
	     void print_para_relation(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;
    
    for (i = 0; i < sp->Para_M_num; i++)
	if (sp->para_manager[i].parent == NULL)
	    print_para_manager(sp, &sp->para_manager[i], 0);
}

/*====================================================================
	                木構造表示(from JK)
====================================================================*/
static int max_width;			/* 木の最大幅 */

/*==================================================================*/
			   int mylog(int n)
/*==================================================================*/
{
    int i, num = 1;
    for (i=0; i<n; i++)
	num = num*2;
    return(num);
}

/*==================================================================*/
       void calc_self_space(BNST_DATA *ptr, int depth2)
/*==================================================================*/
{
    if (ptr->para_top_p == TRUE) 
	ptr->space = 4;
    else if (OptDisplay == OPT_NORMAL || OptDisplay == OPT_SIMPLE)
	ptr->space = ptr->length;
    else if (ptr->type == IS_MRPH_DATA)
	ptr->space = ptr->length + 1;
    else
	ptr->space = ptr->length + ptr->mrph_num; /* *4 */

    if (ptr->para_type == PARA_NORMAL || 
	ptr->para_type == PARA_INCOMP ||
	ptr->to_para_p == TRUE)
	ptr->space += 1;
    ptr->space += (depth2-1)*8;
}

/*==================================================================*/
       void calc_tree_width(BNST_DATA *ptr, int depth2)
/*==================================================================*/
{
    int i;
    
    calc_self_space(ptr, depth2);

    if ( ptr->space > max_width )
	max_width = ptr->space;

    if ( ptr->child[0] )
	for ( i=0; ptr->child[i]; i++ )
	    calc_tree_width(ptr->child[i], depth2+1);
}

/*==================================================================*/
void show_link(int depth, char *ans_flag, char para_type, char to_para_p)
/*==================================================================*/
{
    int i;
    
    if (depth != 1) {

	/* 親への枝 (兄弟を考慮) */

	if (para_type == PARA_NORMAL || 
	    para_type == PARA_INCOMP ||
	    to_para_p == TRUE) {
	    if (OptExpress != OPT_TABLE)
		fprintf(Outfp, "─");
	}
	else {
	    if (OptExpress == OPT_TABLE)
		fprintf(Outfp, "─");
	    else
		fprintf(Outfp, "──");
	}

	if (ans_flag[depth-1] == '1') 
	    fprintf(Outfp, "┤");
	else 
	    fprintf(Outfp, "┐");

	if (OptExpress == OPT_TABLE)
	    fprintf(Outfp, "&nbsp;&nbsp;");
	else
	    fprintf(Outfp, "　");

	/* 祖先の兄弟の枝 */

	for (i = depth - 1; i > 1; i--) {
	    if (OptExpress == OPT_TABLE)
		fprintf(Outfp, "&nbsp;&nbsp;&nbsp;&nbsp;");
	    else
		fprintf(Outfp, "　　");
	    if (ans_flag[i-1] == '1') 
		fprintf(Outfp, "│");
	    else if (OptExpress == OPT_TABLE)
		fprintf(Outfp, "&nbsp;&nbsp;");
	    else
		fprintf(Outfp, "　");
	    if (OptExpress == OPT_TABLE)
		fprintf(Outfp, "&nbsp;&nbsp;");
	    else
		fprintf(Outfp, "　");
	}
    }
}

/*==================================================================*/
 void show_self(BNST_DATA *ptr, int depth, char *ans_flag_p, int flag)
/*==================================================================*/
{
    /* 
       depth は自分の深さ(根が1)

       ans_flag は自分と祖先が最後の子かどうかの履歴
       深さnの祖先(または自分)が最後の子であれば ans_flag[n-1] が '0'
       そうでなければ '1'(この場合枝の描画が必要)
    */

    int i, j;
    char ans_flag[BNST_MAX];

    if (ans_flag_p) {
	strncpy(ans_flag, ans_flag_p, BNST_MAX);
    } else {
	ans_flag[0] = '0';	/* 最初に呼ばれるとき */
    }

    if (ptr->child[0]) {
	for (i = 0; ptr->child[i]; i++);

	/* 最後の子は ans_flag を 0 に */ 
	ans_flag[depth] = '0';
	show_self(ptr->child[i-1], depth+1, ans_flag, 0);

	if (i > 1) {
	    /* 他の子は ans_flag を 1 に */ 
	    ans_flag[depth] = '1';
	    for (j = i - 2; j > 0; j--) {
		show_self(ptr->child[j], depth+1, ans_flag, 0);
	    }

	    /* flag: 1: ─PARA 2: -<P>PARA */
	    if (ptr->para_top_p == TRUE && 
		ptr->para_type == PARA_NIL &&
		ptr->to_para_p == FALSE) {
		show_self(ptr->child[0], depth+1, ans_flag, 1);
	    } else if (ptr->para_top_p == TRUE) {
		show_self(ptr->child[0], depth+1, ans_flag, 2);
	    } else {
		show_self(ptr->child[0], depth+1, ans_flag, 0);
	    }
	}
    }

    calc_self_space(ptr, depth);
    if (OptExpress != OPT_TABLE) {
	if ( ptr->para_top_p != TRUE ) {
	    for (i = 0; i < max_width - ptr->space; i++) 
		fputc(' ', Outfp);
	}
    }

    if (OptExpress & OPT_MRPH) {
	print_mrph_with_para((MRPH_DATA *)ptr, NULL);
    }
    else {
	print_bnst(ptr, NULL);
    }
    
    if (flag == 0) {
	show_link(depth, ans_flag, ptr->para_type, ptr->to_para_p);
	if (OptExpress == OPT_TREEF) {
	    print_some_feature(ptr->f, Outfp);
	}
	fputc('\n', Outfp);	
    } else if ( flag == 1 ) {
	if (OptExpress != OPT_TABLE)
	    fprintf(Outfp, "─");
    } else if ( flag == 2 ) {
	if (OptExpress != OPT_TABLE)
	    fprintf(Outfp, "-");
    }
}

/*==================================================================*/
	 void show_sexp(BNST_DATA *ptr, int depth, int pars)
/*==================================================================*/
{
    int i;

    for (i = 0; i < depth; i++) fputc(' ', Outfp);
    fprintf(Outfp, "(");

    if ( ptr->para_top_p == TRUE ) {
	if (ptr->child[1] && 
	    ptr->child[1]->para_key_type == PARA_KEY_N)
	  fprintf(Outfp, "(noun_para"); 
	else
	  fprintf(Outfp, "(pred_para");

	if (ptr->child[0]) {
	    fputc('\n', Outfp);
	    i = 0;
	    while (ptr->child[i+1] && ptr->child[i+1]->para_type != PARA_NIL) {
		/* <P>の最後以外 */
		/* UCHI fputc(',', Outfp); */
		show_sexp(ptr->child[i], depth + 3, 0);	i ++;
	    }
	    if (ptr->child[i+1]) { /* その他がある場合 */
		/* <P>の最後 */
		/* UCHI fputc(',', Outfp); */
		show_sexp(ptr->child[i], depth + 3, 1);	i ++;
		/* その他の最後以外 */
		while (ptr->child[i+1]) {
		    /* UCHI fputc(',', Outfp); */
		    show_sexp(ptr->child[i], depth + 3, 0); i ++;
		}
		/* その他の最後 */
		/* UCHI fputc(',', Outfp); */
		show_sexp(ptr->child[i], depth + 3, pars + 1);
	    }
	    else {
		/* <P>の最後 */
		/* UCHI fputc(',', Outfp); */
		show_sexp(ptr->child[i], depth + 3, pars + 1 + 1);
	    }
	}
    }
    else {
	
	print_bnst_detail(ptr);

	if (ptr->child[0]) {
	    fputc('\n', Outfp);
	    for ( i=0; ptr->child[i+1]; i++ ) {
		/* UCHI fputc(',', Outfp); */
		show_sexp(ptr->child[i], depth + 3, 0);
	    }
	    /* UCHI fputc(',', Outfp); */
	    show_sexp(ptr->child[i], depth + 3, pars + 1);
	} else {
	    for (i = 0; i < pars + 1; i++) fputc(')', Outfp);
	    fputc('\n', Outfp);
	}
    }
}

/*==================================================================*/
     void print_kakari(SENTENCE_DATA *sp, int type, int eos_flag)
/*==================================================================*/
{
    int i, last_b_offset = 1, last_t_offset = 1;

    /* 最後の文節、基本句がマージされている場合があるので、
       本当の最後の文節、基本句を探す */
    if (OptPostProcess) {
	for (i = sp->Bnst_num - 1; i >= 0; i--) {
	    if ((sp->bnst_data + i)->num != -1) {
		last_b_offset = sp->Bnst_num - i;
		break;
	    }
	}
	for (i = sp->Tag_num - 1; i >= 0; i--) {
	    if ((sp->tag_data + i)->num != -1) {
		last_t_offset = sp->Tag_num - i;
		break;
	    }
	}
    }

    /* 依存構造木の表示 */

    if (type == OPT_SEXP) {
	show_sexp((sp->bnst_data + sp->Bnst_num - last_b_offset), 0, 0);
    }
    /* 文節のtreeを描くとき */
    else if (type & OPT_NOTAG) {
	max_width = 0;

	calc_tree_width((sp->bnst_data + sp->Bnst_num - last_b_offset), 1);
	show_self((sp->bnst_data + sp->Bnst_num - last_b_offset), 1, NULL, 0);
    }
    /* 形態素のtreeを描くとき */
    else if (type & OPT_MRPH) {
	max_width = 0;

	calc_tree_width((BNST_DATA *)(sp->mrph_data + sp->Mrph_num - 1), 1);
	show_self((BNST_DATA *)(sp->mrph_data + sp->Mrph_num - 1), 1, NULL, 0);
    }
    /* tag単位のtreeを描くとき */
    else {
	max_width = 0;

	calc_tree_width((BNST_DATA *)(sp->tag_data + sp->Tag_num - last_t_offset), 1);
	show_self((BNST_DATA *)(sp->tag_data + sp->Tag_num - last_t_offset), 1, NULL, 0);
    }

    if (OptExpress == OPT_TABLE) {
	Tag_Num = 1;
	Sen_Num++;
    }
    else {
	print_eos(eos_flag);
    }
}

/*====================================================================
			      チェック用
====================================================================*/

/*==================================================================*/
		  void check_bnst(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int 	i, j;
    BNST_DATA 	*ptr;
    char b_buffer[BNST_LENGTH_MAX];

    for (i = 0; i < sp->Bnst_num; i++) {
	ptr = &sp->bnst_data[i];
	
	b_buffer[0] = '\0';

	for (j = 0; j < ptr->mrph_num; j++) {
	    /* buffer overflow */
	    if (strlen(b_buffer) + strlen((ptr->mrph_ptr + j)->Goi2) + 4 > BNST_LENGTH_MAX) {
		break;
	    }

	    if (ptr->mrph_ptr + j == ptr->head_ptr) {
		strcat(b_buffer, "[");
		strcat(b_buffer, (ptr->mrph_ptr + j)->Goi2);
		strcat(b_buffer, "]");
	    }
	    else {
		strcat(b_buffer, (ptr->mrph_ptr + j)->Goi2);
	    }
	    strcat(b_buffer, " ");
	}
	fprintf(Outfp, "%-20s", b_buffer);

	print_feature(ptr->f, Outfp);

	if (check_feature(ptr->f, "用言") ||
	    check_feature(ptr->f, "準用言")) {

	    fprintf(Outfp, " <表層格:");
	    if (ptr->SCASE_code[case2num("ガ格")])
	      fprintf(Outfp, "ガ,");
	    if (ptr->SCASE_code[case2num("ヲ格")])
	      fprintf(Outfp, "ヲ,");
	    if (ptr->SCASE_code[case2num("ニ格")])
	      fprintf(Outfp, "ニ,");
	    if (ptr->SCASE_code[case2num("デ格")])
	      fprintf(Outfp, "デ,");
	    if (ptr->SCASE_code[case2num("カラ格")])
	      fprintf(Outfp, "カラ,");
	    if (ptr->SCASE_code[case2num("ト格")])
	      fprintf(Outfp, "ト,");
	    if (ptr->SCASE_code[case2num("ヨリ格")])
	      fprintf(Outfp, "ヨリ,");
	    if (ptr->SCASE_code[case2num("ヘ格")])
	      fprintf(Outfp, "ヘ,");
	    if (ptr->SCASE_code[case2num("マデ格")])
	      fprintf(Outfp, "マデ,");
	    if (ptr->SCASE_code[case2num("ノ格")])
	      fprintf(Outfp, "ノ,");
	    if (ptr->SCASE_code[case2num("ガ２")])
	      fprintf(Outfp, "ガ２,");
	    fprintf(Outfp, ">");
	}

	fputc('\n', Outfp);
    }
}

/*==================================================================*/
	    void print_case_for_table(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;
    char *cp, *next, buf1[SMALL_DATA_LEN2], buf2[SMALL_DATA_LEN2], buf3[SMALL_DATA_LEN2];

    for (i = 0; i < sp->Tag_num; i++) {
	if ((cp = check_feature((sp->tag_data + i)->f, "格解析結果"))) {

	    /* OPT_TABLE */
	    if (OptExpress == OPT_TABLE) {
		fprintf(Outfp, "%%%% %d %d 2 LABEL=%d_%dd style=white-space:nowrap\n", 
			Sen_Num - 1, i + 2, Sen_Num - 1, i + 1);
		fprintf(Outfp, "*\n");
	    }
	    /* O */
	    cp = check_feature((sp->tag_data + i)->f, "格解析結果");
	    while (next = strstr(cp, "/O/")) {
		cp = next;
		while (cp[0] != ';' && cp[0] != ':') cp--;
		if (sscanf(cp, "%*[:;]%[^/]%*[/]%[^/]%*[/]%[^/]%*[/]", buf1, buf2, buf3)) {
		    fprintf(Outfp, "%%%% %d %d 2 style=white-space:nowrap\n", Sen_Num - 1, i + 2);
		    fprintf(Outfp, "&nbsp;%s:%s&nbsp;\n", buf1, buf3);
		    cp = strstr(cp, buf2) + 1;
		}
	    }
	    /* C */
	    cp = check_feature((sp->tag_data + i)->f, "格解析結果");
	    while (next = strstr(cp, "/C/")) {
		cp = next;
		while (cp[0] != ';' && cp[0] != ':') cp--;
		if (sscanf(cp, "%*[:;]%[^/]%*[/]%[^/]%*[/]%[^/]%*[/]", buf1, buf2, buf3)) {
		    fprintf(Outfp, "%%%% %d %d 2 style=white-space:nowrap\n", Sen_Num - 1, i + 2);
		    fprintf(Outfp, "&nbsp;[%s:%s]&nbsp;\n", buf1, buf3);
		    cp = strstr(cp, buf2) + 1;
		}
	    }
	    /* N */
	    cp = check_feature((sp->tag_data + i)->f, "格解析結果");
	    while (next = strstr(cp, "/N/")) {
		cp = next;
		while (cp[0] != ';' && cp[0] != ':') cp--;
		if (sscanf(cp, "%*[:;]%[^/]%*[/]%[^/]%*[/]%[^/]%*[/]", buf1, buf2, buf3)) {
		    fprintf(Outfp, "%%%% %d %d 2 style=white-space:nowrap\n", Sen_Num - 1, i + 2);
		    fprintf(Outfp, "&nbsp;[%s:%s]&nbsp;\n", buf1, buf3);
		    cp = strstr(cp, buf2) + 1;
		}
	    }
	}
    }
}

/*==================================================================*/
	    void print_corefer_for_table(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, s_num, t_num;
    char *cp;

    for (i = 0; i < sp->Tag_num; i++) {
	if ((cp = check_feature((sp->tag_data + i)->f, "COREFER_ID"))) {

	    fprintf(Outfp, "%%%% %d %d 2 style=white-space:nowrap\n", Sen_Num - 1, i + 2);
	    fprintf(Outfp, "&nbsp;ID=%s&nbsp;\n", cp + 11);

	    if (check_feature((sp->tag_data + i)->f, "REFERRED")) {
		sscanf(check_feature((sp->tag_data + i)->f, "REFERRED"),
		       "REFERRED:%d-%d", &s_num, &t_num);
		fprintf(Outfp, "%%%% %d %d 2 style=white-space:nowrap\n", Sen_Num - s_num - 1, t_num + 2);
		fprintf(Outfp, "&nbsp;ID=%s&nbsp;\n", cp + 11);
	    }
	}
    }
}

/*==================================================================*/
	    void print_ne_for_table(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;
    char *cp;

    for (i = 0; i < sp->Tag_num; i++) {
	if ((cp = check_feature((sp->tag_data + i)->f, "NE"))) {
	    
	    fprintf(Outfp, "%%%% %d %d 2 style=white-space:nowrap\n", Sen_Num - 1, i + 2);
	    fprintf(Outfp, "&nbsp;%s&nbsp;\n", cp + 3);
	}
    }
}

/*==================================================================*/
void print_result(SENTENCE_DATA *sp, int case_print_flag, int eos_flag)
/*==================================================================*/
{
    /* case_print_flag: 格解析結果を出力 */

    char *date_p, time_string[64];
    time_t t;
    struct tm *tms;
    TOTAL_MGR *tm = sp->Best_mgr;

    /* 時間の取得 */
    t = time(NULL);
    tms = localtime(&t);
    if (!strftime(time_string, 64, "%Y/%m/%d", tms))
	time_string[0] = '\0';
    
    /* PS出力の場合
       dpnd_info_to_bnst(&(tm->dpnd));
       make_dpnd_tree();
       print_kakari2ps();
       return;
    */ 

    /* 既解析へのパターンマッチで, マッチがなければ出力しない
       if (OptAnalysis == OPT_AssignF && !PM_Memo[0]) return;
    */

    /* ヘッダの出力 */   
    if (OptExpress == OPT_TABLE) {
	if (OptAnalysis == OPT_CASE || OptAnalysis == OPT_CASE2 || OptNE) {
	    fprintf(Outfp, "%%%% %d %d 2\n", Sen_Num, Tag_Num);
	    fprintf(Outfp, "解析結果\n");
	}
	fprintf(Outfp, "%%%% %d %d 1 style=white-space:nowrap\n", Sen_Num, Tag_Num++);
    }

    /* S-ID */
    if (sp->KNPSID) {
	fprintf(Outfp, "# %s", sp->KNPSID);
    }
    else {
	fprintf(Outfp, "# S-ID:%d", sp->Sen_num);
    }

    /* コメント */
    if (sp->Comment) {
	fprintf(Outfp, " %s", sp->Comment);
    }
    
    if (OptInput == OPT_INPUT_RAW) {
	fprintf(Outfp, " KNP:%s-%s", REVISION_VERSION, REVISION_ID);

	if ((date_p = (char *)getenv("DATE")))
	    fprintf(Outfp, " DATE:%s", date_p);
	else if (time_string[0])
	    fprintf(Outfp, " DATE:%s", time_string);
    }

    /* スコアを出力 (CKY時、通常入力時) */
    if (OptCKY && !(OptInput & OPT_INPUT_PARSED)) {
	fprintf(Outfp, " SCORE:%.5f", sp->score);
    }

    /* エラーがあれば、エラーの内容 */
    if (ErrorComment) {
	fprintf(Outfp, " ERROR:%s", ErrorComment);
	free(ErrorComment);
	ErrorComment = NULL;
    }

    /* 警告があれば、警告の内容 */
    if (WarningComment) {
	fprintf(Outfp, " WARNING:%s", WarningComment);
	free(WarningComment);
	WarningComment = NULL;
    }

    if (PM_Memo[0]) {
	if (sp->Comment && strstr(sp->Comment, "MEMO")) {
	    fprintf(Outfp, "%s", PM_Memo);
	} else {
	    fprintf(Outfp, " MEMO:%s", PM_Memo);
	}	
    }

    fprintf(Outfp, "\n");

    /* 解析結果のメインの出力 */

    if (OptExpress == OPT_MRPH) {
	print_mrphs(sp, eos_flag);
    }
    else if (OptExpress == OPT_TAB) {
	print_tags(sp, 1, eos_flag);
    }
    else if (OptExpress == OPT_NOTAG) {
	print_bnst_with_mrphs(sp, 1, eos_flag);
    }
    else if (OptExpress == OPT_PA) {
	/* FIXME: 格解析結果の整合性をとる必要がある */
	print_pa_structure(sp, eos_flag);
    }
    else if (OptExpress == OPT_BNSTTREE) {
	/* 文節のtree出力 */
	if (make_dpnd_tree(sp)) {
	    print_kakari(sp, OptExpress, eos_flag);
	}
	else {
	    print_eos(eos_flag);
	}
    }
    else if (OptExpress == OPT_MRPHTREE) {
	/* 形態素のtree出力 */
	if (make_dpnd_tree(sp)) {
	    bnst_to_mrph_tree(sp); /* 形態素の木へ */
	    print_kakari(sp, OptExpress, eos_flag);
	}
	else {
	    print_eos(eos_flag);
	}
    }
    else {
	/* タグ単位のtree出力 */
	if (make_dpnd_tree(sp)) {
	    bnst_to_tag_tree(sp); /* タグ単位の木へ */
	    print_kakari(sp, OptExpress, eos_flag); /* OPT_TREE */
	}
	else {
	    print_eos(eos_flag);
	}
    }

    if (OptExpress == OPT_TABLE) {
	print_tags(sp, 1, eos_flag);
	if (OptAnalysis == OPT_CASE || OptAnalysis == OPT_CASE2) 
	    print_case_for_table(sp);
	if (OptNE)
	    print_ne_for_table(sp);
	if (OptEllipsis & OPT_COREFER) 
	    print_corefer_for_table(sp);
    }

    /* nbestオプションなどではこの関数が複数回呼ばれるので後処理を元に戻しておく */
    if (OptPostProcess) {
	undo_tag_bnst_postprocess(sp);
    }

    /* 格解析を行なった場合の出力 */
    if (case_print_flag && 
	!OptArticle && /* 過去の記事のBest_mgrを保存していないのでセグフォする */
	(((OptAnalysis == OPT_CASE || 
	   OptAnalysis == OPT_CASE2) && 
	  (OptDisplay == OPT_DETAIL || 
	   OptDisplay == OPT_DEBUG ||
	   OptExpress == OPT_TABLE)) || 
	 (OptEllipsis && 
	  VerboseLevel >= VERBOSE1))) {

	print_case_result(sp, Sen_Num);

	/* 次の解析のために初期化しておく */
	tm->pred_num = 0;
    }
}

/*==================================================================*/
		void do_postprocess(SENTENCE_DATA *sp)
/*==================================================================*/
{
    /* 後処理 */
    if (make_dpnd_tree(sp)) {
	bnst_to_tag_tree(sp); /* タグ単位の木へ */
	if (OptExpress == OPT_TAB || 
	    OptExpress == OPT_NOTAG) {
	    tag_bnst_postprocess(sp, 1);
	}
	else {
	    tag_bnst_postprocess(sp, 0); /* 木構造出力のため、num, dpnd_head の番号の付け替えはしない */
	}
    }
}

/*==================================================================*/
    void push_entity(char ***list, char *key, int count, int *max)
/*==================================================================*/
{
    if (*max == 0) {
	*max = ALLOCATION_STEP;
	*list = (char **)malloc_data(sizeof(char *)*(*max), "push_entity");
    }
    else if (*max <= count) {
	*list = (char **)realloc_data(*list, sizeof(char *)*(*max <<= 1), "push_entity");
    }

    *(*list+count) = key;
}

/*==================================================================*/
		  void prepare_entity(BNST_DATA *bp)
/*==================================================================*/
{
    int count = 0, max = 0, i, flag = 0;
    char *cp, **list, *str;

    /* モダリティ */
    flag = 0;
    for (i = 0; i < bp->mrph_num; i++) {
	if (!(flag & 0x0001) && (cp = check_feature((bp->mrph_ptr+i)->f, "Modality-意思-依頼"))) {
	    push_entity(&list, cp, count++, &max);
	    flag |= 0x0001;
	}
	if (!(flag & 0x0002) && (cp = check_feature((bp->mrph_ptr+i)->f, "Modality-意思-意志"))) {
	    push_entity(&list, cp, count++, &max);
	    flag |= 0x0002;
	}
	if (!(flag & 0x0004) && (cp = check_feature((bp->mrph_ptr+i)->f, "Modality-意思-勧誘"))) {
	    push_entity(&list, cp, count++, &max);
	    flag |= 0x0004;
	}
	if (!(flag & 0x0008) && (cp = check_feature((bp->mrph_ptr+i)->f, "Modality-意思-願望"))) {
	    push_entity(&list, cp, count++, &max);
	    flag |= 0x0008;
	}
	if (!(flag & 0x0010) && (cp = check_feature((bp->mrph_ptr+i)->f, "Modality-意思-禁止"))) {
	    push_entity(&list, cp, count++, &max);
	    flag |= 0x0010;
	}
	if (!(flag & 0x0020) && (cp = check_feature((bp->mrph_ptr+i)->f, "Modality-意思-三人称意志"))) {
	    push_entity(&list, cp, count++, &max);
	    flag |= 0x0020;
	}
	if (!(flag & 0x0040) && (cp = check_feature((bp->mrph_ptr+i)->f, "Modality-意思-申し出"))) {
	    push_entity(&list, cp, count++, &max);
	    flag |= 0x0040;
	}
	if (!(flag & 0x0080) && (cp = check_feature((bp->mrph_ptr+i)->f, "Modality-意思-推量"))) {
	    push_entity(&list, cp, count++, &max);
	    flag |= 0x0080;
	}
	if (!(flag & 0x0100) && (cp = check_feature((bp->mrph_ptr+i)->f, "Modality-意思-命令"))) {
	    push_entity(&list, cp, count++, &max);
	    flag |= 0x0100;
	}
	if (!(flag & 0x0200) && (cp = check_feature((bp->mrph_ptr+i)->f, "Modality-当為"))) {
	    push_entity(&list, cp, count++, &max);
	    flag |= 0x0200;
	}
	if (!(flag & 0x0400) && (cp = check_feature((bp->mrph_ptr+i)->f, "Modality-当為-許可"))) {
	    push_entity(&list, cp, count++, &max);
	    flag |= 0x0400;
	}
	if (!(flag & 0x0800) && (cp = check_feature((bp->mrph_ptr+i)->f, "Modality-判断-可能性"))) {
	    push_entity(&list, cp, count++, &max);
	    flag |= 0x0800;
	}
	if (!(flag & 0x1000) && (cp = check_feature((bp->mrph_ptr+i)->f, "Modality-判断-可能性-不可能"))) {
	    push_entity(&list, cp, count++, &max);
	    flag |= 0x1000;
	}
	if (!(flag & 0x2000) && (cp = check_feature((bp->mrph_ptr+i)->f, "Modality-判断-推量"))) {
	    push_entity(&list, cp, count++, &max);
	    flag |= 0x2000;
	}
	if (!(flag & 0x4000) && (cp = check_feature((bp->mrph_ptr+i)->f, "Modality-判断-伝聞"))) {
	    push_entity(&list, cp, count++, &max);
	    flag |= 0x4000;
	}
	if (!(flag & 0x8000) && (cp = check_feature((bp->mrph_ptr+i)->f, "Modality-判断-様態"))) {
	    push_entity(&list, cp, count++, &max);
	    flag |= 0x8000;
	}
    }

    /* 出力するfeatureがあれば出力 */
    if (count) {
	int i, len = 0;
	for (i = 0; i < count; i++) {
	    len += strlen(list[i])+1;
	}
	str = (char *)malloc_data(sizeof(char)*(len+2), "print_entity");
	strcpy(str, "C:");
	for (i = 0; i < count; i++) {
	    if (i != 0) strcat(str, " ");
	    strcat(str, list[i]);
	}
	assign_cfeature(&(bp->f), str, FALSE);
	free(str);
	free(list);
    }
}

/*==================================================================*/
	      void prepare_all_entity(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;

    for (i = 0; i < sp->Bnst_num; i++) {
	prepare_entity(sp->bnst_data+i);
    }
}

/*==================================================================*/
	    void print_tree_for_chinese(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int		i, j, k, max_len, len, max_inverse_len;
    char*       up_corner = "┌─";
    char*       down_corner = "└─";
    char*       middle_corner = "├─";
    char*       link = "│";
    char*       para = "<P>";
    char*       para_head = "<PARA><P>";

    BNST_DATA	*b_ptr;

    /* initialization */
    for (i = 0; i < sp->Bnst_num; i++) {
	for (j = 0; j < TREE_WIDTH_MAX; j++) {
	    bnst_tree[i][j] = "";
	}
    }

    /* read data */
    for (i = 0, b_ptr = sp->bnst_data; i < sp->Bnst_num; i++, b_ptr++) {
	bnst_word[i] = b_ptr->head_ptr->Goi;
	bnst_pos[i] = b_ptr->head_ptr->Pos;
	bnst_dpnd[i] = b_ptr->dpnd_head;
	bnst_level[i] = -1;
    }

    /* get root level */
    for (i = 0; i < sp->Bnst_num; i++) {
	while (bnst_level[i] == -1) {
	    j = i;
	    while (bnst_dpnd[j] != -1 && bnst_level[bnst_dpnd[j]] == -1) {
		j = bnst_dpnd[j];
	    }
	    if (bnst_dpnd[j] == -1) {
		bnst_level[j] = 0;
	    }
	    else {
		bnst_level[j] = bnst_level[bnst_dpnd[j]] + 1;
	    }
	}
    }

    /* get print tree */
    max_len = -1;
    for (i = 0; i < sp->Bnst_num; i++) {
	len = 0;
	for (j = 0; j < (bnst_level[i] * 4); j++) {
	    if (bnst_dpnd[i] != -1 && j < (bnst_level[bnst_dpnd[i]] * 4)) {
		if (len >= TREE_WIDTH_MAX) {
		    fprintf(Outfp, ">>>tree width exceeds maximum length\n");
		    return;
		}
		bnst_tree[i][len] = " ";
		len++;
	    }
	    else if (bnst_dpnd[i] != -1 && j == (bnst_level[bnst_dpnd[i]] * 4)) {
		if (bnst_dpnd[i] != -1 && bnst_dpnd[i] < i) {
		    if (len >= TREE_WIDTH_MAX) {
			fprintf(Outfp, ">>>tree width exceeds maximum length\n");
			return;
		    }
		    bnst_tree[i][len] = down_corner;
		    len++;
		}
		else if (bnst_dpnd[i] > i) {
		    if (len >= TREE_WIDTH_MAX) {
			fprintf(Outfp, ">>>tree width exceeds maximum length\n");
			return;
		    }
		    bnst_tree[i][len] = up_corner;
		    len++;
		}
	    }
	}
	if (len >= TREE_WIDTH_MAX) {
	    fprintf(Outfp, ">>>tree width exceeds maximum length\n");
	    return;
	}
	if ((sp->bnst_data + i)->is_para == 1) {
	    bnst_tree[i][len] = para;
	    len++;
	}
	else if ((sp->bnst_data + i)->is_para == 2) {
	    bnst_tree[i][len] = para_head;
	    len++;
	}
	bnst_tree[i][len] = bnst_word[i];
	len++;
	bnst_tree[i][len] = "/";
	len++;
	bnst_tree[i][len] = bnst_pos[i];
	len++;

	if (len > max_len) {
	    max_len = len;
	}
    }
    
    for (i = 0; i < sp->Bnst_num; i++) {
	for (j = 0; j < max_len; j++) {
	    if (bnst_tree[i][j] == "") {
		bnst_tree[i][j] = "***";
	    }
	}
    }

    /* inverse the tree */
    max_inverse_len = -1;
    for (i = 0; i < max_len; i++) {
	len = 0;
	for (j = sp->Bnst_num - 1; j > -1; j--) {
	    bnst_inverse_tree[i][len] = bnst_tree[j][i];
	    len++;
	}
	if (len > max_inverse_len) {
	    max_inverse_len = len;
	}
    }

    /* change bnst_inverse_tree */
    for (i = 0; i < max_len; i++) {
	for (j = 0; j < sp->Bnst_num; j++) {
	    if (bnst_inverse_tree[i][j] == down_corner) {
		for (k = j + 1; k < sp->Bnst_num; k++) {
		    if (bnst_inverse_tree[i][k] == down_corner) {
			bnst_inverse_tree[i][k] = middle_corner;
		    }
		    else if (bnst_inverse_tree[i][k] == " ") {
			bnst_inverse_tree[i][k] = link;
		    }
		    else {
			break;
		    }
		}
	    }
	    else if (bnst_inverse_tree[i][j] == up_corner) {
		for (k = j - 1; k > -1; k--) {
		    if (bnst_inverse_tree[i][k] == up_corner) {
			bnst_inverse_tree[i][k] = middle_corner;
		    }
		    else if (bnst_inverse_tree[i][k] == " ") {
			bnst_inverse_tree[i][k] = link;
		    }
		    else {
			break;
		    }
		}
	    }
	}
    }

    /* inverse tree again and print */
    for (i = max_inverse_len - 1; i > -1; i--) {
	if (max_inverse_len - 1 - i < 10) {
	    fprintf(Outfp, "%d   ", max_inverse_len - 1 - i);
	}
	else if (max_inverse_len - 1 - i < 100) {
	    fprintf(Outfp, "%d  ", max_inverse_len - 1 - i);
	}
	else {
	    fprintf(Outfp, "%d ", max_inverse_len - 1 - i);
	}
	for (j = 0; j < max_len; j++) {
	    if (bnst_inverse_tree[j][i] != "***") {
		fprintf(Outfp, "%s", bnst_inverse_tree[j][i]);
	    }
	    else {
		fprintf(Outfp, " ");
	    }
	}
	fprintf(Outfp, "\n");
    }
}

/*====================================================================
                               END
====================================================================*/
