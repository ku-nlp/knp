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
int 	        CurHomoRuleSize = 0;

KoouRule	KoouRuleArray[KoouRule_MAX];
int		CurKoouRuleSize = 0;

DpndRule	DpndRuleArray[DpndRule_MAX];
int		CurDpndRuleSize = 0;

BnstRule 	ContRuleArray[ContRule_MAX];
int 	        ContRuleSize = 0;

void		*EtcRuleArray = NULL;
int		CurEtcRuleSize = 0;
int		ExistEtcRule = 0;

GeneralRuleType	*GeneralRuleArray = NULL;
int		GeneralRuleNum = 0;
int		GeneralRuleMax = 0;

extern int 	LineNo;
extern int 	LineNoForError;

/*==================================================================*/
void read_mrph_rule(char *file_name, MrphRule *rp, int *count, int max)
/*==================================================================*/
{
    FILE     *fp;
    CELL     *body_cell;

    /* 重複してルールファイルが指定されているとき */
    if (*count) {
	fprintf(stderr, ";; Mrph rule is duplicated (%s) !!\n", file_name);
	exit(1);
    }

    file_name = check_rule_filename(file_name);

    if ( (fp = fopen(file_name, "r")) == NULL ) {
	fprintf(stderr, ";; Cannot open file (%s) !!\n", file_name);
	exit(1);
    }

    if (OptDisplay == OPT_DEBUG) {
	fprintf(Outfp, "Reading %s ... ", file_name);
    }

    LineNo = 1;

    while (!s_feof(fp)) {
	LineNoForError = LineNo;

	body_cell = s_read(fp);
	store_regexpmrphs(&(rp->pre_pattern), car(body_cell));
	store_regexpmrphs(&(rp->self_pattern), car(cdr(body_cell)));
	store_regexpmrphs(&(rp->post_pattern), car(cdr(cdr(body_cell))));

	rp->f = NULL;
	list2feature(cdr(cdr(cdr(body_cell))), &(rp->f));

	if (++(*count) == max) {
	    fprintf(stderr, ";; Too many Rule for %s.\n", file_name);
	    exit(1);
	}
	
	rp++;
    }

    if (OptDisplay == OPT_DEBUG) {
	fputs("done.\n", Outfp);
    }

    free(file_name);    
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
    KoouRule  *rp = KoouRuleArray;

    /* 重複してルールファイルが指定されているとき */
    if (CurKoouRuleSize) {
	fprintf(stderr, ";; Koou rule is duplicated (%s) !!\n", file_name);
	exit(1);
    }

    file_name = check_rule_filename(file_name);

    if ( (fp = fopen(file_name, "r")) == NULL ) {
	fprintf(stderr, ";; Cannot open file (%s) !!\n", file_name);
	exit(1);
    }

    if (OptDisplay == OPT_DEBUG) {
	fprintf(Outfp, "Reading %s ... ", file_name);
    }

    free(file_name);

    LineNo = 1;

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
	    fprintf(stderr, ";; Too many KoouRule.");
	    exit(1);
	}

	rp++;
    }

    if (OptDisplay == OPT_DEBUG) {
	fputs("done.\n", Outfp);
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

    /* 重複してルールファイルが指定されているとき */
    if (CurHomoRuleSize) {
	fprintf(stderr, ";; Homo rule is duplicated (%s) !!\n", file_name);
	exit(1);
    }

    file_name = check_rule_filename(file_name);

    if ( (fp = fopen(file_name, "r")) == NULL ) {
	fprintf(stderr, ";; Cannot open file (%s) !!\n", file_name);
	exit(1);
    }

    if (OptDisplay == OPT_DEBUG) {
	fprintf(Outfp, "Reading %s ... ", file_name);
    }
    
    free(file_name);

    LineNo = 1;

    while (!s_feof(fp)) {
	LineNoForError = LineNo;

	body_cell = s_read(fp);

	/* 前の形態素列ルールの読込 */
	store_regexpmrphs(&(rp->pre_pattern), car(body_cell));

	/* homoの形態素列ルールの読込 */
	store_regexpmrphs(&(rp->pattern), car(cdr(body_cell)));

	list2feature(cdr(cdr(body_cell)), &(rp->f));

	if (++CurHomoRuleSize == HomoRule_MAX) {
	    fprintf(stderr, ";; Too many HomoRule.");
	    exit(1);
	}
	rp++;
    }

    if (OptDisplay == OPT_DEBUG) {
	fputs("done.\n", Outfp);
    }

    fclose(fp);
}


