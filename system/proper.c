/*====================================================================

			     固有表現処理

                                               R.SASANO 05. 7. 31

    $Id$
====================================================================*/
#include "knp.h"

/*
  svmのfeature文字列について
  末尾が0以外(〜a) : 文字列 (a:1〜5:対象の形態素の位置)   
  末尾が00   (a00) : 文節始
  末尾が10 (〜a10) : 品詞
  末尾が20 (〜a20) : 文字種
  末尾が30 (〜a30) : キャッシュ
  末尾が40 (〜a40) : 末尾
  末尾が50 (〜a50) : 文字数
  末尾が6    (〜6) : 係り先の主辞(自分自身)
  末尾が60  (〜60) : 表層格(自分自身)
  末尾が7    (〜7) : 係り先の主辞(文節末)
  末尾が70  (〜70) : 表層格(文節末)
  末尾が80 (〜b80) : 格フレームの意味 (b:1〜4)
  末尾が9    (〜9) : 自分自身の主辞
  末尾が90  (〜90) : 文節主辞
*/

#define SIZE               2
#define NE_TAG_NUMBER      9
#define NE_POSITION_NUMBER 4
#define FEATURE_MAX        1024 /* CRLデータにおける最長は272文字 */
#define TAG_POSITION_NAME  20 /* 最長は ORGANIZATION:single など19文字 */
#define HEAD               0
#define MIDDLE             1
#define TAIL               2
#define SINGLE             3
#define SIGX               10 /* SVMの結果を確率に近似するシグモイド関数の係数 */

DBM_FILE ne_db;
char *DBforNE;
char TagPosition[NE_MODEL_NUMBER][TAG_POSITION_NAME];
char *Tag_name[] = {
    "ORGANIZATION", "PERSON", "LOCATION", "ARTIFACT",
    "DATE", "TIME", "MONEY", "PERCENT", "OTHER", "OPTIONAL", "\0"};
char *Position_name[] = {
    "head", "middle", "tail", "single", "\0"};
char *Imi_feature[] = {"組織", "人", "主体", "場所", "\0"};
char *Chara_name[] = {
    "漢字", "ひらがな", "かな漢字", "カタカナ", "記号", "英記号", "数字", "その他", "\0"};

struct NE_MANAGER {
    char feature[FEATURE_MAX];          /* 素性 */
    int notHEAD;                        /* head, singleにはならない場合1 */
    int NEresult;                       /* NEの解析結果 */
    double prob[NE_MODEL_NUMBER];       /* 各タグ・ポジションとなる確率 */
    double max[NE_MODEL_NUMBER];        /* そこまでの最大スコア */
    int parent[NE_MODEL_NUMBER];        /* 最大スコアの経路 */
} NE_mgr[MRPH_MAX];

typedef struct ne_cache {
    char            *key;
    int	            ne_result[NE_MODEL_NUMBER];
    struct ne_cache *next;
} NE_CACHE;

NE_CACHE *ne_cache[TBLSIZE];

/*====================================================================
		     タグ・ポジション−コード対応
====================================================================*/
void init_tagposition()
{
    int i, j;

    for (i = 0; i < NE_TAG_NUMBER - 1; i++) {
	for (j = 0; j < NE_POSITION_NUMBER; j++) {
	    strcpy(TagPosition[i * NE_POSITION_NUMBER + j], Tag_name[i]);
	    strcat(TagPosition[i * NE_POSITION_NUMBER + j], ":");
	    strcat(TagPosition[i * NE_POSITION_NUMBER + j], Position_name[j]);
	}
    }
    strcpy(TagPosition[32], "OTHER:single");
}

/*====================================================================
		   タグ・ポジション−コード対応関数
====================================================================*/
int ne_tagposition_to_code(char *cp)
{
    int i;
    for (i = 0; TagPosition[i]; i++)
	if (str_eq(TagPosition[i], cp))
	    return i;
    return -1;
}

char *ne_code_to_tagposition(int num)
{    
    return TagPosition[num];
}

/*==================================================================*/
			void init_db_for_NE()
/*==================================================================*/
{
    char *db_filename;

    db_filename = check_dict_filename(DBforNE, TRUE);

    if ((ne_db = DB_open(db_filename, O_RDONLY, 0)) == NULL) {
	if (OptDisplay == OPT_DEBUG) {
	    fprintf(Outfp, "Opening %s ... failed.\n", db_filename);
	}
	fprintf(stderr, ";; Cannot open POS table for NE <%s>.\n", db_filename);
	exit(1);
    } 
    else {
	if (OptDisplay == OPT_DEBUG) {
	    fprintf(Outfp, "Opening %s ... done.\n", db_filename);
	}
    }
    free(db_filename);
}

/*==================================================================*/
			void close_db_for_NE()
/*==================================================================*/
{
    DB_close(ne_db);
}

/*==================================================================*/
			 void init_ne_cache()
/*==================================================================*/
{
    memset(ne_cache, 0, sizeof(NE_CACHE *)*TBLSIZE);
}

/*==================================================================*/
			void clear_ne_cache()
/*==================================================================*/
{
    int i;
    NE_CACHE *ncp, *next;

    for (i = 0; i < TBLSIZE; i++) {
	if (!ne_cache[i]) continue;
	ncp = ne_cache[i];
	while (ncp) {
	    free(ncp->key);
	    next = ncp->next;
	    free(ncp);
	    ncp = next;
	}
    }
    init_ne_cache();
}

