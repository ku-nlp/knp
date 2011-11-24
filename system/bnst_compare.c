/*====================================================================

		       文節間の比較・類似度計算

                                               S.Kurohashi 91. 6.25
                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

/*==================================================================*/
              int str_part_cmp(char *c1, char *c2)
/*==================================================================*/
{
    int len, len1, len2, pre, post, match;
    
    len1 = strlen(c1);
    len2 = strlen(c2);    
    len = len1 < len2 ? len1 : len2;
    
    pre = 0;
    while (len > pre && *(c1 + pre) == *(c2 + pre)) {
	pre++;
    }

    post = 0;
    while (len > post && *(c1 + len1 - post - 1) == *(c2 + len2 - post - 1)) {
	post++;
    }
    
    match = pre > post ? pre : post;
    match -= match % BYTES4CHAR;
    match = 2 * match / BYTES4CHAR; /* 5文字で10点 */
    return match;
}

/*==================================================================*/
  int check_fuzoku(BNST_DATA *ptr, int Hinshi, int Bunrui, char *cp)
/*==================================================================*/
{
    int	i;

    /* 一致する付属語があれば真 */

    if (ptr == NULL) return 0;
    for (i = ptr->mrph_num - 1; i >= 0 ; i--) {
	if (check_feature((ptr->mrph_ptr + i)->f, "付属")) {
	    if ((Hinshi == 0 || Hinshi == (ptr->mrph_ptr + i)->Hinshi) &&
		(Bunrui == 0 || Bunrui == (ptr->mrph_ptr + i)->Bunrui) &&
		(cp == NULL  || str_eq((ptr->mrph_ptr + i)->Goi, cp))) {
		return 1;
	    }
	}
	/* 自立語など */
	else {
	    return 0;
	}
    }
    return 0;
}

/*==================================================================*/
int check_fuzoku_substr(BNST_DATA *ptr, int Hinshi, int Bunrui, char *cp)
/*==================================================================*/
{
    int	i;

    if (ptr == NULL) return 0;
    for (i = ptr->mrph_num - 1; i >= 0 ; i--) {
	if (check_feature((ptr->mrph_ptr + i)->f, "付属")) {
	    if ((Hinshi == 0 || Hinshi == (ptr->mrph_ptr + i)->Hinshi) &&
		(Bunrui == 0 || Bunrui == (ptr->mrph_ptr + i)->Bunrui) &&
		(cp == NULL  || strstr((ptr->mrph_ptr + i)->Goi, cp))) {
		return 1;
	    }
	}
	/* 自立語など */
	else {
	    return 0;
	}
    }
    return 0;
}

/*==================================================================*/
int check_bnst_substr(BNST_DATA *ptr, int Hinshi, int Bunrui, char *cp)
/*==================================================================*/
{
    int	i;

    if (ptr == NULL) return 0;
    for (i = 0; i < ptr->mrph_num; i++) 
      if ((Hinshi == 0 || Hinshi == (ptr->mrph_ptr + i)->Hinshi) &&
	  (Bunrui == 0 || Bunrui == (ptr->mrph_ptr + i)->Bunrui) &&
	  (cp == NULL  || strstr((ptr->mrph_ptr + i)->Goi, cp)))
	return 1;
    return 0;
}

/*==================================================================*/
int jiritu_fuzoku_check(BNST_DATA *ptr1, BNST_DATA *ptr2, char *cp)
/*==================================================================*/
{
    if ((str_eq(ptr1->head_ptr->Goi, cp) && check_fuzoku(ptr2, 0, 0, cp)) || 
	(str_eq(ptr2->head_ptr->Goi, cp) && check_fuzoku(ptr1, 0, 0, cp)))
	return 1;
    else 
	return 0;
}

/*==================================================================*/
          int bgh_match(BNST_DATA *ptr1, BNST_DATA *ptr2)
