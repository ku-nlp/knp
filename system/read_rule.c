/*====================================================================

			  規則データ読み込み

                                               K.Yamagami
                                               S.Kurohashi 93. 5.31

    $Id$
====================================================================*/
#include "knp.h"

extern REGEXPMRPHS *regexpmrphs_alloc();

/* global variable declaration */
HomoRule	HomoRuleArray[HomoRule_MAX];
int 	        CurHomoRuleSize;

KoouRule	KoouRuleArray[KoouRule_MAX];
int		CurKoouRuleSize;

DpndRule	DpndRuleArray[DpndRule_MAX];
int		CurDpndRuleSize;

BnstRule 	ContRuleArray[ContRule_MAX];
int 	        ContRuleSize;

MrphRule	NERuleArray[NERule_MAX];
int 	        CurNERuleSize;

MrphRule	CNpreRuleArray[CNRule_MAX];
int 	        CurCNpreRuleSize;

MrphRule	CNRuleArray[CNRule_MAX];
int 	        CurCNRuleSize;

MrphRule	CNauxRuleArray[CNRule_MAX];
int 	        CurCNauxRuleSize;

MrphRule        HelpsysArray[Helpsys_MAX];
int             CurHelpsysSize;

void		*EtcRuleArray = NULL;
int		CurEtcRuleSize;
int		ExistEtcRule = 0;

GeneralRuleType	*GeneralRuleArray = NULL;
int		GeneralRuleNum = 0;
int		GeneralRuleMax = 0;

DicForRule	*DicForRuleVArray;
int		CurDicForRuleVSize;
DicForRule	*DicForRulePArray;
int		CurDicForRulePSize;

extern int 	LineNo;
extern int 	LineNoForError;

DBM_FILE dic_for_rulev_db;
DBM_FILE dic_for_rulep_db;
int DicForRuleDBExist = FALSE;

/*==================================================================*/
void read_mrph_rule(char *file_name, MrphRule *rp, int *count, int max)
/*==================================================================*/
{
    FILE     *fp;
    CELL     *body_cell;

    file_name = (char *)check_filename(file_name);
    
    if ( (fp = fopen(file_name, "r")) == NULL ) {
	fprintf(stderr, "Cannot open file (%s) !!\n", file_name);
	exit(1);
    }

    free(file_name);

    LineNo = 1;
    *count = 0;

    while (!s_feof(fp)) {
	LineNoForError = LineNo;

	body_cell = s_read(fp);
	store_regexpmrphs(&(rp->pre_pattern), car(body_cell));
	store_regexpmrphs(&(rp->self_pattern), car(cdr(body_cell)));
	store_regexpmrphs(&(rp->post_pattern), car(cdr(cdr(body_cell))));

	list2feature(cdr(cdr(cdr(body_cell))), &(rp->f));

	if (++(*count) == max) {
	    fprintf(stderr, "Too many Rule for %s.\n", file_name);
	    exit(1);
	}
	
	rp++;
    }
    
    fclose(fp);
}

/*==================================================================*/
		     int case2num(char *cp)
/*==================================================================*/
{
    int i;
    
    for (i = 0; *Case_name[i]; i++)
	if (str_eq(cp, Case_name[i]))
	    return i;
    return -1;
}

/*==================================================================*/
		void read_koou_rule(char *file_name)
/*==================================================================*/
{
    FILE     *fp;
    CELL     *body_cell;
    REGEXPMRPHS *pattern_bp;
    KoouRule  *rp = KoouRuleArray;

    file_name = (char *)check_filename(file_name);
    
    if ( (fp = fopen(file_name, "r")) == NULL ) {
	fprintf(stderr, "Cannot open file (%s) !!\n", file_name);
	exit(1);
    }

    free(file_name);

    LineNo = 1;
    CurKoouRuleSize = 0;

    while (!s_feof(fp)) {
	LineNoForError = LineNo;

	body_cell = s_read(fp);
	store_regexpmrphs(&(rp->start_pattern), car(body_cell));

	body_cell = cdr(body_cell);
	store_regexpmrphs(&(rp->end_pattern), car(body_cell));

	body_cell = cdr(body_cell);
	if (!Null(car(body_cell))) {
	    store_regexpmrphs(&(rp->uke_pattern), car(body_cell));
	} else {
	    rp->uke_pattern = NULL;
	}

	if (++CurKoouRuleSize == KoouRule_MAX) {
	    fprintf(stderr, "Too many KoouRule.");
	    exit(1);
	}

	rp++;
    }
    
    fclose(fp);
}

