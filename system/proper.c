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

DBM_FILE	proper_db, properc_db, propercase_db;
int		PROPERExist = 0;

/*==================================================================*/
			   void init_proper()
/*==================================================================*/
{
    if ((proper_db = DBM_open(PROPER_DB_NAME, O_RDONLY, 0)) == NULL || 
	(properc_db = DBM_open(PROPERC_DB_NAME, O_RDONLY, 0)) == NULL || 
	(propercase_db = DBM_open(PROPERCASE_DB_NAME, O_RDONLY, 0)) == NULL) {
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
	DBM_close(propercase_db);
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
	    p->Location += atoi(token+strlen(type)+1);
	}
	else if (str_eq(type, "人名")) {
	    p->Person += atoi(token+strlen(type)+1);
	}
	else if (str_eq(type, "組織名")) {
	    p->Organization += atoi(token+strlen(type)+1);
	}
	else if (str_eq(type, "固有名詞")) {
	    p->Artifact += atoi(token+strlen(type)+1);
	}
	else if (str_eq(type, "その他")) {
	    p->Others += atoi(token+strlen(type)+1);
	}
	token = strtok(NULL, " ");
    }
}

/*==================================================================*/
		   char *check_class(MRPH_DATA *mp)
/*==================================================================*/
{
    if (check_feature(mp->f, "かな漢字"))
	return "かな漢字";
    else if (check_feature(mp->f, "カタカナ"))
	return "カタカナ";
    else if (check_feature(mp->f, "英記号"))
	return "英記号";
    else if (check_feature(mp->f, "数字"))
	return "数字";
    return NULL;
}

/*==================================================================*/
	 void store_NE(NamedEntity *np, char *feature, int i)
/*==================================================================*/
{
    char type[256], *mtype;
    int offset, pos, j;

    sscanf(feature, "%[^:]", type);

    if (str_eq(type, "AのB")) {
	_store_NE(&(np->AnoB), feature+strlen(type)+1);
    }
    else if (str_eq(type, "BのA")) {
	_store_NE(&(np->BnoA), feature+strlen(type)+1);
    }
    else if (str_eq(type, "前")) {
	offset = strlen(type)+1;
	sscanf(feature+offset, "%[^:]", type);

	pos = -1;
	for (j = i-1; j >= 0; j--) {
	    if (check_feature(mrph_data[j].f, "自立") || 
		(mrph_data[j].Hinshi == 14 && mrph_data[j].Bunrui == 2) || 
		(mrph_data[j].Hinshi == 13 && mrph_data[j].Bunrui == 1)) {
		pos = j;
		break;
	    }
	}
	if (pos != -1) {
	    mtype = check_class(&(mrph_data[pos]));
	    if (str_eq(type, mtype)) {
		offset += strlen(type)+1;
		_store_NE(&(np->before), feature+offset);
	    }
	}
    }
    else if (str_eq(type, "単語")) {
	_store_NE(&(np->self), feature+strlen(type)+1);
    }
    else if (str_eq(type, "後")) {
	offset = strlen(type)+1;
	sscanf(feature+offset, "%[^:]", type);

	pos = -1;
	for (j = i+1; j < Mrph_num; j++) {
	    if (check_feature(mrph_data[j].f, "自立") || 
		(mrph_data[j].Hinshi == 14 && mrph_data[j].Bunrui == 2) || 
		(mrph_data[j].Hinshi == 13 && mrph_data[j].Bunrui == 1)) {
		pos = j;
		break;
	    }
	}
	if (pos != -1) {
	    mtype = check_class(&(mrph_data[pos]));
	    if (str_eq(type, mtype)) {
		offset += strlen(type)+1;
		_store_NE(&(np->after), feature+offset);
	    }
	}
    }
}

/*==================================================================*/
		  float merge_ratio(int n1, int n2)
/*==================================================================*/
{
    return (float)n1/(n1+5);
}

/*==================================================================*/
   float calculate_NE(int v1, int n1, int v2, int n2, float ratio)
/*==================================================================*/
{
    if (n1 && n2)
	return (float)v1/n1*100*ratio+(float)v2/n2*100*(1-ratio);
    else if (n1)
	return (float)v1/n1*100*ratio;
    else if (n2)
	return (float)v2/n2*100*(1-ratio);
    return 0;
}

/*==================================================================*/
struct _pos_s _NE2mrph(struct _pos_s *p1, struct _pos_s *p2, MRPH_DATA *mp, char *type)
/*==================================================================*/
{
    int n1, n2;
    float ratio;
    struct _pos_s r;

    _init_NE(&r);

    n1 = p1->Location + p1->Person + p1->Organization + p1->Artifact + p1->Others;
    n2 = p2->Location + p2->Person + p2->Organization + p2->Artifact + p2->Others;

    /* 単語レベルの情報と意味素レベルの情報をマージする割り合いの計算 */
    ratio = merge_ratio(n1, n2);

    if (n1 || n2) {
	if (p1->Location || p2->Location) {
	    r.Location = calculate_NE(p1->Location, n1, p2->Location, n2, ratio);
	}
	if (p1->Person || p2->Person) {
	    r.Person = calculate_NE(p1->Person, n1, p2->Person, n2, ratio);
	}
	if (p1->Organization || p2->Organization) {
	    r.Organization = calculate_NE(p1->Organization, n1, p2->Organization, n2, ratio);
	}
	if (p1->Artifact || p2->Artifact) {
	    r.Artifact = calculate_NE(p1->Artifact, n1, p2->Artifact, n2, ratio);
	}
	if (p1->Others || p2->Others) {
	    r.Others = calculate_NE(p1->Others, n1, p2->Others, n2, ratio);
	}
    }
    return r;
}

