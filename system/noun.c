/*====================================================================

			      ÃæªÏ∂Á≤Ú¿œ

                                               S.Kurohashi 2000.10.11

    $Id$
====================================================================*/
#include "knp.h"

DBM_FILE	noun_db;
int	NounExist = 0;

#define noun_get(word) (NounExist ? db_get(noun_db, word) : NULL)

/*==================================================================*/
			   void init_noun()
/*==================================================================*/
{
    char *db_filename;

    if (DICT[NOUN_DB]) {
	db_filename = (char *)check_dict_filename(DICT[NOUN_DB], TRUE);
    }
    else {
	db_filename = (char *)check_dict_filename(NOUN_DB_NAME, FALSE);
    }

    if ((noun_db = db_open(db_filename, O_RDONLY, 0)) == NULL) {
	fprintf(stderr, "Cannot open Noun Database <%s>.\n", db_filename);
	exit(1);
    }

    NounExist = 1;
    free(db_filename);
}

/*==================================================================*/
			  void close_noun()
/*==================================================================*/
{
    if (NounExist) {
	db_close(noun_db);
    }
}

/*==================================================================*/
		 char **GetDictDefinition(char *word)
/*==================================================================*/
{
    int breakflag = 0, msize = 5, count = 0;
    char *string, *cp, *startp, **ret;

    string = noun_get(word);

    if (!string) {
	return NULL;
    }

    ret = (char **)malloc_data(sizeof(char *)*msize, "GetDictDefinition");

    for (cp = startp = string; ; cp++) {
	if (*cp == '/') {
	    *cp = '\0';
	}
	else if (*cp == '\0') {
	    breakflag = 1;
	}
	else {
	    continue;
	}

	if (count >= msize-1) {
	    ret = (char **)realloc_data(ret, sizeof(char *)*(msize <<= 1), "GetDictDefinition");
	}

	*(ret+count++) = strdup(startp);

	if (breakflag) {
	    break;
	}
	startp = cp+1;
    }

    *(ret+count++) = NULL;
    free(string);
    return ret;
}

/*==================================================================*/
	   char **GetDefinitionFromBunsetsu(BNST_DATA *bp)
/*==================================================================*/
{
    int start, stop, i;
    char **def, buf[DATA_LEN];

    for (stop = 0; stop < bp->fuzoku_num; stop++) {
	if (!strcmp(Class[(bp->fuzoku_ptr+stop)->Hinshi][0].id, "ΩıªÏ") || 
	    !strcmp(Class[(bp->fuzoku_ptr+stop)->Hinshi][0].id, "»ΩƒÍªÏ") || 
	    !strcmp(Class[(bp->fuzoku_ptr+stop)->Hinshi][0].id, "Ωı∆∞ªÏ") || 
	    !strcmp(Class[(bp->fuzoku_ptr+stop)->Hinshi][0].id, "∆√ºÏ") || 
	    (!strcmp(Class[(bp->fuzoku_ptr+stop)->Hinshi][0].id, "¿‹»¯º≠") && 
	    strcmp(Class[(bp->fuzoku_ptr+stop)->Bunrui][0].id, "ÃæªÏ¿≠ÃæªÏ¿‹»¯º≠"))) {
	    break;
	}
    }

    stop += bp->settou_num+bp->jiritu_num;

    for (start =0 ; start < (bp->settou_num+bp->jiritu_num); start++) {
	*buf = '\0';
	for (i = start; i < stop; i++) {
	    if (strlen(buf)+strlen((bp->mrph_ptr+i)->Goi2)+2 > BNST_LENGTH_MAX) {
		fprintf(stderr, "Too long key <%s> in %s.\n", buf, "GetDefinitionFromBunsetsu");
		return NULL;
	    }
	    strcat(buf, (bp->mrph_ptr+i)->Goi2);
	}

	def = GetDictDefinition(buf);
	if (def) {
	    return def;
	}
    }
    return NULL;
}

/*====================================================================
                               END
====================================================================*/
