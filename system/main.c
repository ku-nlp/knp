/*====================================================================

		     KNP (Kurohashi-Nagao Parser)

    $Id$
====================================================================*/
#include "knp.h"

MRPH_DATA 	mrph_data[MRPH_MAX];		/* 形態素データ */
BNST_DATA 	bnst_data[BNST_MAX];		/* 文節データ */
PARA_DATA 	para_data[PARA_MAX]; 		/* 並列データ */
PARA_MANAGER	para_manager[PARA_MAX];		/* 並列管理データ */
TOTAL_MGR	Best_mgr;			/* 依存・格解析管理データ */

int 		Mrph_num;			/* 形態素数 */
int	 	Mrph_all_num;			/* 全形態素数 */
int 		Bnst_num;			/* 文節数 */
int		New_Bnst_num;			/* 追加文節数 */
int 		Para_num;			/* 並列構造数 */
int 		Para_M_num;			/* 並列管理マネージャー数 */
int 		Revised_para_num;			

int 		Sen_num;			/* 文カウント 1〜 */
char		Comment[256];			/* コメント行 */
char		PM_Memo[256];			/* パターンマッチ結果 */

char  		key_str[DBM_KEY_MAX], cont_str[DBM_CON_MAX];
datum 		key, content;			/* ハッシュデータベース用 */

int		match_matrix[BNST_MAX][BNST_MAX];
int		path_matrix[BNST_MAX][BNST_MAX];
int		restrict_matrix[BNST_MAX][BNST_MAX];
int		restrict_table[BNST_MAX];
int 		Dpnd_matrix[BNST_MAX][BNST_MAX]; /* 係り可能性 0, D, P, A */
int 		Quote_matrix[BNST_MAX][BNST_MAX];/* 括弧マスク 0, 1 */
int 		Mask_matrix[BNST_MAX][BNST_MAX]; /* 並列マスク
						    0:係り受け禁止
						    1:係り受けOK
						    2:並列のhead間,
						    3:並列のgapとhead間 */
char		G_Feature[100][64];		/* FEATUREの変数格納 */

int 		OptAnalysis;
int 		OptExpress;
int 		OptDisplay;
int		OptExpandP;
int		OptInhibit;
int		OptCheck;
int		OptNE;
char		OptIgnoreChar;

char *Case_name[] = {
    		"ガ格", "ヲ格", "ニ格", "デ格", "カラ格", 
		"ト格", "ヨリ格", "ヘ格", "マデ格", "ノ格",
		"ガ２", ""};

char 		*ProgName;
extern int 	Case_frame_num;
extern CLASS    Class[CLASSIFY_NO + 1][CLASSIFY_NO + 1];
extern TYPE     Type[TYPE_NO];
extern FORM     Form[TYPE_NO][FORM_NO];

#include "extern.h"

extern void read_bnst_rule(char *file_neme, BnstRule *rp, 
			   int *count, int max);

char *ClauseDBname = NULL;
char *ClauseCDBname = NULL;
char *CasePredicateDBname = NULL;
char *OptionalCaseDBname = NULL;

jmp_buf timeout;

/*==================================================================*/
	       void usage()
/*==================================================================*/
{
    fprintf(stderr, "Usage: knp "
	    "[-case|dpnd|bnst|-disc] [-tree|sexp] [-normal|detail|debug] [-expand]\n");
    exit(1);    
}

/*==================================================================*/
	       void option_proc(int argc, char **argv)
