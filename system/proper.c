/*====================================================================

			     固有表現処理

                                               R.SASANO 05. 7. 31

    $Id$
====================================================================*/
#include "knp.h"

#define SIZE               2
#define NE_TAG_NUMBER      9
#define NE_POSITION_NUMBER 4
#define FEATIRE_MAX        1024
#define TAG_POSITION_MAX   20
#define HEAD               0
#define MIDDLE             1
#define TAIL               2
#define SINGLE             3
#define SIGX               10 /* SVMの結果を確率に近似するシグモイド関数の係数 */
#define NE_LEN_MAX         12 /* modify_resultにおけるチェックする最長の形態素数 */

DBM_FILE ne_db;

char *DBforNE;
char TagPosition[NE_MODEL_NUMBER][TAG_POSITION_MAX];
char *Tag_name[] = {
    "ORGANIZATION", "PERSON", "LOCATION", "ARTIFACT",
    "DATE", "TIME", "MONEY", "PERCENT", "OTHER"};
char *Position_name[] = {
    "head", "middle", "tail", "single"};
struct NE_MANAGER {
    char feature[FEATIRE_MAX];          /* 素性 */
    int notHEAD;                         /* head, singleにはならない場合1 */
    int NEresult;                       /* NEの解析結果 */
    double SVMscore[NE_MODEL_NUMBER];   /* 各タグ・ポジションとなる確率 */
    double max[NE_MODEL_NUMBER];        /* そこまでの最大スコア */
    int parent[NE_MODEL_NUMBER];        /* 最大スコアの経路 */
} NE_mgr[MRPH_MAX];

