/*====================================================================

		       ³Ê¹½Â¤²òÀÏ: ³Ê¥Õ¥ì¡¼¥àÂ¦

                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

FILE *ipal_fp;
DBM_FILE ipal_db;
char  key_str[DBM_KEY_MAX], cont_str[DBM_CON_MAX];
datum key, content;

CASE_FRAME 	Case_frame_array[ALL_CASE_FRAME_MAX]; 	/* ³Ê¥Õ¥ì¡¼¥à */
int 	   	Case_frame_num;				/* ³Ê¥Õ¥ì¡¼¥à¿ô */

IPAL_FRAME Ipal_frame;
char address_str[DBM_CON_MAX];

int	IPALExist;

/*
 *	IPAL ¤Î ÆÉ¤ß½Ð¤·
 */

/*==================================================================*/
			  void init_ipal()
/*==================================================================*/
{
    if ((ipal_fp = fopen(IPAL_DAT_NAME, "r")) == NULL) {
	/*
	  fprintf(stderr, "Cannot open IPAL data <%s>.\n", IPAL_DAT_NAME);
	  exit(1);
	  */
	IPALExist = FALSE;
    }
    else if ((ipal_db = DBM_open(IPAL_DB_NAME, O_RDONLY, 0)) == NULL) {
	fprintf(stderr, "Cannot open Database <%s>.\n", IPAL_DB_NAME);
	exit(1);
    } 
    else {
	IPALExist = TRUE;
    }
}

/*==================================================================*/
			  void close_ipal()
/*==================================================================*/
{
    if (IPALExist == TRUE) {
	fclose(ipal_fp);
	DBM_close(ipal_db);
    }
}

/*==================================================================*/
       int get_ipal_address(unsigned char *word, char *address)
/*==================================================================*/
{
    if (IPALExist == FALSE) return FALSE;

    key.dptr = word;
    if ((key.dsize = strlen(key.dptr)) >= DBM_KEY_MAX) {
	fprintf(stderr, "Too long key <%s>.\n", key_str);
	return 0;
    }  

    content = DBM_fetch(ipal_db, key);
    if (content.dptr) {
	strncpy(cont_str, content.dptr, content.dsize);
	cont_str[content.dsize] = '\0';
#ifdef	GDBM
	free(content.dptr);
	content.dsize = 0;
#endif
	strcpy(address, cont_str);
	return TRUE;
    }
    else {
	address[0] = '\0';
	return FALSE;
    }
}

/*==================================================================*/
       void get_ipal_frame(IPAL_FRAME *i_ptr, int address)
/*==================================================================*/
{
    fseek(ipal_fp, (long)address, 0);
    if (fread(i_ptr, sizeof(IPAL_FRAME), 1, ipal_fp) < 1) {
	fprintf(stderr, "Error in fread.\n");
	exit(1);
    }
}

/*
 *	IPAL -> ³Ê¥Õ¥ì¡¼¥à
 */

unsigned char ipal_str_buf[256];

/*==================================================================*/
unsigned char *extract_ipal_str(unsigned char *dat, unsigned char *ret)
/*==================================================================*/
{
    if (*dat == '\0' || !strcmp(dat, "¡ö")) 
      return NULL;

    while (1) {
	if (*dat == '\0') {
	    *ret = '\0';
	    return dat;
	}
	else if (*dat < 0x80) {
	    *ret++ = *dat++;
	}
	else if (!strncmp(dat, "¡¿", 2) || 
		 !strncmp(dat, "¡¤", 2) ||
		 !strncmp(dat, "¡¢", 2) ||
		 !strncmp(dat, "¡ö", 2)) {
	    *ret = '\0';
	    return dat+2;
	}
	else {
	    *ret++ = *dat++;
	    *ret++ = *dat++;
	}
    }
}