/*==================================================================*/
{
    /* 引数処理 */

    OptAnalysis = OPT_DPND;
    OptExpress = OPT_TREE;
    OptDisplay = OPT_NORMAL;
    OptExpandP = FALSE;
    /* デフォルトで禁止するオプション */
    OptInhibit = OPT_INHIBIT_CLAUSE | OPT_INHIBIT_CASE_PREDICATE | OPT_INHIBIT_BARRIER | OPT_INHIBIT_OPTIONAL_CASE | OPT_INHIBIT_C_CLAUSE;
    OptCheck = FALSE;
    OptNE = OPT_NORMAL;
    OptIgnoreChar = (char)NULL;

    while ((--argc > 0) && ((*++argv)[0] == '-')) {
	if (str_eq(argv[0], "-case"))        OptAnalysis = OPT_CASE;
	else if (str_eq(argv[0], "-case2"))  OptAnalysis = OPT_CASE2;
	else if (str_eq(argv[0], "-dpnd"))   OptAnalysis = OPT_DPND;
	else if (str_eq(argv[0], "-bnst"))   OptAnalysis = OPT_BNST;
	else if (str_eq(argv[0], "-disc"))   OptAnalysis = OPT_DISC;
	else if (str_eq(argv[0], "-tree"))   OptExpress = OPT_TREE;
	else if (str_eq(argv[0], "-treef"))  OptExpress = OPT_TREEF;
	else if (str_eq(argv[0], "-sexp"))   OptExpress = OPT_SEXP;
	else if (str_eq(argv[0], "-tab"))    OptExpress = OPT_TAB;
	else if (str_eq(argv[0], "-normal")) OptDisplay = OPT_NORMAL;
	else if (str_eq(argv[0], "-detail")) OptDisplay = OPT_DETAIL;
	else if (str_eq(argv[0], "-debug"))  OptDisplay = OPT_DEBUG;
	else if (str_eq(argv[0], "-expand")) OptExpandP = TRUE;
	else if (str_eq(argv[0], "-check"))  OptCheck = TRUE;
	else if (str_eq(argv[0], "-nenosm")) OptNE = OPT_NENOSM;
	else if (str_eq(argv[0], "-ne"))     OptNE = OPT_NE;
	else if (str_eq(argv[0], "-cc"))     OptInhibit &= ~OPT_INHIBIT_CLAUSE;
	else if (str_eq(argv[0], "-ck"))     OptInhibit &= ~OPT_INHIBIT_CASE_PREDICATE;
	else if (str_eq(argv[0], "-cb"))     OptInhibit &= ~OPT_INHIBIT_BARRIER;
	else if (str_eq(argv[0], "-co"))     OptInhibit &= ~OPT_INHIBIT_OPTIONAL_CASE;
	else if (str_eq(argv[0], "-i")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    OptIgnoreChar = *argv[0];
	}
	else if (str_eq(argv[0], "-cdb")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    ClauseDBname = argv[0];
	    OptInhibit &= ~OPT_INHIBIT_CLAUSE;
	}
	else if (str_eq(argv[0], "-ccdb")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    ClauseCDBname = argv[0];
	    OptInhibit &= ~OPT_INHIBIT_C_CLAUSE;
	}
	else if (str_eq(argv[0], "-kdb")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    if (CasePredicateDBname) {
		if (strcmp(CasePredicateDBname, argv[0]))
		    usage();
	    }
	    else
		CasePredicateDBname = argv[0];
	    OptInhibit &= ~OPT_INHIBIT_CASE_PREDICATE;
	}
	else if (str_eq(argv[0], "-bdb")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    if (CasePredicateDBname) {
		if (strcmp(CasePredicateDBname, argv[0]))
		    usage();
	    }
	    else
		CasePredicateDBname = argv[0];
	    OptInhibit &= ~OPT_INHIBIT_BARRIER;
	}
	else if (str_eq(argv[0], "-odb")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    OptionalCaseDBname = argv[0];
	    OptInhibit &= ~OPT_INHIBIT_OPTIONAL_CASE;
	}
	else {
	    usage();
	}
    }
    if (argc != 0) {
	usage();
    }
}

/*==================================================================*/
			void init_juman(void)
/*==================================================================*/
{
    int i, j; 

    set_jumanrc_fileptr(NULL, TRUE);
    set_jumangram_dirname();
    /*read_jumanpathrc();*/			/* JUMAN辞書のパス */
    grammar(NULL);				/* 文法辞書 */
    katuyou(NULL);				/* 活用辞書 */

    /* 品詞 活用の出力

    for (i = 1; Class[i][0].id; i++) {
	printf("%s", Class[i][0].id);
	for (j = 1; Class[i][j].id; j++) {
	    printf(" %s", Class[i][j].id);
	}
	printf("\n");
    }
    for (i = 1; Type[i].name; i++) {
	printf("%s", Type[i].name);
	for (j = 1; Form[i][j].name; j++) {
	    printf(" %s(%s)", Form[i][j].name, Form[i][j].gobi);
	}
	printf("\n");
    }
    exit(1);
    */
}

/*==================================================================*/
			void read_rules(void)
/*==================================================================*/
{
    read_homo_rule(HOMO_FILE);			/* 同形異義語ルール */
    read_mrph_rule(MRPH_FILE);			/* 形態素ルール */
    read_bnst_rule(BNST1_FILE, BnstRule1Array, 
		   &CurBnstRule1Size, BnstRule_MAX);
    						/* 文節の一般的ルール */
    read_bnst_rule(UKE_FILE, UkeRuleArray, 
		   &CurUkeRuleSize, UkeRule_MAX);
    						/* 文節の受けルール */
    read_bnst_rule(BNST2_FILE, BnstRule2Array, 
		   &CurBnstRule2Size, BnstRule_MAX);
    						/* 文節の例外ルール */
    read_bnst_rule(KAKARI_FILE, KakariRuleArray, 
		   &CurKakariRuleSize, KakariRule_MAX);
    						/* 文節の係りルール */
    read_bnst_rule(BNST3_FILE, BnstRule3Array, 
		   &CurBnstRule3Size, BnstRule_MAX);
    						/* 文節の例外ルール */
    read_dpnd_rule(DPND_FILE);			/* 係り受けルール */
    read_koou_rule(KOOU_FILE);			/* 呼応表現ルール */
    if (OptNE != OPT_NORMAL) {
	read_NE_rule(NE_FILE);			/* 固有名詞ルール */
	read_CN_rule(CN_FILE);			/* 固有名詞ルール */
    }

    read_bnst_rule(CONT_FILE, ContRuleArray,	/* 文脈処理のルール */
		   &ContRuleSize, ContRule_MAX);
}

