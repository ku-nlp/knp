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
    while (len > pre && *(c1+pre) == *(c2+pre)) 
	pre++;

    post = 0;
    while (len > post && *(c1+len1-post-1) == *(c2+len2-post-1))
	post++;
    
    match = pre > post ? pre : post;
    match % 2 ? match -= 1 : NULL;
    return match;
}

/*==================================================================*/
  int check_fuzoku(BNST_DATA *ptr, int Hinshi, int Bunrui, char *cp)
/*==================================================================*/
{
    int	i;

    if (ptr == NULL) return 0;
    for (i = 0; i < ptr->fuzoku_num; i++) 
      if ((Hinshi == 0 || Hinshi == (ptr->fuzoku_ptr+i)->Hinshi) &&
	  (Bunrui == 0 || Bunrui == (ptr->fuzoku_ptr+i)->Bunrui) &&
	  (cp == NULL  || str_eq((ptr->fuzoku_ptr+i)->Goi, cp)))
	return 1;
    return 0;
}

/*==================================================================*/
int check_fuzoku_substr(BNST_DATA *ptr, int Hinshi, int Bunrui, char *cp)
/*==================================================================*/
{
    int	i;

    if (ptr == NULL) return 0;
    for (i = 0; i < ptr->fuzoku_num; i++) 
      if ((Hinshi == 0 || Hinshi == (ptr->fuzoku_ptr+i)->Hinshi) &&
	  (Bunrui == 0 || Bunrui == (ptr->fuzoku_ptr+i)->Bunrui) &&
	  (cp == NULL  || strstr((ptr->fuzoku_ptr+i)->Goi, cp)))
	return 1;
    return 0;
}

/*==================================================================*/
int check_bnst_substr(BNST_DATA *ptr, int Hinshi, int Bunrui, char *cp)
/*==================================================================*/
{
    int	i;

    if (ptr == NULL) return 0;
    for (i = 0; i < ptr->mrph_num; i++) 
      if ((Hinshi == 0 || Hinshi == (ptr->mrph_ptr+i)->Hinshi) &&
	  (Bunrui == 0 || Bunrui == (ptr->mrph_ptr+i)->Bunrui) &&
	  (cp == NULL  || strstr((ptr->mrph_ptr+i)->Goi, cp)))
	return 1;
    return 0;
}

/*==================================================================*/
int jiritu_fuzoku_check(BNST_DATA *ptr1, BNST_DATA *ptr2, char *cp)
/*==================================================================*/
{
    if ((str_eq(ptr1->Jiritu_Go, cp) && check_fuzoku(ptr2, 0, 0, cp)) ||
	(str_eq(ptr2->Jiritu_Go, cp) && check_fuzoku(ptr1, 0, 0, cp)))
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
    
    if (max_point < 3) 
	return 0;
    else
	return (max_point-2);
}

/*==================================================================*/
          int ntt_match(BNST_DATA *ptr1, BNST_DATA *ptr2)