/*==================================================================*/
void _make_ipal_cframe_pp(CASE_FRAME *c_ptr, unsigned char *cp, int num)
/*==================================================================*/
{
    /* ½õ»ì¤ÎÆÉ¤ß¤À¤· */

    unsigned char *point;
    int pp_num = 0;

    if (!strcmp(cp+strlen(cp)-2, "¡ö"))
      c_ptr->oblig[num] = FALSE;
    else
      c_ptr->oblig[num] = TRUE;

    point = cp; 
    while (point = extract_ipal_str(point, ipal_str_buf))
      c_ptr->pp[num][pp_num++] = pp_kstr_to_code(ipal_str_buf);
    c_ptr->pp[num][pp_num] = END_M;
}
    
/*==================================================================*/
void _make_ipal_cframe_sm(CASE_FRAME *c_ptr, unsigned char *cp, int num)
/*==================================================================*/
{
    /* °ÕÌ£¥Þ¡¼¥«¤ÎÆÉ¤ß¤À¤· */

    unsigned char *point;
    int sm_num = 0;

    point = cp;
    c_ptr->sm[num][0] = '\0';
    while (point = extract_ipal_str(point, ipal_str_buf)) {
        if (ipal_str_buf[0] == '-') {
	  c_ptr->sm_flag[num][sm_num] = FALSE;
	  strcpy(ipal_str_buf, &ipal_str_buf[1]);
	}
	else
	  c_ptr->sm_flag[num][sm_num] = TRUE;

	strcpy(c_ptr->sm[num]+SM_CODE_SIZE*sm_num, 
	       (char *)sm2code(ipal_str_buf));
	sm_num++;
	if (sm_num >= SM_ELEMENT_MAX){
	  fprintf(stderr, "Not enough sm_num !!\n");
	  exit(1);
	}

	/* c_ptr->sm[num][sm_num++] = sm_zstr_to_code(); */
    }
}

/*==================================================================*/
void _make_ipal_cframe_ex(CASE_FRAME *c_ptr, unsigned char *cp, int num)
/*==================================================================*/
{
    /* Îã¤ÎÆÉ¤ß¤À¤· */

    unsigned char *point, *point2;
    int i;
    extern char *get_bgh();

    point = cp;
    c_ptr->ex[num][0] = '\0';
    while (point = extract_ipal_str(point, ipal_str_buf)) {
	
	/* ¡Ö£Á¤Î£Â¡×¤Î¡Ö£Â¡×¤À¤±¤ò½èÍý */
	point2 = ipal_str_buf;
	for (i = strlen(point2) - 2; i > 2; i -= 2) {
	    if (!strncmp(point2+i-2, "¤Î", 2)) {
		point2 += i;
		break;
	    }
	}
	strcat(c_ptr->ex[num], get_bgh(point2));
	if (strlen(c_ptr->ex[num]) >= EX_ELEMENT_MAX*10) {
	    fprintf(stderr, "Too many EX <%s>.\n", ipal_str_buf);
	    break;
	}
    }
}

/*==================================================================*/
		int check_agentive(unsigned char *cp)
/*==================================================================*/
{
    unsigned char *point;

    point = cp;
    while (point = extract_ipal_str(point, ipal_str_buf))
      if (!strcmp(ipal_str_buf, "£Á")) return TRUE;
    return FALSE;
}

/*==================================================================*/
     void _make_ipal_cframe(CASE_FRAME *cf_ptr, int address)