/*==================================================================*/
		 void read_homo_rule(char *file_name)
/*==================================================================*/
{
    FILE     *fp;
    CELL     *body_cell;
    HomoRule  *rp = HomoRuleArray;
    FEATURE   **fpp;	

    file_name = (char *)check_filename(file_name);
    
    if ( (fp = fopen(file_name, "r")) == NULL ) {
	fprintf(stderr, "Cannot open file (%s) !!\n", file_name);
	exit(1);
    }

    free(file_name);
    
    LineNo = 1;
    CurHomoRuleSize = 0;

    while (!s_feof(fp)) {
	LineNoForError = LineNo;

	body_cell = s_read(fp);
	/* 形態素列ルールの読込 */
	store_regexpmrphs(&(rp->pattern), car(body_cell));

	list2feature(cdr(body_cell), &(rp->f));

	
	if (++CurHomoRuleSize == HomoRule_MAX) {
	    fprintf(stderr, "Too many HomoRule.");
	    exit(1);
	}
	rp++;
    }
    
    fclose(fp);
}


/*==================================================================*/
void read_bnst_rule(char *file_name, BnstRule *rp, int *count, int max)
/*==================================================================*/
{
    FILE     *fp;
    CELL     *body_cell;
    FEATURE   **fpp;	

    file_name = (char *)check_filename(file_name);
    
    if ( (fp = fopen(file_name, "r")) == NULL ) {
	fprintf(stderr, "Cannot open file (%s) !!\n", file_name);
	exit(1);
    }

    free(file_name);

    LineNo = 1;
    *count = 0;

    while (!s_feof(fp)) {
	LineNoForError = LineNo;

	body_cell = s_read(fp);
	store_regexpbnsts(&(rp->pre_pattern), car(body_cell));
	store_regexpbnsts(&(rp->self_pattern), car(cdr(body_cell)));
	store_regexpbnsts(&(rp->post_pattern), car(cdr(cdr(body_cell))));

	list2feature(cdr(cdr(cdr(body_cell))), &(rp->f));
	
	if (++(*count) == max) {
	    fprintf(stderr, "Too many BnstRule.");
	    exit(1);
	}
	rp++;
    }
    
    fclose(fp);
}

/*==================================================================*/
	      void read_dpnd_rule(char *file_name)
/*==================================================================*/
{
    int		i;
    FILE	*fp;
    CELL	*body_cell, *loop_cell;
    DpndRule	*rp = DpndRuleArray;
    FEATURE	**fpp;	

    file_name = (char *)check_filename(file_name);

    if ( (fp = fopen(file_name, "r")) == NULL ) {
	fprintf(stderr, "Cannot open file (%s) !!\n", file_name);
	exit(1);
    }

    free(file_name);

    LineNo = 1;
    CurDpndRuleSize = 0;

    while (!s_feof(fp)) {
	LineNoForError = LineNo;

	body_cell = s_read(fp);

	list2feature_pattern(&(rp->dependant), car(body_cell));	
	loop_cell = car(cdr(body_cell));
	i = 0;
	while (!Null(car(loop_cell))) {
	    list2feature_pattern(&(rp->governor[i]), car(car(loop_cell)));
	    rp->dpnd_type[i] = *(_Atom(car(cdr(car(loop_cell)))));
	    loop_cell = cdr(loop_cell);
	    if (++i == DpndRule_G_MAX) {
		fprintf(stderr, "Too many Governors in a DpndRule.");
		exit(1);
	    }
	}
	rp->dpnd_type[i] = 0;	/* dpnd_type[i] != 0 がgovernorのある印 */

	list2feature_pattern(&(rp->barrier), car(cdr(cdr(body_cell))));
	rp->preference = atoi(_Atom(car(cdr(cdr(cdr(body_cell))))));

	if (++CurDpndRuleSize == DpndRule_MAX) {
	    fprintf(stderr, "Too many DpndRule.");
	    exit(1);
	}
	
	rp++;
    }
    
    fclose(fp);
}