/*==================================================================*/
{
    /* 返り値
       	一方でも分類語彙表コードがない場合 	: -1
	3桁未満の一致				: 0
	3桁以上一致している場合			: (一致桁数 - 2)
     */

    int i, j, point, max_point = 0;

    if (! *(ptr1->BGH_code) || ! *(ptr2->BGH_code))
	return -1;

    for (i = 0; ptr1->BGH_code[i]; i+=BGH_CODE_SIZE)
	for (j = 0; ptr2->BGH_code[j]; j+=BGH_CODE_SIZE) {
	    point = bgh_code_match(ptr1->BGH_code+i, ptr2->BGH_code+j);
	    if (max_point < point) max_point = point;
	}

    return Max(max_point - 2, 0);
}

/*==================================================================*/
	    int sm_match(BNST_DATA *ptr1, BNST_DATA *ptr2)
/*==================================================================*/
{
    /* 返り値
       	一方でも NTT コードがない場合 	: -1
	満点				: BGH_CODE_SIZE-2 == 8	
     */

    int i, j, code_size;
    float point, max_point = 0;

    if (! *(ptr1->SM_code) || ! *(ptr2->SM_code))
	return -1;

    code_size = THESAURUS[ParaThesaurus].code_size;

    for (i = 0; ptr1->SM_code[i]; i+=code_size)
	for (j = 0; ptr2->SM_code[j]; j+=code_size) {
	    if (ParaThesaurus == USE_NTT) {
		point = ntt_code_match(ptr1->SM_code+i, ptr2->SM_code+j, SM_EXPAND_NE);
	    }
	    else {
		point = general_code_match(&THESAURUS[ParaThesaurus], ptr1->SM_code+i, ptr2->SM_code+j);
	    }
	    if (max_point < point) max_point = point;
	}

    /* 類似度 0.4 以下は切る */
    max_point = (max_point-0.4)*(BGH_CODE_SIZE-2)/(BGH_CODE_SIZE-4)*BGH_CODE_SIZE;
    if (max_point < 0)
	return 0;
    else
	return (int)(max_point);
}

/*==================================================================*/
    int subordinate_level_comp(BNST_DATA *ptr1, BNST_DATA *ptr2)
/*==================================================================*/
{
    char *level1, *level2;

    level1 = check_feature(ptr1->f, "レベル");
    level2 = check_feature(ptr2->f, "レベル");

    if (level1 == NULL) return TRUE;		/* なし:何でも -> T */
    else if (level2 == NULL) return FALSE;	/* 何でも:なし -> F */
    else if (levelcmp(level1 + strlen("レベル:"), 
		      level2 + strlen("レベル:")) <= 0)	/* ptr1 <= ptr2 -> T */
	return TRUE;
    else return FALSE;
}

/*==================================================================*/
	int subordinate_level_check(char *cp, FEATURE *f)
/*==================================================================*/
{
    char *level1, *level2;

    level1 = cp;
    level2 = check_feature(f, "レベル");

    if (level1 == NULL) return TRUE;		/* なし:何でも -> T */
    else if (level2 == NULL) return FALSE;	/* 何でも:なし -> F */
    else if (levelcmp(level1, level2 + strlen("レベル:")) <= 0)
	return TRUE;				/* cp <= f -> T */
    else return FALSE;
}

/*==================================================================*/
		  int levelcmp(char *cp1, char *cp2)
/*==================================================================*/
{
    int level1, level2;
    if (!strcmp(cp1, "A-"))      level1 = 1;
    else if (!strcmp(cp1, "A"))  level1 = 2;
    else if (!strcmp(cp1, "B-")) level1 = 3;
    else if (!strcmp(cp1, "B"))  level1 = 4;
    else if (!strcmp(cp1, "B+")) level1 = 5;
    else if (!strcmp(cp1, "C"))  level1 = 6;
    else fprintf(stderr, "Invalid level (%s)\n", cp1);
    if (!strcmp(cp2, "A-"))      level2 = 1;
    else if (!strcmp(cp2, "A"))  level2 = 2;
    else if (!strcmp(cp2, "B-")) level2 = 3;
    else if (!strcmp(cp2, "B"))  level2 = 4;
    else if (!strcmp(cp2, "B+")) level2 = 5;
    else if (!strcmp(cp2, "C"))  level2 = 6;
    else fprintf(stderr, "Invalid level (%s)\n", cp2);
    return level1 - level2;
}

