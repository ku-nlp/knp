/*====================================================================

		      意味素  検索プログラム

                                               S.Kurohashi 91. 6.25
                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

DBM_FILE	sm_db;
DBM_FILE	sm2code_db;
int		SMExist;
int		SM2CODEExist;

/*==================================================================*/
                    void init_sm()
/*==================================================================*/
{
    if ((sm_db = DBM_open(SM_DB_NAME, O_RDONLY, 0)) == NULL) {
        /* 
	   fprintf(stderr, "Can not open Database <%s>.\n", SM_DB_NAME);
	   exit(1);
	*/
	SMExist = FALSE;
    } else {
	SMExist = TRUE;
    }
}

/*==================================================================*/
                    void close_sm()
/*==================================================================*/
{
    if (SMExist == TRUE)
	DBM_close(sm_db);
}

/*==================================================================*/
                    char *get_sm(char *cp)
/*==================================================================*/
{
    int i, pos;

    key.dptr = cp;
    if ((key.dsize = strlen(cp)) >= DBM_KEY_MAX) {
	fprintf(stderr, "Too long key <%s>.\n", key_str);
	cont_str[0] = '\0';
	return cont_str;
    }  
    
    content = DBM_fetch(sm_db, key);
    if (content.dptr) {
	if (content.dsize > SM_CODE_SIZE*SM_CODE_MAX) {
	    fprintf(stderr, "Too long SM content <%.*s>.\n", content.dsize, content.dptr);
	    content.dsize = SM_CODE_SIZE*SM_CODE_MAX;
	}
	strncpy(cont_str, content.dptr, content.dsize);
	cont_str[content.dsize] = '\0';
#ifdef	GDBM
	free(content.dptr);
	content.dsize = 0;
#endif
    }
    else {
	cont_str[0] = '\0';
    }

    pos = 0;
    for (i = 0; cont_str[i]; i+=12) {
	if (cont_str[i] == '3' ||
	    cont_str[i] == '8' ||
	    cont_str[i] == '9' ||
	    cont_str[i] == 'b' ||
	    cont_str[i] == 'e' ||
	    cont_str[i] == 'f' ||
	    cont_str[i] == 'm' ||
	    cont_str[i] == 'n') {
	    strncpy(cont_str+pos, cont_str+i, 12);
	    pos += 12;
	}
    }
    cont_str[pos] = '\0';

    return cont_str;
}

/*==================================================================*/
             void get_sm_code(BNST_DATA *ptr)