/*==================================================================*/
	      void read_dic_for_rule(char *file_name, DicForRule **Array, int *Num)
/*==================================================================*/
{
    FILE *fp;
    char buffer[DATA_LEN];
    char key[DATA_LEN], value[DATA_LEN];
    int i, exist_flag = 0, max = 0;

    buffer[DATA_LEN-1] = '\n';
    key[DATA_LEN-1] = '\n';
    value[DATA_LEN-1] = '\n';

    /* ファイルオープン */
    if (!(fp = fopen(file_name, "r"))) {
	fprintf(stderr, "Cannot open file (%s) !!\n", file_name);
	exit(1);
    }

    /* 読み込み */
    while (fgets(buffer, DATA_LEN, fp)) {
	if (buffer[DATA_LEN-1] != '\n') {
	    fprintf(stderr, "Buffer overflow (read_dic_for_rule)\n");
	    exit(1);
	}

	/* 一行読み込み */
	sscanf(buffer, "%s %[^\n]\n", key, value);

	if (key[DATA_LEN-1] != '\n') {
	    fprintf(stderr, "Buffer overflow (read_dic_for_rule)\n");
	    exit(1);
	}
	if (value[DATA_LEN-1] != '\n') {
	    fprintf(stderr, "Buffer overflow (read_dic_for_rule)\n");
	    exit(1);
	}

	/* メモリ確保 */
	if (*Num >= max) {
	    max += ALLOCATION_STEP;
	    if (!(*Array = (DicForRule *)realloc(*Array, sizeof(DicForRule)*max))) {
		fprintf(stderr, "Memory allocation Error.\n");
		exit(1);
	    }
	}

	/* キーが存在するときは feature だけ追加 */
	for (i = 0; i < *Num; i++) {
	    if (!strcmp((*Array)[i].key, key)) {
		assign_cfeature(&((*Array)[i].f), value);
		exist_flag = 1;
		break;
	    }
	}

	/* キーが存在しないとき */
	if (!exist_flag) {
	    (*Array)[*Num].key = strdup(key);
	    (*Array)[*Num].f = NULL;
	    assign_cfeature(&((*Array)[*Num].f), value);
	    (*Num)++;
	}
	else
	    exist_flag = 0;
    }
    fclose(fp);
}

/*==================================================================*/
			  int init_dic_for_rule()
/*==================================================================*/
{
    if ((dic_for_rulev_db = DBM_open(RULEV_DB_NAME, O_RDONLY, 0)) == NULL || 
	(dic_for_rulep_db = DBM_open(RULEP_DB_NAME, O_RDONLY, 0)) == NULL) {
	read_dic_for_rule(RULEV_DIC_FILE, &DicForRuleVArray, &CurDicForRuleVSize);
	read_dic_for_rule(RULEP_DIC_FILE, &DicForRulePArray, &CurDicForRulePSize);
    } 
    else
	DicForRuleDBExist = TRUE;
    return DicForRuleDBExist;
}

/*==================================================================*/
		      void close_dic_for_rule()
/*==================================================================*/
{
    if (DicForRuleDBExist == TRUE) {
	DBM_close(dic_for_rulev_db);
	DBM_close(dic_for_rulep_db);
    }
}

/*==================================================================*/
		      char *get_rulev(char *cp)
/*==================================================================*/
{
    return db_get(dic_for_rulev_db, cp);
}

/*==================================================================*/
		      char *get_rulep(char *cp)
/*==================================================================*/
{
    return db_get(dic_for_rulep_db, cp);
}