/*==================================================================*/
{
    /* 返り値
       	一方でも NTT コードがない場合 	: -1
	満点				: BGH_CODE_SIZE-2 == 8	
     */

    int i, j;
    float point, max_point = 0;

    if (! *(ptr1->SM_code) || ! *(ptr2->SM_code))
	return -1;

    for (i = 0; ptr1->SM_code[i]; i+=SM_CODE_SIZE)
	for (j = 0; ptr2->SM_code[j]; j+=SM_CODE_SIZE) {
	    point = ntt_code_match(ptr1->SM_code+i, ptr2->SM_code+j, SM_EXPAND_NE);
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

    level1 = (char *)check_feature(ptr1->f, "レベル");
    level2 = (char *)check_feature(ptr2->f, "レベル");

    if (level1 == NULL) return TRUE;		/* なし:何でも -> T */
    else if (level2 == NULL) return FALSE;	/* 何でも:なし -> F */
    else if (levelcmp(level1 + strlen("レベル:"), 
		      level2 + strlen("レベル:")) <= 0)	/* ptr1 <= ptr2 -> T */
	return TRUE;
    else return FALSE;
}

/*==================================================================*/
	int subordinate_level_check(char *cp, BNST_DATA *ptr2)
/*==================================================================*/
{
    char *level1, *level2;

    level1 = cp;
    level2 = (char *)check_feature(ptr2->f, "レベル");

    if (level1 == NULL) return TRUE;		/* なし:何でも -> T */
    else if (level2 == NULL) return FALSE;	/* 何でも:なし -> F */
    else if (levelcmp(level1, level2 + strlen("レベル:")) <= 0)
	return TRUE;				/* cp <= ptr2 -> T */
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
    BNST_DATA 	*ptr1, *ptr2;

    ptr1 = &(sp->bnst_data[pre]);
    ptr2 = &(sp->bnst_data[pos]);

    /* 用言，体言 */

    if ((check_feature(ptr1->f, "用言") &&
	 check_feature(ptr2->f, "用言")) ||

	(check_feature(ptr1->f, "体言") &&
	 check_feature(ptr2->f, "体言")) || 
	
	/* 「的，」と「的だ」 */
	(check_feature(ptr1->f, "並キ:名") && 
	 check_feature(ptr1->f, "的") && 
	 check_feature(ptr2->f, "的"))
	 /* check_bnst_substr(ptr1, 0, 0, "的") && 
	    check_bnst_substr(ptr2, 0, 0, "的だ")) */
	) {

	/* ただし，判定詞 -- 体言 の類似度は 0 */
	if (check_feature(ptr1->f, "用言:判") &&
	    !check_feature(ptr1->f, "用言:判:?") && /* 「〜で」を除く */
	    check_feature(ptr2->f, "体言") &&
	    !check_feature(ptr2->f, "用言:判")) return 0;
	
	/* 「ため」「せい」と他の体言に類似度を与えないように */

	if ((check_feature(ptr1->f, "ため-せい") &&
	     !check_feature(ptr2->f, "ため-せい")) ||
	    (!check_feature(ptr1->f, "ため-せい") &&
	     check_feature(ptr2->f, "ため-せい"))) return 0;

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
		for (i = 0; i < ptr1->mrph_num; i++) 
		    for (j = 0; j < ptr2->mrph_num; j++) 
			if (str_eq((ptr1->mrph_ptr+i)->Goi, 
				   (ptr2->mrph_ptr+j)->Goi) &&
			    check_feature((ptr1->mrph_ptr+i)->f, "カウンタ") &&
			    check_feature((ptr2->mrph_ptr+j)->f, "カウンタ")) {
			    point += 5;
			    goto Counter_check;
			}
	      Counter_check:
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
	
	    if (str_eq(ptr1->Jiritu_Go, ptr2->Jiritu_Go)) {
		point += 10;
		
	    } else {

		/* シソーラスによる類似度 */

		if (ParaThesaurus == USE_NTT) {
		    mt_point = ntt_match(ptr1, ptr2) * 2;
		}
		else if (ParaThesaurus == USE_NONE) {
		    mt_point = -1;
		}
		else {
		    mt_point = bgh_match(ptr1, ptr2) * 2;
		}

		/* 自立語の部分一致 (少なくとも一方の意味属性コードがない場合) */
	    
		part_mt_point = 0;
		if (mt_point < 0) {
		    mt_point = 0;
		    if (check_feature(ptr1->f, "体言") &&
			check_feature(ptr2->f, "体言"))
			part_mt_point = str_part_cmp(ptr1->Jiritu_Go, ptr2->Jiritu_Go);
		}

		/* シソーラスと部分一致の得点は最大10 */

		if ((part_mt_point + mt_point) > 10)
		    point += 10;
		else
		    point += (part_mt_point + mt_point);
	    }
	}

	/* 付属語の一致 */
	
	for (i = 0; i < ptr1->fuzoku_num; i++) 
	    for (j = 0; j < ptr2->fuzoku_num; j++) 
	      if (str_eq((ptr1->fuzoku_ptr+i)->Goi, 
			 (ptr2->fuzoku_ptr+j)->Goi))
		  point += 2; /* 3 */

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
    
    return point;
}

/*==================================================================*/
	      void calc_match_matrix(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j;
    
    for (i = 0; i < sp->Bnst_num; i++) 
	for (j = i+1; j < sp->Bnst_num; j++)
	    match_matrix[i][j] = calc_match(sp, i, j);
}

/*====================================================================
                               END
====================================================================*/