/*==================================================================*/
	 int calc_match(SENTENCE_DATA *sp, int pre, int pos)
/*==================================================================*/
{
    int		i, j, part_mt_point, mt_point, point = 0;
    int		flag1, flag2, content_word_match;
    char	*counter1, *counter2;
    char        str1[4], str2[4];
    char        str1_bk[WORD_LEN_MAX], str2_bk[WORD_LEN_MAX];
    char	*cp1, *cp2;
    BNST_DATA 	*ptr1, *ptr2;
    float       similarity;

    ptr1 = &(sp->bnst_data[pre]);
    ptr2 = &(sp->bnst_data[pos]);

    /* 用言，体言 */

    if (Language != CHINESE) {
	if (((cp1 = check_feature(ptr1->f, "用言")) &&
	     (cp2 = check_feature(ptr2->f, "用言")) && 
	     (!strcmp(cp1, "用言:判") || 
	      strcmp(cp2, "用言:判") || 
	      !check_feature(ptr2->f, "形副名詞"))) || /* 前側が動詞、形容詞なら、後側は判定詞の形副名詞ではない */

	    (check_feature(ptr1->f, "名詞的形容詞語幹") && /* 「公正jかつ自由なj競争」 */
	     check_feature(ptr2->f, "用言:形")) || 

	    (check_feature(ptr1->f, "体言") &&
	     check_feature(ptr2->f, "体言")) || 

	    (check_feature(ptr1->f, "数量") && /* 「一、二泊する」では体言、用言となるため */
	     check_feature(ptr2->f, "数量")) || 
	
	    /* 「的，」と「的だ」 */
	    (check_feature(ptr1->f, "並キ:名") && 
	     check_feature(ptr1->f, "類似計算:的") && 
	     check_feature(ptr2->f, "類似計算:的"))
	    /* check_bnst_substr(ptr1, 0, 0, "的") && 
	       check_bnst_substr(ptr2, 0, 0, "的だ")) */
	    ) {

	    /* ただし，判定詞 -- 体言 の類似度は 0 */
	    if (check_feature(ptr1->f, "用言:判") &&
		!check_feature(ptr1->f, "並キ:？") && /* 「〜ではなく」「ですとか」を除く */
		check_feature(ptr2->f, "体言") &&
		!check_feature(ptr2->f, "用言:判")) return 0;
	
	    /* 「ため」「せい」と他の体言に類似度を与えないように */

	    if ((check_feature(ptr1->f, "ため-せい") &&
		 !check_feature(ptr2->f, "ため-せい")) ||
		(!check_feature(ptr1->f, "ため-せい") &&
		 check_feature(ptr2->f, "ため-せい"))) return 0;

	    /* 複合辞とそれ以外も類似度 0 */

	    if ((check_feature(ptr1->f, "複合辞") &&
		 !check_feature(ptr2->f, "複合辞")) ||
		(!check_feature(ptr1->f, "複合辞") &&
		 check_feature(ptr2->f, "複合辞"))) return 0;

	    point += 2;

	    if (check_feature(ptr1->f, "体言") &&
		check_feature(ptr2->f, "体言")) {

		/* 
		   体言同士の場合
		   ・人名同士 -- 5
		   ・地名同士 -- 5
		   ・組織名同士 -- 5
		   ・人名地名組織名 -- 2 (形態素解析のズレを考慮)
		   ・数量同士 -- 2 (続く名詞(助数辞)で評価)
		   ※ 助数辞が一致しなくても類似することもある
		   例)「人口は八万七千人だったが、人口増加率は一位で、…」
		   ・時間同士 -- 2			
		   ・その他同士 -- 自立語の比較
		*/

		if (check_feature(ptr1->f, "人名")) {
		    flag1 = 0;
		} else if (check_feature(ptr1->f, "地名")) {
		    flag1 = 1;
		} else if (check_feature(ptr1->f, "組織名")) {
		    flag1 = 2;
		} else if (check_feature(ptr1->f, "数量")) {
		    flag1 = 3;
		    /* } else if (check_feature(ptr1->f, "時間")) {
		       flag1 = 4; */
		} else {
		    flag1 = 5;
		}

		if (check_feature(ptr2->f, "人名")) {
		    flag2 = 0;
		} else if (check_feature(ptr2->f, "地名")) {
		    flag2 = 1;
		} else if (check_feature(ptr2->f, "組織名")) {
		    flag2 = 2;
		} else if (check_feature(ptr2->f, "数量")) {
		    flag2 = 3;
		    /* } else if (check_feature(ptr2->f, "時間")) {
		       flag2 = 4; */
		} else {
		    flag2 = 5;
		}

		if (flag1 == 0 && flag2 == 0) {
		    point += 5;
		    content_word_match = 0;
		}
		else if (flag1 == 1 && flag2 == 1) {
		    point += 5;
		    content_word_match = 0;
		}
		else if (flag1 == 2 && flag2 == 2) {
		    point += 5;
		    content_word_match = 0;
		}
		else if ((flag1 == 0 || flag1 == 1 || flag1 == 2) &&
			 (flag2 == 0 || flag2 == 1 || flag2 == 2)) {
		    point += 2;	/* 組織と人名などの対応を考慮 */
		    content_word_match = 0;
		}
		else if (flag1 == 3 && flag2 == 3) {
		    point += 2;

		    counter1 = check_feature(ptr1->f, "カウンタ");
		    counter2 = check_feature(ptr2->f, "カウンタ");
		    if ((!counter1 && !counter2) ||
			!counter1 ||
			(counter1 && counter2 && !strcmp(counter1, counter2))) {
			point += 5;
		    }
		    content_word_match = 0;
		}
		else if (flag1 == 4 && flag2 == 4) {
		    point += 2;
		    content_word_match = 0;
		}
		else if (flag1 == 5 && flag2 == 5) {
		    content_word_match = 1;
		}
		else {
		    content_word_match = 0;
		}
	    }
	    else {
		content_word_match = 1;
	    }

	    if (content_word_match == 1) {

		/* 自立語の一致 */
	
		/* if (str_eq(ptr1->head_ptr->Goi, ptr2->head_ptr->Goi)) { */
		if (str_eq(ptr1->Jiritu_Go, ptr2->Jiritu_Go)) {
		    point += 10;
		
		} else {

		    /* シソーラスによる類似度 */

		    if (ParaThesaurus == USE_NONE) {
			mt_point = -1;
		    }
                    else if (ParaThesaurus == USE_DISTSIM) {
                        mt_point = calc_distsim_from_bnst(ptr1, ptr2) * 2;
                    }
		    else if (ParaThesaurus == USE_BGH) {
			mt_point = bgh_match(ptr1, ptr2) * 2;
		    }
		    else {
			mt_point = sm_match(ptr1, ptr2) * 2;
		    }

		    if (check_feature(ptr1->f, "用言") &&
			check_feature(ptr2->f, "用言")) {
		    
			/* ★要整理★ 「する」のシソーラス類似度は最大2 */
			if (str_eq(ptr1->Jiritu_Go, "する") ||
			    str_eq(ptr2->Jiritu_Go, "する")) {
			    mt_point = Min(mt_point, 2);
			}
		
			/* 前が敬語，後が敬語でなければ類似度をおさえる
			   例)今日決心できるかどうか[分かりませんが、構わなければ]見せて欲しいんです */
			if (check_feature(ptr1->f, "敬語") &&
			    !check_feature(ptr2->f, "敬語")) {
			    mt_point = Min(mt_point, 2);
			}
		    }		    

		    /* 自立語の部分一致 (少なくとも一方の意味属性コードがない場合) */
	    
		    part_mt_point = 0;
		    if (mt_point < 0) {
			mt_point = 0;
			if (check_feature(ptr1->f, "体言") &&
			    check_feature(ptr2->f, "体言"))
			    part_mt_point = str_part_cmp(ptr1->head_ptr->Goi, ptr2->head_ptr->Goi);
		    }

		    /* シソーラスと部分一致の得点は最大10 */
		    point += Min(part_mt_point + mt_point, 10);
		}
	    }

	    /* 主辞形態素より後, 接尾辞以外の付属語の一致 */

	    for (i = ptr1->mrph_num - 1; i >= 0 ; i--) {
		if (check_feature((ptr1->mrph_ptr + i)->f, "付属") && 
		    ptr1->mrph_ptr + i > ptr1->head_ptr) {
		    if (!strcmp(Class[(ptr1->mrph_ptr + i)->Hinshi][0].id, "接尾辞")) {
			continue;
		    }
		    for (j = ptr2->mrph_num - 1; j >= 0 ; j--) {
			if (check_feature((ptr2->mrph_ptr + j)->f, "付属") && 
			    ptr2->mrph_ptr + j > ptr2->head_ptr) {
			    if (!strcmp(Class[(ptr2->mrph_ptr + j)->Hinshi][0].id, "接尾辞")) {
				continue;
			    }
			    if (str_eq((ptr1->mrph_ptr + i)->Goi, 
				       (ptr2->mrph_ptr + j)->Goi)) {
				point += 2; /* 3 */
			    }
			}
			else {
			    break;
			}
		    }
		}
		else {
		    break;
		}
	    }

	    if ((check_feature(ptr1->f, "〜れる") &&
		 check_feature(ptr2->f, "〜られる")) ||
		(check_feature(ptr1->f, "〜られる") &&
		 check_feature(ptr2->f, "〜れる"))) { 
		point += 2;
	    }
	    if ((check_feature(ptr1->f, "〜せる") &&
		 check_feature(ptr2->f, "〜させる")) ||
		(check_feature(ptr1->f, "〜させる") &&
		 check_feature(ptr2->f, "〜せる"))) { 
		point += 2;
	    }
	    if ((check_feature(ptr1->f, "〜ない") &&
		 check_feature(ptr2->f, "〜ぬ")) ||
		(check_feature(ptr1->f, "〜ぬ") &&
		 check_feature(ptr2->f, "〜ない"))) { 
		point += 2;
	    }
	    if (check_feature(ptr1->f, "タリ") &&
		check_feature(ptr2->f, "タリ")) { 
		point += 2;
	    }

	    /* 追加 */

	    if (check_feature(ptr1->f, "提題") &&
		check_feature(ptr2->f, "提題"))
		point += 3;

	    /* 「する」,「できる」などの自立語付属語のずれ */

	    if (jiritu_fuzoku_check(ptr1, ptr2, "する"))
		point += 1;

	    if (jiritu_fuzoku_check(ptr1, ptr2, "できる") ||
		jiritu_fuzoku_check(ptr1, ptr2, "出来る"))
		point += 3;
	}
    }
    else { /*for Chinese*/
	/* if there is a PU between two words except for DunHao, similarity is 0 */
	for (i = ptr1->num + 1; i < ptr2->num; i++) {
	    if (check_feature((sp->bnst_data+i)->f, "PU") && strcmp((sp->bnst_data+i)->head_ptr->Goi, "、") && strcmp((sp->bnst_data+i)->head_ptr->Goi, "；")) {
		point = 0;
		return point;
	    }
	}

	/* Add point for nouns with similar characters */
	if ((check_feature(ptr1->f, "NN") && check_feature(ptr2->f, "NN")) ||
	    (check_feature(ptr1->f, "NR") && check_feature(ptr2->f, "NR"))) {
	    for (i = 3; i <= (strlen(ptr1->head_ptr->Goi)  < strlen(ptr2->head_ptr->Goi) ? strlen(ptr1->head_ptr->Goi) : strlen(ptr2->head_ptr->Goi)); i += 3) {
		strcpy(str1, "   ");
		strcpy(str2, "   ");
		strncpy(str1, ptr1->head_ptr->Goi + (strlen(ptr1->head_ptr->Goi) - i), 3);
		strncpy(str2, ptr2->head_ptr->Goi + (strlen(ptr2->head_ptr->Goi) - i), 3);
		if (strcmp(str1,str2) != 0) {
		    break;
		}
	    }
	    if (i > 3 && i < strlen(ptr1->head_ptr->Goi) && i < strlen(ptr2->head_ptr->Goi)) {
		point += 5;
	    }
	}
	
	/* Normalize figures and add point for similar words regardless of figures */
	if ((check_feature(ptr1->f, "CD") && check_feature(ptr2->f, "CD")) || 
	    (check_feature(ptr1->f, "OD") && check_feature(ptr2->f, "OD")) || 
	    ((check_feature(ptr1->f, "NT") || check_feature(ptr1->f, "NT-SHORT"))&& check_feature(ptr2->f, "NT"))) {
	    strcpy(str1_bk, "");
	    strcpy(str2_bk, "");
	    for (i = 0; i < strlen(ptr1->head_ptr->Goi) - 3; i+=3) {
		strcpy(str1, "   ");
		strncpy(str1, ptr1->head_ptr->Goi + i, 3);
		if (!is_figure(str1)) {
		    strcat(str1_bk, str1);
		}
	    }
	    for (i = 0; i < strlen(ptr2->head_ptr->Goi) - 3; i+=3) {
		strcpy(str2, "   ");
		strncpy(str2, ptr2->head_ptr->Goi + i, 3);
		if (!is_figure(str2)) {
		    strcat(str2_bk, str2);
		}
	    }
	    if (!strcmp(str1_bk, str2_bk)) {
		point += 5;
	    }
	}


	if (!strcmp(ptr1->head_ptr->Goi, ptr2->head_ptr->Goi)) {
	    point += 5;
	}
	else {
	        /* Do not calculate similarity for PU */
	    if (!check_feature(ptr1->f, "PU") && !check_feature(ptr2->f, "PU")) {
		similarity = similarity_chinese(ptr1->head_ptr->Goi, ptr2->head_ptr->Goi);
	    }

	    point = 10 * similarity;
	}
	    
	if (!strcmp(ptr1->head_ptr->Pos, ptr2->head_ptr->Pos)) {
	    point += 2;

	}
	if (point > 12) {
	    point = 12;
	}
    }
    
    return point;
}