/*==================================================================*/
  void NE2mrph(NamedEntity *np1, NamedEntity *np2, MRPH_DATA *mp)
/*==================================================================*/
{
    mp->NE.AnoB = _NE2mrph(&(np1->AnoB), &(np2->AnoB), mp, "AのB");
    mp->NE.BnoA = _NE2mrph(&(np1->BnoA), &(np2->BnoA), mp, "BのA");
    mp->NE.before = _NE2mrph(&(np1->before), &(np2->before), mp, "前");
    mp->NE.self = _NE2mrph(&(np1->self), &(np2->self), mp, "単語");
    mp->NE.after = _NE2mrph(&(np1->after), &(np2->after), mp, "後");
}

/*==================================================================*/
    void _NE2feature(struct _pos_s *p, MRPH_DATA *mp, char *type)
/*==================================================================*/
{
    int n, length, i, first = 0;
    char *buffer, element[5][13];

    n = p->Location + p->Person + p->Organization + p->Artifact + p->Others;

    if (n) {
	for (i = 0; i < 5; i++) {
	    element[i][0] = '\0';
	}
	if (p->Location) {
	    sprintf(element[0], "地名:%d", p->Location);
	}
	if (p->Person) {
	    sprintf(element[1], "人名:%d", p->Person);
	}
	if (p->Organization) {
	    sprintf(element[2], "組織名:%d", p->Organization);
	}
	if (p->Artifact) {
	    sprintf(element[3], "固有名詞:%d", p->Artifact);
	}
	if (p->Others) {
	    sprintf(element[4], "その他:%d", p->Others);
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
		if (first++)
		    strcat(buffer, " ");
		strcat(buffer, element[i]);
	    }
	}

	assign_cfeature(&(mp->f), buffer);
	free(buffer);
    }
}

/*==================================================================*/
		    void NE2feature(MRPH_DATA *mp)
/*==================================================================*/
{
    _NE2feature(&(mp->eNE.AnoB), mp, "AのB");
    _NE2feature(&(mp->eNE.BnoA), mp, "BのA");
    _NE2feature(&(mp->eNE.before), mp, "前");
    _NE2feature(&(mp->eNE.self), mp, "単語");
    _NE2feature(&(mp->eNE.after), mp, "後");
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
		   char *get_proper_case(char *cp)
/*==================================================================*/
{
    char *dic_content, *pre_pos;

    dic_content = get_proper(cp, propercase_db);
    if (*dic_content != NULL) {
	for (cp = pre_pos = dic_content; *cp; cp++) {
	    if (*cp == '/') {
		*cp = '\0';
		/* store_NE(&ne[0], pre_pos); */
		pre_pos = cp + 1;
	    }
	}
	/* store_NE(&ne[0], pre_pos); */
    }
}

/*==================================================================*/
		   void assign_f_from_dic(int num)
/*==================================================================*/
{
    char *dic_content, *pre_pos, *cp, *sm, *type;
    char code[13];
    int i, smn;
    NamedEntity ne[2];
    MRPH_DATA *mp;

    code[12] = '\0';

    mp = &(mrph_data[num]);

    /* 初期化 */
    init_NE(&ne[0]);
    init_NE(&ne[1]);

    /* 表記による検索 */
    dic_content = get_proper(mp->Goi, proper_db);
    if (*dic_content != NULL) {
	for (cp = pre_pos = dic_content; *cp; cp++) {
	    if (*cp == '/') {
		*cp = '\0';
		store_NE(&ne[0], pre_pos, num);
		pre_pos = cp + 1;
	    }
	}
	store_NE(&ne[0], pre_pos, num);
    }

    /* ここで入力形態素に意味素を与えておく */
    sm = (char *)get_sm(mp->Goi);

    /* 意味素による検索 */
    if (*sm) {
	smn = strlen(sm);
	strncpy(mp->SM, sm, smn);	/* ★ */
	smn = smn/SM_CODE_SIZE;

	for (i = 0; i < smn; i++) {
	    code[0] = '1';
	    code[1] = '\0';
	    strncat(code, mp->SM+SM_CODE_SIZE*i+1, SM_CODE_SIZE-1);
	    dic_content = get_proper(code, properc_db);
	    if (*dic_content != NULL) {
		for (cp = pre_pos = dic_content; *cp; cp++) {
		    if (*cp == '/') {
			*cp = '\0';
			store_NE(&ne[1], pre_pos, num);
			pre_pos = cp + 1;
		    }
		}
		store_NE(&ne[1], pre_pos, num);
	    }
	}
    }

    /* 文字種の取得 */
    type = check_class(mp);

    /* 文字種による検索 */
    if (type) {
	dic_content = get_proper(type, properc_db);
	if (*dic_content != NULL) {
	    store_NE(&ne[1], dic_content, num);
	}
    }

    NE2mrph(&ne[0], &ne[1], mp);
}

/*====================================================================
                               END
====================================================================*/