/*==================================================================*/
       void register_ne_cache(char *key, int NEresult)
/*==================================================================*/
{
    /* NEの解析結果を登録する */
    int i;
    NE_CACHE **ncpp;

    ncpp = &(ne_cache[hash(key, strlen(key))]);
    while (*ncpp && strcmp((*ncpp)->key, key)) {
	ncpp = &((*ncpp)->next);
    }
    if (!(*ncpp)) {
	*ncpp = (NE_CACHE *)malloc_data(sizeof(NE_CACHE), "register_ne_cache");
	for (i = 0; i < NE_MODEL_NUMBER; i++) (*ncpp)->ne_result[i] = 0;
	(*ncpp)->key = strdup(key);
	(*ncpp)->next = NULL;
    }
    (*ncpp)->ne_result[NEresult] = 1;
}

/*==================================================================*/
	     int check_ne_cache(char *key, int NEresult)
/*==================================================================*/
{
    NE_CACHE *ncp;

    ncp = ne_cache[hash(key, strlen(key))];
    while (ncp) {
	if (!strcmp(ncp->key, key)) {
	    return ncp->ne_result[NEresult];
	}
	ncp = ncp->next;
    }
    return 0;
}

/*==================================================================*/
		     int get_mrph_ne(FEATURE *fp)
/*==================================================================*/
{
    int i;
    char cp[32];

    for (i = 0; i < NE_MODEL_NUMBER - 1; i++) {
	sprintf(cp, "NE:%s", ne_code_to_tagposition(i));
	if (check_feature(fp, cp)) return i;
    }
    for (i = 0; i < NE_POSITION_NUMBER; i++) {
	sprintf(cp, "NE:OPTIONAL:%s", Position_name[i]);
	if (check_feature(fp, cp)) {
	    return NE_MODEL_NUMBER + 3 + i;
	}
    }
    return NE_MODEL_NUMBER - 1;
}

/*==================================================================*/
		 int get_chara(MRPH_DATA *mrph_data)
/*==================================================================*/
{
    int i;

    if (mrph_data->Goi && !strncmp(mrph_data->Goi, "・", BYTES4CHAR)) return 5; /* 記号 */
    for (i = 0; strcmp(Chara_name[i], "その他"); i++)
	if (check_feature(mrph_data->f, Chara_name[i]))
            break;
    return i + 1;
}

/*==================================================================*/
	     char *get_pos(char *ret, MRPH_DATA *mrph_data, int num)
/*==================================================================*/
{
    int i, j, flag = 0;
    char buf[SMALL_DATA_LEN], pos[SMALL_DATA_LEN];

    ret[0] = '\0'; /* 再帰的に代入するため */
    /* 品詞曖昧性のある場合 */
    for (i = 0; i < CLASSIFY_NO + 1; i++) {    
	for (j = 0; j < CLASSIFY_NO + 1; j++) {
	    if (!Class[i][j].id) continue;
	    sprintf(pos, "品曖-%s", Class[i][j].id);   
	    if (check_feature(mrph_data->f, pos)) {
		if (OptNECRF) {
		    sprintf(buf, "%s:%s", ret, Class[i][j].id);
		}
		else {
		    sprintf(buf, "%s%d%d%d10:1 ", ret, i, j, num);
		}
		strcpy(ret, buf);
		flag++;
	    }
	}
    }
    if (flag > 1 || flag && OptNECRF) {
    	return ret;
    }
    
    /* 品詞曖昧性のない場合 */
    if (OptNECRF) {
	sprintf(ret, ":%s", Class[mrph_data->Hinshi][mrph_data->Bunrui].id);
	return ret;
    }
    
    if (mrph_data->Bunrui)
	sprintf(ret, "%d%d%d10:1 ", mrph_data->Hinshi, mrph_data->Bunrui, num);
    else
	sprintf(ret, "%d0%d10:1 ", mrph_data->Hinshi, num);
    
    return ret;
}

/*==================================================================*/
		  char *get_cache(char *ret, char *key, int num)
/*==================================================================*/
{
    int NEresult;
    NE_CACHE *ncp;
    char buf[SMALL_DATA_LEN2];

    ret[0] = '\0'; /* 再帰的に代入するため */
    for (NEresult = 0; NEresult < NE_MODEL_NUMBER - 1; NEresult++) {
	if (check_ne_cache(key, NEresult)) {
	    if (OptNECRF) sprintf(buf, "%s:%d", ret, NEresult);
	    else sprintf(buf, "%s%d%d30:1 ", ret, NEresult + 1, num);
	    strcpy(ret, buf);
	}
    }
    return ret;
}

/*==================================================================*/
	     char *get_feature(char *ret, MRPH_DATA *mrph_data, int num)