/*==================================================================*/
void read_NE_rule(char *file_name, MrphRule *rp, int *count, int max)
/*==================================================================*/
{
    FILE     *fp;
    CELL     *body_cell;

    file_name = (char *)check_filename(file_name);
    
    if ( (fp = fopen(file_name, "r")) == NULL ) {
	fprintf(stderr, "Cannot open file (%s) !!\n", file_name);
	exit(1);
    }
    
    LineNo = 1;
    *count = 0;
    
    while (!s_feof(fp)) {
	LineNoForError = LineNo;

	body_cell = s_read(fp);
	store_regexpmrphs(&(rp->pre_pattern), car(body_cell));
	store_regexpmrphs(&(rp->self_pattern), car(cdr(body_cell)));
	store_regexpmrphs(&(rp->post_pattern), car(cdr(cdr(body_cell))));

	list2feature(cdr(cdr(cdr(body_cell))), &(rp->f));

	if (++(*count) == max) {
	    fprintf(stderr, "Too many Rule for %s.\n", file_name);
	    exit(1);
	}
	
	rp++;
    }

    free(file_name);
    fclose(fp);
}

/*==================================================================*/
		     void init_etc_rule(int flag)
/*==================================================================*/
{
    if (ExistEtcRule)
	usage();
    ExistEtcRule = flag;
    if (flag == IsMrphRule || flag == IsMrph2Rule) {
	EtcRuleArray = (MrphRule *)malloc_data(sizeof(MrphRule)*EtcRule_MAX, "init_etc_rule");
    }
    else if (flag == IsBnstRule) {
	EtcRuleArray = (BnstRule *)malloc_data(sizeof(BnstRule)*EtcRule_MAX, "init_etc_rule");
    }
}

/*==================================================================*/
       void assign_etc_feature(void *r_ptr, int size, int mode)
/*==================================================================*/
{
    if (ExistEtcRule == IsMrphRule) {
	assign_mrph_feature((MrphRule *)r_ptr, size);
    }
    else if (ExistEtcRule == IsMrph2Rule) {
	assign_mrph_feature_new((MrphRule *)r_ptr, size, mrph_data, Mrph_num, 
				RLOOP_MRM, RLOOP_BREAK_JUMP);
    }
    else if (ExistEtcRule == IsBnstRule) {
	assign_bnst_feature((BnstRule *)r_ptr, size, mode);
    }
}

/*==================================================================*/
  void read_etc_rule(char *file_name, void *rp, int *count, int max)
/*==================================================================*/
{
    if (ExistEtcRule == IsMrphRule || ExistEtcRule == IsMrph2Rule) {
	read_mrph_rule(file_name, (MrphRule *)rp, count, max);
    }
    else if (ExistEtcRule == IsBnstRule) {
	read_bnst_rule(file_name, (BnstRule *)rp, count, max);
    }
}

/*==================================================================*/
	       void read_general_rule(RuleVector *rule)
/*==================================================================*/
{
    if (GeneralRuleNum >= GeneralRuleMax) {
	GeneralRuleMax += RuleIncrementStep;
	GeneralRuleArray = (GeneralRuleType *)realloc(GeneralRuleArray, 
						      sizeof(GeneralRuleType)*GeneralRuleMax);
    }
    (GeneralRuleArray+GeneralRuleNum)->type = rule->type;
    (GeneralRuleArray+GeneralRuleNum)->mode = rule->mode;
    (GeneralRuleArray+GeneralRuleNum)->breakmode = rule->breakmode;
    (GeneralRuleArray+GeneralRuleNum)->direction = rule->direction;

    if ((GeneralRuleArray+GeneralRuleNum)->type == MorphRuleType) {
	(GeneralRuleArray+GeneralRuleNum)->RuleArray = 
	    (MrphRule *)malloc_data(sizeof(MrphRule)*GeneralRule_MAX, "read_general_rule");
	read_mrph_rule(rule->file, (MrphRule *)((GeneralRuleArray+GeneralRuleNum)->RuleArray), 
		       &((GeneralRuleArray+GeneralRuleNum)->CurRuleSize), GeneralRule_MAX);
    }
    else if ((GeneralRuleArray+GeneralRuleNum)->type == BnstRuleType) {
	(GeneralRuleArray+GeneralRuleNum)->RuleArray = 
	    (BnstRule *)malloc_data(sizeof(BnstRule)*GeneralRule_MAX, "read_general_rule");
	read_bnst_rule(rule->file, (BnstRule *)((GeneralRuleArray+GeneralRuleNum)->RuleArray), 
		       &((GeneralRuleArray+GeneralRuleNum)->CurRuleSize), GeneralRule_MAX);
    }

    GeneralRuleNum++;
}

/*====================================================================
                               END
====================================================================*/