/*==================================================================*/
{
    int i, j = 0, ga_p = NULL;
    IPAL_FRAME *i_ptr = &Ipal_frame;
    char ast_cap[32];

    cf_ptr->ipal_address = address;
    strcpy(cf_ptr->ipal_id, i_ptr->DATA+i_ptr->id); 
    strcpy(cf_ptr->imi, i_ptr->DATA+i_ptr->imi);

    /* ³ÊÍ×ÁÇ¤ÎÄÉ²Ã */

    if (cf_ptr->voice == FRAME_PASSIVE_I ||
	cf_ptr->voice == FRAME_CAUSATIVE_WO_NI ||
	cf_ptr->voice == FRAME_CAUSATIVE_WO ||
	cf_ptr->voice == FRAME_CAUSATIVE_NI) {
	_make_ipal_cframe_pp(cf_ptr, "¥¬", j);
	_make_ipal_cframe_sm(cf_ptr, "£Ä£É£Ö¡¿£È£Õ£Í", j);
	_make_ipal_cframe_ex(cf_ptr, "Èà", j);
	j++;
    }

    /* ³Æ³ÊÍ×ÁÇ¤Î½èÍý */

    for (i = 0; i < 5 && *(i_ptr->DATA+i_ptr->kaku_keishiki[i]); i++, j++) { 
	_make_ipal_cframe_pp(cf_ptr, i_ptr->DATA+i_ptr->kaku_keishiki[i], j);
	_make_ipal_cframe_sm(cf_ptr, i_ptr->DATA+i_ptr->imisosei[i], j);
	_make_ipal_cframe_ex(cf_ptr, i_ptr->DATA+i_ptr->meishiku[i], j);

	/* Ç½Æ° : Agentive ¥¬³Ê¤òÇ¤°ÕÅª¤È¤¹¤ë¾ì¹ç
	if (cf_ptr->voice == FRAME_ACTIVE &&
	    i == 0 && 
	    cf_ptr->pp[i][0] == pp_kstr_to_code("¥¬") &&
	    check_agentive(i_ptr->DATA+i_ptr->jyutugoso) == TRUE)
	  cf_ptr->oblig[i] = FALSE;
	*/

	if (cf_ptr->voice == FRAME_CAUSATIVE_WO_NI ||	/* »ÈÌò */
	    cf_ptr->voice == FRAME_CAUSATIVE_WO ||
	    cf_ptr->voice == FRAME_CAUSATIVE_NI) {
	    /* ¥¬ ¢ª ¥ò¡¤¥Ë */
	    if (cf_ptr->pp[j][0] == pp_kstr_to_code("¥¬")) {
		if (ga_p == NULL) {
		    ga_p = TRUE;
		    if (cf_ptr->voice == FRAME_CAUSATIVE_WO_NI)
		      _make_ipal_cframe_pp(cf_ptr, "¥ò¡¿¥Ë", j);
		    else if (cf_ptr->voice == FRAME_CAUSATIVE_WO)
		      _make_ipal_cframe_pp(cf_ptr, "¥ò", j);
		    else if (cf_ptr->voice == FRAME_CAUSATIVE_NI)
		      _make_ipal_cframe_pp(cf_ptr, "¥Ë", j);
		} else {
		    _make_ipal_cframe_pp(cf_ptr, "¥ò", j); /* ¥¬¡¦¥¬¹½Ê¸ */
		}
	    }
	}
	else if (cf_ptr->voice == FRAME_PASSIVE_I ||	/* ¼õ¿È */
		 cf_ptr->voice == FRAME_PASSIVE_1 ||
		 cf_ptr->voice == FRAME_PASSIVE_2) {
	    /* ´ÖÀÜ ¥¬¢ª¥Ë¡¤Ä¾ÀÜ ¥¬¢ª¥Ë¡¿¥Ë¥è¥Ã¥Æ¡¿¡¥¡¥ */
	    if (!strcmp(i_ptr->DATA+i_ptr->kaku_keishiki[i], "¥¬")) {
		if (cf_ptr->voice == FRAME_PASSIVE_I)
		  _make_ipal_cframe_pp(cf_ptr, "¥Ë", j);
		else if (cf_ptr->voice == FRAME_PASSIVE_1)
		  _make_ipal_cframe_pp(cf_ptr, 
				       i_ptr->DATA+i_ptr->tyoku_ukemi1, j);
		else if (cf_ptr->voice == FRAME_PASSIVE_2)
		  _make_ipal_cframe_pp(cf_ptr, 
				       i_ptr->DATA+i_ptr->tyoku_ukemi2, j);
	    }
	    /* Ä¾ÀÜ ¥Ë¡¿¥Ë¥è¥Ã¥Æ¡¿¡¥¡¥¢ª¥¬ */
	    else if ((cf_ptr->voice == FRAME_PASSIVE_1 && 
		      (!strcmp(i_ptr->DATA+i_ptr->kaku_keishiki[i], 
			       i_ptr->DATA+i_ptr->tyoku_noudou1) ||
		       (sprintf(ast_cap, "%s¡ö", 
				i_ptr->DATA+i_ptr->tyoku_noudou1) &&
			!strcmp(i_ptr->DATA+i_ptr->kaku_keishiki[i], 
				ast_cap)))) ||
		     (cf_ptr->voice == FRAME_PASSIVE_2 && 
		      (!strcmp(i_ptr->DATA+i_ptr->kaku_keishiki[i], 
			       i_ptr->DATA+i_ptr->tyoku_noudou2) ||
		       (sprintf(ast_cap, "%s¡ö", 
				i_ptr->DATA+i_ptr->tyoku_noudou2) &&
			!strcmp(i_ptr->DATA+i_ptr->kaku_keishiki[i], 
				ast_cap))))) {
		_make_ipal_cframe_pp(cf_ptr, "¥¬", j);
	    }
	}
	else if (cf_ptr->voice == FRAME_POSSIBLE) {	/* ²ÄÇ½ */
	    if (!strcmp(i_ptr->DATA+i_ptr->kaku_keishiki[i], "¥ò")) {
		 _make_ipal_cframe_pp(cf_ptr, "¥¬¡¿¥ò", j);
	     }
	}
	else if (cf_ptr->voice == FRAME_SPONTANE) {	/* ¼«È¯ */
	    if (!strcmp(i_ptr->DATA+i_ptr->kaku_keishiki[i], "¥ò")) {
		 _make_ipal_cframe_pp(cf_ptr, "¥¬", j);
	     }
	}
    }
    cf_ptr->element_num = j;
}