/*==================================================================*/
{
    int i, j;
    char buf[SMALL_DATA_LEN], *cp;
    char *feature_name[] = {"人名末尾", "組織名末尾", '\0'};

    ret[0] = '\0'; /* 再帰的に代入するため */
    /* 文節後方に人名末尾、組織名末尾という語があるか */
    for (j = 1;; j++) {
	if (!(mrph_data + j)->f || 
	    check_feature((mrph_data + j)->f, "文節始") ||
	    check_feature((mrph_data + j)->f, "記号") ||
	    check_feature((mrph_data + j)->f, "括弧")) break;
	for (i = 0; feature_name[i]; i++) {
	    if (check_feature((mrph_data + j)->f, feature_name[i])) {
		if (OptNECRF) sprintf(ret, "H:%s ", feature_name[i]);
		else sprintf(ret, "%d%d40:1 ", i + 3, num);
	    }
	}
    }

    /* 人名末尾、組織名末尾であるか */
    for (i = 0; feature_name[i]; i++) {
	if (check_feature(mrph_data->f, feature_name[i])) {
	    if (OptNECRF) sprintf(buf, "S:%s ", feature_name[i]);
	    else sprintf(buf, "%s%d%d40:1 ", ret, i + 1, num);
	    strcpy(ret, buf);
	}	
    }   

    if (!OptNECRF) return ret;

    /* 以下はOptNECRFの場合のみ実行 */
    if (!ret[0]) sprintf(ret, "NIL ");

    /* カテゴリの情報 */
    strcat(ret, "CT");
    if ((cp = check_feature(mrph_data->f, "カテゴリ"))) {
	strcat(ret, cp + strlen("カテゴリ"));
    }
    
    return ret;
}

/*==================================================================*/
	     char *get_parent(char *ret, MRPH_DATA *mrph_data, int num)
/*==================================================================*/
{
    int j, c;
    char buf[WORD_LEN_MAX * 2], *pcp, *ccp, *ncp;

    ret[0] = '\0'; /* 再帰的に代入するため */
    if (num != SIZE + 1) return ret;

    if ((pcp = check_feature(mrph_data->f, "Ｔ係り先の主辞"))) {
	if (OptNECRF) strcpy(ret, "CS");	
	if ((ccp = check_feature(mrph_data->f, "係"))) {
	    if (OptNECRF) {
		sprintf(buf, "%s:%s", ret, ccp + strlen("係") + 1);
	    }
	    else {
		c = case2num(ccp + strlen("係") + 1) + 3;
		if (!strcmp("係:未格", ccp)) c = 1;
		sprintf(buf, "%s%d60:1 ", ret, c);	
	    }
	    strcpy(ret, buf);	    	    
	}
	if (OptNECRF) sprintf(buf, "%s P:%s", ret, pcp + strlen("Ｔ係り先の主辞") + 1);
	else {
	    ncp = db_get(ne_db, pcp + strlen("Ｔ係り先の主辞") + 1);
	    sprintf(buf, "%s%s6:1 ", ret, ncp ? ncp : "");	
	    free(ncp);
	}
	strcpy(ret, buf);	    
    }
    
    /* 文節後方にあるか */
    if (!OptNECRF || !strstr(ret, "CS")) {
	for (j = 1;; j++) {
	    if (!(mrph_data + j)->f ||
		check_feature((mrph_data + j)->f, "文節始") ||
		check_feature((mrph_data + j)->f, "括弧")) break;
	    if ((pcp = check_feature((mrph_data + j)->f, "Ｔ係り先の主辞"))) {
		if (OptNECRF) strcpy(ret, "CS");	
		if ((ccp = check_feature((mrph_data + j)->f, "係"))) {
		    if (OptNECRF) {
			sprintf(buf, "%s:%s", ret, ccp + strlen("係") + 1);
		    }
		    else {
			c = case2num(ccp + strlen("係") + 1) + 3;
			if (!strcmp("係:未格", ccp)) c = 1;
			sprintf(buf, "%s%d70:1 ", ret, c);	
		    }
		    strcpy(ret, buf);	    	    
		}
		if (OptNECRF) {
		    sprintf(buf, "%s P:%s", ret, pcp + strlen("Ｔ係り先の主辞") + 1);
		}
		else {
		    ncp = db_get(ne_db, pcp + strlen("Ｔ係り先の主辞") + 1);
		    sprintf(buf, "%s%s7:1 ", ret, ncp ? ncp : "");	
		    free(ncp);
		}
		strcpy(ret, buf);	    
		break;
	    }
	}
    }

    if (OptNECRF && !strstr(ret, "P"))
	strcat(ret, "NIL NIL");

    if (OptNECRF && !(check_feature(mrph_data->f, "文節主辞") ||
		      check_feature(mrph_data->f, "Ｔ文節主辞")) &&
	!check_feature(mrph_data->f, "Ｔ主辞")) 
	strcat(ret, " NIL");

    if ((pcp = check_feature(mrph_data->f, "Ｔ主辞"))) {
	if (OptNECRF) {
	    sprintf(buf, "%s H:%s", ret, pcp + strlen("Ｔ主辞") + 1);
	}
	else {
	    ncp = db_get(ne_db, pcp + strlen("Ｔ主辞") + 1);
	    sprintf(buf, "%s%s9:1 ", ret, ncp ? ncp : "");	
	    free(ncp);
	}
	strcpy(ret, buf);	    
    }
    if (OptNECRF && (check_feature(mrph_data->f, "文節主辞") ||
		     check_feature(mrph_data->f, "Ｔ文節主辞"))) {
	if (strlen(mrph_data->Goi2) < WORD_LEN_MAX / BYTES4CHAR) {
	    sprintf(buf, "%s S:%s", ret, mrph_data->Goi2);
	}
	else {
	    sprintf(buf, "%s S:LONG_WORD", ret);
	}
	strcpy(ret, buf);
    }

    if (OptNECRF) {
	if (check_feature(mrph_data->f, "文節始") && 
	    (check_feature(mrph_data->f, "文節主辞") ||
	     check_feature(mrph_data->f, "Ｔ文節主辞"))) {
	    strcat(ret, " SINGLE");
	}
	else if (check_feature(mrph_data->f, "文節始")) {
	    strcat(ret, " START");
	}
	else if (check_feature(mrph_data->f, "文節主辞") ||
		 check_feature(mrph_data->f, "Ｔ文節主辞")) {
	    strcat(ret, " END");
	}
	else if (check_feature(mrph_data->f, "Ｔ主辞")) {
	    strcat(ret, " INTER");
	}
	else {
	    strcat(ret, " OTHER");
	}
    }

    return ret;
}