/*==================================================================*/
{
    int strt, end, last, stop, i, overflow_flag;
    char str_buffer[BNST_LENGTH_MAX];    
    char feature_buffer[SM_CODE_SIZE*SM_CODE_MAX+1];

    str_buffer[BNST_LENGTH_MAX-1] = GUARD;

    /* 
       複合語の扱い
       		まず付属語を固定，自立語を減らしていく
		各形態素列に対してまず表記列で調べ，次に読み列で調べる
    */

    if (SMExist == FALSE) return;
    /* ptr->SM_num はinit_bnstで0に初期化されている */

    for (stop = 0; stop < ptr->fuzoku_num; stop++) 
	if (!strcmp(Class[(ptr->fuzoku_ptr + stop)->Hinshi][0].id, "助詞") ||
	    !strcmp(Class[(ptr->fuzoku_ptr + stop)->Hinshi][0].id, "判定詞"))
	    break;

    for (last = stop; last >= 0; last--) {
	end = ptr->settou_num + ptr->jiritu_num + last;
	for (strt =0 ; strt < (ptr->settou_num + ptr->jiritu_num); strt++) {

	    /* 表記のまま */

	    *str_buffer = '\0';
	    for (i = strt; i < end; i++) {
		strcat(str_buffer, (ptr->mrph_ptr + i)->Goi2);
		if (str_buffer[BNST_LENGTH_MAX-1] != GUARD) {
		    overflow_flag = 1;
		    overflowed_function(str_buffer, BNST_LENGTH_MAX, "get_sm_code");
		    break;
		}
	    }

	    if (overflow_flag) {
		overflow_flag = 0;
		continue;
	    }

	    strcpy(ptr->SM_code, get_sm(str_buffer));
	    if (*(ptr->SM_code)) goto Match;

	    /* 表記，最後原形 */

	    if (!str_eq((ptr->mrph_ptr + end - 1)->Goi,
			(ptr->mrph_ptr + end - 1)->Goi2)) {
		*str_buffer = '\0';
		for (i = strt; i < end - 1; i++) {
		    strcat(str_buffer, (ptr->mrph_ptr + i)->Goi2);
		    if (str_buffer[BNST_LENGTH_MAX-1] != GUARD) {
			overflow_flag = 1;
			overflowed_function(str_buffer, BNST_LENGTH_MAX, "get_sm_code");
			break;
		    }
		}

		if (overflow_flag) {
		    overflow_flag = 0;
		    continue;
		}

		strcat(str_buffer, (ptr->mrph_ptr + end - 1)->Goi);
		if (str_buffer[BNST_LENGTH_MAX-1] != GUARD) {
		    overflow_flag = 1;
		    overflowed_function(str_buffer, BNST_LENGTH_MAX, "get_sm_code");
		    break;
		}

		if (overflow_flag) {
		    overflow_flag = 0;
		    continue;
		}

		/* ナ形容詞の場合は語幹で検索 */
		if (str_eq(Class[(ptr->mrph_ptr + end - 1)->Hinshi][0].id,
			   "形容詞") &&
		    (str_eq(Type[(ptr->mrph_ptr + end - 1)->Katuyou_Kata].name,
			   "ナ形容詞") ||
		     str_eq(Type[(ptr->mrph_ptr + end - 1)->Katuyou_Kata].name,
			   "ナ形容詞特殊") ||
		     str_eq(Type[(ptr->mrph_ptr + end - 1)->Katuyou_Kata].name,
			   "ナノ形容詞"))) 
		    str_buffer[strlen(str_buffer)-2] = NULL;

		strcpy(ptr->SM_code, get_sm(str_buffer));
		if (*(ptr->SM_code)) goto Match;
	    }

	    /* 読みのまま */

	    *str_buffer = '\0';
	    for (i = strt; i < end; i++) {
		strcat(str_buffer, (ptr->mrph_ptr + i)->Yomi);
		if (str_buffer[BNST_LENGTH_MAX-1] != GUARD) {
		    overflow_flag = 1;
		    overflowed_function(str_buffer, BNST_LENGTH_MAX, "get_sm_code");
		    break;
		}
	    }

	    if (overflow_flag) {
		overflow_flag = 0;
		continue;
	    }

	    strcpy(ptr->SM_code, get_sm(str_buffer));
	    if (*(ptr->SM_code)) goto Match;
	}
    }
  Match:
    ptr->SM_num = strlen(ptr->SM_code) / SM_CODE_SIZE;

    sprintf(feature_buffer, "SM:%s", ptr->SM_code);
    assign_cfeature(&(ptr->f), feature_buffer);
}

/*==================================================================*/
                    void init_sm2code()
/*==================================================================*/
{
    if ((sm2code_db = DBM_open(SM2CODE_DB_NAME, O_RDONLY, 0)) == NULL) {
        /* 
	   fprintf(stderr, "Can not open Database <%s>.\n", SM2CODE_DB_NAME);
	   exit(1);
	*/
	SM2CODEExist = FALSE;
    } else {
	SM2CODEExist = TRUE;
    }
}

/*==================================================================*/
                    void close_sm2code()
/*==================================================================*/
{
    if (SM2CODEExist == TRUE)
	DBM_close(sm2code_db);
}

/*==================================================================*/
                    char *sm2code(char *cp)
/*==================================================================*/
{
    key.dptr = cp;
    if ((key.dsize = strlen(cp)) >= DBM_KEY_MAX) {
	fprintf(stderr, "Too long key <%s>.\n", key_str);
	cont_str[0] = '\0';
	return cont_str;
    }  
    
    content = DBM_fetch(sm2code_db, key);
    if (content.dptr) {
	strncpy(cont_str, content.dptr, content.dsize);
	cont_str[content.dsize] = '\0';
#ifdef	GDBM
	free(content.dptr);
	content.dsize = 0;
#endif
    }
    else {
	cont_str[0] = '\0';
    }

    return cont_str;
}

/*====================================================================
                               END
====================================================================*/
