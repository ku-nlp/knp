/*====================================================================

			    用言の共起情報

                                         Daisuke Kawahara 2003. 6. 26

    $Id$
====================================================================*/

#include "knp.h"

DBM_FILE	event_db;
int		EventDicExist;

/*==================================================================*/
			  void init_event()
/*==================================================================*/
{
    char *filename;

    if (DICT[EVENT_DB]) {
	filename = check_dict_filename(DICT[EVENT_DB], FALSE);
    }
    else {
	filename = check_dict_filename(EVENT_DB_NAME, FALSE);
    }

    if (OptDisplay == OPT_DEBUG) {
	fprintf(Outfp, "Opening %s ... ", filename);
    }

    if ((event_db = DB_open(filename, O_RDONLY, 0)) == NULL) {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("failed.\n", Outfp);
	}
	EventDicExist = FALSE;
#ifdef DEBUG
	fprintf(stderr, ";; Cannot open EVENT dictionary <%s>.\n", filename);
#endif
    } else {
	if (OptDisplay == OPT_DEBUG) {
	    fputs("done.\n", Outfp);
	}
	EventDicExist = TRUE;
    }
    free(filename);
}

/*==================================================================*/
			  void close_event()
/*==================================================================*/
{
    if (EventDicExist == TRUE) {
      DB_close(event_db);
    }
}

/*==================================================================*/
		      float get_event(char *cp)
/*==================================================================*/
{
    int i;
    char *value;
    float retval;

    value = db_get(event_db, cp);
    
    if (value) {
	retval = atof(value);
	free(value);
	return retval;
    }
    return 0;
}

/*==================================================================*/
		 char *make_voice_str(TAG_DATA *ptr)
/*==================================================================*/
{
    if (ptr->voice & VOICE_UKEMI) {
	return ":P";
    }
    else if (ptr->voice & VOICE_SHIEKI) {
	return ":C";
    }
    else if (ptr->voice & VOICE_SHIEKI_UKEMI) {
	return ":PC";
    }
    return NULL;
}

/*==================================================================*/
char *make_pred_str_with_cc(SENTENCE_DATA *sp, TAG_DATA *ptr, int flag)
/*==================================================================*/
{
    char *cp, *vtype, voice[3], *str, *ccstr;
    int closest;

    if (cp = check_feature(ptr->f, "用言")) {
	vtype = strdup(cp + 4);
    }
    else {
	vtype = strdup(":動");
    }

    if (cp = make_voice_str(ptr)) {
	strcpy(voice, cp);
    }
    else {
	voice[0] = '\0';
    }

    /* 用言部分 */
    str = make_pred_string(ptr);
    strcat(str, vtype);
    if (voice[0]) strcat(str, voice);
    free(vtype);

    if (flag == TRUE) {
	/* 直前格要素の取得 */
	closest = get_closest_case_component(sp, ptr->cpm_ptr);

	if (closest > -1) {
	    cp = pp_code_to_kstr(ptr->cpm_ptr->cf.pp[closest][0]);
	    ccstr = (char *)malloc_data(strlen(cp) + 
					strlen(ptr->cpm_ptr->elem_b_ptr[closest]->head_ptr->Goi) + 
					strlen(str) + 3, 
					"make_pred_str_with_cc");
	    sprintf(ccstr, "%s-%s-%s", ptr->cpm_ptr->elem_b_ptr[closest]->head_ptr->Goi, 
		    cp, str);
	    free(str);
	    return ccstr;
	}
    }
    return str;
}

/*==================================================================*/
	  float get_event_value(SENTENCE_DATA *sp1, TAG_DATA *p1, 
				SENTENCE_DATA *sp2, TAG_DATA *p2)
/*==================================================================*/
{
    char *cp, *str1, *str2, *buf;
    float val;

    if (EventDicExist == TRUE) {
	str1 = make_pred_str_with_cc(sp1, p1, TRUE);
	str2 = make_pred_str_with_cc(sp2, p2, TRUE);

	buf = (char *)malloc_data(strlen(str1) + strlen(str2) + 2, "get_event_value");
	sprintf(buf, "%s|%s", str1, str2);
	free(str1);
	free(str2);

	val = get_event(buf);

	/* backoff */
	if (val == 0) {
	    str1 = make_pred_str_with_cc(sp1, p1, FALSE);
	    str2 = make_pred_str_with_cc(sp2, p2, FALSE);

	    sprintf(buf, "%s|%s", str1, str2);
	    free(str1);
	    free(str2);

	    val = get_event(buf);
	}

	free(buf);
	return val;
    }
    return -1;
}

/*====================================================================
                               END
====================================================================*/