/*==================================================================*/
	     char *get_imi(char *ret, MRPH_DATA *mrph_data, int num)
/*==================================================================*/
{
    int i, j;
    char buf[SMALL_DATA_LEN2], cp[WORD_LEN_MAX];

    ret[0] = '\0'; /* 再帰的に代入するため */
    if (num != SIZE + 1) return ret;

    /* 組織、人、主体、場所 */
    for (i = 0; i < 4; i++) {
	sprintf (cp, "意味-%s", Imi_feature[i]);
	/* 意味素性があるか */
	if (check_feature(mrph_data->f, cp)) {
	    sprintf(buf, "%s%d180:1 ", ret, i + 1);
	    strcpy(ret, buf);
	}
	/* 文節後方にあるか */
	for (j = 1;; j++) {
	    if (!(mrph_data + j)->f ||
		check_feature((mrph_data + j)->f, "文節始") ||
		check_feature((mrph_data + j)->f, "括弧")) break;
	    if (check_feature((mrph_data + j)->f, cp)) {
		sprintf(buf, "%s%d280:1 ", ret, i + 1);
		strcpy(ret, buf);
		break;
	    }
	}
    }

    /* 固有表現 */
    for (i = 0; i < NE_TAG_NUMBER - 1; i++) {
	sprintf (cp, "意味-%s", Tag_name[i]);
	/* 意味素性があるか */
	if (check_feature(mrph_data->f, cp)) {
	    sprintf(buf, "%s%d380:1 ", ret, i + 1);
	    strcpy(ret, buf);
	}
	/* 文節後方にあるか */
	for (j = 1;; j++) {
	    if (!(mrph_data + j)->f ||
		check_feature((mrph_data + j)->f, "文節始") ||
		check_feature((mrph_data + j)->f, "括弧")) break;
	    if (check_feature((mrph_data + j)->f, cp)) {
		sprintf(buf, "%s%d480:1 ", ret, i + 1);
		strcpy(ret, buf);
		break;
	    }
	}
    }

    return ret;
}

/*==================================================================*/
	       int intcmp(const void *a, const void *b)
/*==================================================================*/
{
    return *((int *)b) - *((int *)a);
}

/*==================================================================*/
	       void make_crf_feature(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, k;
    char s[4][SMALL_DATA_LEN2];

    for (i = 0; i < sp->Mrph_num; i++) {

	get_pos(s[0], sp->mrph_data + i, 0);
	get_feature(s[1], sp->mrph_data + i, 0);
	get_parent(s[2], sp->mrph_data + i, SIZE + 1);
	get_cache(s[3], sp->mrph_data[i].Goi2, 0);
	
	/* 見出し 品詞 品詞細分類 品詞曖昧性 文字種 文字数
	   (表層格 係り先の主辞 主辞 文節内位置) キャッシュ */
	/* featureは1024字まで */
	sprintf(NE_mgr[i].feature, "%s %s %s A%s %s L:%d %s %s C%s",
		(strlen(sp->mrph_data[i].Goi2) < WORD_LEN_MAX / BYTES4CHAR) ? 
		sp->mrph_data[i].Goi2 : "LONG_WORD", /* MAX 64文字 */
		Class[sp->mrph_data[i].Hinshi][0].id, /* MAX 8+1文字(未定義語) */
		Class[sp->mrph_data[i].Hinshi][sp->mrph_data[i].Bunrui].id,
		/* MAX 18+1文字 (形容詞性述語接尾辞) */
		s[0], /* MAX 22+2文字? (格助詞:接続助詞:その他) */
		Chara_name[get_chara(sp->mrph_data + i) - 1], /* MAX 8+1文字 (ひらがな) */
		strlen(sp->mrph_data[i].Goi2) / BYTES4CHAR, /* MAX 3+2文字 */
		s[1], /* MAX 128文字 */ 
		s[2], /* MAX 256字 */
		OptNEcache ? "" : s[3]  /* MAX 256文字 */
	    );
    }
}

