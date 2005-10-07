/*====================================================================

			     固有名詞処理

                                               S.Kurohashi 96. 7. 4

    $Id$
====================================================================*/
#include "knp.h"

#define SIZE               2
#define NE_TAG_NUMBER      9
#define NE_POSITION_NUMBER 4
#define FEATIRE_MAX        1024
#define HEAD               0
#define MIDDLE             1
#define TAIL               2
#define SINGLE             3

DBM_FILE ne_db;

char *DBforNE;
char TagPosition[NE_MODEL_NUMBER][20];
char *Tag_name[] = {
    "ORGANIZATION", "PERSON", "LOCATION", "ARTIFACT",
    "DATE", "TIME", "MONEY", "PERCENT", "OTHER"};
char *Position_name[] = {
    "head", "middle", "tail", "single"};
struct NE_MANAGER {
    char feature[FEATIRE_MAX];          /* 素性 */
    int NEresult;                       /* NEの解析結果 */
    double SVMscore[NE_MODEL_NUMBER];   /* 各タグ・ポジションとなる確率 */
    double max[NE_MODEL_NUMBER];        /* そこまでの最大スコア */
    int parent[NE_MODEL_NUMBER];        /* 最大スコアの経路 */
} NE_mgr[MRPH_MAX];

typedef struct ne_cache {
    char            *key;
    int	            ne_result[NE_MODEL_NUMBER];
    struct ne_cache *next;
} NE_CACHE;

NE_CACHE ne_cache[TBLSIZE];