typedef struct ne_cache {
    char            *key;
    int	            ne_result[NE_MODEL_NUMBER + 1];
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
		 int get_chara(FEATURE *f, char *Goi)
/*==================================================================*/
{
    int i;
    char *Chara_name[] = {
	"漢字", "ひらがな", "かな漢字", "カタカナ", "記号", "英記号", "数字"};

    if (Goi && !strcmp(Goi, "・")) return 5; /* 記号 */
    for (i = 0; *Chara_name[i]; i++)
	if (check_feature(f, Chara_name[i]))
	    break;
    return i + 1;
}

/*==================================================================*/
	     char *get_pos(MRPH_DATA *mrph_data, int num)
/*==================================================================*/
{
    int i, j;
    char *ret, pos[SMALL_DATA_LEN];
    ret = (char *)malloc_data(SMALL_DATA_LEN, "get_pos");
    ret[0] = '\0'; /* 再帰的に代入するため */

    if (!check_feature(mrph_data->f, "品曖")) {
	if (mrph_data->Bunrui)
	    sprintf(ret, "%d%d%d10:1 ", mrph_data->Hinshi, mrph_data->Bunrui, num);
	else
	    sprintf(ret, "%d0%d10:1 ", mrph_data->Hinshi, num);
	return ret;
    }
	
    /* 品詞曖昧性のある場合 */
    for (i = 0; i < CLASSIFY_NO + 1; i++) {
	for (j = 0; j < CLASSIFY_NO + 1; j++) {
	    if (!Class[i][j].id) break;
	    sprintf(pos, "品曖-%s", Class[i][j].id);   
	    if (check_feature(mrph_data->f, pos))
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
       void register_ne_cache(char *key, int NEresult, int tag)
/*==================================================================*/
{
    /* NEの解析結果を登録する */
    /* ひとつの文章が終了するまで破棄しないのでメモリを必要とする可能性あり */
    /* NEresult = NE_MODEL_NUMBERの場合はNE全体の情報を保持 */
    /* その場合のみtagが与えられ、TAGの種類+1を記憶する */
    /* この場合、古いデータがあれば上書きされる */

    NE_CACHE *ncp;

    ncp = &(ne_cache[hash(key, strlen(key))]);
    while (ncp && ncp->key && strcmp(ncp->key, key)) {
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
    ncp->ne_result[NEresult] = tag;
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
	    return ncp->ne_result[NEresult];
	}
	ncp = ncp->next;
    }
    return 0;
}

/*==================================================================*/
		  char *get_cache(char *key, int num)
/*==================================================================*/
{
    int NEresult;
    NE_CACHE *ncp;
    char *ret;

    ret = (char *)malloc_data(SMALL_DATA_LEN, "get_cache");
    ret[0] = '\0'; /* 再帰的に代入するため */

    for (NEresult = 0; NEresult < NE_MODEL_NUMBER - 1; NEresult++) {
	if (check_ne_cache(key, NEresult))
	    sprintf(ret, "%s%d%d30:1 ", ret, NEresult + 1, num);
    }
    return ret;
}

/*==================================================================*/
	     char *get_tail(MRPH_DATA *mrph_data, int num)
/*==================================================================*/
{
    int i, j;
    char *ret;
    char *feature_name[] = {"人名末尾", "組織名末尾"};

    ret = (char *)malloc_data(SMALL_DATA_LEN, "get_tail");
    ret[0] = '\0'; /* 再帰的に代入するため */

     /* 文節後方に人名末尾、組織名末尾という語があるか */
    for (j = 1;; j++) {
	if (!(mrph_data + j)->f || 
	    check_feature((mrph_data + j)->f, "文節始") ||
	    check_feature((mrph_data + j)->f, "記号") ||
	    check_feature((mrph_data + j)->f, "括弧")) break;
	for (i = 0; i < 2; i++) {
	    if (check_feature((mrph_data + j)->f, feature_name[i]))
		sprintf(ret, "%d%d40:1 ", i + 3, num);
	}
    }

    /* 人名末尾、組織名末尾であるか */
    for (i = 0; i < 2; i++) {
	if (check_feature(mrph_data->f, feature_name[i]))
	    sprintf(ret, "%s%d%d40:1 ", ret, i + 1, num);
    }
    
    return ret;
}

/*==================================================================*/
		 void make_feature(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j, k;
    char *s[4];

    for (i = 0; i < sp->Mrph_num; i++) {
	
	/* 括弧始を除く記号は固有表現の先頭にはならないというルール  */
	NE_mgr[i].notHEAD = 0;
	if (get_chara(sp->mrph_data[i].f, sp->mrph_data[i].Goi) == 5 &&
	    !check_feature(sp->mrph_data[i].f, "括弧始"))
	    NE_mgr[i].notHEAD = 1;
	
	for (j = i - SIZE; j <= i + SIZE; j++) {
	    if (j < 0 || j >= sp->Mrph_num)
		continue;
	    
	    s[0] = db_get(ne_db, sp->mrph_data[j].Goi2);
	    s[1] = get_pos(sp->mrph_data + j, i - j + SIZE + 1);       /* 末尾空白*/
	    s[2] = get_cache(sp->mrph_data[j].Goi2, i - j + SIZE + 1); /* 末尾空白*/
	    s[3] = get_tail(sp->mrph_data + j, i - j + SIZE + 1);      /* 末尾空白*/
	    k = i - j + SIZE + 1;
	    sprintf(NE_mgr[i].feature, "%s%s%d:1 %s%d%d20:1 %s%s",
		    NE_mgr[i].feature, s[0] ? s[0] : "", k,
		    s[1], get_chara(sp->mrph_data[j].f, sp->mrph_data[j].Goi), k,
		    s[2], s[3]);
	    free(s[0]);
	    free(s[1]);
	    free(s[2]);
	    free(s[3]);
	}
    }       
}

/*==================================================================*/
	       void apply_svm_model(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j;
    
    for (i = 0; i < sp->Mrph_num; i++) {
	if (OptDisplay == OPT_DEBUG)
	    fprintf(stderr, "%d %s\t%s\n", i, sp->mrph_data[i].Goi2, NE_mgr[i].feature);

	for (j = 0; j < NE_MODEL_NUMBER; j++) {
#ifdef USE_SVM
	    if (NE_mgr[i].notHEAD && 
		j  != NE_MODEL_NUMBER - 1 &&
		(j % 4 == HEAD || j % 4 == SINGLE))
		NE_mgr[i].SVMscore[j] = 0; /* ヒューリスティックルール */
	    else
		NE_mgr[i].SVMscore[j] 
		    = 1/(1+exp(-svm_classify_for_NE(NE_mgr[i].feature, j) * SIGX));

	    if (OptDisplay == OPT_DEBUG) {
		fprintf(stderr, "%2d %f\t", j, NE_mgr[i].SVMscore[j]);
		if (j % 4 == SINGLE && j  != NE_MODEL_NUMBER - 2) fprintf(stderr, "\n");
		if (j  == NE_MODEL_NUMBER - 1) fprintf(stderr, "\n\n");
	    }
#endif
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
		NE_mgr[i].max[j] = NE_mgr[i].SVMscore[j];
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
void _additional_ne_analysis(SENTENCE_DATA *sp, MRPH_DATA *mp, int flag)
/*==================================================================*/
{
    /* 格フレームに組織、人名、主体が与えられている場合の処理 */
  
    /* flag = 1, 3 : 組織 */
    if (flag == 1) {
	if (NE_mgr[mp - sp->mrph_data].NEresult == NE_MODEL_NUMBER - 1 && /* OTHERsingle */
	    check_feature(mp->f, "品曖-アルファベット")) {
	    NE_mgr[mp - sp->mrph_data].NEresult = 
		ne_tagposition_to_code("ORGANIZATIONsingle");
	}
    }

    /* flag = 2, 3 : 人名 */
    if (flag == 2 || flag == 3) {
	if (check_feature(mp->f, "呼掛")) {
	    mp--;
	}
	if (NE_mgr[mp - sp->mrph_data].NEresult == NE_MODEL_NUMBER - 1 && /* OTHERsingle */
	    check_feature(mp->f, "品曖-カタカナ")) {
	    NE_mgr[mp - sp->mrph_data].NEresult = 
		ne_tagposition_to_code("PERSONsingle");
	}
    }
}

/*==================================================================*/
		void modify_result(SENTENCE_DATA *sp)
/*==================================================================*/
{
    /* SVMによる固有表現認識を修正する */
    /* 前文までで見つかった固有表現が認識されなかった場合は追加 */
    /* より短い固有表現と認識されている場合は上書きする */

    int len, start, end, position, ne_tag;
    char word[WORD_LEN_MAX];

    /* チェックする形態素列の形態素数 */
    for (len = sp->Mrph_num; len > 0; len--) {
	if (len > NE_LEN_MAX) continue;

	/* チェックする形態素列の開始位置(k=0のとき文先頭で開始) */
	for (start = 0; start + len < sp->Mrph_num; start++) {
	    end = start + len - 1;

	    /* 対象の形態素列を含む固有表現解析結果がある場合は修正しない */
	    if (NE_mgr[start].NEresult % 4 == TAIL ||
		NE_mgr[start].NEresult % 4 == MIDDLE ||
		NE_mgr[end].NEresult != NE_MODEL_NUMBER - 1 &&
		NE_mgr[end].NEresult % 4 == HEAD ||
		NE_mgr[end].NEresult % 4 == MIDDLE ||
		NE_mgr[start].NEresult != NE_MODEL_NUMBER - 1 &&
		NE_mgr[start].NEresult % 4 == HEAD &&
		NE_mgr[end].NEresult % 4 == TAIL) continue;

	    word[0] = '\0';
	    for (position = 0; position < len; position++) {
		strcat(word, (sp->mrph_data + start + position)->Goi2);
	    }

	    if ((ne_tag = check_ne_cache(word, NE_MODEL_NUMBER))) {
		ne_tag--; /* check_ne_cacheは1ずれている */
		/* NEresultに記録 */
		if (len == 1) {
		    NE_mgr[start].NEresult = ne_tag * 4 + 3; /* single */
		}
		else for (position = 0; position < len; position++) {
		    if (position == 0) {
			NE_mgr[start + position].NEresult = ne_tag * 4; /* head */
		    }
		    else if (position == len - 1) {
			NE_mgr[start + position].NEresult = ne_tag * 4 + 2; /* tail */
		    }
		    else {
			NE_mgr[start + position].NEresult = ne_tag * 4 + 1; /* middle */
		    }
		}
	    }
	}
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
	    void assign_ne_feature_mrph(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i;
    char cp[WORD_LEN_MAX];

    /* 形態素に付与 */
    for (i = 0; i < sp->Mrph_num; i++) {
	if (NE_mgr[i].NEresult == NE_MODEL_NUMBER -1) continue; /* OTHERの場合 */
	sprintf(cp, "NE:%s", ne_code_to_tagposition(NE_mgr[i].NEresult));
	assign_cfeature(&(sp->mrph_data[i].f), cp);
    }
}

/*==================================================================*/
	    void assign_ne_feature_tag(SENTENCE_DATA *sp)
/*==================================================================*/
{
    int i, j;
    char cp[WORD_LEN_MAX];

    /* タグに付与 */
    for (j = 0; j < sp->Tag_num; j++) { /* 同一タグの固有表現は一種類まで */
	for (i = 0; i < sp->tag_data[j].mrph_num; i++) {
	    if (check_feature((sp->tag_data[j].mrph_ptr + i)->f, "NE")) break;
	}
	/* 対象のタグに固有表現が無ければ次のタグへ */
	if (i == sp->tag_data[j].mrph_num) continue;

	/* ORGANIZATION、PERSONの場合は意味素として与える */
	if (!strcmp(Tag_name[get_mrph_ne((sp->tag_data[j].mrph_ptr + i)->f) / 4],
		    "ORGANIZATION")) {
	    assign_sm((BNST_DATA *)(sp->tag_data +j), "組織");
	}
	else if (!strcmp(Tag_name[get_mrph_ne((sp->tag_data[j].mrph_ptr + i)->f) / 4],
			 "PERSON")) {
	    assign_sm((BNST_DATA *)(sp->tag_data + j), "人");
	}
 
	sprintf(cp, "NE:(%s)",
		Tag_name[get_mrph_ne((sp->tag_data[j].mrph_ptr + i)->f) / 4]);
	while(1) {
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
		 void ne_analysis(SENTENCE_DATA *sp)
/*==================================================================*/
{
    init_NE_mgr();
    /* SVMを用いた固有表現解析 */
    make_feature(sp);
    apply_svm_model(sp);
    viterbi(sp);
    /* 前文までの解析結果を用いた修正 */
    if (OptEllipsis & OPT_COREFER) {
	modify_result(sp);
    }
    /* 結果を付与 */
    assign_ne_feature_mrph(sp);
}

/*==================================================================*/
	    void additional_ne_analysis(SENTENCE_DATA *sp)
/*==================================================================*/
{
    /* 格解析結果から、組織名と人名を付与する */
    /* ガ、ヲ、ニ格でかつその格フレームに<主体>が与えられている場合 */

    int i, j, num;
    CF_PRED_MGR *cpm_ptr;

    /* タグを後からチェック */
    for (j = sp->Tag_num - 1; j > 0 ; j--) {
	if (!(cpm_ptr = sp->tag_data[j].cpm_ptr)) continue;
	for (i = 0; i < cpm_ptr->cf.element_num; i++) {
	    num = cpm_ptr->cmm[0].result_lists_d[0].flag[i];
	    if (MatchPP(cpm_ptr->cmm[0].cf_ptr->pp[num][0], "ガ") ||
		MatchPP(cpm_ptr->cmm[0].cf_ptr->pp[num][0], "ヲ") ||
		MatchPP(cpm_ptr->cmm[0].cf_ptr->pp[num][0], "ニ")) {
		if (cf_match_element(cpm_ptr->cmm[0].cf_ptr->sm[num], "組織", TRUE) &&
		    cf_match_element(cpm_ptr->cmm[0].cf_ptr->sm[num], "人", TRUE)) {
		    _additional_ne_analysis(sp, (cpm_ptr->elem_b_ptr[i])->mrph_ptr, 3);
		} 
		else if (cf_match_element(cpm_ptr->cmm[0].cf_ptr->sm[num], "組織", TRUE)) {
		    _additional_ne_analysis(sp, (cpm_ptr->elem_b_ptr[i])->mrph_ptr, 1);
		}		    
		else if (cf_match_element(cpm_ptr->cmm[0].cf_ptr->sm[num], "人", TRUE)) {
		    _additional_ne_analysis(sp, (cpm_ptr->elem_b_ptr[i])->mrph_ptr, 2);
		}		    
		else if (cf_match_element(cpm_ptr->cmm[0].cf_ptr->sm[num], "主体", TRUE)) {
		    _additional_ne_analysis(sp, (cpm_ptr->elem_b_ptr[i])->mrph_ptr, 3);
		}
	    }
	}
    }
    /* 結果を付与 */
    assign_ne_feature_mrph(sp);      
    assign_ne_feature_tag(sp);      
}

/*==================================================================*/
  int ne_corefer(SENTENCE_DATA *sp, int i, char *anaphor, char *ne)
/*==================================================================*/
{
    /* 固有表現と共参照関係にあると判断された固有表現タグの付与されていない表現に */
    /* 固有表現タグを付与する */
   
    int start, end, ne_tag, j, k;
    char cp[WORD_LEN_MAX], word[WORD_LEN_MAX];

    for (ne_tag = 0; ne_tag < NE_TAG_NUMBER; ne_tag++) {
	/* どのタグであるかを"NE:("に続く4文字で判断する */
	if (!strncmp(ne + 4, Tag_name[ne_tag], 4)) break;
    }

    end = (sp->tag_data + i)->head_ptr - sp->mrph_data;  /* 接尾辞を含むものには未対応 */
    for (start = ((sp->tag_data + i)->b_ptr)->mrph_ptr - sp->mrph_data; 
	 start <= end; start++) {
	word[0] = '\0';
	for (k = start; k <= end; k++) {
	    strcat(word, (sp->mrph_data + k)->Goi2); /* 先行詞候補 */
	}
	if (!strcmp(word, anaphor)) break;
    }

    /* とりあえずORGANIZATIONの場合のみ、start > endとなることはないはず */
    if (strcmp(Tag_name[ne_tag], "ORGANIZATION") || start > end) return 0;

    /* タグに付与 */
    sprintf(cp, "NE:(%s)%s", Tag_name[ne_tag], anaphor);
    assign_cfeature(&(sp->tag_data[i].f), cp);
    
    /* 形態素に付与、NEresultに記録 */
    if ((j = start) == end) {
	sprintf(cp, "NE:%ssingle", Tag_name[ne_tag]);
	assign_cfeature(&(sp->mrph_data[j].f), cp);
	NE_mgr[j].NEresult = ne_tag * 4 + 3; /* single */
    }
    else for (j = start; j <= end; j++) {
	if (j == start) {
	    sprintf(cp, "NE:%shead", Tag_name[ne_tag]);
	    assign_cfeature(&(sp->mrph_data[j].f), cp);
	    NE_mgr[j].NEresult = ne_tag * 4; /* head */
	}
	else if (j == end) {
	    sprintf(cp, "NE:%stail", Tag_name[ne_tag]);
	    assign_cfeature(&(sp->mrph_data[j].f), cp);
	    NE_mgr[j].NEresult = ne_tag * 4 + 2; /* tail */
	}
	else {
	    sprintf(cp, "NE:%smiddle", Tag_name[ne_tag]);
	    assign_cfeature(&(sp->mrph_data[j].f), cp);
	    NE_mgr[j].NEresult = ne_tag * 4 + 1; /* middle */
	}
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
	register_ne_cache(sp->mrph_data[i].Goi2, NE_mgr[i].NEresult, 1);
    }

    /* 固有表現と認識された文字列を記憶 */
    for (i = 0; i < sp->Tag_num; i++) {   
	if ((cp = check_feature(sp->tag_data[i].f, "NE"))) {
	    for (ne_tag = 0; ne_tag < NE_TAG_NUMBER; ne_tag++) {
		/* どのタグであるかを"NE:("に続く4文字で判断する */
		if (!strncmp(cp + 4, Tag_name[ne_tag], 4)) break;
	    }   
	    while (strncmp(cp, ")", 1)) cp++;
	    register_ne_cache(cp + 1, NE_MODEL_NUMBER, ne_tag + 1);
	} 
    }
}
	  
/*====================================================================
                               END
====================================================================*/