/*==================================================================*/
		 void make_svm_feature(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, k, f[FEATURE_MAX];
    char buf[FEATURE_MAX], s[5][SMALL_DATA_LEN2], *id, bnstb[7], bnsth[7], *cp, tmp[16];

    for (i = 0; i < sp->Mrph_num; i++) {
	buf[0] = '\0';
	
	/* 括弧始を除く記号は固有表現の先頭にはならない(ルール)  */
	NE_mgr[i].notHEAD = 0;
	if (get_chara(sp->mrph_data + i) == 5 &&
	    !check_feature(sp->mrph_data[i].f, "括弧始"))
	    NE_mgr[i].notHEAD = 1;
	
	for (j = i - SIZE; j <= i + SIZE; j++) {
	    if (j < 0 || j >= sp->Mrph_num)
		continue;

	    k = i - j + SIZE + 1;           
	    id = db_get(ne_db, sp->mrph_data[j].Goi2);
	    get_pos(s[0], sp->mrph_data + j, k);       /* 末尾空白 */
	    get_cache(s[1], sp->mrph_data[j].Goi2, k); /* 末尾空白 */
	    get_feature(s[2], sp->mrph_data + j, k);   /* 末尾空白 */
	    get_parent(s[3], sp->mrph_data + j, k);    /* 末尾空白 */
	    get_imi(s[4], sp->mrph_data + j, k);       /* 末尾空白 */
	    check_feature(sp->mrph_data[j].f, "文節始") ? 
		sprintf(bnstb, "%d00:1 ", k) : (bnstb[0] = '\0');   /* 末尾空白 */
	    check_feature(sp->mrph_data[j].f, "Ｔ文節主辞") ? 
		sprintf(bnsth, "%d90:1 ", k) : (bnsth[0] = '\0');   /* 末尾空白 */
	    
	    sprintf(buf, "%s%s%d:1 %s%s%s%d%d20:1 %s%s%d%d50:1 %s%s",
		    buf, id ? id : "", k,
		    bnstb[0] ? bnstb : "",
		    bnsth[0] ? bnsth : "",
		    s[0], 
		    get_chara(sp->mrph_data + j), k,
		    OptNEcache ? "" : s[1],
		    OptNEend ? "" : s[2],
		    strlen(sp->mrph_data[j].Goi2) / BYTES4CHAR, k, 
		    OptNEparent ? "" : s[3],
		    OptNEcase ? s[4] : "");
	    free(id);
	}

	/* svm_lightでは素性が昇順である必要があるためソートする */
	for (j = 0, cp = buf; sscanf(cp, "%d:1", &(f[j])); j++) {
	    if (!(cp = strstr(cp, " "))) break;
	    cp++;
	}
	qsort(f, j, sizeof(int), intcmp);
	NE_mgr[i].feature[0] = '\0';
	while (j-- > 0) {
	    sprintf(tmp, "%d:1 ", f[j]);
	    strcat(NE_mgr[i].feature, tmp);
	}    
    }       
}

/*==================================================================*/
	      void output_feature(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, code;
    char *cp;

    for (i = 0; i < sp->Mrph_num; i++) {
	if ((cp = check_feature(sp->mrph_data[i].f, "NE"))) {
	    code = ne_tagposition_to_code(cp + 3);
	}
	else {
	    code = NE_MODEL_NUMBER - 1;
	}  
	NE_mgr[i].NEresult = code;
	if (OptDisplay == OPT_DEBUG) {
	    fprintf(stderr, "%d %s\t%s\n", code, sp->mrph_data[i].Goi2, NE_mgr[i].feature);
	}
	else {
	    if (OptNECRF) {
		/* CRFの学習結果の文字としてのソート順と、数字としてのソート順を一致させるため
		   codeに100を足している */
		fprintf(stderr, "%s %d\n", NE_mgr[i].feature, code + 100);
	    }
	    else {
		for (j = 0; j < NE_MODEL_NUMBER; j++) {
		    fprintf(stderr, (j == code) ? "+1 " : "-1 ");
		}
		fprintf(stderr, "%s\n", NE_mgr[i].feature);
	    }
	}
    }
    fprintf(stderr, "\n");
}

/*==================================================================*/
	       void apply_model(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j;
    char *EUCbuffer;

#ifdef USE_CRF
    if (OptNECRF) {   
	clear_crf();    
	for (i = 0; i < sp->Mrph_num; i++) {
	    if (OptDisplayNE == OPT_DEBUG)
		fprintf(stderr, "%d %s\t%s\n", i, sp->mrph_data[i].Goi2, NE_mgr[i].feature);
#ifdef IO_ENCODING_SJIS
            /* SJIS版はEUCのモデルファイルを用いる */
            EUCbuffer = toStringEUC(NE_mgr[i].feature);
	    crf_add(EUCbuffer);
            free(EUCbuffer);
#else
	    crf_add(NE_mgr[i].feature);
#endif
	}
	crf_parse();
    }
#endif	       
    
    for (i = 0; i < sp->Mrph_num; i++) {
	if (OptDisplayNE == OPT_DEBUG)
	    fprintf(stderr, "%d %s\t%s\n", i, sp->mrph_data[i].Goi2, NE_mgr[i].feature);
	
	for (j = 0; j < NE_MODEL_NUMBER; j++) {
	    if (NE_mgr[i].notHEAD && 
		j  != NE_MODEL_NUMBER - 1 &&
		(j % 4 == HEAD || j % 4 == SINGLE)) {
		NE_mgr[i].prob[j] = 0; /* ヒューリスティックルール */
	    }
	    else {
#ifdef USE_CRF
		if (OptNECRF) {
		    get_crf_prob(i, j, &(NE_mgr[i].prob[j]));
		}
#endif	       
#ifdef USE_SVM
		if (!OptNECRF) {
		    NE_mgr[i].prob[j] 
			= 1/(1+exp(-svm_classify_for_NE(NE_mgr[i].feature, j) * SIGX));
		}
#endif	       
		if (OptDisplayNE == OPT_DEBUG) {
		    fprintf(stderr, "%2d %f\t", j, NE_mgr[i].prob[j]);
		    if (j % 4 == SINGLE && j  != NE_MODEL_NUMBER - 2) fprintf(stderr, "\n");
		    if (j  == NE_MODEL_NUMBER - 1) fprintf(stderr, "\n\n");
		}
	    }
	}
    }
}