/*====================================================================
			 タグ・ポジション−コード対応
====================================================================*/
void init_tagposition()
{
    int i, j;
    for (i = 0; i < NE_TAG_NUMBER - 1; i++) {
	for (j = 0; j < NE_POSITION_NUMBER; j++) {
	    strcpy(TagPosition[i * NE_POSITION_NUMBER + j], Tag_name[i]);
	    strcat(TagPosition[i * NE_POSITION_NUMBER + j], Position_name[j]);
	}
    }
    strcpy(TagPosition[32], "OTHERsingle");
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
               void init_NE_mgr()
/*==================================================================*/
{
    memset(NE_mgr, 0, sizeof(struct NE_MANAGER)*MRPH_MAX);
}

/*==================================================================*/
		void init_ne_cache()
/*==================================================================*/
{
    memset(ne_cache, 0, sizeof(NE_CACHE)*TBLSIZE);
}

/*==================================================================*/
	       int get_chara(FEATURE *f)
/*==================================================================*/
{
    int i;
    char *Chara_name[] = {
	"漢字", "ひらがな", "かな漢字", "カタカナ", "記号", "英字", "数字", ""};

    for (i = 0; *Chara_name[i]; i++)
	if (check_feature(f, Chara_name[i]))
	    break;
    return i + 1;
}

/*==================================================================*/
	       char *get_pos(MRPH_DATA mrph_data, int num)
/*==================================================================*/
{
    int i, j;
    char *ret, pos[32];
    ret = (char *)malloc_data(64, "get_pos");
    memset(ret, 0, sizeof(char)*64);

    if (!check_feature(mrph_data.f, "品曖")) {
	if (mrph_data.Bunrui)
	    sprintf(ret, "%d%d%d10:1 ", mrph_data.Hinshi, mrph_data.Bunrui, num);
	else
	    sprintf(ret, "%d0%d10:1 ", mrph_data.Hinshi, num);
	return ret;
    }
	
    /* 品詞曖昧性のある場合 */
    for (i = 0; i < CLASSIFY_NO + 1; i++) {
	for (j = 0; j < CLASSIFY_NO + 1; j++) {
	    if (!Class[i][j].id) break;
	    sprintf(pos, "品曖-%s", Class[i][j].id);   
	    if (check_feature(mrph_data.f, pos))
		sprintf(ret, "%s%d%d%d10:1 ", ret, i, j, num);
	}
    }
    return ret;
}

/*==================================================================*/
		void clear_ne_cache()
/*==================================================================*/
{
    int i;
    NE_CACHE *ncp, *next;

    for (i = 0; i < TBLSIZE; i++) {
	ncp = ne_cache[i].next;
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
    /* 解析が終了するまで破棄しないのでメモリを必要とする可能性あり */

    NE_CACHE *ncp;

    ncp = &(ne_cache[hash(key, strlen(key))]);
    while (ncp && ncp->key && !strcmp(ncp->key, key)) {
	ncp = ncp->next;
    }
    if (!ncp) {
	ncp = (NE_CACHE *)malloc_data(sizeof(NE_CACHE), "register_ne_cache");
	memset(ne_cache, 0, sizeof(NE_CACHE));
    }
    if (!ncp->key) {
	ncp->key = strdup(key);
	ncp->next = NULL;
    }
    ncp->ne_result[NEresult] = 1;
}

/*==================================================================*/
		 int check_ne_cache(char *key, int NEresult)
/*==================================================================*/
{
    NE_CACHE *ncp;

    ncp = &(ne_cache[hash(key, strlen(key))]);
    if (!ncp->key) {
	return 0;
    }
    while (ncp) {
	if (!strcmp(ncp->key, key)) {
	    if (ncp->ne_result[NEresult]) return 1;
	    return 0;
	}
	ncp = ncp->next;
    }
    return 0;
}

/*==================================================================*/
		 char *get_cache(char *key, int i)
/*==================================================================*/
{
    int NEresult;
    NE_CACHE *ncp;
    char *ret;

    ret = (char *)malloc_data(32, "get_cache");
    memset(ret, 0, sizeof(char)*32);
    /* return ret; */

    for (NEresult = 0; NEresult < NE_MODEL_NUMBER-1; NEresult++) {
	if (check_ne_cache(key, NEresult))
	    sprintf(ret, "%s%d%d30:1 ", ret, NEresult+1, i);
    }
    return ret;
}

/*==================================================================*/
               void make_feature(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, k;
    char *s[3];

    for (i = 0; i < sp->Mrph_num; i++) {
	for (j = i - SIZE; j <= i + SIZE; j++) {
	    if (j < 0 || j >= sp->Mrph_num)
		continue;
	    
	    s[0] = db_get(ne_db, sp->mrph_data[j].Goi2);
	    s[1] = get_pos(sp->mrph_data[j], i - j + SIZE + 1);
	    s[2] = get_cache(sp->mrph_data[j].Goi2, i - j + SIZE + 1);
	    k = i - j + SIZE + 1;
	    sprintf(NE_mgr[i].feature, "%s%s%d:1 %s%d%d20:1 %s",
		    NE_mgr[i].feature, s[0] ? s[0] : "", k,
		    s[1], get_chara(sp->mrph_data[j].f), k, s[2]);
	    free(s[0]);
	    free(s[1]);
	    free(s[2]);
	}
	if (OptDisplay == OPT_DEBUG)
	    fprintf(stderr, "%s\t%s\n", sp->mrph_data[i].Goi2, NE_mgr[i].feature);
    }       
}

/*==================================================================*/
               void apply_svm_model(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j;
    
    for (i = 0; i < sp->Mrph_num; i++) {
	for (j = 0; j < NE_MODEL_NUMBER; j++) {
#ifdef USE_SVM
	    NE_mgr[i].SVMscore[j] 
		= 1/(1+exp(-svm_classify_for_NE(NE_mgr[i].feature, j)*5));
	    /* fprintf(stderr, "%f\t", NE_mgr[i].SVMscore[j]); */
#endif
	}
    }
}

/*==================================================================*/
               int constraint(int pre, int self)
/*==================================================================*/
{
    /* 前後に来れるタグの制約に違反すれば1を返す  */
    if (pre  == NE_MODEL_NUMBER - 1) pre  += 3;
    if (self == NE_MODEL_NUMBER - 1) self += 3;

    if (pre == -1) {
	if (self % 4 == MIDDLE || self % 4 == TAIL) return 1;
	return 0;
    }
	
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
		if (constraint(-1, j)) continue;
		NE_mgr[i].max[j] = NE_mgr[i].SVMscore[j];
		NE_mgr[i].parent[j] = -1; /* 文頭 */
		continue;
	    }
	    
	    /* 文頭以外 */
	    NE_mgr[i].max[j] = 0;
	    for (k = 0; k < NE_MODEL_NUMBER; k++) {
		if (constraint(k, j)) continue;
		score = NE_mgr[i-1].max[k]*NE_mgr[i].SVMscore[j];
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
               int get_mrph_ne(FEATURE *fp)
/*==================================================================*/
{
    int i;
    char cp[32];

    for (i = 0; i < NE_MODEL_NUMBER - 1; i++) {
	sprintf(cp, "NE:%s", ne_code_to_tagposition(i));
	if (check_feature(fp, cp)) return i;
    }
    return i;
}

/*==================================================================*/
	       void assign_ne_feature(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, mrph_num;
    char cp[128];

    /* 形態素に付与 */
    for (i = 0; i < sp->Mrph_num; i++) {
	if (NE_mgr[i].NEresult == NE_MODEL_NUMBER -1) continue;
	sprintf(cp, "NE:%s", ne_code_to_tagposition(NE_mgr[i].NEresult));
	assign_cfeature(&(sp->mrph_data[i].f), cp);
    }
    /* タグに付与 */
    for (j = 0; j < sp->Tag_num; j++) { /* 同一タグの固有表現は一種類まで */
	for (i = 0; i < sp->tag_data[j].mrph_num; i++) {
	    if (check_feature((sp->tag_data[j].mrph_ptr + i)->f, "NE")) break;
	}
	/* 対象のタグに固有表現が無ければ次のタグへ */
	if (i == sp->tag_data[j].mrph_num) continue;

	memset(cp, 0, sizeof(char)*128);
	sprintf(cp, "NE:(%s)",
		Tag_name[get_mrph_ne((sp->tag_data[j].mrph_ptr + i)->f) / 4]);
	for (mrph_num = 1;; mrph_num++) {
	    strcat(cp, (sp->tag_data[j].mrph_ptr + i)->Goi2);
	    if (get_mrph_ne((sp->tag_data[j].mrph_ptr + i)->f) % 4 == SINGLE || 
		get_mrph_ne((sp->tag_data[j].mrph_ptr + i)->f) % 4 == TAIL) {
		assign_cfeature(&(sp->tag_data[j].f), cp);
		break;
	    }
	    /* 複数のタグにまたがっている場合は次のタグに進む */
	    i++;
	    if (i == sp->tag_data[j].mrph_num) {
		i = 0;
		j++;
	    }
	}
    }
}

/*==================================================================*/
	       void make_ne_cache(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;

    for (i = 0; i < sp->Mrph_num; i++) {
	register_ne_cache(sp->mrph_data[i].Goi2, NE_mgr[i].NEresult);
    }
}

/*==================================================================*/
	       void ne_analysis(SENTENCE_DATA *sp)
/*==================================================================*/
{
    init_NE_mgr();
    make_feature(sp);
    apply_svm_model(sp);
    viterbi(sp);
    assign_ne_feature(sp);
    make_ne_cache(sp);
}

/* /\*==================================================================*\/ */
/* 		   char *NEcheck(BNST_DATA *b_ptr) */
/* /\*==================================================================*\/ */
/* { */
/*     char *class; */

/*     if (!(class = check_feature(b_ptr->f, "人名")) &&  */
/* 	!(class = check_feature(b_ptr->f, "地名")) &&  */
/* 	!(class = check_feature(b_ptr->f, "組織名")) &&  */
/* 	!(class = check_feature(b_ptr->f, "固有名詞"))) { */
/* 	return NULL; */
/*     } */
/*     return class; */
/* } */

/* /\*==================================================================*\/ */
/* 	 int NEparaCheck(BNST_DATA *b_ptr, BNST_DATA *h_ptr) */
/* /\*==================================================================*\/ */
/* { */
/*     char *class, str[11]; */

/*     /\* b_ptr が固有表現である *\/ */
/*     if ((class = NEcheck(b_ptr)) == NULL) { */
/* 	return FALSE; */
/*     } */

/*     sprintf(str, "%s疑", class); */

/*     /\* h_ptr は固有表現ではない */
/*        class と一致する「〜疑」がついている *\/ */
/*     if (NEcheck(h_ptr) ||  */
/* 	!check_feature(h_ptr->f, str)) { */
/* 	return FALSE; */
/*     } */

/*     /\* h_ptr を同じ固有表現クラスにする *\/ */
/*     assign_cfeature(&(h_ptr->f), class); */
/*     assign_cfeature(&(h_ptr->f), "固並列OK"); */

/*     return TRUE; */
/* } */

/* /\*==================================================================*\/ */
/* 	       void ne_para_analysis(SENTENCE_DATA *sp) */
/* /\*==================================================================*\/ */
/* { */
/*     int i, value; */
/*     char *cp; */

/*     for (i = 0; i < sp->Bnst_num; i++) { */
/* 	/\* 住所が入る並列はやめておく *\/ */
/* 	if (check_feature(sp->bnst_data[i].f, "住所") ||  */
/* 	    !check_feature(sp->bnst_data[i].f, "体言") ||  */
/* 	    (sp->bnst_data[i].dpnd_head > 0 &&  */
/* 	     !check_feature(sp->bnst_data[sp->bnst_data[i].dpnd_head].f, "体言"))) { */
/* 	    continue; */
/* 	} */
/* 	cp = check_feature(sp->bnst_data[i].f, "並結句数"); */
/* 	if (cp) { */
/* 	    value = atoi(cp+strlen("並結句数:")); */
/* 	    if (value > 2) { */
/* 		/\* 係り側を調べて固有名詞じゃなかったら、受け側を調べる *\/ */
/* 		if (NEparaCheck(&(sp->bnst_data[i]), &(sp->bnst_data[sp->bnst_data[i].dpnd_head])) == FALSE) */
/* 		    NEparaCheck(&(sp->bnst_data[sp->bnst_data[i].dpnd_head]), &(sp->bnst_data[i])); */
/* 		continue; */
/* 	    } */
/* 	} */

/* 	cp = check_feature(sp->bnst_data[i].f, "並結文節数"); */
/* 	if (cp) { */
/* 	    value = atoi(cp+strlen("並結文節数:")); */
/* 	    if (value > 1) { */
/* 		if (NEparaCheck(&(sp->bnst_data[i]), &(sp->bnst_data[sp->bnst_data[i].dpnd_head])) == FALSE) */
/* 		    NEparaCheck(&(sp->bnst_data[sp->bnst_data[i].dpnd_head]), &(sp->bnst_data[i])); */
/* 	    } */
/* 	} */
/*     } */
/* } */

/*====================================================================
                               END
====================================================================*/
