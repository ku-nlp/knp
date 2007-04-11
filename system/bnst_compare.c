/*====================================================================

		        ∏¿·¥÷§Œ»Ê≥”°¶Œ‡ª˜≈Ÿ∑◊ªª

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
    match = 2 * match / BYTES4CHAR; /* 5 ∏ª˙§«10≈¿ */
    return match;
}

/*==================================================================*/
  int check_fuzoku(BNST_DATA *ptr, int Hinshi, int Bunrui, char *cp)
/*==================================================================*/
{
    int	i;

    /* ∞Ï√◊§π§Î…’¬∞∏Ï§¨§¢§Ï§–øø */

    if (ptr == NULL) return 0;
    for (i = ptr->mrph_num - 1; i >= 0 ; i--) {
	if (check_feature((ptr->mrph_ptr + i)->f, "…’¬∞")) {
	    if ((Hinshi == 0 || Hinshi == (ptr->mrph_ptr + i)->Hinshi) &&
		(Bunrui == 0 || Bunrui == (ptr->mrph_ptr + i)->Bunrui) &&
		(cp == NULL  || str_eq((ptr->mrph_ptr + i)->Goi, cp))) {
		return 1;
	    }
	}
	/* º´Œ©∏Ï§ §… */
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
	if (check_feature((ptr->mrph_ptr + i)->f, "…’¬∞")) {
	    if ((Hinshi == 0 || Hinshi == (ptr->mrph_ptr + i)->Hinshi) &&
		(Bunrui == 0 || Bunrui == (ptr->mrph_ptr + i)->Bunrui) &&
		(cp == NULL  || strstr((ptr->mrph_ptr + i)->Goi, cp))) {
		return 1;
	    }
	}
	/* º´Œ©∏Ï§ §… */
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
    /*  ÷§Í√Õ
       	∞Ï ˝§«§‚ ¨Œ‡∏Ï◊√…Ω•≥°º•…§¨§ §§æÏπÁ 	: -1
	3∑ÂÃ§À˛§Œ∞Ï√◊				: 0
	3∑Â∞ æÂ∞Ï√◊§∑§∆§§§ÎæÏπÁ			: (∞Ï√◊∑ÂøÙ - 2)
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
    /*  ÷§Í√Õ
       	∞Ï ˝§«§‚ NTT •≥°º•…§¨§ §§æÏπÁ 	: -1
	À˛≈¿				: BGH_CODE_SIZE-2 == 8	
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

    /* Œ‡ª˜≈Ÿ 0.4 ∞ ≤º§œ¿⁄§Î */
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

    level1 = check_feature(ptr1->f, "•Ï•Ÿ•Î");
    level2 = check_feature(ptr2->f, "•Ï•Ÿ•Î");

    if (level1 == NULL) return TRUE;		/* § §∑:≤ø§«§‚ -> T */
    else if (level2 == NULL) return FALSE;	/* ≤ø§«§‚:§ §∑ -> F */
    else if (levelcmp(level1 + strlen("•Ï•Ÿ•Î:"), 
		      level2 + strlen("•Ï•Ÿ•Î:")) <= 0)	/* ptr1 <= ptr2 -> T */
	return TRUE;
    else return FALSE;
}

/*==================================================================*/
	int subordinate_level_check(char *cp, FEATURE *f)
/*==================================================================*/
{
    char *level1, *level2;

    level1 = cp;
    level2 = check_feature(f, "•Ï•Ÿ•Î");

    if (level1 == NULL) return TRUE;		/* § §∑:≤ø§«§‚ -> T */
    else if (level2 == NULL) return FALSE;	/* ≤ø§«§‚:§ §∑ -> F */
    else if (levelcmp(level1, level2 + strlen("•Ï•Ÿ•Î:")) <= 0)
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
    BNST_DATA 	*ptr1, *ptr2;
    float       similarity;

    ptr1 = &(sp->bnst_data[pre]);
    ptr2 = &(sp->bnst_data[pos]);

    /* Õ—∏¿°§¬Œ∏¿ */

    if (Language != CHINESE) {
	if ((check_feature(ptr1->f, "Õ—∏¿") &&
	     check_feature(ptr2->f, "Õ—∏¿")) ||

	    (check_feature(ptr1->f, "¬Œ∏¿") &&
	     check_feature(ptr2->f, "¬Œ∏¿")) || 

	    (check_feature(ptr1->f, "øÙŒÃ") && /* °÷∞Ï°¢∆Û«Ò§π§Î°◊§«§œ¬Œ∏¿°¢Õ—∏¿§»§ §Î§ø§· */
	     check_feature(ptr2->f, "øÙŒÃ")) || 
	
	    /* °÷≈™°§°◊§»°÷≈™§¿°◊ */
	    (check_feature(ptr1->f, " ¬•≠:Ãæ") && 
	     check_feature(ptr1->f, "Œ‡ª˜∑◊ªª:≈™") && 
	     check_feature(ptr2->f, "Œ‡ª˜∑◊ªª:≈™"))
	    /* check_bnst_substr(ptr1, 0, 0, "≈™") && 
	       check_bnst_substr(ptr2, 0, 0, "≈™§¿")) */
	    ) {

	    /* §ø§¿§∑°§»ΩƒÍªÏ -- ¬Œ∏¿ §ŒŒ‡ª˜≈Ÿ§œ 0 */
	    if (check_feature(ptr1->f, "Õ—∏¿:»Ω") &&
		!check_feature(ptr1->f, " ¬•≠:°©") && /* °÷°¡§«§œ§ §Ø°◊°÷§«§π§»§´°◊§ÚΩ¸§Ø */
		check_feature(ptr2->f, "¬Œ∏¿") &&
		!check_feature(ptr2->f, "Õ—∏¿:»Ω")) return 0;
	
	    /* °÷§ø§·°◊°÷§ª§§°◊§»¬æ§Œ¬Œ∏¿§ÀŒ‡ª˜≈Ÿ§ÚÕø§®§ §§§Ë§¶§À */

	    if ((check_feature(ptr1->f, "§ø§·-§ª§§") &&
		 !check_feature(ptr2->f, "§ø§·-§ª§§")) ||
		(!check_feature(ptr1->f, "§ø§·-§ª§§") &&
		 check_feature(ptr2->f, "§ø§·-§ª§§"))) return 0;

	    /*  £πÁº≠§»§Ω§Ï∞ ≥∞§‚Œ‡ª˜≈Ÿ 0 */

	    if ((check_feature(ptr1->f, " £πÁº≠") &&
		 !check_feature(ptr2->f, " £πÁº≠")) ||
		(!check_feature(ptr1->f, " £πÁº≠") &&
		 check_feature(ptr2->f, " £πÁº≠"))) return 0;

	    point += 2;

	    if (check_feature(ptr1->f, "¬Œ∏¿") &&
		check_feature(ptr2->f, "¬Œ∏¿")) {

		/* 
		   ¬Œ∏¿∆±ªŒ§ŒæÏπÁ
		   °¶øÕÃæ∆±ªŒ -- 5
		   °¶√œÃæ∆±ªŒ -- 5
		   °¶¡»ø•Ãæ∆±ªŒ -- 5
		   °¶øÕÃæ√œÃæ¡»ø•Ãæ -- 2 (∑¡¬÷¡«≤Ú¿œ§Œ•∫•Ï§ÚπÕŒ∏)
		   °¶øÙŒÃ∆±ªŒ -- 2 (¬≥§ØÃæªÏ(ΩıøÙº≠)§«…æ≤¡)
		   ¢® ΩıøÙº≠§¨∞Ï√◊§∑§ §Ø§∆§‚Œ‡ª˜§π§Î§≥§»§‚§¢§Î
		   Œ„)°÷øÕ∏˝§œ»¨À¸º∑¿ÈøÕ§¿§√§ø§¨°¢øÕ∏˝¡˝≤√Œ®§œ∞Ï∞Ã§«°¢°ƒ°◊
		   °¶ª˛¥÷∆±ªŒ -- 2			
		   °¶§Ω§Œ¬æ∆±ªŒ -- º´Œ©∏Ï§Œ»Ê≥”
		*/

		if (check_feature(ptr1->f, "øÕÃæ")) {
		    flag1 = 0;
		} else if (check_feature(ptr1->f, "√œÃæ")) {
		    flag1 = 1;
		} else if (check_feature(ptr1->f, "¡»ø•Ãæ")) {
		    flag1 = 2;
		} else if (check_feature(ptr1->f, "øÙŒÃ")) {
		    flag1 = 3;
		    /* } else if (check_feature(ptr1->f, "ª˛¥÷")) {
		       flag1 = 4; */
		} else {
		    flag1 = 5;
		}

		if (check_feature(ptr2->f, "øÕÃæ")) {
		    flag2 = 0;
		} else if (check_feature(ptr2->f, "√œÃæ")) {
		    flag2 = 1;
		} else if (check_feature(ptr2->f, "¡»ø•Ãæ")) {
		    flag2 = 2;
		} else if (check_feature(ptr2->f, "øÙŒÃ")) {
		    flag2 = 3;
		    /* } else if (check_feature(ptr2->f, "ª˛¥÷")) {
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
		    point += 2;	/* ¡»ø•§»øÕÃæ§ §…§Œ¬–±˛§ÚπÕŒ∏ */
		    content_word_match = 0;
		}
		else if (flag1 == 3 && flag2 == 3) {
		    point += 2;

		    counter1 = check_feature(ptr1->f, "•´•¶•Û•ø");
		    counter2 = check_feature(ptr2->f, "•´•¶•Û•ø");
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

		/* º´Œ©∏Ï§Œ∞Ï√◊ */
	
		/* if (str_eq(ptr1->head_ptr->Goi, ptr2->head_ptr->Goi)) { */
		if (str_eq(ptr1->Jiritu_Go, ptr2->Jiritu_Go)) {
		    point += 10;
		
		} else {

		    /* •∑•Ω°º•È•π§À§Ë§ÎŒ‡ª˜≈Ÿ */

		    if (ParaThesaurus == USE_NONE) {
			mt_point = -1;
		    }
		    else if (ParaThesaurus == USE_BGH) {
			mt_point = bgh_match(ptr1, ptr2) * 2;
		    }
		    else {
			mt_point = sm_match(ptr1, ptr2) * 2;
		    }

		    if (check_feature(ptr1->f, "Õ—∏¿") &&
			check_feature(ptr2->f, "Õ—∏¿")) {
		    
			/* °˙Õ◊¿∞Õ˝°˙ °÷§π§Î°◊§Œ•∑•Ω°º•È•πŒ‡ª˜≈Ÿ§œ∫«¬Á2 */
			if (str_eq(ptr1->Jiritu_Go, "§π§Î") ||
			    str_eq(ptr2->Jiritu_Go, "§π§Î")) {
			    mt_point = Min(mt_point, 2);
			}
		
			/* ¡∞§¨∑…∏Ï°§∏Â§¨∑…∏Ï§«§ §±§Ï§–Œ‡ª˜≈Ÿ§Ú§™§µ§®§Î
			   Œ„)∫£∆¸∑Ëø¥§«§≠§Î§´§…§¶§´[ ¨§´§Í§ﬁ§ª§Û§¨°¢πΩ§Ô§ §±§Ï§–]∏´§ª§∆Õﬂ§∑§§§Û§«§π */
			if (check_feature(ptr1->f, "∑…∏Ï") &&
			    !check_feature(ptr2->f, "∑…∏Ï")) {
			    mt_point = Min(mt_point, 2);
			}
		    }		    

		    /* º´Œ©∏Ï§Œ…Ù ¨∞Ï√◊ (æØ§ §Ø§»§‚∞Ï ˝§Œ∞’Ã£¬∞¿≠•≥°º•…§¨§ §§æÏπÁ) */
	    
		    part_mt_point = 0;
		    if (mt_point < 0) {
			mt_point = 0;
			if (check_feature(ptr1->f, "¬Œ∏¿") &&
			    check_feature(ptr2->f, "¬Œ∏¿"))
			    part_mt_point = str_part_cmp(ptr1->head_ptr->Goi, ptr2->head_ptr->Goi);
		    }

		    /* •∑•Ω°º•È•π§»…Ù ¨∞Ï√◊§Œ∆¿≈¿§œ∫«¬Á10 */
		    point += Min(part_mt_point + mt_point, 10);
		}
	    }

	    /* ºÁº≠∑¡¬÷¡«§Ë§Í∏Â, ¿‹»¯º≠∞ ≥∞§Œ…’¬∞∏Ï§Œ∞Ï√◊ */

	    for (i = ptr1->mrph_num - 1; i >= 0 ; i--) {
		if (check_feature((ptr1->mrph_ptr + i)->f, "…’¬∞") && 
		    ptr1->mrph_ptr + i > ptr1->head_ptr) {
		    if (!strcmp(Class[(ptr1->mrph_ptr + i)->Hinshi][0].id, "¿‹»¯º≠")) {
			continue;
		    }
		    for (j = ptr2->mrph_num - 1; j >= 0 ; j--) {
			if (check_feature((ptr2->mrph_ptr + j)->f, "…’¬∞") && 
			    ptr2->mrph_ptr + j > ptr2->head_ptr) {
			    if (!strcmp(Class[(ptr2->mrph_ptr + j)->Hinshi][0].id, "¿‹»¯º≠")) {
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

	    if ((check_feature(ptr1->f, "°¡§Ï§Î") &&
		 check_feature(ptr2->f, "°¡§È§Ï§Î")) ||
		(check_feature(ptr1->f, "°¡§È§Ï§Î") &&
		 check_feature(ptr2->f, "°¡§Ï§Î"))) { 
		point += 2;
	    }
	    if ((check_feature(ptr1->f, "°¡§ª§Î") &&
		 check_feature(ptr2->f, "°¡§µ§ª§Î")) ||
		(check_feature(ptr1->f, "°¡§µ§ª§Î") &&
		 check_feature(ptr2->f, "°¡§ª§Î"))) { 
		point += 2;
	    }
	    if ((check_feature(ptr1->f, "°¡§ §§") &&
		 check_feature(ptr2->f, "°¡§Ã")) ||
		(check_feature(ptr1->f, "°¡§Ã") &&
		 check_feature(ptr2->f, "°¡§ §§"))) { 
		point += 2;
	    }
	    if (check_feature(ptr1->f, "•ø•Í") &&
		check_feature(ptr2->f, "•ø•Í")) { 
		point += 2;
	    }

	    /* ƒ…≤√ */

	    if (check_feature(ptr1->f, "ƒÛ¬Í") &&
		check_feature(ptr2->f, "ƒÛ¬Í"))
		point += 3;

	    /* °÷§π§Î°◊,°÷§«§≠§Î°◊§ §…§Œº´Œ©∏Ï…’¬∞∏Ï§Œ§∫§Ï */

	    if (jiritu_fuzoku_check(ptr1, ptr2, "§π§Î"))
		point += 1;

	    if (jiritu_fuzoku_check(ptr1, ptr2, "§«§≠§Î") ||
		jiritu_fuzoku_check(ptr1, ptr2, "Ω–ÕË§Î"))
		point += 3;
	}
    }
    else { /*for Chinese*/
	/* if there is a PU between two words except for DunHao, similarity is 0 */
	for (i = ptr1->num + 1; i < ptr2->num; i++) {
	    if (check_feature((sp->bnst_data+i)->f, "PU") && strcmp((sp->bnst_data+i)->head_ptr->Goi, "°¢") && strcmp((sp->bnst_data+i)->head_ptr->Goi, "°®")) {
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

	similarity = similarity_chinese(ptr1, ptr2);

	if (similarity > 0.29) {
	    point = 10 * similarity + 7.1;
	}
	else {
	    point = 10 * similarity;
	}

	if (!strcmp(ptr1->head_ptr->Goi, ptr2->head_ptr->Goi)) {
	    point += 5;
	}
	if (!strcmp(ptr1->head_ptr->Pos, ptr2->head_ptr->Pos)) {
	    point += 2;

	}
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

/*==================================================================*/
	      int is_figure(char *s)
/*==================================================================*/
{
    int value = 0;
    if (!strcmp(s, "£∞") || 
	!strcmp(s, "£±") || 
	!strcmp(s, "£≤") || 
	!strcmp(s, "£≥") || 
	!strcmp(s, "£¥") || 
	!strcmp(s, "£µ") || 
	!strcmp(s, "£∂") || 
	!strcmp(s, "£∑") || 
	!strcmp(s, "£∏") || 
	!strcmp(s, "£π") || 
	!strcmp(s, "°•") || 
	!strcmp(s, "∞Ï") || 
	!strcmp(s, "∆Û") || 
	!strcmp(s, "è∞®") || 
	!strcmp(s, "ª∞") || 
	!strcmp(s, "ªÕ") || 
	!strcmp(s, "∏ﬁ") || 
	!strcmp(s, "œª") || 
	!strcmp(s, "º∑") || 
	!strcmp(s, "»¨") || 
	!strcmp(s, "∂Â") || 
	!strcmp(s, "ΩΩ") || 
	!strcmp(s, "ŒÌ") || 
	!strcmp(s, "…¥") || 
	!strcmp(s, "¿È") || 
	!strcmp(s, "À¸") || 
	!strcmp(s, "$ARZ(B") || 
	!strcmp(s, "≈¿") || 
	!strcmp(s, " ¨") || 
	!strcmp(s, "«∑") ||
	!strcmp(s, "«Ø") || 
	!strcmp(s, "∑Ó") || 
	!strcmp(s, "∆¸")) {
	value = 1;
    }
    return value;
}

/*====================================================================
                               END
====================================================================*/
