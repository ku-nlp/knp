/*====================================================================

			     固有名詞処理

                                               S.Kurohashi 96. 7. 4
====================================================================*/
/* $Id$ */

#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <juman.h>
#include "path.h"
#include "const.h"
#include "dbm.h"
#include "extern.h"

DBM_FILE	proper_db, properc_db;
int		PROPERExist = 0;

NamedEntity NE_data[MRPH_MAX];

/*==================================================================*/
			   void init_proper()
/*==================================================================*/
{
    if ((proper_db = DBM_open(PROPER_DB_NAME, O_RDONLY, 0)) == NULL || 
	(properc_db = DBM_open(PROPERC_DB_NAME, O_RDONLY, 0)) == NULL) {
        /* 
	   fprintf(stderr, "Can not open Database <%s>.\n", PROPER_DB_NAME);
	   exit(1);
	*/
	PROPERExist = FALSE;
    } else {
	PROPERExist = TRUE;
    }
}

/*==================================================================*/
                    void close_proper()
/*==================================================================*/
{
    if (PROPERExist == TRUE) {
	DBM_close(proper_db);
	DBM_close(properc_db);
    }
}

/*==================================================================*/
                    char *get_proper(char *cp, DBM_FILE db)
/*==================================================================*/
{
    if (PROPERExist == FALSE) {
	cont_str[0] = '\0';
	return cont_str;
    }

    key.dptr = cp;
    if ((key.dsize = strlen(cp)) >= DBM_KEY_MAX) {
	fprintf(stderr, "Too long key <%s>.\n", key.dptr);
	cont_str[0] = '\0';
	return cont_str;
    }  
    
    content = DBM_fetch(db, key);
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

/*==================================================================*/
		   void _init_NE(struct _pos_s *p)
/*==================================================================*/
{
    p->Location = 0;
    p->Person = 0;
    p->Organization = 0;
    p->Artifact = 0;
    p->Others = 0;
}

/*==================================================================*/
		    void init_NE(NamedEntity *np)
/*==================================================================*/
{
    _init_NE(&(np->AnoB));
    _init_NE(&(np->BnoA));
    _init_NE(&(np->before));
    _init_NE(&(np->self));
    _init_NE(&(np->after));
}

/*==================================================================*/
	    void _store_NE(struct _pos_s *p, char *string)
/*==================================================================*/
{
    char *token, type[256];

    /* スペースで切るのです */
    token = strtok(string, " ");
    while (token) {
	sscanf(token, "%[^:]", type);
	if (str_eq(type, "地名")) {
	    p->Location = atoi(token+strlen(type)+1);
	}
	else if (str_eq(type, "人名")) {
	    p->Person = atoi(token+strlen(type)+1);
	}
	else if (str_eq(type, "組織名")) {
	    p->Organization = atoi(token+strlen(type)+1);
	}
	else if (str_eq(type, "固有名詞")) {
	    p->Artifact = atoi(token+strlen(type)+1);
	}
	else if (str_eq(type, "その他")) {
	    p->Others = atoi(token+strlen(type)+1);
	}
	token = strtok(NULL, " ");
    }
}

/*==================================================================*/
	    void store_NE(NamedEntity *np, char *feature)
/*==================================================================*/
{
    char type[256];

    sscanf(feature, "%[^:]", type);

    if (str_eq(type, "AのB")) {
	_store_NE(&(np->AnoB), feature+strlen(type)+1);
    }
    else if (str_eq(type, "BのA")) {
	_store_NE(&(np->BnoA), feature+strlen(type)+1);
    }
    else if (str_eq(type, "前")) {
	_store_NE(&(np->before), feature+strlen(type)+1);
    }
    else if (str_eq(type, "単語")) {
	_store_NE(&(np->self), feature+strlen(type)+1);
    }
    else if (str_eq(type, "後")) {
	_store_NE(&(np->after), feature+strlen(type)+1);
    }
}

/*==================================================================*/
		  float merge_ratio(int n1, int n2)
/*==================================================================*/
{
    return (float)n1/(n1+5);
}

/*==================================================================*/
void _NE2feature(struct _pos_s *p1, struct _pos_s *p2, FEATURE **fpp, char *type)
/*==================================================================*/
{
    int n1, n2, length, i;
    float ratio;
    char *buffer, element[5][13];

    n1 = p1->Location + p1->Person + p1->Organization + p1->Artifact + p1->Others;
    n2 = p2->Location + p2->Person + p2->Organization + p2->Artifact + p2->Others;

    ratio = merge_ratio(n1, n2);

    if (n1 || n2) {
	for (i = 0; i < 5; i++) {
	    element[i][0] = '\0';
	}
	if (p1->Location || p2->Location) {
	    sprintf(element[0], "地名:%.0f", 
		    (float)p1->Location/n1*100*ratio+(float)p2->Location/n2*100*(1-ratio));
	}
	if (p1->Person || p2->Person) {
	    sprintf(element[1], "人名:%.0f", 
		    (float)p1->Person/n1*100*ratio+(float)p2->Person/n2*100*(1-ratio));
	}
	if (p1->Organization || p2->Organization) {
	    sprintf(element[2], "組織名:%.0f", 
		    (float)p1->Organization/n1*100*ratio+(float)p2->Organization/n2*100*(1-ratio));
	}
	if (p1->Artifact || p2->Artifact) {
	    sprintf(element[3], "固有名詞:%.0f", 
		    (float)p1->Artifact/n1*100*ratio+(float)p2->Artifact/n2*100*(1-ratio));
	}
	if (p1->Others || p2->Others) {
	    sprintf(element[4], "その他:%.0f", 
		    (float)p1->Others/n1*100*ratio+(float)p2->Others/n2*100*(1-ratio));
	}

	length = 0;
	for (i = 0; i < 5; i++) {
	    if (element[i][0])
		length += strlen(element[i])+1;
	}
	buffer = (char *)malloc_data(strlen(type)+length+1, "_NE2feature");
	sprintf(buffer, "%s:", type);
	for (i = 0; i < 5; i++) {
	    if (element[i][0]) {
		strcat(buffer, element[i]);
		if (i != 4) strcat(buffer, " ");
	    }
	}

	assign_cfeature(fpp, buffer);
	free(buffer);
    }
}

/*==================================================================*/
  void NE2feature(NamedEntity *np1, NamedEntity *np2, FEATURE **fpp)
/*==================================================================*/
{
    _NE2feature(&(np1->AnoB), &(np2->AnoB), fpp, "AのB");
    _NE2feature(&(np1->BnoA), &(np2->BnoA), fpp, "BのA");
    _NE2feature(&(np1->before), &(np2->before), fpp, "前");
    _NE2feature(&(np1->self), &(np2->self), fpp, "単語");
    _NE2feature(&(np1->after), &(np2->after), fpp, "後");
}

/*==================================================================*/
		   char *check_class(MRPH_DATA *mp)
/*==================================================================*/
{
    if (check_feature(mp->f, "漢字"))
	return "漢字";
    else if (check_feature(mp->f, "ひらがな"))
	return "ひらがな";
    else if (check_feature(mp->f, "カタカナ"))
	return "カタカナ";
    else if (check_feature(mp->f, "混合"))
	return "混合";
    else if (check_feature(mp->f, "英語"))
	return "英語";
    else if (check_feature(mp->f, "数字"))
	return "数字";
    else if (check_feature(mp->f, "その他"))
	return "その他";
    return NULL;
}

/*==================================================================*/
    struct _pos_s _merge_NE(struct _pos_s *p1, struct _pos_s *p2)
/*==================================================================*/
{
    struct _pos_s p;
    int n1, n2;
    float ratio;

    n1 = p1->Location + p1->Person + p1->Organization + p1->Artifact + p1->Others;
    n2 = p2->Location + p2->Person + p2->Organization + p2->Artifact + p2->Others;
    ratio = merge_ratio(n1, n2);

    p.Location = ratio*p1->Location+(1-ratio)*p2->Location;
    p.Person = ratio*p1->Person+(1-ratio)*p2->Person;
    p.Organization = ratio*p1->Organization+(1-ratio)*p2->Organization;
    p.Artifact = ratio*p1->Artifact+(1-ratio)*p2->Artifact;
    p.Others = ratio*p1->Others+(1-ratio)*p2->Others;

    return p;
}

/*==================================================================*/
       NamedEntity merge_NE(NamedEntity *np1, NamedEntity *np2)
/*==================================================================*/
{
    NamedEntity ne;

    ne.AnoB = _merge_NE(&(np1->AnoB), &(np2->AnoB));
    ne.BnoA = _merge_NE(&(np1->BnoA), &(np2->BnoA));
    ne.before = _merge_NE(&(np1->before), &(np2->before));
    ne.self = _merge_NE(&(np1->self), &(np2->self));
    ne.after = _merge_NE(&(np1->after), &(np2->after));

    return ne;
}

/*==================================================================*/
		void assign_f_from_dic(MRPH_DATA *mp)
/*==================================================================*/
{
    char *dic_content, *pre_pos, *cp, *sm;
    char code[13];
    int i, smn;
    NamedEntity ne[2];

    code[12] = '\0';

    init_NE(&ne[0]);
    init_NE(&ne[1]);

    /* 表記による検索 */
    dic_content = get_proper(mp->Goi, proper_db);
    if (*dic_content != NULL) {
	for (cp = pre_pos = dic_content; *cp; cp++) {
	    if (*cp == '/') {
		*cp = '\0';
		store_NE(&ne[0], pre_pos);
		pre_pos = cp + 1;
	    }
	}
	store_NE(&ne[0], pre_pos);
    }

    /* ここで入力形態素に意味素を与えておく */
    sm = get_sm(mp->Goi);
    smn = strlen(sm);
    strncpy(mp->SM, sm, smn);	/* ★ */
    smn = smn/SM_CODE_SIZE;

    /* 意味素による検索 */
    if (!mp->SM[0]) return;
    for (i = 0; i < smn; i++) {
	code[0] = '1';
	code[1] = '\0';
	strncat(code, mp->SM+SM_CODE_SIZE*i+1, SM_CODE_SIZE-1);
	dic_content = get_proper(code, properc_db);
	if (*dic_content != NULL) {
	    for (cp = pre_pos = dic_content; *cp; cp++) {
		if (*cp == '/') {
		    *cp = '\0';
		    store_NE(&ne[1], pre_pos);
		    pre_pos = cp + 1;
		}
	    }
	    store_NE(&ne[1], pre_pos);
	}
    }
    /* 文字種による検索 */
    cp = check_class(mp);
    if (cp) {
	dic_content = get_proper(cp, properc_db);
	if (*dic_content != NULL) {
	    store_NE(&ne[1], dic_content);
	}
    }

    NE2feature(&ne[0], &ne[1], &(mp->f));
}

/*====================================================================
                               END
====================================================================*/