/*==================================================================*/
		  int constraint(int pre, int self, int last)
/*==================================================================*/
{
    /* 前後に来れるタグの制約に違反すれば1を返す  */
    if (pre  == NE_MODEL_NUMBER - 1) pre  += 3;
    if (self == NE_MODEL_NUMBER - 1) self += 3;

    if (pre == -1) {
	if (self % 4 == MIDDLE || self % 4 == TAIL) return 1;
	return 0;
    }

    if (last && (self % 4 == HEAD || self % 4 == MIDDLE)) return 1;
	
    if ((pre  % 4 == HEAD   || pre  % 4 == MIDDLE) &&
	 self % 4 != MIDDLE && self % 4 != TAIL  ) return 1;
    if ( pre  % 4 != HEAD   && pre  % 4 != MIDDLE  &&
	(self % 4 == MIDDLE || self % 4 == TAIL  )) return 1;  
    if ((pre  % 4 == HEAD   || pre  % 4 == MIDDLE) &&
	pre/4 != self/4) return 1;   
    return 0;
}

/*==================================================================*/
		   void viterbi(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, k;
    double score, max;
  
    for (i = 0; i < sp->Mrph_num; i++) {
	for (j = 0; j < NE_MODEL_NUMBER; j++) {

	    /* 文頭の場合 */
	    if (i == 0) {
		if (constraint(-1, j, 0)) continue;
		NE_mgr[i].max[j] = NE_mgr[i].prob[j];
		NE_mgr[i].parent[j] = -1; /* 文頭 */
		continue;
	    }

	    /* 文頭、文末以外 */
	    NE_mgr[i].max[j] = 0;
	    for (k = 0; k < NE_MODEL_NUMBER; k++) {
		if (i == sp->Mrph_num - 1) {
		    if (constraint(k, j, 1)) continue;
		}
		else {
		    if (constraint(k, j, 0)) continue;
		}
		score = NE_mgr[i-1].max[k]*NE_mgr[i].prob[j];
		if (score > NE_mgr[i].max[j]) { /* 同点の場合は無視 */
		    NE_mgr[i].max[j] = score;
		    NE_mgr[i].parent[j] = k;
		}
	    }
	}
    }       
    max = 0;
    for (j = 0; j < NE_MODEL_NUMBER; j++) {
	if (NE_mgr[sp->Mrph_num-1].max[j] > max) {
	    max = NE_mgr[sp->Mrph_num-1].max[j];
	    NE_mgr[sp->Mrph_num-1].NEresult = j;
	}
    }
    for (i = sp->Mrph_num - 1; i > 0; i--) {
	NE_mgr[i-1].NEresult = NE_mgr[i].parent[NE_mgr[i].NEresult];
    }
}

/*==================================================================*/
	    void assign_ne_feature_mrph(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;
    char cp[WORD_LEN_MAX];

    /* 形態素に付与 */
    for (i = 0; i < sp->Mrph_num; i++) {
	if (NE_mgr[i].NEresult == NE_MODEL_NUMBER -1) continue; /* OTHERの場合 */
	sprintf(cp, "NE:%s", ne_code_to_tagposition(NE_mgr[i].NEresult));
	assign_cfeature(&(sp->mrph_data[i].f), cp, FALSE);
    }
}

/*==================================================================*/
	    void assign_ne_feature_tag(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j;
    char cp[WORD_LEN_MAX * 16];
    char cp_nai[WORD_LEN_MAX * 16];

    /* タグに付与 */
    for (j = 0; j < sp->Tag_num; j++) { /* 同一タグの固有表現は一種類まで */
	for (i = 0; i < sp->tag_data[j].mrph_num; i++) {
	    if (check_feature((sp->tag_data[j].mrph_ptr + i)->f, "NE")) break;
	}
	/* 対象のタグに固有表現が無ければ次のタグへ */
	if (i == sp->tag_data[j].mrph_num) continue;

	/* ORGANIZATION、PERSONの場合は意味素として与える */
	if (!strcmp(Tag_name[get_mrph_ne((sp->tag_data[j].mrph_ptr + i)->f) / 4], "ORGANIZATION")) {
	    assign_sm((BNST_DATA *)(sp->tag_data +j), "組織");
	}
	else if (!strcmp(Tag_name[get_mrph_ne((sp->tag_data[j].mrph_ptr + i)->f) / 4], "PERSON")) {
	    assign_sm((BNST_DATA *)(sp->tag_data + j), "人");
	}
 
	sprintf(cp, "NE:%s:",
		Tag_name[get_mrph_ne((sp->tag_data[j].mrph_ptr + i)->f) / 4]);
	while(1) {
	    if (get_mrph_ne((sp->tag_data[j].mrph_ptr + i)->f) == NE_MODEL_NUMBER - 1) {
		OptNElearn ?
		    fprintf(stdout, "Illegal NE ending %s \"%s %s\"!!\n", 
			    sp->KNPSID, 
			    (sp->tag_data[j].mrph_ptr + i - 1)->Goi2,
			    (sp->tag_data[j].mrph_ptr + i)->Goi2) :
		    fprintf(stderr, "Illegal NE ending %s \"%s %s\"!!\n", 
			    sp->KNPSID, 
			    (sp->tag_data[j].mrph_ptr + i - 1)->Goi2,
			    (sp->tag_data[j].mrph_ptr + i)->Goi2);		   
		break;
	    }    
	    
	    if (strlen(cp) + strlen((sp->tag_data[j].mrph_ptr + i)->Goi2) >= WORD_LEN_MAX * 16) {
		fprintf(stderr, ";; Too long tag data for %s... .\n", cp);
		exit(1);
	    }

	    strcat(cp, (sp->tag_data[j].mrph_ptr + i)->Goi2);
	    if (get_mrph_ne((sp->tag_data[j].mrph_ptr + i)->f) % 4 == SINGLE ||
		get_mrph_ne((sp->tag_data[j].mrph_ptr + i)->f) % 4 == TAIL) {
		assign_cfeature(&(sp->tag_data[j].f), cp, FALSE);
		break;
	    }
	    /* 複数のタグにまたがっている場合は次のタグに進む */
	    i++;
	    if (i == sp->tag_data[j].mrph_num) {
		i = 0;
		sprintf(cp_nai, "NE内:%s",
			Tag_name[get_mrph_ne((sp->tag_data[j].mrph_ptr + i)->f) / 4]);
		assign_cfeature(&(sp->tag_data[j].f), cp_nai, FALSE);
		j++;
	    }
	}
    }
}