/*==================================================================*/
void read_bnst_rule(char *file_name, BnstRule *rp, int *count, int max)
/*==================================================================*/
{
    FILE     *fp;
    CELL     *body_cell;

    /* 重複してルールファイルが指定されているとき */
    if (*count) {
	fprintf(stderr, ";; Bnst rule is duplicated (%s) !!\n", file_name);
	exit(1);
    }

    file_name = check_rule_filename(file_name);
    
    if ( (fp = fopen(file_name, "r")) == NULL ) {
	fprintf(stderr, ";; Cannot open file (%s) !!\n", file_name);
	exit(1);
    }

    if (OptDisplay == OPT_DEBUG) {
	fprintf(Outfp, "Reading %s ... ", file_name);
    }

    free(file_name);

    LineNo = 1;

    while (!s_feof(fp)) {
	LineNoForError = LineNo;

	body_cell = s_read(fp);
	store_regexpbnsts(&(rp->pre_pattern), car(body_cell));
	store_regexpbnsts(&(rp->self_pattern), car(cdr(body_cell)));
	store_regexpbnsts(&(rp->post_pattern), car(cdr(cdr(body_cell))));

	rp->f = NULL;
	list2feature(cdr(cdr(cdr(body_cell))), &(rp->f));
	
	if (++(*count) == max) {
	    fprintf(stderr, ";; Too many BnstRule.");
	    exit(1);
	}
	rp++;
    }

    if (OptDisplay == OPT_DEBUG) {
	fputs("done.\n", Outfp);
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

    /* 重複してルールファイルが指定されているとき */
    if (CurDpndRuleSize) {
	fprintf(stderr, ";; Dpnd rule is duplicated (%s) !!\n", file_name);
	exit(1);
    }

    file_name = check_rule_filename(file_name);

    if ( (fp = fopen(file_name, "r")) == NULL ) {
	fprintf(stderr, ";; Cannot open file (%s) !!\n", file_name);
	exit(1);
    }

    if (OptDisplay == OPT_DEBUG) {
	fprintf(Outfp, "Reading %s ... ", file_name);
    }

    free(file_name);

    LineNo = 1;

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
		fprintf(stderr, ";; Too many Governors in a DpndRule.");
		exit(1);
	    }
	}
	rp->dpnd_type[i] = 0;	/* dpnd_type[i] != 0 がgovernorのある印 */

	list2feature_pattern(&(rp->barrier), car(cdr(cdr(body_cell))));
	rp->preference = atoi(_Atom(car(cdr(cdr(cdr(body_cell))))));

	/* 一意に決定するかどうか */
	if (!Null(car(cdr(cdr(cdr(cdr(body_cell)))))) && 
	    str_eq(_Atom(car(cdr(cdr(cdr(cdr(body_cell)))))), "U"))
	    rp->decide = 1;
	else
	    rp->decide = 0;

	if (++CurDpndRuleSize == DpndRule_MAX) {
	    fprintf(stderr, ";; Too many DpndRule.");
	    exit(1);
	}
	
	rp++;
    }

    if (OptDisplay == OPT_DEBUG) {
	fputs("done.\n", Outfp);
    }

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

    /* 各種タイプ, モードの伝播 */
    (GeneralRuleArray+GeneralRuleNum)->type = rule->type;
    (GeneralRuleArray+GeneralRuleNum)->mode = rule->mode;
    (GeneralRuleArray+GeneralRuleNum)->breakmode = rule->breakmode;
    (GeneralRuleArray+GeneralRuleNum)->direction = rule->direction;
    (GeneralRuleArray+GeneralRuleNum)->CurRuleSize = 0;

    if ((GeneralRuleArray+GeneralRuleNum)->type == MorphRuleType ||
	(GeneralRuleArray+GeneralRuleNum)->type == NeMorphRuleType) {
	(GeneralRuleArray+GeneralRuleNum)->RuleArray = 
	    (MrphRule *)malloc_data(sizeof(MrphRule)*GeneralRule_MAX, "read_general_rule");
	read_mrph_rule(rule->file, (MrphRule *)((GeneralRuleArray+GeneralRuleNum)->RuleArray), 
		       &((GeneralRuleArray+GeneralRuleNum)->CurRuleSize), GeneralRule_MAX);
    }
    else if ((GeneralRuleArray+GeneralRuleNum)->type == TagRuleType || 
	     (GeneralRuleArray+GeneralRuleNum)->type == BnstRuleType || 
	     (GeneralRuleArray+GeneralRuleNum)->type == AfterDpndBnstRuleType) {
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
