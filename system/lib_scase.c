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
	filename = check_dict_filename(DICT[SCASE_DB], FALSE);
    }
    else {
	filename = check_dict_filename(SCASE_DB_NAME, FALSE);
    }

    if (OptDisplay == OPT_DEBUG) {
	fprintf(Outfp, "Opening %s ... ", filename);
    }

    if ((scase_db = DB_open(filename, O_RDONLY, 0)) == NULL) {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("failed.\n", Outfp);
	}
	ScaseDicExist = FALSE;
#ifdef DEBUG
	fprintf(stderr, ";; Cannot open SCASE dictionary <%s>.\n", filename);
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
      DB_close(scase_db);
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
    int strt, end, i;
    char *cp, *ans, *anscp, *str_buffer, *vtype, voice[3];

    /* 入力は文節, タグ単位には対応していない */
    if (ptr->type != IS_BNST_DATA) {
	return;
    }

    /* 初期化: init_bnst でもしている */
    for (i = 0, cp = ptr->SCASE_code; i < SCASE_CODE_SIZE; i++, cp++) *cp = 0;

    if (ScaseDicExist == TRUE && 
	(vtype = check_feature(ptr->f, "用言")) && 
	strcmp(vtype, "用言:判")) { /* 判定詞ではない場合 */
	vtype += 5;

	if (ptr->voice == VOICE_UKEMI || 
	    ptr->voice == VOICE_MORAU) {
	    strcpy(voice, ":P");
	}
	else if (ptr->voice == VOICE_SHIEKI) {
	    strcpy(voice, ":C");
	}
	else {
	    voice[0] = '\0';
	}

	str_buffer = make_pred_string(ptr->head_tag_ptr); /* 最後のタグ単位 (「〜のは」の場合は1つ前) */
	strcat(str_buffer, ":");
	strcat(str_buffer, vtype);
	if (voice[0]) strcat(str_buffer, voice);

	ans = get_scase(str_buffer);

	if (ans != NULL) {
	    /* DEBUG 表示 */
	    if (OptDisplay == OPT_DEBUG) {
		char *print_buffer;

		print_buffer = (char *)malloc_data(strlen(str_buffer) + 10, "get_scase_code");
		sprintf(print_buffer, "SCASEUSE:%s", str_buffer);
		assign_cfeature(&(ptr->f), print_buffer);
		free(print_buffer);
	    }

	    cp = ptr->SCASE_code;
	    anscp = ans;
	    for (i = 0; i < SCASE_CODE_SIZE; i++) *cp++ = *anscp++;
	    free(ans);
	    free(str_buffer);
	    goto Match;
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
