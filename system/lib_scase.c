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
    if ((scase_db = DBM_open(SCASE_DB_NAME, O_RDONLY, 0)) == NULL) {
        /* fprintf(stderr, 
	   "Can not open Database <%s>.\n", SCASE_DB_NAME);
	   exit(1);
	*/
	ScaseDicExist = FALSE;
    } else {
	ScaseDicExist = TRUE;
    }
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

    key.dptr = cp;
    if ((key.dsize = strlen(cp)) >= DBM_KEY_MAX) {
	fprintf(stderr, "Too long key <%s>.\n", key_str);
	return NULL;
    }  
    
    content = DBM_fetch(scase_db, key);
    if (content.dptr) {
	strncpy(cont_str, content.dptr, content.dsize);
	for (i = 0; i < content.dsize; i++)
	  cont_str[i] -= '0';
	cont_str[content.dsize] = '\0';
#ifdef	GDBM
	free(content.dptr);
	content.dsize = 0;
#endif
	return cont_str;
    }
    else {
	return NULL;
    }
}

/*==================================================================*/
             void get_scase_code(BNST_DATA *ptr)
/*==================================================================*/
{
    int strt, end, last, stop, i;
    char *cp, *ans, str_buffer[256];    

    for (i = 0, cp = ptr->SCASE_code; i < 11; i++, cp++) *cp = 0;
    /* init_bnst でもしている */

    if (ScaseDicExist == TRUE &&
	(check_feature(ptr->f, "用言:強:動") ||
	 check_feature(ptr->f, "用言:強:形") ||
	 check_feature(ptr->f, "用言:弱"))) {

	/* まず付属語を固定，自立語を減らしていく */

	for (stop = 0; stop < ptr->fuzoku_num; stop++) 
	    if (!strcmp(Class[(ptr->fuzoku_ptr + stop)->Hinshi][0].id, "助詞"))
		break;

	for (last = stop; last >= 0; last--) {
	    end = ptr->settou_num + ptr->jiritu_num + last;
	    for (strt=0 ; strt<(ptr->settou_num + ptr->jiritu_num); strt++) {
		*str_buffer = '\0';
		for (i = strt; i < end; i++)
		  strcat(str_buffer, (ptr->mrph_ptr + i)->Goi);
		if ((ans = get_scase(str_buffer)) != NULL) {
		    cp = ptr->SCASE_code;
		    for (i = 0; i < 11; i++) *cp++ = *ans++;
		    goto Match;
		}
	    }
	}
    }

    /* 判定詞などの場合,
       表層格辞書がない場合, 
       または辞書にない用言の場合 */
    
    if (check_feature(ptr->f, "用言:強:判")) {
	ptr->SCASE_code[case2num("ガ格")] = 1;
    } 
    else if (check_feature(ptr->f, "用言:強:形")) {
	ptr->SCASE_code[case2num("ガ格")] = 1;
	ptr->SCASE_code[case2num("ニ格")] = 1;
	ptr->SCASE_code[case2num("ヨリ格")] = 1;
	ptr->SCASE_code[case2num("ト格")] = 1;
    } 
    else if (check_feature(ptr->f, "用言:強:動")) {
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

/*==================================================================*/
		      void set_pred_caseframe()
/*==================================================================*/
{
    int i;
    BNST_DATA  *b_ptr;

    for (i = 0, b_ptr = bnst_data; i < Bnst_num; i++, b_ptr++)
	if (check_feature(b_ptr->f, "用言:強") ||
	    check_feature(b_ptr->f, "用言:弱")) {

	    /* 以下の2つの処理はfeatureレベルで起動している */
	    /* set_pred_voice(b_ptr); ヴォイス */
	    /* get_scase_code(b_ptr); 表層格 */

 	    if (OptAnalysis == OPT_CASE ||
 		OptAnalysis == OPT_CASE2 ||
 		OptAnalysis == OPT_DISC)
		make_case_frames(b_ptr);/* 格フレーム */
	}
}

/*====================================================================
                               END
====================================================================*/