/*==================================================================*/
	      void calc_match_matrix(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, calc_flag;
    
    for (i = 0; i < sp->Bnst_num; i++) {
	calc_flag = 1;
	for (j = i+1; j < sp->Bnst_num; j++) {
	    if (calc_flag) {
		match_matrix[i][j] = calc_match(sp, i, j);

		if (OptParaFix == FALSE && /* 確率的並列構造解析のとき */
		    !check_feature(sp->bnst_data[i].f, "用言") && /* サ変動詞は止めないようにするため「用言」の否定 */
		    check_feature(sp->bnst_data[j].f, "Ｔ名並終点")) { /* 名並終点以降は並列にしない */
		    calc_flag = 0;
		}
	    }
	    else {
		match_matrix[i][j] = 0;
	    }
	}
    }
}

/*==================================================================*/
	      int is_figure(char *s)
/*==================================================================*/
{
    int value = 0;
    if (!strcmp(s, "０") || 
	!strcmp(s, "１") || 
	!strcmp(s, "２") || 
	!strcmp(s, "３") || 
	!strcmp(s, "４") || 
	!strcmp(s, "５") || 
	!strcmp(s, "６") || 
	!strcmp(s, "７") || 
	!strcmp(s, "８") || 
	!strcmp(s, "９") || 
	!strcmp(s, "．") || 
	!strcmp(s, "一") || 
	!strcmp(s, "二") || 
	!strcmp(s, "两") || 
	!strcmp(s, "三") || 
	!strcmp(s, "四") || 
	!strcmp(s, "五") || 
	!strcmp(s, "六") || 
	!strcmp(s, "七") || 
	!strcmp(s, "八") || 
	!strcmp(s, "九") || 
	!strcmp(s, "十") || 
	!strcmp(s, "零") || 
	!strcmp(s, "百") || 
	!strcmp(s, "千") || 
	!strcmp(s, "万") || 
	!strcmp(s, "$ARZ(B") || 
	!strcmp(s, "点") || 
	!strcmp(s, "分") || 
	!strcmp(s, "之") ||
	!strcmp(s, "年") || 
	!strcmp(s, "月") || 
	!strcmp(s, "日")) {
	value = 1;
    }
    return value;
}

/*====================================================================
                               END
====================================================================*/