/*==================================================================*/
		 void ne_analysis(SENTENCE_DATA *sp)
/*==================================================================*/
{
    if (OptNECRF) {
	make_crf_feature(sp);
    }
    else {
	make_svm_feature(sp);
    }	
	
    if (OptNElearn) {
	output_feature(sp);
    }
    else {
	/* モデルを適用 */
	apply_model(sp);
	/* 文全体で最適化 */
	viterbi(sp);
	/* 結果を付与 */
	assign_ne_feature_mrph(sp);
	/* 人名をひとつのタグにするためのルールを読む */
	assign_general_feature(sp->mrph_data, sp->Mrph_num, NeMorphRuleType, FALSE, FALSE);
    }
}

/*==================================================================*/
		 void read_ne(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, code;
    char *cp;

    for (i = 0; i < sp->Mrph_num; i++) {
	if ((cp = check_feature(sp->mrph_data[i].f, "NE:OPTIONAL"))) {
	    for (j = 0; j < NE_POSITION_NUMBER; j++) {
		!strcmp(Position_name[j], cp + strlen("NE:OPTIONAL"));
		code = NE_MODEL_NUMBER - 1 + j;
	    }
	}
	else if ((cp = check_feature(sp->mrph_data[i].f, "NE"))) {
	    code = ne_tagposition_to_code(cp + 3);
	}
	else {
	    code = NE_MODEL_NUMBER - 1;
	}  
	NE_mgr[i].NEresult = code;
    }
    /* 人名をひとつのタグにするためのルールを読む */
    assign_general_feature(sp->mrph_data, sp->Mrph_num, NeMorphRuleType, FALSE, FALSE);
}

/*==================================================================*/
	    void for_ne_analysis(SENTENCE_DATA *sp)
/*==================================================================*/
{
    /* 構文・格解析結果から、固有表現解析用のfeatureを付与する */
   
    int i, j, k, l, num;
    char cp[WORD_LEN_MAX];
    CF_PRED_MGR *cpm_ptr;

    /* 主辞の情報 */
    for (j = 0; j < sp->Bnst_num - 1; j++) {
	assign_cfeature(&((sp->bnst_data[j].head_ptr)->f), "Ｔ文節主辞", FALSE);

	(strlen((sp->bnst_data[j].head_ptr)->Goi) < WORD_LEN_MAX / BYTES4CHAR) ? 
	    sprintf(cp, "Ｔ主辞:%s", (sp->bnst_data[j].head_ptr)->Goi) :
	    sprintf(cp, "Ｔ主辞:LONG_WORD");
	for (i = 1; (sp->bnst_data[j].head_ptr - i)->f; i++) {
	    if (!(sp->bnst_data[j].head_ptr - i)->f ||
		check_feature((sp->bnst_data[j].head_ptr - i + 1)->f, "文節始")) break;
	    assign_cfeature(&((sp->bnst_data[j].head_ptr - i)->f), cp, FALSE);
	}
    }

    /* 親の情報 */
    if (!OptNEparent) {
	/* 文節を前からチェック */
	for (j = 0; j < sp->Bnst_num - 1; j++) {	    
	    (strlen((sp->bnst_data[sp->bnst_data[j].dpnd_head].head_ptr)->Goi) < WORD_LEN_MAX / BYTES4CHAR) ? 
		sprintf(cp, "Ｔ係り先の主辞:%s",
			 (sp->bnst_data[sp->bnst_data[j].dpnd_head].head_ptr)->Goi) :
		sprintf(cp, "Ｔ係り先の主辞:LONG_WORD");
			 
	    assign_cfeature(&((sp->bnst_data[j].head_ptr)->f), cp, FALSE);
	    assign_cfeature(&((sp->bnst_data[j].head_ptr)->f), 
			    check_feature(sp->bnst_data[j].f, "係"), FALSE);    
	}
    }

    /* 格フレームの意味情報 */
    if (OptNEcase) {
	/* タグを後からチェック */
	for (j = sp->Tag_num - 1; j > 0 ; j--) {
	    if (!(cpm_ptr = sp->tag_data[j].cpm_ptr)) continue;

	    for (i = 0; i < cpm_ptr->cf.element_num; i++) {
		num = cpm_ptr->cmm[0].result_lists_d[0].flag[i];

		/* 前方に存在し、主辞の直後に助詞が来る場合のみ */
		if (((cpm_ptr->elem_b_ptr[i])->head_ptr + 1)->Hinshi != 9 ||
		    cpm_ptr->elem_b_ptr[i]->num > j) continue;
		
		/* 組織、人、主体、場所 */
		for (k = 0; k < 4; k++) {
		    if (cf_match_element(cpm_ptr->cmm[0].cf_ptr->sm[num], 
					 Imi_feature[k], TRUE)) {
			sprintf (cp, "意味-%s", Imi_feature[k]);
			assign_cfeature(&((cpm_ptr->elem_b_ptr[i])->head_ptr->f), 
					cp, FALSE);
		    }
		}
		/* 固有表現 */
		for (k = 0; k < NE_TAG_NUMBER - 1; k++) {
		    if (cf_match_element(cpm_ptr->cmm[0].cf_ptr->sm[num], 
					 Tag_name[k], TRUE)) {
			sprintf (cp, "意味-%s", Tag_name[k]);
			assign_cfeature(&((cpm_ptr->elem_b_ptr[i])->head_ptr->f), 
					cp, FALSE);
		    }
		}
	    }
	}
    }
}