/*==================================================================*/
   int make_ipal_cframe(BNST_DATA *b_ptr, CASE_FRAME *cf_ptr)
/*==================================================================*/
{
    IPAL_FRAME *i_ptr = &Ipal_frame;
    int f_num = 0, address;
    char *pre_pos, *cp;
    
    get_ipal_address(L_Jiritu_M(b_ptr)->Goi, address_str);

    if (address_str[0]) strcat(address_str, "/");
				/* ¥¢¥É¥ì¥¹¤Î¶èÀÚ¤ê"/"¤òËöÈø¤ËÉÕ²Ã */
 		 		/* ¼¡¤Îfor¥ë¡¼¥×¤òÃ±½ã¤Ë¤¹¤ë¤¿¤á */
    for (cp = pre_pos = address_str; *cp; cp++) {
	if (*cp == '/') {
	    *cp = '\0';
	    
	    /* IPAL¥Ç¡¼¥¿¤ÎÆÉ¤ß¤À¤· */
	    address = atoi(pre_pos);
	    get_ipal_frame(i_ptr, address);
	    pre_pos = cp + 1;

	    /* Ç½Æ°ÂÖ */
	    if (b_ptr->voice == NULL) {
		(cf_ptr+f_num)->voice = FRAME_ACTIVE;
		_make_ipal_cframe(cf_ptr+f_num, address);
		f_num++;
	    }

	    /* »ÈÌò */
	    if (b_ptr->voice == VOICE_SHIEKI ||
		b_ptr->voice == VOICE_MORAU) {
		if (!strcmp(i_ptr->DATA+i_ptr->sase, "¥ò»ÈÌò¡¤¥Ë»ÈÌò"))
		  (cf_ptr+f_num)->voice = FRAME_CAUSATIVE_WO_NI;
		else if (!strcmp(i_ptr->DATA+i_ptr->sase, "¥ò»ÈÌò"))
		  (cf_ptr+f_num)->voice = FRAME_CAUSATIVE_WO;
		else if (!strcmp(i_ptr->DATA+i_ptr->sase, "¥Ë»ÈÌò"))
		  (cf_ptr+f_num)->voice = FRAME_CAUSATIVE_NI;
		
		_make_ipal_cframe(cf_ptr+f_num, address);
		f_num++;
	    }
	    
	    /* ¼õ¿È */
	    if (b_ptr->voice == VOICE_UKEMI ||
		b_ptr->voice == VOICE_MORAU) {
		/* Ä¾ÀÜ¼õ¿È£± */
		if (*(i_ptr->DATA+i_ptr->tyoku_noudou1)) {
		    (cf_ptr+f_num)->voice = FRAME_PASSIVE_1;
		    _make_ipal_cframe(cf_ptr+f_num, address);
		    f_num++;
		}
		/* Ä¾ÀÜ¼õ¿È£² */
		if (*(i_ptr->DATA+i_ptr->tyoku_noudou2)) {
		    (cf_ptr+f_num)->voice = FRAME_PASSIVE_2;
		    _make_ipal_cframe(cf_ptr+f_num, address);
		    f_num++;
		}
		/* ´ÖÀÜ¼õ¿È */
		if (str_part_eq(i_ptr->DATA+i_ptr->rare, "´Ö¼õ")) {
		    (cf_ptr+f_num)->voice = FRAME_PASSIVE_I;
		    _make_ipal_cframe(cf_ptr+f_num, address);
		    f_num++;
		}
	    }
	    /* ²ÄÇ½¡¤Âº·É¡¤¼«È¯ */
	    if (b_ptr->voice == VOICE_UKEMI) {
		if (str_part_eq(i_ptr->DATA+i_ptr->rare, "²ÄÇ½")) {
		    (cf_ptr+f_num)->voice = FRAME_POSSIBLE;
		    _make_ipal_cframe(cf_ptr+f_num, address);
		    f_num++;
		}
		if (str_part_eq(i_ptr->DATA+i_ptr->rare, "Âº·É")) {
		    (cf_ptr+f_num)->voice = FRAME_POLITE;
		    _make_ipal_cframe(cf_ptr+f_num, address);
		    f_num++;
		}
		if (str_part_eq(i_ptr->DATA+i_ptr->rare, "¼«È¯")) {
		    (cf_ptr+f_num)->voice = FRAME_SPONTANE;
		    _make_ipal_cframe(cf_ptr+f_num, address);
		    f_num++;
		}
	    }
	}
    }
    return f_num;
}


