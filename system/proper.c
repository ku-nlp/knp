/*====================================================================

			     固有名詞処理

                                               S.Kurohashi 96. 7. 4

    $Id$
====================================================================*/
#include "knp.h"

DBM_FILE	proper_db = NULL, properc_db = NULL, propercase_db = NULL;
int		PROPERExist = 0;

char *CaseList[] = {"カラ格", "ガ格", "デ格", "ト格", "ニ格", "ノ格", 
		    "ヘ格", "マデ格", "ヨリ格", "ヲ格", "体言NONE", 
		    "中断線", "同格未格", "同格連体", "同格連用", 
		    "文末", "未格", "無格", "用言強NONE", "隣接", 
		    "連格", "連体", "連用", ""};

PreservedNamedEntity *pNE = NULL;

char *TableNE[] = {"人名", "地名", "組織名", "固有名詞", ""};

/*==================================================================*/
		  void init_proper(SENTENCE_DATA *s)
/*==================================================================*/
{
    int i;

    if ((proper_db = DBM_open(check_dict_filename(PROPER_DB_NAME, FALSE), O_RDONLY, 0)) == NULL || 
	(properc_db = DBM_open(check_dict_filename(PROPERC_DB_NAME, FALSE), O_RDONLY, 0)) == NULL || 
	(propercase_db = DBM_open(check_dict_filename(PROPERCASE_DB_NAME, FALSE), O_RDONLY, 0)) == NULL) {
	PROPERExist = FALSE;
    } else {
	PROPERExist = TRUE;
    }

    for (i = 0; i < MRPH_MAX; i++) {
	(s->mrph_data+i)->SM = (char *)malloc_data(sizeof(char)*SM_ELEMENT_MAX*SM_CODE_SIZE+1, 
						    "init_proper");
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
		   void _init_NE(struct _pos_s *p)
/*==================================================================*/
{
    p->Location = 0;
    p->Person = 0;
    p->Organization = 0;
    p->Artifact = 0;
    p->Others = 0;
    p->Type[0] = '\0';
    p->Count = 0;
}

/*==================================================================*/
		    void init_NE(NamedEntity *np)
/*==================================================================*/
{
    _init_NE(&(np->AnoX));
    _init_NE(&(np->XnoB));
    _init_NE(&(np->XB));
    _init_NE(&(np->AX));
    _init_NE(&(np->self));
    _init_NE(&(np->selfSM));
    _init_NE(&(np->Case));
}

/*==================================================================*/
     void _store_NE(struct _pos_s *p, char *string, char *mtype)
/*==================================================================*/
{
    char *token, type[256];	/* ★ */

    if (mtype)
	strcpy(p->Type, mtype);
    else
	p->Type[0] = '\0';

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
void store_NE(SENTENCE_DATA *sp, NamedEntity *np, char *feature, int i)
/*==================================================================*/
{
    char type[256], mtype[256], class[256];	/* ★ */
    int offset;

    sscanf(feature, "%[^:]", type);

    if (str_eq(type, "AのX")) {
	offset = strlen(type)+1;
	sscanf(feature+offset, "%[^:]", class);

	if (i < sp->Mrph_num-2 && 
	    ((check_feature(sp->mrph_data[i].f, "自立") && 
	      sp->mrph_data[i].Hinshi == 6) || 
	      (sp->mrph_data[i].Hinshi == 14 && sp->mrph_data[i].Bunrui == 2) || 
	      (sp->mrph_data[i].Hinshi == 13 && sp->mrph_data[i].Bunrui == 1)) && 
	    check_feature(sp->mrph_data[i+2].f, "自立") && 
	    sp->mrph_data[i+2].Hinshi == 6) {
	    /* "の"の後の自立語の文字種 */
	    strcpy(mtype, check_class(&(sp->mrph_data[i+2])));
	    if (str_eq(class, mtype)) {
		offset += strlen(class)+1;
		_store_NE(&(np->AnoX), feature+offset, class);
	    }
	}
    }
    else if (str_eq(type, "XのB")) {
	offset = strlen(type)+1;
	sscanf(feature+offset, "%[^:]", class);

	if (i > 1 && 
	    ((check_feature(sp->mrph_data[i].f, "自立") && 
	      sp->mrph_data[i].Hinshi == 6) || 
	      (sp->mrph_data[i].Hinshi == 14 && sp->mrph_data[i].Bunrui == 2) || 
	      (sp->mrph_data[i].Hinshi == 13 && sp->mrph_data[i].Bunrui == 1)) && 
	    check_feature(sp->mrph_data[i-2].f, "自立") && 
	    sp->mrph_data[i-2].Hinshi == 6) {
	    /* "の"の前の自立語の文字種 */
	    strcpy(mtype, check_class(&(sp->mrph_data[i-2])));
	    if (str_eq(class, mtype)) {
		offset += strlen(class)+1;
		_store_NE(&(np->XnoB), feature+offset, class);
	    }
	}
    }
    else if (str_eq(type, "XB")) {
	offset = strlen(type)+1;
	sscanf(feature+offset, "%[^:]", class);

	if (i > 0 && 
	    ((check_feature(sp->mrph_data[i].f, "自立") && 
	      sp->mrph_data[i].Hinshi == 6) || 
	      (sp->mrph_data[i].Hinshi == 14 && sp->mrph_data[i].Bunrui == 2) || 
	      (sp->mrph_data[i].Hinshi == 13 && sp->mrph_data[i].Bunrui == 1)) && 
	     (check_feature(sp->mrph_data[i-1].f, "自立") && 
	      sp->mrph_data[i-1].Hinshi == 6)) {
	    /* ひとつ前の自立語(または接辞)の文字種 */
	    strcpy(mtype, check_class(&(sp->mrph_data[i-1])));
	    if (str_eq(class, mtype)) {
		offset += strlen(class)+1;
		_store_NE(&(np->XB), feature+offset, class);
	    }
	}
    }
    else if (str_eq(type, "単語")) {
	_store_NE(&(np->self), feature+strlen(type)+1, NULL);
    }
    else if (str_eq(type, "文字種")) {
	_store_NE(&(np->selfSM), feature+strlen(type)+1, NULL);
    }
    else if (str_eq(type, "AX")) {
	offset = strlen(type)+1;
	sscanf(feature+offset, "%[^:]", class);

	if (i < sp->Mrph_num-1 && 
	    ((check_feature(sp->mrph_data[i].f, "自立") && 
	      sp->mrph_data[i].Hinshi == 6) || 
	      (sp->mrph_data[i].Hinshi == 14 && sp->mrph_data[i].Bunrui == 2) || 
	      (sp->mrph_data[i].Hinshi == 13 && sp->mrph_data[i].Bunrui == 1)) && 
	     (check_feature(sp->mrph_data[i+1].f, "自立") && 
	      sp->mrph_data[i+1].Hinshi == 6)) {
	    strcpy(mtype, check_class(&(sp->mrph_data[i+1])));
	    if (str_eq(class, mtype)) {
		offset += strlen(class)+1;
		_store_NE(&(np->AX), feature+offset, class);
	    }
	}
    }
    else if (str_eq(type, "格情報")) {
	if (!(str_eq(sp->mrph_data[i].Goi, "する") && 
	      str_eq(sp->mrph_data[i].Goi, "ある") &&  
	      str_eq(sp->mrph_data[i].Goi, "なる")))
	    assign_cfeature(&(sp->mrph_data[i].f), feature+strlen(type)+1);
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
	return (float)v1/n1*100;
    /* return (float)v1/n1*100*ratio; */
    else if (n2)
	return (float)v2/n2*100*(1-ratio);
    return 0;
}

/*==================================================================*/
     struct _pos_s _NE2mrph(struct _pos_s *p1, struct _pos_s *p2)
/*==================================================================*/
{
    int n1, n2;
    float ratio;
    struct _pos_s r;

    _init_NE(&r);

    /* 文字種制約の伝播 */
    if (p1->Type[0])
	strcpy(r.Type, p1->Type);
    else if (p2->Type[0])
	strcpy(r.Type, p2->Type);
	

    n1 = p1->Location + p1->Person + p1->Organization + p1->Artifact + p1->Others;
    n2 = p2->Location + p2->Person + p2->Organization + p2->Artifact + p2->Others;

    r.Count = n1;

    /* 単語レベルの情報と意味素レベルの情報をマージする割り合いの計算 */
    ratio = merge_ratio(n1, n2);

    if (n1 || n2) {
	if (p1->Location || p2->Location)
	    r.Location = calculate_NE(p1->Location, n1, p2->Location, n2, ratio);
	if (p1->Person || p2->Person)
	    r.Person = calculate_NE(p1->Person, n1, p2->Person, n2, ratio);
	if (p1->Organization || p2->Organization)
	    r.Organization = calculate_NE(p1->Organization, n1, p2->Organization, n2, ratio);
	if (p1->Artifact || p2->Artifact)
	    r.Artifact = calculate_NE(p1->Artifact, n1, p2->Artifact, n2, ratio);
	if (p1->Others || p2->Others)
	    r.Others = calculate_NE(p1->Others, n1, p2->Others, n2, ratio);
    }
    return r;
}

/*==================================================================*/
	      struct _pos_s _NE2mrphS(struct _pos_s *p)
/*==================================================================*/
{
    int n;
    struct _pos_s r;

    _init_NE(&r);

    n = p->Location + p->Person + p->Organization + p->Artifact + p->Others;

    r.Count = n;

    if (n) {
	if (p->Location) {
	    r.Location = (float)p->Location/n*100;
	}
	if (p->Person) {
	    r.Person = (float)p->Person/n*100;
	}
	if (p->Organization) {
	    r.Organization = (float)p->Organization/n*100;
	}
	if (p->Artifact) {
	    r.Artifact = (float)p->Artifact/n*100;
	}
	if (p->Others) {
	    r.Others = (float)p->Others/n*100;
	}
    }
    return r;
}

/*==================================================================*/
  void NE2mrph(NamedEntity *np1, NamedEntity *np2, MRPH_DATA *mp)
/*==================================================================*/
{
    mp->NE.AnoX = _NE2mrph(&(np1->AnoX), &(np2->AnoX));
    mp->NE.XnoB = _NE2mrph(&(np1->XnoB), &(np2->XnoB));
    mp->NE.XB = _NE2mrph(&(np1->XB), &(np2->XB));
    mp->NE.AX = _NE2mrph(&(np1->AX), &(np2->AX));
    mp->NE.self = _NE2mrphS(&(np1->self));
    mp->NE.selfSM = _NE2mrphS(&(np2->selfSM));
}

/*==================================================================*/
void _NE2feature(struct _pos_s *p, MRPH_DATA *mp, char *type, int flag)
/*==================================================================*/
{
    int n, i, first = 0;
    char buffer[256], element[5][13];	/* ★ */

    n = p->Location + p->Person + p->Organization + p->Artifact + p->Others;

    if (n || flag == 2) {
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

	/* length = 0;
	for (i = 0; i < 5; i++) {
	    if (element[i][0])
		length += strlen(element[i])+1;
	}
	buffer = (char *)malloc_data(strlen(type)+length+1+(int)log10(p->Count)+8, "_NE2feature"); */

	if (!flag)
	    sprintf(buffer, "%s:", type);
	else if (flag == 1)
	    sprintf(buffer, "%s:%s:", type, p->Type);
	else if (flag == 2) {
	    if (p->Count)
		sprintf(buffer, "%s%%頻度 %d:", type, p->Count);
	    else
		sprintf(buffer, "%s%%頻度 %d", type, p->Count);
	}

	for (i = 0; i < 5; i++) {
	    if (element[i][0]) {
		if (first++)
		    strcat(buffer, " ");
		strcat(buffer, element[i]);
	    }
	}

	assign_cfeature(&(mp->f), buffer);
	/* free(buffer); */
    }
}

/*==================================================================*/
		    void NE2feature(MRPH_DATA *mp)
/*==================================================================*/
{
    _NE2feature(&(mp->eNE.AnoX), mp, "AのX", 1);
    _NE2feature(&(mp->eNE.XnoB), mp, "XのB", 1);
    _NE2feature(&(mp->eNE.XB), mp, "XB", 1);
    _NE2feature(&(mp->eNE.AX), mp, "AX", 1);
    _NE2feature(&(mp->eNE.self), mp, "単語", 2);
    _NE2feature(&(mp->eNE.selfSM), mp, "文字種", 0);
    _NE2feature(&(mp->eNE.Case), mp, "格", 0);
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

    ne.AnoX = _merge_NE(&(np1->AnoX), &(np2->AnoX));
    ne.XnoB = _merge_NE(&(np1->XnoB), &(np2->XnoB));
    ne.XB = _merge_NE(&(np1->XB), &(np2->XB));
    ne.self = _merge_NE(&(np1->self), &(np2->self));
    ne.AX = _merge_NE(&(np1->AX), &(np2->AX));

    return ne;
}

/*==================================================================*/
       void store_NEC(SENTENCE_DATA *sp, char *feature, int i)
/*==================================================================*/
{
    char type[256];	/* ★ */
    int j, offset;
    struct _pos_s ne;

    _init_NE(&ne);

    sscanf(feature, "%[^:]", type);
    offset = strlen(type)+1;

    for (j = 0; CaseList[j][0]; j++) {
	if (str_eq(type, CaseList[j])) {
	    _store_NE(&ne, feature+offset, NULL);
	    sp->mrph_data[i].Case[j] = _NE2mrphS(&ne);
	    break;
	}
    }
}

/*==================================================================*/
   int CheckJiritsuGoFeature(SENTENCE_DATA *sp, int i, char *type)
/*==================================================================*/
{
    int j;

    /* 係り先の自立語が「ガ主体」などを持っているかどうか */
    for (j = 0; j < sp->bnst_data[i].jiritu_num; j++) {
	if (check_feature((sp->bnst_data[i].jiritu_ptr+j)->f, type))
	    return 1;
    }
    return 0;
}

/*==================================================================*/
	  void assign_f_from_dic(SENTENCE_DATA *sp, int num)
/*==================================================================*/
{
    char *dic_content, *pre_pos, *cp, *type;
    char code[SM_CODE_SIZE+1];
    int i, smn;
    NamedEntity ne[2];
    MRPH_DATA *mp;

    code[12] = '\0';

    mp = &(sp->mrph_data[num]);

    /* 初期化 */
    init_NE(&ne[0]);
    init_NE(&ne[1]);

    /* 表記による検索 */
    dic_content = db_get(proper_db, mp->Goi);
    if (dic_content != NULL) {
	for (cp = pre_pos = dic_content; *cp; cp++) {
	    if (*cp == '/') {
		*cp = '\0';
		store_NE(sp, &ne[0], pre_pos, num);
		pre_pos = cp + 1;
	    }
	}
	store_NE(sp, &ne[0], pre_pos, num);
	free(dic_content);
    }

    /* ここで入力形態素に意味素を与えておく */
    /* sm = (char *)get_sm(mp->Goi); */

    /* 意味素による検索 */
    if (OptNE == OPT_NESM && mp->SM[0]) {
	/* smn = strlen(sm);
	   strncpy(mp->SM, sm, smn);
	   smn = smn/SM_CODE_SIZE; */
	smn = strlen(mp->SM)/SM_CODE_SIZE;

	for (i = 0; i < smn; i++) {
	    code[0] = '1';
	    code[1] = '\0';
	    strncat(code, mp->SM+SM_CODE_SIZE*i+1, SM_CODE_SIZE-1);
	    dic_content = db_get(properc_db, code);
	    if (dic_content != NULL) {
		for (cp = pre_pos = dic_content; *cp; cp++) {
		    if (*cp == '/') {
			*cp = '\0';
			store_NE(sp, &ne[1], pre_pos, num);
			pre_pos = cp + 1;
		    }
		}
		store_NE(sp, &ne[1], pre_pos, num);
		free(dic_content);
	    }
	}
    }

    /* 文字種の取得 */
    type = check_class(mp);

    /* 文字種による検索 */
    if (type) {
	dic_content = db_get(properc_db, type);
	if (dic_content != NULL) {
	    store_NE(sp, &ne[1], dic_content, num);
	    free(dic_content);
	}
    }

    /* 格 */
    dic_content = db_get(propercase_db, mp->Goi);
    if (dic_content != NULL) {
	for (cp = pre_pos = dic_content; *cp; cp++) {
	    if (*cp == '/') {
		*cp = '\0';
		store_NEC(sp, pre_pos, num);
		pre_pos = cp + 1;
	    }
	}
	store_NEC(sp, pre_pos, num);
	free(dic_content);
    }

    /*            ne[0]    ne[1]
	   -----------------
	   XB     表記     意味素
	   AX     表記     意味素
	   AnoX   表記     意味素
	   XnoB   表記     意味素
	   self   表記
	   selfSM          文字種 */

    NE2mrph(&ne[0], &ne[1], mp);
}

/*==================================================================*/
		void assign_ne_rule(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;

    /* 固有表現句ルール */
    for (i = 0; i < sp->Bnst_num; i++) {
	assign_mrph_feature(CNRuleArray, CurCNRuleSize, 
			    sp->bnst_data[i].mrph_ptr, 
			    sp->bnst_data[i].mrph_num, 
			    RLOOP_RMM, FALSE, LtoR);
    }

    /* 補助的ルール */
    assign_mrph_feature(CNauxRuleArray, CurCNauxRuleSize,
			sp->mrph_data, sp->Mrph_num,
			RLOOP_RMM, FALSE, LtoR);
}

/*==================================================================*/
	    int CheckJumanProper(SENTENCE_DATA *sp, int i)
/*==================================================================*/
{
    if (sp->mrph_data[i].Hinshi != 6 || sp->mrph_data[i].Bunrui == 1 || 
	sp->mrph_data[i].Bunrui == 2)
	return 0;

    /* 品曖があるとき */
    if (!check_feature(sp->mrph_data[i].f, "品曖-その他") && 
	!check_feature(sp->mrph_data[i].f, "品曖-カタカナ") && 
	!check_feature(sp->mrph_data[i].f, "品曖-アルファベット")) {

	/* どれかの固有名詞になるとき */
	if (check_feature(sp->mrph_data[i].f, "品曖-地名") || 
	    check_feature(sp->mrph_data[i].f, "品曖-人名") || 
	    check_feature(sp->mrph_data[i].f, "品曖-組織名") || 
	    check_feature(sp->mrph_data[i].f, "品曖-固有名詞") || 
	    sp->mrph_data[i].Bunrui == 4 || 
	    sp->mrph_data[i].Bunrui == 5 || 
	    sp->mrph_data[i].Bunrui == 6 || 
	    sp->mrph_data[i].Bunrui == 3)
	    return 1;
    }
    return 0;
}

/*==================================================================*/
	    int CheckNormalNoun(SENTENCE_DATA *sp, int i)
/*==================================================================*/
{
    if (sp->mrph_data[i].Hinshi != 6 || 
	!(sp->mrph_data[i].Bunrui == 1 || sp->mrph_data[i].Bunrui == 2) || 
	check_feature(sp->mrph_data[i].f, "品曖-その他") || 
	check_feature(sp->mrph_data[i].f, "品曖-カタカナ") || 
	check_feature(sp->mrph_data[i].f, "品曖-アルファベット") || 
	check_feature(sp->mrph_data[i].f, "品曖-地名") || 
	check_feature(sp->mrph_data[i].f, "品曖-人名") || 
	check_feature(sp->mrph_data[i].f, "品曖-組織名") || 
	check_feature(sp->mrph_data[i].f, "品曖-固有名詞"))
	return 0;
    return 1;
}

/*==================================================================*/
   int NEassignAgentFromHead(SENTENCE_DATA *sp, int i, char *type)
/*==================================================================*/
{
    int j;

    /* 係り先の自立語が「ガ主体」などを持っているかどうか */
    for (j = 0; j < sp->bnst_data[sp->bnst_data[i].dpnd_head].jiritu_num; j++) {
	if ((char *)check_feature((sp->bnst_data[sp->bnst_data[i].dpnd_head].jiritu_ptr+j)->f, type)) {
	    /* 格要素側の全自立語にマーク
	    for (k = 0; k < sp->bnst_data[i].jiritu_num; k++) {
		assign_cfeature(&((sp->bnst_data[i].jiritu_ptr+k)->f), "主体");
	    } */
	    return 1;;
	}
    }
    return 0;
}

/*==================================================================*/
		      int ReturnNEcode(char *cp)
/*==================================================================*/
{
    int i = 0;

    while (TableNE[i][0]) {
	if (str_eq(cp, TableNE[i]))
	    return i;
	i++;
    }
    return -1;
}

/*==================================================================*/
int check_correspond_NE_longest(SENTENCE_DATA *sp, int i, char *rule)
/*==================================================================*/
{
    int code, old = i;
    PreservedNamedEntity *p = pNE;
    MRPH_P *mp;

    code = ReturnNEcode(rule);
    if (code == -1)
	return 0;

    while (p) {
	mp = p->mrph;
	if (code == p->Type && str_eq(sp->mrph_data[i].Goi, mp->data.Goi)) {
	    i++;
	    mp = mp->next;
	    while (mp && i < sp->Mrph_num) {
		if (str_eq(sp->mrph_data[i].Goi, mp->data.Goi)) {
		    i++;
		    mp = mp->next;
		}
		else
		    break;
	    }
	    return i-old;
	}
	p = p->next;
    }
    return 0;
}

/*==================================================================*/
	 int NEparaAssign(BNST_DATA *b_ptr, BNST_DATA *h_ptr)
/*==================================================================*/
{
    int j, code;
    char *class = NULL, *preclass = NULL;

    /* 自立語 Loop */
    for (j = 0; j < b_ptr->jiritu_num; j++) {
	if ((class = (char *)check_feature((b_ptr->jiritu_ptr+j)->f, "複固"))) {
	    code = ReturnNEcode(class+strlen("複固:"));
	    if (code == -1)
		return TRUE;
	    /* 文節内で単一のクラスではないときは無視 */
	    if (preclass && strcmp(class, preclass))
		return TRUE;
	    preclass = class;
	}
    }

    if (class) {
	/* 係り先の自立語すべてを同じ固有名詞クラスにしよう */
	for (j = 0; j < h_ptr->jiritu_num; j++) {
	    if (!check_feature((h_ptr->jiritu_ptr+j)->f, "複固")) {
		assign_cfeature(&((h_ptr->jiritu_ptr+j)->f), class);
		assign_cfeature(&((h_ptr->jiritu_ptr+j)->f), "固並列OK");
	    }
	}
	return TRUE;
    }
    return FALSE;
}

/*==================================================================*/
		 void NE_analysis(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, k, h, apos = 0, flag = 0, value;
    char *cp;
    TOTAL_MGR *tm = sp->Best_mgr;

    for (i = 0; i < sp->Mrph_num; i++) {
	/* sp->mrph_data[i].SM[0] = '\0'; */
	init_NE(&(sp->mrph_data[i].NE));
	init_NE(&(sp->mrph_data[i].eNE));

	for (j = 0; CaseList[j][0]; j++) {
	    _init_NE(&(sp->mrph_data[i].Case[j]));
	}

	assign_f_from_dic(sp, i);

	/* 単語と文字種のコピー */
	sp->mrph_data[i].eNE.self = sp->mrph_data[i].NE.self;
	sp->mrph_data[i].eNE.selfSM = sp->mrph_data[i].NE.selfSM;

	if (CheckJumanProper(sp, i))
	    assign_cfeature(&(sp->mrph_data[i].f), "固有名詞のみ");
	if (CheckNormalNoun(sp, i))
	    assign_cfeature(&(sp->mrph_data[i].f), "普通名詞");
    }

    /* とりあえずすべての形態素にふっておこう */
    for (i = 0; i < sp->Mrph_num; i++) {
	/* 前の隣接語 (自立語, 名詞性名詞接尾辞, 名詞接頭辞だけ) */
	if (i > 0)
	    sp->mrph_data[i].eNE.AX = sp->mrph_data[i-1].NE.AX;

	/* 後の隣接語 (自立語, 名詞性名詞接尾辞, 名詞接頭辞だけ) */
	if (i < sp->Mrph_num-1)
	    sp->mrph_data[i].eNE.XB = sp->mrph_data[i+1].NE.XB;

	/* A の B */
	if (flag != 2 && sp->mrph_data[i].Hinshi == 6 && check_feature(sp->mrph_data[i].f, "自立")) {
	    flag = 1;
	    apos = i;
	}
	else if (flag == 1 && str_eq(sp->mrph_data[i].Goi, "の") && sp->mrph_data[i].Hinshi == 9) {
	    flag = 2;
	}
	else if (flag == 2 && sp->mrph_data[i].Hinshi == 6 && check_feature(sp->mrph_data[i].f, "自立")) {
	    sp->mrph_data[i].eNE.AnoX = sp->mrph_data[apos].NE.AnoX;
	    sp->mrph_data[apos].eNE.XnoB = sp->mrph_data[i].NE.XnoB;

	    flag = 1;
	    apos = i;
	}
	else {
	    flag = 0;
	}
    }

    /* 格 */
    for (i = 0; i < sp->Bnst_num-1; i++) {
	h = tm->dpnd.head[i];
	cp = (char *)check_feature(sp->bnst_data[i].f, "係");
	if (cp) {
	    for (j = 0; CaseList[j][0]; j++) {
		if (str_eq(cp+3, CaseList[j])) {
		    /* 自立語すべてに与える */
		    for (k = 0; k < sp->bnst_data[i].jiritu_num; k++)
			(sp->bnst_data[i].jiritu_ptr+k)->eNE.Case = sp->bnst_data[h].mrph_ptr->Case[j];
		    break;
		}
	    }
	}
    }

    /* 主体のマークの伝播 */
    for (i = 0; i < sp->Bnst_num; i++) {
	if ( i > 0 && check_feature(sp->bnst_data[i-1].f, "役割の〜")) 
	    assign_cfeature(&((sp->bnst_data[i].jiritu_ptr)->f), "役割の人名");
	else if ( i > 0 && check_feature(sp->bnst_data[i-1].f, "組織の〜")) 
	    assign_cfeature(&((sp->bnst_data[i].jiritu_ptr)->f), "組織の組織名");
	else {
	    if (sp->bnst_data[i].dpnd_head != -1 && !check_feature(sp->bnst_data[sp->bnst_data[i].dpnd_head].f, "〜れる") && !check_feature(sp->bnst_data[sp->bnst_data[i].dpnd_head].f, "〜せる")) {
		if ( check_feature(sp->bnst_data[i].f, "係:ガ格") || check_feature(sp->bnst_data[i].f, "係:未格")) {
		    if (check_feature(sp->bnst_data[sp->bnst_data[i].dpnd_head].f, "〜役割だ")) 
			assign_cfeature(&((sp->bnst_data[i].jiritu_ptr)->f), "人名が役割だ");
		    else if (check_feature(sp->bnst_data[sp->bnst_data[i].dpnd_head].f, "〜組織だ")) 
			assign_cfeature(&((sp->bnst_data[i].jiritu_ptr)->f), "組織名が組織だ");
		}

		/* 組織 */
		if ((check_feature(sp->bnst_data[i].f, "係:ガ格") && NEassignAgentFromHead(sp, i, "ガ組織")) || 
		    (check_feature(sp->bnst_data[i].f, "係:ヲ格") && NEassignAgentFromHead(sp, i, "ヲ組織"))) {
		    assign_cfeature(&((sp->bnst_data[i].jiritu_ptr)->f), "組織がを用言");
		}
		/* 人 */
		else if ((check_feature(sp->bnst_data[i].f, "係:ガ格") && (NEassignAgentFromHead(sp, i, "ガ主体") || NEassignAgentFromHead(sp, i, "ガ人"))) || 
		    (check_feature(sp->bnst_data[i].f, "係:ヲ格") && (NEassignAgentFromHead(sp, i, "ヲ主体")) || NEassignAgentFromHead(sp, i, "ヲ人"))) {
		    assign_cfeature(&((sp->bnst_data[i].jiritu_ptr)->f), "人がを用言");
		}
	    }
	}
    }

    /* 形態素に対するルールの適用 (eNE に対して) */
    for (i = 0; i < sp->Mrph_num; i++) {
	NE2feature(sp->mrph_data + i);
    }
    assign_mrph_feature(NERuleArray, CurNERuleSize,
			sp->mrph_data, sp->Mrph_num, 
			RLOOP_MRM, RLOOP_BREAK_NORMAL, LtoR);

    /* 照応処理 */
    if (1) {
	for (i = 0; i < sp->Mrph_num; i++) {
	    if ((value = check_correspond_NE_longest(sp, i, "人名"))) {
		for (j = 0; j < value; j++) {
		    assign_cfeature(&(sp->mrph_data[i+j].f), "単固:人名");
		    assign_cfeature(&(sp->mrph_data[i+j].f), "固照応OK");
		}
	    }
	}
	for (i = 0; i < sp->Mrph_num; i++) {
	    if ((value = check_correspond_NE_longest(sp, i, "地名"))) {
		for (j = 0; j < value; j++) {
		    assign_cfeature(&(sp->mrph_data[i+j].f), "単固:地名");
		    assign_cfeature(&(sp->mrph_data[i+j].f), "固照応OK");
		}
	    }
	}
    }

    /* 複合名詞ルールの適用 */
    assign_mrph_feature(CNpreRuleArray, CurCNpreRuleSize,
			sp->mrph_data, sp->Mrph_num,
			RLOOP_RMM, FALSE, LtoR);
    assign_ne_rule(sp);

    /* 並列処理
       並列句の数が 3 つ以上または、大きさが 2 文節以上のとき */
    if (1) {
	for (i = 0; i < sp->Bnst_num; i++) {
	    /* 住所が入る並列はやめておく */
	    if (CheckJiritsuGoFeature(sp, i, "住所") || 
		!check_feature(sp->bnst_data[i].f, "体言") || 
		(sp->bnst_data[i].dpnd_head > 0 && !check_feature(sp->bnst_data[sp->bnst_data[i].dpnd_head].f, "体言")))
		continue;
	    cp = (char *)check_feature(sp->bnst_data[i].f, "並結句数");
	    if (cp) {
		value = atoi(cp+strlen("並結句数:"));
		if (value > 2) {
		    /* 係り側を調べて固有名詞じゃなかったら、受け側を調べる */
		    if (NEparaAssign(&(sp->bnst_data[i]), &(sp->bnst_data[sp->bnst_data[i].dpnd_head])) == FALSE)
			NEparaAssign(&(sp->bnst_data[sp->bnst_data[i].dpnd_head]), &(sp->bnst_data[i]));
		    continue;
		}
	    }

	    cp = (char *)check_feature(sp->bnst_data[i].f, "並結文節数");
	    if (cp) {
		value = atoi(cp+strlen("並結文節数:"));
		if (value > 1) {
		    if (NEparaAssign(&(sp->bnst_data[i]), &(sp->bnst_data[sp->bnst_data[i].dpnd_head])) == FALSE)
			NEparaAssign(&(sp->bnst_data[sp->bnst_data[i].dpnd_head]), &(sp->bnst_data[i]));
		}
	    }

	    /* 並列のとき
	       if (sp->bnst_data[i].dpnd_type == 'P') {
	       if (NEparaAssign(&(sp->bnst_data[i]), &(sp->bnst_data[sp->bnst_data[i].dpnd_head])) == FALSE)
	       NEparaAssign(&(sp->bnst_data[sp->bnst_data[i].dpnd_head]), &(sp->bnst_data[i]));
	       }
	    */
	}
    }
}

/*==================================================================*/
void allocateMRPH(SENTENCE_DATA *sp, PreservedNamedEntity **p, int i)
/*==================================================================*/
{
    MRPH_P **mp;

    while ((*p)->next != NULL)
	p = &((*p)->next);
    mp = &((*p)->mrph);
    while (*mp != NULL)
	mp = &((*mp)->next);
    *mp = (MRPH_P *)malloc_data(sizeof(MRPH_P), "allocateMRPH");
    (*mp)->data = sp->mrph_data[i];
    (*mp)->next = NULL;
}

/*==================================================================*/
void allocateNE(SENTENCE_DATA *sp, PreservedNamedEntity **p, int code, int i)
/*==================================================================*/
{
    while (*p != NULL)
	p = &((*p)->next);
    *p = (PreservedNamedEntity *)malloc_data(sizeof(PreservedNamedEntity), "allocateNE");
    (*p)->mrph = NULL;
    (*p)->next = NULL;
    allocateMRPH(sp, p, i);
    (*p)->Type = code;
}

/*==================================================================*/
		  void preserveNE(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, code, precode = -1, nameflag = 0;
    char *cp;

    for (i = 0; i < sp->Mrph_num; i++) {
	if ((cp = (char *)check_feature(sp->mrph_data[i].f, "複固"))) {
	    code = ReturnNEcode(cp+strlen("複固:"));
	    /* 保存しない固有名詞 */
	    if (code == -1)
		continue;
	    /* カタカナ人名の「・」のあとも登録したい */
	    if (code == 0 && str_eq(sp->mrph_data[i].Goi, "・"))
		    nameflag = 1;
	    /* 違う種類の固有名詞になるか、固有名詞が始まったとき */
	    if (code != precode)
		allocateNE(sp, &pNE, code, i);
	    /* 同じ種類の固有名詞 */
	    else
		allocateMRPH(sp, &pNE, i);
	    /* これが末尾でないと困る */
	    if (nameflag && code == 0 && check_feature(sp->mrph_data[i].f, "カタカナ")) {
		allocateNE(sp, &pNE, code, i);
		nameflag = 0;
	    }
	    precode = code;
	}
	else
	    precode = -1;
    }
}

/*==================================================================*/
			    void printNE()
/*==================================================================*/
{
    PreservedNamedEntity *p = pNE;
    MRPH_P *mp;

    fprintf(Outfp, "<固有名詞 スタック>\n");

    while (p) {
	mp = p->mrph;
	fprintf(Outfp, "%d", p->Type);
	while (mp) {
	    fprintf(Outfp, " %s", mp->data.Goi2);
	    mp = mp->next;
	}
	putchar('\n');
	p = p->next;
    }
}

/*==================================================================*/
	 int check_correspond_NE(MRPH_DATA *data, char *rule)
/*==================================================================*/
{
    int code;
    PreservedNamedEntity *p = pNE;
    MRPH_P *mp;

    code = ReturnNEcode(rule);
    if (code == -1)
	return FALSE;

    while (p) {
	mp = p->mrph;
	/* とりあえず、先頭の形態素だけチェックしておく */
	if (code == p->Type && str_eq(data->Goi, mp->data.Goi))
	    return TRUE;
	p = p->next;
    }
    return FALSE;
}

/*==================================================================*/
		 void assign_agent(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, child, flag, num;
    char *cp;
    char Childs[128], Case[128], SM[128];	/* ★ */
    char sm[SM_CODE_SIZE+1];

    sm[SM_CODE_SIZE] = '\0';

    for (i = 0; i < sp->Bnst_num; i++) {
	if ((cp = (char *)check_feature(sp->bnst_data[i].f, "深層格N1"))) {
	    num = sscanf(cp, "%*[^:]:%[^:]:%[^:]:%[^:]", 
		   Childs, Case, SM);
	    /* 意味素がないときなど */
	    if (num != 3)
		continue;
	    /* 格要素なしのとき */
	    if (str_eq(Childs, "NIL")) 
		continue;
	    /* 格要素の番号 */
	    child = atoi(Childs);
	    flag = 0;
	    for (j = 0; SM[j]; j+=SM_CODE_SIZE) {
		if (SM[j] == '/')
		    j++;
		strncpy(sm, &(SM[j]), SM_CODE_SIZE);
		/* 意味素が主体以下である場合 */
		if (comp_sm(sm2code("主体"), sm, 1))
		    flag |= 0x01;
		/* 意味素が主体以下ではない場合 */
		else
		    flag |= 0x02;
	    }

	    /* 自立語全てに feature を与える */

	    /* 主体もありうるとき */
	    if ((flag & 0x03) == 0x03)
		for (j = 0; j < sp->bnst_data[child].jiritu_num; j++)
		    assign_cfeature(&((sp->bnst_data[child].jiritu_ptr+j)->f), "主体*");
	    /* 主体しかとらないとき */
	    else if (flag & 0x01)
		for (j = 0; j < sp->bnst_data[child].jiritu_num; j++)
		    assign_cfeature(&((sp->bnst_data[child].jiritu_ptr+j)->f), "主体");
	}
    }
}

/*==================================================================*/
		      void clearMRPH(MRPH_P *p)
/*==================================================================*/
{
    MRPH_P *old;

    while (p) {
	old = p->next;
	free(p);
	p = old;
    }
}

/*==================================================================*/
			    void clearNE()
/*==================================================================*/
{
    PreservedNamedEntity *p = pNE;
    PreservedNamedEntity *old;

    pNE = NULL;

    while (p) {
	old = p->next;
	clearMRPH(p->mrph);
	free(p);
	p = old;
    }
}

/*==================================================================*/
	    void assign_ntt_dict(SENTENCE_DATA *sp, int i)
/*==================================================================*/
{
    int j, flag = 0;
    char sm[SM_CODE_SIZE+1];

    sm[SM_CODE_SIZE] = '\0';

    for (j = 0; sp->mrph_data[i].SM[j]; j+=SM_CODE_SIZE) {
	strncpy(sm, &(sp->mrph_data[i].SM[j]), SM_CODE_SIZE);
	if (!(flag & 0x01) && comp_sm(sm2code("地名"), sm, 0))
	    flag |= 0x01;
	else if (!(flag & 0x02) && comp_sm(sm2code("人名（固）"), sm, 0))
	    flag |= 0x02;
	else if (!(flag & 0x04) && comp_sm(sm2code("組織名"), sm, 0))
	    flag |= 0x04;
	else if (!(flag & 0x08) && comp_sm(sm2code("その他の固有名詞"), sm, 0))
	    flag |= 0x08;
    }

    if (flag & 0x01)
	assign_cfeature(&(sp->mrph_data[i].f), "NTT-地名");
    if (flag & 0x02)
	assign_cfeature(&(sp->mrph_data[i].f), "NTT-人名");
    if (flag & 0x04)
	assign_cfeature(&(sp->mrph_data[i].f), "NTT-組織名");
    if (flag & 0x08)
	assign_cfeature(&(sp->mrph_data[i].f), "NTT-固有名詞");
}

/*==================================================================*/
		   char *NEcheck(BNST_DATA *b_ptr)
/*==================================================================*/
{
    char *class;

    if (!(class = check_feature(b_ptr->f, "人名")) && 
	!(class = check_feature(b_ptr->f, "地名")) && 
	!(class = check_feature(b_ptr->f, "組織名")) && 
	!(class = check_feature(b_ptr->f, "固有名詞"))) {
	return NULL;
    }
    return class;
}

/*==================================================================*/
	 int NEparaCheck(BNST_DATA *b_ptr, BNST_DATA *h_ptr)
/*==================================================================*/
{
    char *class, str[11];

    /* b_ptr が固有表現である */
    if ((class = NEcheck(b_ptr)) == NULL) {
	return FALSE;
    }

    sprintf(str, "%s疑", class);

    /* h_ptr は固有表現ではない
       class と一致する「〜疑」がついている */
    if (NEcheck(h_ptr) || 
	!check_feature(h_ptr->f, str)) {
	return FALSE;
    }

    /* h_ptr を同じ固有表現クラスにする */
    assign_cfeature(&(h_ptr->f), class);
    assign_cfeature(&(h_ptr->f), "固並列OK");

    return TRUE;
}

/*==================================================================*/
		void NEparaAnalysis(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, value;
    char *cp;

    for (i = 0; i < sp->Bnst_num; i++) {
	/* 住所が入る並列はやめておく */
	if (check_feature(sp->bnst_data[i].f, "住所") || 
	    !check_feature(sp->bnst_data[i].f, "体言") || 
	    (sp->bnst_data[i].dpnd_head > 0 && 
	     !check_feature(sp->bnst_data[sp->bnst_data[i].dpnd_head].f, "体言"))) {
	    continue;
	}
	cp = check_feature(sp->bnst_data[i].f, "並結句数");
	if (cp) {
	    value = atoi(cp+strlen("並結句数:"));
	    if (value > 2) {
		/* 係り側を調べて固有名詞じゃなかったら、受け側を調べる */
		if (NEparaCheck(&(sp->bnst_data[i]), &(sp->bnst_data[sp->bnst_data[i].dpnd_head])) == FALSE)
		    NEparaCheck(&(sp->bnst_data[sp->bnst_data[i].dpnd_head]), &(sp->bnst_data[i]));
		continue;
	    }
	}

	cp = check_feature(sp->bnst_data[i].f, "並結文節数");
	if (cp) {
	    value = atoi(cp+strlen("並結文節数:"));
	    if (value > 1) {
		if (NEparaCheck(&(sp->bnst_data[i]), &(sp->bnst_data[sp->bnst_data[i].dpnd_head])) == FALSE)
		    NEparaCheck(&(sp->bnst_data[sp->bnst_data[i].dpnd_head]), &(sp->bnst_data[i]));
	    }
	}
    }
}

/*====================================================================
                               END
====================================================================*/