/*==================================================================*/
int ne_corefer(SENTENCE_DATA *sp, int i, char *anaphor, char *ne, int yomi_flag)
/*==================================================================*/
{
    /* 固有表現(ORGANIZATION)と */
    /* 共参照関係にあると判断された固有表現タグの付与されていない表現に */
    /* 固有表現タグを付与する */
   
    int start, end, ne_tag, j, k;
    char cp[WORD_LEN_MAX], word[WORD_LEN_MAX];

    if (strlen(anaphor) == BYTES4CHAR) return 0;

    for (ne_tag = 0; ne_tag < NE_TAG_NUMBER; ne_tag++) {
	/* どのタグであるかを"NE:"に続く4文字で判断する */
	if (!strncmp(ne + 3, Tag_name[ne_tag], 4)) break;
    }

    /* ORGANIZATION、PERSONの場合のみ */
    if (strcmp(Tag_name[ne_tag], "ORGANIZATION") &&
	(strcmp(Tag_name[ne_tag], "PERSON") || !yomi_flag)) return 0;

    end = (sp->tag_data + i)->head_ptr - sp->mrph_data;  /* 接尾辞を含むものには未対応 */
    if (check_feature(((sp->tag_data + i)->head_ptr)->f, "記号")) end--;
    
    for (start = end; start >= 0; start--) {
	word[0] = '\0';
	for (k = start; k <= end; k++) {
	    strcat(word, (sp->mrph_data + k)->Goi2); /* 先行詞候補 */
	}
	if (!strcmp(word, anaphor)) break;
    }    
    if (strcmp(word, anaphor)) return 0;

    /* 形態素に付与、NEresultに記録 */
    if ((j = start) == end) {
	sprintf(cp, "NE:%s:single", Tag_name[ne_tag]);
	assign_cfeature(&(sp->mrph_data[j].f), cp, FALSE);
	NE_mgr[j].NEresult = ne_tag * 4 + 3; /* single */
    }
    else for (j = start; j <= end; j++) {
	if (j == start) {
	    sprintf(cp, "NE:%s:head", Tag_name[ne_tag]);
	    assign_cfeature(&(sp->mrph_data[j].f), cp, FALSE);
	    NE_mgr[j].NEresult = ne_tag * 4; /* head */
	}
	else if (j == end) {
	    sprintf(cp, "NE:%s:tail", Tag_name[ne_tag]);
	    assign_cfeature(&(sp->mrph_data[j].f), cp, FALSE);
	    NE_mgr[j].NEresult = ne_tag * 4 + 2; /* tail */
	}
	else {
	    sprintf(cp, "NE:%s:middle", Tag_name[ne_tag]);
	    assign_cfeature(&(sp->mrph_data[j].f), cp, FALSE);
	    NE_mgr[j].NEresult = ne_tag * 4 + 1; /* middle */
	}
    }

    /* ORGANIZATION、PERSONの場合は意味素として与える */
    if (!strcmp(Tag_name[get_mrph_ne((sp->tag_data[i].head_ptr)->f) / 4],
		"ORGANIZATION")) {
	assign_sm((BNST_DATA *)(sp->tag_data + i), "組織");
    }
    else if (!strcmp(Tag_name[get_mrph_ne((sp->tag_data[i].head_ptr)->f) / 4],
		     "PERSON")) {
	assign_sm((BNST_DATA *)(sp->tag_data + i), "人");
    }

    /* タグに付与 */
    sprintf(cp, "NE:%s:%s", Tag_name[ne_tag], anaphor);
    assign_cfeature(&(sp->tag_data[i].f), cp, FALSE);  
    sprintf(cp, "NE内:%s", Tag_name[ne_tag]);
    for (k = 0; start < sp->tag_data[i - k].mrph_ptr - sp->mrph_data;) {
	k++;
	assign_cfeature(&(sp->tag_data[i - k].f), cp, FALSE);
    }
    
    return 1;
}

/*==================================================================*/
		void make_ne_cache(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, ne_tag;
    char *cp;

    /* 各形態素の情報を記憶 */
    for (i = 0; i < sp->Mrph_num; i++) {
	register_ne_cache(sp->mrph_data[i].Goi2, NE_mgr[i].NEresult);
    }
}
	  
/*====================================================================
                               END
====================================================================*/
