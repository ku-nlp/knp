/*====================================================================

			      表層格情報

                                               S.Kurohashi 92.10.21
                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

DBM_FILE	scase_db;
int		ScaseDicExist;

/*==================================================================*/
			  void init_scase()
/*==================================================================*/
{
    char *filename;

    if (DICT[SCASE_DB]) {
	filename = (char *)check_dict_filename(DICT[SCASE_DB], FALSE);
    }
    else {
	filename = (char *)check_dict_filename(SCASE_DB_NAME, FALSE);
    }

    if (OptDisplay == OPT_DEBUG) {
	fprintf(Outfp, "Opening %s ... ", filename);
    }

    if ((scase_db = DBM_open(filename, O_RDONLY, 0)) == NULL) {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("failed.\n", Outfp);
	}
	ScaseDicExist = FALSE;
#ifdef DEBUG
	fprintf(stderr, "Cannot open SCASE dictionary <%s>.\n", filename);
#endif
    } else {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("done.\n", Outfp);
	}
	ScaseDicExist = TRUE;
    }
    free(filename);
}

/*==================================================================*/
                    void close_scase()
/*==================================================================*/
{
    if (ScaseDicExist == TRUE)
      DBM_close(scase_db);
}

/*==================================================================*/
                    char *get_scase(char *cp)
/*==================================================================*/
{
    int i;
    char *value;

    if (ScaseDicExist == FALSE)
	return NULL;

    value = db_get(scase_db, cp);
    
    if (value) {
	for (i = 0; *(value+i) != '\0'; i++)
	  *(value+i) -= '0';
	return value;
    }
    else {
	return NULL;
    }
}

/*==================================================================*/
		 void get_scase_code(BNST_DATA *ptr)
/*==================================================================*/
{
    int strt, end, last, stop, i, overflow_flag = 0;
    char *cp, *ans, *anscp, str_buffer[2*BNST_LENGTH_MAX], *vtype, *predicate;

    str_buffer[BNST_LENGTH_MAX-1] = GUARD;

    for (i = 0, cp = ptr->SCASE_code; i < 11; i++, cp++) *cp = 0;
    /* init_bnst でもしている */

    if (ScaseDicExist == TRUE &&
	(vtype = check_feature(ptr->f, "用言")) && 
	strcmp(vtype, "用言:判")) {
	vtype += 5;

	/* まず付属語を固定，自立語を減らしていく */

	for (stop = 0; stop < ptr->fuzoku_num; stop++) 
	    if (!strcmp(Class[(ptr->fuzoku_ptr + stop)->Hinshi][0].id, "助詞"))
		break;

	for (last = stop; last >= 0; last--) {
	    end = ptr->settou_num + ptr->jiritu_num + last;
	    for (strt=0 ; strt<(ptr->settou_num + ptr->jiritu_num); strt++) {
		*str_buffer = '\0';
		for (i = strt; i < end; i++) {
		    strcat(str_buffer, (ptr->mrph_ptr + i)->Goi);
		    if (str_buffer[BNST_LENGTH_MAX-1] != GUARD) {
			overflow_flag = 1;
			overflowed_function(str_buffer, BNST_LENGTH_MAX, "get_scase_code");
			break;
		    }
		}

		if (overflow_flag) {
		    overflow_flag = 0;
		    return;
		}

		/* 用言タイプを含まない辞書の場合 */
		ans = get_scase(str_buffer);
		if (ans == NULL) {
		    /* 用言タイプを含む辞書の場合 */
		    predicate = strdup(str_buffer);
		    if (ptr->num > 0) {
			cp = check_feature((ptr-1)->f, "係");
			if (cp) {
			    sprintf(str_buffer, "%s:%s:%s:%s", L_Jiritu_M((ptr-1)), cp+3, predicate, vtype);
			    ans = get_scase(str_buffer);
			}
		    }
		    if (ans == NULL) {
			sprintf(str_buffer, "%s:%s", predicate, vtype);
			ans = get_scase(str_buffer);
		    }
		    /* DEBUG 表示 */
		    if (OptDisplay == OPT_DEBUG) {
			if (ans != NULL) {
			    char print_buffer[2*BNST_LENGTH_MAX];
			    sprintf(print_buffer, "SCASEUSE:%s", str_buffer);
			    assign_cfeature(&(ptr->f), print_buffer);
			}
		    }
		    free(predicate);
		}
		if (ans != NULL) {
		    cp = ptr->SCASE_code;
		    anscp = ans;
		    for (i = 0; i < 11; i++) *cp++ = *anscp++;
		    free(ans);
		    goto Match;
		}
	    }
	}
    }

    /* 判定詞などの場合,
       表層格辞書がない場合, 
       または辞書にない用言の場合 */
    
    if (check_feature(ptr->f, "用言:判")) {
	ptr->SCASE_code[case2num("ガ格")] = 1;
    } 
    else if (check_feature(ptr->f, "用言:形")) {
	ptr->SCASE_code[case2num("ガ格")] = 1;
	ptr->SCASE_code[case2num("ニ格")] = 1;
	/* 形容詞の表層格の付与は副作用が多いので制限
	ptr->SCASE_code[case2num("ヨリ格")] = 1;
	ptr->SCASE_code[case2num("ト格")] = 1;
	*/
    } 
    else if (check_feature(ptr->f, "用言:動")) {
	ptr->SCASE_code[case2num("ガ格")] = 1;
	ptr->SCASE_code[case2num("ヲ格")] = 1;
	ptr->SCASE_code[case2num("ニ格")] = 1;
	ptr->SCASE_code[case2num("ヘ格")] = 1;
	ptr->SCASE_code[case2num("ト格")] = 1;
    }

  Match:

    /* ヴォイスによる修正 */

    if (ptr->voice == VOICE_SHIEKI) {
	ptr->SCASE_code[case2num("ヲ格")] = 1;
	ptr->SCASE_code[case2num("ニ格")] = 1;
    } else if (ptr->voice == VOICE_UKEMI) {
	ptr->SCASE_code[case2num("ニ格")] = 1;
    } else if (ptr->voice == VOICE_MORAU) {
	ptr->SCASE_code[case2num("ヲ格")] = 1;
	ptr->SCASE_code[case2num("ニ格")] = 1;
    }
}

/*====================================================================
                               END
====================================================================*/