/*==================================================================*/
   int make_default_cframe(BNST_DATA *b_ptr, CASE_FRAME *cf_ptr)
/*==================================================================*/
{
    int i, num = 0;

    _make_ipal_cframe_pp(cf_ptr, "¥¬¡ö", num++);
    _make_ipal_cframe_pp(cf_ptr, "¥ò¡ö", num++);
    _make_ipal_cframe_pp(cf_ptr, "¥Ë¡ö", num++);
    _make_ipal_cframe_pp(cf_ptr, "¥Ø¡ö", num++);
    _make_ipal_cframe_pp(cf_ptr, "¥è¥ê¡ö", num++);

    cf_ptr->ipal_address = -1;
    cf_ptr->element_num = num;

    for (i = 0; i < num; i++) {
	cf_ptr->pp[i][1] = END_M;
	cf_ptr->sm[i][0] = END_M;
	cf_ptr->ex[i][0] = '\0';
    }

    return 1;
}

/*==================================================================*/
	      void make_case_frames(BNST_DATA *b_ptr)
/*==================================================================*/
{
    int f_num;
	
    if ((f_num = make_ipal_cframe(b_ptr, Case_frame_array + Case_frame_num)) 
	!= 0) {
	b_ptr->cf_ptr = Case_frame_array + Case_frame_num;
	b_ptr->cf_num = f_num;
	
	if ((Case_frame_num += f_num) > ALL_CASE_FRAME_MAX) {
	    fprintf(stderr, "Not enough Case_frame_array !!\n");
	    exit(1);
	}
    } else {
	make_default_cframe(b_ptr, Case_frame_array + Case_frame_num);
	b_ptr->cf_ptr = Case_frame_array + Case_frame_num;
	b_ptr->cf_num = 1;
	/* fprintf(stderr, "No entry for <%s> in IPAL.\n",b_ptr->Jiritu_Go); */

	if ((Case_frame_num += 1) > ALL_CASE_FRAME_MAX) {
	    fprintf(stderr, "Not enough Case_frame_array !!\n");
	    exit(1);
	}
    }
}

/*====================================================================
                               END
====================================================================*/