/*==================================================================*/
			void make_bnst_data()
/*==================================================================*/
{
    int i;

    for (i = 0; i < BNST_MAX; i++) {
	bnst_data[i].Jiritu_Go = (char *)malloc_data(WORD_LEN_MAX, "Jiritsu_Go");
	bnst_data[i].Jiritu_Go_Size = WORD_LEN_MAX;
    }
}

/*==================================================================*/
	       static void timeout_function(int signal)
/*==================================================================*/
{
    /* signal(SIGALRM, SIG_IGN);
       sigignore(SIGALRM); */
    longjmp(timeout, 1);
}

/*==================================================================*/
		   void main(int argc, char **argv)
/*==================================================================*/
{
    int i, j, flag;
    int relation_error, d_struct_error;

    option_proc(argc, argv);

    /* 初期化 */

    init_juman();	/* JUMAN関係 */
    init_ipal();	/* 格フレームオープン */
    init_bgh();		/* シソーラスオープン */
    init_sm();		/* 意味素オープン */
    init_sm2code();	/* 意味素コードオープン */
    init_scase();	/* 表層格辞書オープン */
    if (OptNE != OPT_NORMAL)
	init_proper();	/* 固有名詞解析辞書オープン */
    if (!(OptInhibit & OPT_INHIBIT_CLAUSE))
	init_clause();
    if (!((OptInhibit & OPT_INHIBIT_CASE_PREDICATE) && (OptInhibit & OPT_INHIBIT_BARRIER)))
	init_case_pred();
    if (!(OptInhibit & OPT_INHIBIT_OPTIONAL_CASE))
	init_optional_case();
    read_rules();	/* ルール読み込み */
    /* init_dic_for_rule(); */

    /* メイン・ルーチン */

    Mrph_num = 0;
    Bnst_num = 0;
    New_Bnst_num = 0;
    Sen_num = 0;

    make_bnst_data();

    while ( 1 ) {

	if (setjmp(timeout))
	    fprintf(stderr, "Parse timeout.\n");

	for (i = 0; i < Mrph_num; i++) clear_feature(&(mrph_data[i].f));
	for (i = 0; i < Bnst_num; i++) clear_feature(&(bnst_data[i].f));
	for (i = Bnst_num; i < Bnst_num + New_Bnst_num; i++)
	    bnst_data[i].f = NULL;

	if ((flag = read_mrph(stdin)) == EOF) break;

	Sen_num++;

	if (flag == FALSE) continue;

	/* 形態素への情報付与 --> 文節 */

	assign_cfeature(&(mrph_data[0].f), "文頭");
	assign_cfeature(&(mrph_data[Mrph_num-1].f), "文末");
	assign_mrph_prop();

	if (OptAnalysis == OPT_PM) {
	    if (make_bunsetsu_pm() == FALSE) continue;
	} else {
	    if (make_bunsetsu() == FALSE) continue;
	}

	if (OptAnalysis == OPT_BNST) {
	    print_mrphs(0); continue;	/* 文節化だけの場合 */
	}	
	if (OptDisplay == OPT_DETAIL || OptDisplay == OPT_DEBUG)
	    print_mrphs(0);
	

	/* 文節への情報付与 */

	assign_cfeature(&(bnst_data[0].f), "文頭");
	assign_cfeature(&(bnst_data[Bnst_num-1].f), "文末");

	assign_bnst_feature(BnstRule1Array, CurBnstRule1Size, LOOP_ALL);
						/* 一般的FEATURE */
	assign_bnst_feature(UkeRuleArray, CurUkeRuleSize, LOOP_BREAK);
						/* 受けのFEATURE */
	assign_bnst_feature(BnstRule2Array, CurBnstRule2Size, LOOP_ALL);
						/* 例外的FEATURE */

	assign_bnst_feature(KakariRuleArray, CurKakariRuleSize, LOOP_BREAK);
						/* 係りのFEATURE */
	assign_bnst_feature(BnstRule3Array, CurBnstRule3Size, LOOP_ALL);
						/* 例外的FEATURE */	


	if (OptAnalysis == OPT_PM) {		/* 解析済みデータのPM */
	    print_result();
	    fflush(stdout);
	    continue;
	}

	assign_dpnd_rule();			/* 係り受け規則 */

	Case_frame_num = 0;
	set_pred_caseframe();			/* 用言の格フレーム */

	for (i = 0; i < Bnst_num; i++) {
	  get_bgh_code(bnst_data+i);		/* シソーラス */
	  get_sm_code(bnst_data+i);		/* 意味素 */
	}

	if (OptDisplay == OPT_DETAIL || OptDisplay == OPT_DEBUG)
	    check_bnst();

	/* continue; 文節のみのチェックの場合 */

	/*
	if (Bnst_num > 30) {
	    fprintf(stdout, "Too long sentence (%d bnst)\n", Bnst_num);
	    continue;
	}
	*/

	/* 本格的解析 */

	calc_dpnd_matrix();			/* 依存可能性計算 */
	if (OptDisplay == OPT_DEBUG) print_matrix(PRINT_DPND, 0);

	/* 呼応表現の処理 */

	if (koou() == TRUE && OptDisplay == OPT_DEBUG)
	    print_matrix(PRINT_DPND, 0);

	/* 鍵括弧の処理 */

	if ((flag = quote()) == TRUE && OptDisplay == OPT_DEBUG)
	    print_matrix(PRINT_QUOTE, 0);

	if (flag == CONTINUE) continue;

	/* 係り受け関係がない場合の弛緩 */
	
	if (relax_dpnd_matrix() == TRUE && OptDisplay == OPT_DEBUG) {
	    fprintf(stdout, "Relaxation ... \n");
	    print_matrix(PRINT_DPND, 0);
	}

	/* 並列構造解析 */

	init_mask_matrix();
	Para_num = 0;	
	Para_M_num = 0;
	relation_error = 0;
	d_struct_error = 0;
	Revised_para_num = -1;

	if ((flag = check_para_key()) > 0) {

	    calc_match_matrix();		/* 文節間類似度計算 */

	    detect_all_para_scope();	    	/* 並列構造推定 */

	    do {
		if (OptDisplay == OPT_DETAIL || OptDisplay == OPT_DEBUG) {
		    print_matrix(PRINT_PARA, 0);
		    /*
		    print_matrix2ps(PRINT_PARA, 0);
		    exit(0);
		    */
		}
		
		/* 並列構造間の重なり解析 */

		if (detect_para_relation() == FALSE) {
		    relation_error++; continue;
		}
		if (OptDisplay == OPT_DEBUG) print_para_relation();
		
		/* 並列構造内の依存構造チェック */

		if (check_dpnd_in_para() == FALSE) {
		    d_struct_error++; continue;
		}
		if (OptDisplay == OPT_DEBUG) print_matrix(PRINT_MASK, 0);

		goto ParaOK;		/* 並列構造解析成功 */

	    } while (relation_error <= 3 &&
		     d_struct_error <= 3 &&
		     detect_para_scope(Revised_para_num, TRUE) == TRUE);
	    
	    fprintf(stdout, ";; Cannot detect consistent CS scopes.\n");
	    init_mask_matrix();
	    
	  ParaOK:
	}
	else if (flag == CONTINUE)
	    continue;

	/* 依存・格構造解析 */

	para_postprocess();	/* 各conjunctのheadを提題の係り先に */

	signal(SIGALRM, timeout_function);
	alarm(PARSETIMEOUT);

	if (detect_dpnd_case_struct() == FALSE) {
	    fprintf(stdout, ";; Cannot detect dependency structure.\n");
	    when_no_dpnd_struct();	/* 係り受け構造が求まらない場合
					   すべて文節が隣に係ると扱う */
	}

	alarm(0);

	/* 固有名詞認識処理 */
	if (OptNE != OPT_NORMAL)
	    NE_analysis();

	memo_by_program();	/* メモへの書き込み */

	/* 結果表示 */

	if (OptAnalysis != OPT_DISC) print_result();
	fflush(stdout);

	/* 文脈解析 */

	if (OptAnalysis == OPT_DISC) {
	    discourse_analysis();

	    /* feature の初期化 */
	    for (i = 0; i < MRPH_MAX; i++)
		(mrph_data+i)->f = NULL;
	    for (i = 0; i < BNST_MAX; i++)
		(bnst_data+i)->f = NULL;
	}
    }

    close_ipal();
    close_bgh();
    close_sm();
    close_sm2code();
    close_scase();
    if (OptNE != OPT_NORMAL)
	close_proper();
    if (!(OptInhibit & OPT_INHIBIT_CLAUSE))
	close_clause();
    if (!(OptInhibit & OPT_INHIBIT_CASE_PREDICATE))
	close_case_pred();
    if (!(OptInhibit & OPT_INHIBIT_OPTIONAL_CASE))
	close_optional_case();
    /* close_dic_for_rule(); */
}

/*====================================================================
                               END
====================================================================*/
