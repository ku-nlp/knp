/*====================================================================

		     KNP (Kurohashi-Nagao Parser)

    $Id$
====================================================================*/
#include "knp.h"

/* Server Client Extention */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

SENTENCE_DATA	current_sentence_data;
SENTENCE_DATA	sentence_data[256];

MRPH_DATA 	mrph_data[MRPH_MAX];		/* 形態素データ */
BNST_DATA 	bnst_data[BNST_MAX];		/* 文節データ */
PARA_DATA 	para_data[PARA_MAX]; 		/* 並列データ */
PARA_MANAGER	para_manager[PARA_MAX];		/* 並列管理データ */
TOTAL_MGR	Best_mgr;			/* 依存・格解析管理データ */
TOTAL_MGR	Op_Best_mgr;

int 		Revised_para_num;			

char		*ErrorComment = NULL;		/* エラーコメント */
char		PM_Memo[256];			/* パターンマッチ結果 */

char  		cont_str[DBM_CON_MAX];

int		match_matrix[BNST_MAX][BNST_MAX];
int		path_matrix[BNST_MAX][BNST_MAX];
int		restrict_matrix[BNST_MAX][BNST_MAX];
int 		Dpnd_matrix[BNST_MAX][BNST_MAX]; /* 係り可能性 0, D, P, A */
int 		Quote_matrix[BNST_MAX][BNST_MAX];/* 括弧マスク 0, 1 */
int 		Mask_matrix[BNST_MAX][BNST_MAX]; /* 並列マスク
						    0:係り受け禁止
						    1:係り受けOK
						    2:並列のhead間,
						    3:並列のgapとhead間 */
char		G_Feature[100][64];		/* FEATUREの変数格納 */

int 		OptAnalysis;
int 		OptInput;
int 		OptExpress;
int 		OptDisplay;
int		OptExpandP;
int		OptNE;
int		OptCFMode;
char		OptIgnoreChar;
VerboseType	VerboseLevel = VERBOSE0;

/* Server Client Extention */
int		OptMode = STAND_ALONE_MODE;
int		OptPort = DEFAULT_PORT;
char		OptHostname[256];

FILE		*Infp;
FILE		*Outfp;

char *Case_name[] = {
    		"ガ格", "ヲ格", "ニ格", "デ格", "カラ格", 
		"ト格", "ヨリ格", "ヘ格", "マデ格", "ノ格",
		"ガ２", ""};

char 		*ProgName;
extern FILE	*Jumanrc_Fileptr;
extern CLASS    Class[CLASSIFY_NO + 1][CLASSIFY_NO + 1];
extern TYPE     Type[TYPE_NO];
extern FORM     Form[TYPE_NO][FORM_NO];
int CLASS_num;

jmp_buf timeout;
int	ParseTimeout = DEFAULT_PARSETIMEOUT;
char *Opt_jumanrc = NULL;


/*==================================================================*/
			     void usage()
/*==================================================================*/
{
    fprintf(stderr, "Usage: knp [-case|dpnd|bnst]\n" 
	    "           [-tree|sexp|-tab]\n" 
	    "           [-normal|detail|debug]\n" 
	    "           [-expand]\n"
	    "           [-C host:port] [-S] [-N port]\n"
	    "           [-timeout second] [-r rcfile]\n"
	    "           [-thesaurus [BGH|NTT]] (Default:NTT)\n"
	    "           [-para [BGH|NTT]] (Default:NTT)\n");
    exit(1);    
}

/*==================================================================*/
	       void option_proc(int argc, char **argv)
/*==================================================================*/
{
    /* 引数処理 */

    OptAnalysis = OPT_DPND;
    OptInput = OPT_RAW;
    OptExpress = OPT_TREE;
    OptDisplay = OPT_NORMAL;
    OptExpandP = FALSE;
    OptCFMode = EXAMPLE;
    OptNE = OPT_NORMAL;
    OptIgnoreChar = '\0';

    while ((--argc > 0) && ((*++argv)[0] == '-')) {
	if (str_eq(argv[0], "-case"))         OptAnalysis = OPT_CASE;
	else if (str_eq(argv[0], "-case2"))   OptAnalysis = OPT_CASE2;
	else if (str_eq(argv[0], "-cfsm"))    OptCFMode   = SEMANTIC_MARKER;
	else if (str_eq(argv[0], "-dpnd"))    OptAnalysis = OPT_DPND;
	else if (str_eq(argv[0], "-bnst"))    OptAnalysis = OPT_BNST;
	else if (str_eq(argv[0], "-assignf")) OptAnalysis = OPT_AssignF;
	else if (str_eq(argv[0], "-tree"))    OptExpress  = OPT_TREE;
	else if (str_eq(argv[0], "-treef"))   OptExpress  = OPT_TREEF;
	else if (str_eq(argv[0], "-sexp"))    OptExpress  = OPT_SEXP;
	else if (str_eq(argv[0], "-tab"))     OptExpress  = OPT_TAB;
	else if (str_eq(argv[0], "-normal"))  OptDisplay  = OPT_NORMAL;
	else if (str_eq(argv[0], "-detail"))  OptDisplay  = OPT_DETAIL;
	else if (str_eq(argv[0], "-debug"))   OptDisplay  = OPT_DEBUG;
	else if (str_eq(argv[0], "-expand"))  OptExpandP  = TRUE;
	else if (str_eq(argv[0], "-S"))       OptMode     = SERVER_MODE;
	else if (str_eq(argv[0], "-nesm"))    OptNE       = OPT_NESM;
	else if (str_eq(argv[0], "-ne"))      OptNE       = OPT_NE;
	else if (str_eq(argv[0], "-i")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    OptIgnoreChar = *argv[0];
	}
	else if (str_eq(argv[0], "-N")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    OptPort = atol(argv[0]);
	}
	else if (str_eq(argv[0], "-C")) {
	    OptMode = CLIENT_MODE;
	    argv++; argc--;
	    if (argc < 1) usage();
	    strcpy(OptHostname,argv[0]);
	}
	else if (str_eq(argv[0], "-timeout")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    ParseTimeout = atoi(argv[0]);
	}
	else if (str_eq(argv[0], "-thesaurus")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    if (!strcasecmp(argv[0], "ntt")) {
		Thesaurus = USE_NTT;
	    }
	    else if (!strcasecmp(argv[0], "bgh")) {
		Thesaurus = USE_BGH;
	    }
	    else {
		usage();
	    }
	}
	else if (str_eq(argv[0], "-para")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    if (!strcasecmp(argv[0], "ntt")) {
		ParaThesaurus = USE_NTT;
	    }
	    else if (!strcasecmp(argv[0], "bgh")) {
		ParaThesaurus = USE_BGH;
	    }
	    else if (!strcasecmp(argv[0], "none")) {
		ParaThesaurus = USE_NONE;
	    }
	    else {
		usage();
	    }
	}
	else if (str_eq(argv[0], "-r")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    Opt_jumanrc = argv[0];
	}
	else if (str_eq(argv[0], "-v")) {
	    argv++; argc--;
	    if (argc < 1) usage();
	    VerboseLevel = atoi(argv[0]);
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
    int i;

    /* rcfile をさがす順
       1. -r で指定されたファイル
       2. $HOME/.jumanrc
       3. RC_DEFAULT (Makefile)
       → rcfileがなければエラー
    */

    set_jumanrc_fileptr(Opt_jumanrc, TRUE);
    read_rc(Jumanrc_Fileptr);
    grammar(NULL);				/* 文法辞書 */
    katuyou(NULL);				/* 活用辞書 */

    for (i = 1; Class[i][0].id; i++);
    CLASS_num = i;
}

/*==================================================================*/
			void read_rules(void)
/*==================================================================*/
{
    int i;

    for (i = 0; i < CurrentRuleNum; i++) {
	/* 同形異義語ルール */
	if ((RULE+i)->type == HomoRuleType) {
	    read_homo_rule((RULE+i)->file);
	}
	/* 形態素ルール or 文節ルール */
	else if ((RULE+i)->type == MorphRuleType || (RULE+i)->type == BnstRuleType) {
	    read_general_rule(RULE+i);
	}
	/* 係り受けルール */
	else if ((RULE+i)->type == DpndRuleType) {
	    read_dpnd_rule((RULE+i)->file);
	}
	/* 呼応表現ルール */
	else if ((RULE+i)->type == KoouRuleType) {
	    read_koou_rule((RULE+i)->file);
	}
	/* 固有名詞ルール */
	else if ((RULE+i)->type == NeMorphRuleType) {
	    read_mrph_rule((RULE+i)->file, NERuleArray, &CurNERuleSize, NERule_MAX);
	}
	/* 複合名詞準備ルール */
	else if ((RULE+i)->type == NePhrasePreRuleType) {
	    read_mrph_rule((RULE+i)->file, CNpreRuleArray, &CurCNpreRuleSize, CNRule_MAX);
	}
	/* 複合名詞ルール */
	else if ((RULE+i)->type == NePhraseRuleType) {
	    read_mrph_rule((RULE+i)->file, CNRuleArray, &CurCNRuleSize, CNRule_MAX);
	}
	/* 複合名詞補助ルール */
	else if ((RULE+i)->type == NePhraseAuxRuleType) {
	    read_mrph_rule((RULE+i)->file, CNauxRuleArray, &CurCNauxRuleSize, CNRule_MAX);
	}
	/* 文脈処理のルール */
	else if ((RULE+i)->type == ContextRuleType) {
	    read_bnst_rule((RULE+i)->file, ContRuleArray, &ContRuleSize, ContRule_MAX);
	}
    }
}

/*==================================================================*/
	       static void timeout_function(int signal)
/*==================================================================*/
{
    longjmp(timeout, 1);
}

/*==================================================================*/
			   void init_all()
/*==================================================================*/
{
    int i;

    /* 初期化 */

    init_configfile();	/* 各種ファイル設定初期化 */
    init_juman();	/* JUMAN関係 */
    init_cf();		/* 格フレームオープン */
    init_bgh();		/* シソーラスオープン */
    init_sm();		/* NTT 辞書オープン */
    init_scase();	/* 表層格辞書オープン */

    current_sentence_data.mrph_data = mrph_data;
    current_sentence_data.bnst_data = bnst_data;
    current_sentence_data.para_data = para_data;
    current_sentence_data.para_manager = para_manager;
    current_sentence_data.Sen_num = 0;	/* これだけは増えていく */
    current_sentence_data.Mrph_num = 0;
    current_sentence_data.Bnst_num = 0;
    current_sentence_data.New_Bnst_num = 0;
    current_sentence_data.Best_mgr = &Best_mgr;
    current_sentence_data.KNPSID = NULL;
    current_sentence_data.Comment = NULL;

    for (i = 0; i < BNST_MAX; i++) {
	 current_sentence_data.bnst_data[i].internal_num = 0;
	 current_sentence_data.bnst_data[i].f = NULL;
    }

    /* 固有名詞解析辞書オープン */
    if (OptNE != OPT_NORMAL) {
	init_proper(&current_sentence_data);
    }
}

/*==================================================================*/
	  int main_analysis(SENTENCE_DATA *sp, FILE *input)
/*==================================================================*/
{
    int flag, i;
    int relation_error, d_struct_error;
    char *code;

    sp->Sen_num ++;

    if ((flag = read_mrph(sp, input)) == EOF) return EOF;
    if (flag == FALSE) return FALSE;

    /* 形態素への意味情報付与 */

    if (OptNE != OPT_NORMAL && SMExist == TRUE) {
	for (i = 0; i < sp->Mrph_num; i++) {
	    code = (char *)get_sm(sp->mrph_data[i].Goi);
	    if (code) {
		strcpy(sp->mrph_data[i].SM, code);
		free(code);
	    }
	    assign_ntt_dict(sp, i);
	}
    }

    /* 形態素へのFEATURE付与 */

    assign_cfeature(&(sp->mrph_data[0].f), "文頭");
    assign_cfeature(&(sp->mrph_data[sp->Mrph_num-1].f), "文末");
    assign_general_feature(sp, MorphRuleType);

    /* 形態素を文節にまとめる */

    if (OptInput == OPT_RAW) {
	if (make_bunsetsu(sp) == FALSE) return FALSE;
    } else {
	if (make_bunsetsu_pm(sp) == FALSE) return FALSE;
    }

    /* 文節化だけの場合 */

    if (OptAnalysis == OPT_BNST) return TRUE;

    /* 文節への意味情報付与 */

    for (i = 0; i < sp->Bnst_num; i++) {
	get_bgh_code(sp->bnst_data+i);		/* シソーラス */
	get_sm_code(sp->bnst_data+i);		/* 意味素 */
    }

    /* 文節へのFEATURE付与 */

    assign_cfeature(&(sp->bnst_data[0].f), "文頭");
    if (sp->Bnst_num > 0)
	assign_cfeature(&(sp->bnst_data[sp->Bnst_num-1].f), "文末");
    else
	assign_cfeature(&(sp->bnst_data[0].f), "文末");
    assign_general_feature(sp, BnstRuleType);

    if (OptDisplay == OPT_DETAIL || OptDisplay == OPT_DEBUG)
	print_mrphs(sp, 0);

	/* 時間属性を補助的に付与する */
    for (i = 0; i < sp->Bnst_num; i++) {
	if (!check_feature((sp->bnst_data+i)->f, "時間") && 
	    check_feature((sp->bnst_data+i)->f, "体言") && 
	    sm_time_match((sp->bnst_data+i)->SM_code)) {
	    assign_cfeature(&((sp->bnst_data+i)->f), "時間");
	}
    }

    /* FEATURE付与だけの場合 */

    if (OptAnalysis == OPT_AssignF) return TRUE;

    assign_dpnd_rule(sp);			/* 係り受け規則 */

    if (OptAnalysis == OPT_CASE ||
	OptAnalysis == OPT_CASE2) {

	/* 格解析を行うサ変名詞を含む文節に feature を与え、
	   複合名詞をばらして格要素として認識する */
	MakeInternalBnst(sp);

	/* それぞれの用言の格フレームを取得 */
	set_pred_caseframe(sp);
    }

    if (OptDisplay == OPT_DETAIL || OptDisplay == OPT_DEBUG)
	check_bnst(sp);

    /**************/
    /* 本格的解析 */
    /**************/

    if (OptInput == OPT_PARSED) {
	dpnd_info_to_bnst(sp, &(sp->Best_mgr->dpnd)); 
	para_recovery(sp);
	after_decide_dpnd(sp);
	goto PARSED;
    }

    calc_dpnd_matrix(sp);			/* 依存可能性計算 */
    if (OptDisplay == OPT_DEBUG) print_matrix(sp, PRINT_DPND, 0);

    /* 呼応表現の処理 */

    if (koou(sp) == TRUE && OptDisplay == OPT_DEBUG)
	print_matrix(sp, PRINT_DPND, 0);

	/* 鍵括弧の処理 */

    if ((flag = quote(sp)) == TRUE && OptDisplay == OPT_DEBUG)
	print_matrix(sp, PRINT_QUOTE, 0);

    if (flag == CONTINUE) return FALSE;

    /* 係り受け関係がない場合の弛緩 */
	
    if (relax_dpnd_matrix(sp) == TRUE && OptDisplay == OPT_DEBUG) {
	fprintf(Outfp, "Relaxation ... \n");
	print_matrix(sp, PRINT_DPND, 0);
    }

    /****************/
    /* 並列構造解析 */
    /****************/

    init_mask_matrix(sp);
    sp->Para_num = 0;
    sp->Para_M_num = 0;
    relation_error = 0;
    d_struct_error = 0;
    Revised_para_num = -1;

    if ((flag = check_para_key(sp)) > 0) {
	calc_match_matrix(sp);		/* 文節間類似度計算 */
	detect_all_para_scope(sp);    	/* 並列構造推定 */
	do {
	    assign_para_similarity_feature(sp);
	    if (OptDisplay == OPT_DETAIL || OptDisplay == OPT_DEBUG) {
		print_matrix(sp, PRINT_PARA, 0);
		/*
		  print_matrix2ps(sp, PRINT_PARA, 0);
		  exit(0);
		*/
	    }
	    /* 並列構造間の重なり解析 */
	    if (detect_para_relation(sp) == FALSE) {
		relation_error++;
		continue;
	    }
	    if (OptDisplay == OPT_DEBUG) print_para_relation(sp);
	    /* 並列構造内の依存構造チェック */
	    if (check_dpnd_in_para(sp) == FALSE) {
		d_struct_error++;
		continue;
	    }
	    if (OptDisplay == OPT_DEBUG) print_matrix(sp, PRINT_MASK, 0);
	    goto ParaOK;		/* 並列構造解析成功 */
	} while (relation_error <= 3 &&
		 d_struct_error <= 3 &&
		 detect_para_scope(sp, Revised_para_num, TRUE) == TRUE);
	ErrorComment = strdup("Cannot detect consistent CS scopes");
	init_mask_matrix(sp);
    ParaOK:
    }
    else if (flag == CONTINUE)
	return FALSE;

    /********************/
    /* 依存・格構造解析 */
    /********************/

    para_postprocess(sp);	/* 各conjunctのheadを提題の係り先に */

    signal(SIGALRM, timeout_function);
    alarm(ParseTimeout);
    if (detect_dpnd_case_struct(sp) == FALSE) {
	ErrorComment = strdup("Cannot detect dependency structure");
	when_no_dpnd_struct(sp);	/* 係り受け構造が求まらない場合
					   すべて文節が隣に係ると扱う */
    }
    alarm(0);

PARSED:

    /* 係り受け情報を bnst 構造体に記憶 */
    dpnd_info_to_bnst(sp, &(sp->Best_mgr->dpnd)); 
    para_recovery(sp);

	/* 固有名詞認識処理 */

    if (OptNE != OPT_NORMAL)
	NE_analysis(sp);
    else
	assign_mrph_feature(CNRuleArray, CurCNRuleSize,
			    sp->mrph_data, sp->Mrph_num,
			    RLOOP_RMM, FALSE, LtoR);

    memo_by_program(sp);	/* メモへの書き込み */

    /* 認識した固有名詞を保存しておく */
    if (OptNE != OPT_NORMAL) {
	preserveNE(sp);
	if (OptDisplay == OPT_DEBUG)
	    printNE();
    }

    return TRUE;
}

/*==================================================================*/
			   void knp_main()
/*==================================================================*/
{
    int i, success = 1, flag;

    SENTENCE_DATA *sp = &current_sentence_data;

    /* 格解析の準備 */
    init_cf2(sp);
    init_case_analysis();

    /* ルール読み込み */
    read_rules();

    while ( 1 ) {

	/* Server Mode の場合 前回の出力が成功してない場合は 
	   ERROR とはく Server/Client モードの場合は,出力の同期をこれで行う */
	if (!success && OptMode == SERVER_MODE) {
	    fprintf(Outfp,"EOS ERROR\n");
	    fflush(Outfp);
	}

	/********************/
	/* 前の解析の後始末 */
	/********************/

	/* タイムアウト時 */

	if (setjmp(timeout)) {
#ifdef DEBUG
	    fprintf(stderr, "Parse timeout.\n(");
	    for (i = 0; i < sp->Mrph_num; i++)
		fprintf(stderr, "%s", sp->mrph_data[i].Goi2);
	    fprintf(stderr, ")\n");
#endif
	    ErrorComment = strdup("Parse timeout");
	    when_no_dpnd_struct(sp);
	    dpnd_info_to_bnst(sp, &(sp->Best_mgr->dpnd));
	    print_result(sp);
	    fflush(Outfp);
	}

	/* 格フレームの初期化 */
	if (OptAnalysis == OPT_CASE || 
	    OptAnalysis == OPT_CASE2) {
	    clear_cf();
	}

	/* 初期化 */
	if (sp->KNPSID) {
	    free(sp->KNPSID);
	    sp->KNPSID = NULL;
	}
	if (sp->Comment) {
	    free(sp->Comment);
	    sp->Comment = NULL;
	}

	/* FEATURE の初期化 */
	for (i = 0; i < sp->Mrph_num; i++) 
	    clear_feature(&(sp->mrph_data[i].f));
	for (i = 0; i < sp->Bnst_num; i++) {
	    clear_feature(&(sp->bnst_data[i].f));
	    if (sp->bnst_data[i].internal_num) {
		sp->bnst_data[i].internal_num = 0;
		sp->bnst_data[i].internal_max = 0;
		free(sp->bnst_data[i].internal);
	    }
	}
	/* New_Bnstはもともとpointer */
	for (i = sp->Bnst_num; i < sp->Bnst_num + sp->New_Bnst_num; i++)
	    (sp->bnst_data+i)->f = NULL;

	/**************/
	/* メイン解析 */
	/**************/

	success = 0;
	if ((flag = main_analysis(sp, Infp)) == EOF) break;
	if (flag == FALSE) continue;

	/************/
	/* 結果表示 */
	/************/

	if (OptAnalysis == OPT_BNST) {
	    print_mrphs(sp, 0);
	} else {
	    print_result(sp);
	}
	fflush(Outfp);

	success = 1;	/* OK 成功 */
    }

    close_cf();
    close_bgh();
    close_sm();
    close_scase();

    if (OptNE != OPT_NORMAL)
	close_proper();
}

/*==================================================================*/
			  void server_mode()
/*==================================================================*/
{
    /* サーバモード */

    int sfd,fd;
    struct sockaddr_in sin;

    /* シグナル処理 */
    static void sig_child()
	{
	    int status;
	    while(wait3(&status, WNOHANG, NULL) > 0) {}; 
	    signal(SIGCHLD, sig_child); 
	}

    static void sig_term()
	{
	    shutdown(sfd,2);
	    shutdown(fd, 2);
	    exit(0);
	}

    signal(SIGHUP,  SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, sig_term);
    signal(SIGINT,  sig_term);
    signal(SIGQUIT, sig_term);
    signal(SIGCHLD, sig_child);
  
    if((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	fprintf(stderr,"Socket Error\n");
	exit(1);
    }
  
    memset(&sin, 0, sizeof(sin));
    sin.sin_port        = htons(OptPort);
    sin.sin_family      = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
  
    /* bind */  
    if (bind(sfd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
	fprintf(stderr,"bind Error\n");
	close(sfd);
	exit(1);
    }
  
    /* listen */  
    if (listen(sfd, SOMAXCONN) < 0) {
	fprintf(stderr,"listen Error\n");
	close(sfd);
	exit(1);
    }

    /* accept loop */
    while(1) {
	int pid;

	if((fd = accept(sfd, NULL, NULL)) < 0) {
	    if (errno == EINTR) 
		continue;
	    fprintf(stderr,"accept Error\n");
	    close(sfd);
	    exit(1);
	}
    
	/* 子作り失敗 しくしく */
	if((pid = fork()) < 0) {
	    fprintf(stderr, "Fork Error\n");
	    sleep(1);
	    continue;
	}

	/* 子供できちゃった */
	if(pid == 0) {

	    /* ぉぃぉぃ そんなところで げろはくなぁ */
	    chdir("/tmp");

	    close(sfd);
	    Infp  = fdopen(fd, "r");
	    Outfp = fdopen(fd, "w");

	    /* 良い子に育つには 挨拶しましょうね.. */
	    fprintf(Outfp, "200 Running KNP Server\n");
	    fflush(Outfp);

	    /* オプション解析 */
	    while (1) {
		char buf[1024];

		fgets(buf, sizeof(buf), Infp);

		/* QUIT */
		if (strncasecmp(buf, "QUIT", 4) == 0) {
		    fprintf(Outfp, "200 OK Quit\n");
		    fflush(Outfp);
		    exit(0);
		}

		if (strncasecmp(buf, "RC", 2) == 0) {
		    server_read_rc(Infp);
		    fprintf(Outfp, "200 OK\n");
		    fflush(Outfp);
		    continue;
		}

		/* RUN */
		/* Option 解析は strstr なんかでかなりええかげん 
		   つまり間違ったオプションはエラーにならん... */
		if (strncasecmp(buf, "RUN", 3) == 0) {
		    char *p;

		    if (strstr(buf, "-case"))   OptAnalysis = OPT_CASE;
		    if (strstr(buf, "-case2"))  OptAnalysis = OPT_CASE2;
		    if (strstr(buf, "-dpnd"))   OptAnalysis = OPT_DPND;
		    if (strstr(buf, "-bnst"))   OptAnalysis = OPT_BNST;
		    if (strstr(buf, "-tree"))   OptExpress = OPT_TREE;
		    if (strstr(buf, "-sexp"))   OptExpress = OPT_SEXP;
		    if (strstr(buf, "-tab"))    OptExpress = OPT_TAB;
		    if (strstr(buf, "-normal")) OptDisplay = OPT_NORMAL;
		    if (strstr(buf, "-detail")) OptDisplay = OPT_DETAIL;
		    if (strstr(buf, "-debug"))  OptDisplay = OPT_DEBUG;
		    if (strstr(buf, "-expand")) OptExpandP = TRUE;
		    /* うーん 引数とるのは困るんだなぁ..
		       とおもいつつかなり強引... */
		    if ((p = strstr(buf, "-i")) != NULL) {
			p += 3;
			while(*p != '\0' && (*p == ' ' || *p == '\t')) p++;
			if (*p != '\0') OptIgnoreChar = *p;
		    } 
		    fprintf(Outfp,"200 OK option=[Analysis=%d Express=%d"
			    " Display=%d IgnoreChar=%c]\n",
			    OptAnalysis,OptExpress,OptDisplay,OptIgnoreChar);
		    fflush(Outfp);
		    break;
		} else {
		    fprintf(Outfp,"500 What?\n");
		    fflush(Outfp);
		}
	    }

	    /* 解析 */
	    knp_main();

	    /* 後処理 */
	    shutdown(fd,2);
	    fclose(Infp);
	    fclose(Outfp);
	    close(fd);
	    exit(0); /* これしないと大変なことになるかも */
	}

	/* 親 */
	close(fd);
    }
}

/*==================================================================*/
			  void client_mode()
/*==================================================================*/
{
    /* クライアントモード */

    struct sockaddr_in sin;
    struct hostent *hp;
    int fd;
    FILE *fi,*fo;
    char *p;
    char buf[1024*8];
    char option[1024];
    int  port = DEFAULT_PORT;
    int  strnum = 0;

    /* 文字列を送って ステータスコードを返す */  
    int send_string(char *str)
	{
	    int len,result = 0;
	    char buf[1024];
    
	    if (str != NULL){
		fwrite(str,sizeof(char),strlen(str),fo);
		fflush(fo);
	    }

	    while (fgets(buf,sizeof(buf)-1,fi) != NULL){
		len = strlen(buf);
		if (len >= 3 && buf[3] == ' ') {
		    buf[3] = '\0';
		    result = atoi(&buf[0]);
		    break;
		}
	    }

	    return result;
	} 

    /* host:port って形の場合 */
    if ((p = strchr(OptHostname, ':')) != NULL) {
	*p++ = '\0';
	port = atoi(p);
    }

    /* あとは つなげる準備 */
    if ((hp = gethostbyname(OptHostname)) == NULL) {
	fprintf(stderr,"host unkown\n");
	exit(1);
    }
  
    while ((fd = socket(AF_INET,SOCK_STREAM,0 )) < 0 ){
	fprintf(stderr,"socket error\n");
	exit(1);
    }
  
    sin.sin_family = AF_INET;
    sin.sin_port   = htons(port);
    sin.sin_addr = *((struct in_addr * )hp->h_addr);

    if (connect(fd,(struct sockaddr *)&sin, sizeof(sin)) < 0) {
	fprintf(stderr,"connect error\n");
	exit(1);
    }

    /* Server 用との 通信ハンドルを作成 */
    if ((fi = fdopen(fd, "r")) == NULL || (fo = fdopen(fd, "w")) == NULL) {
	close (fd);
	fprintf(stderr,"fd error\n");
	exit(1);
    }

    /* 挨拶は元気な声で */
    if (send_string(NULL) != 200) {
	fprintf(stderr,"greet error\n");
	exit(1);
    }

    /* オプション解析 もっと スマートなやり方ないかなぁ */
    option[0] = '\0';
    switch (OptAnalysis) {
    case OPT_CASE: strcat(option," -case"); break;
    case OPT_DPND: strcat(option," -dpnd"); break;
    case OPT_BNST: strcat(option," -bnst"); break;
    }

    switch (OptExpress) {
    case OPT_TREE: strcat(option," -tree"); break;
    case OPT_SEXP: strcat(option," -sexp"); break;
    case OPT_TAB:  strcat(option," -tab");  break;
    }

    switch (OptDisplay) {
    case OPT_NORMAL: strcat(option," -normal"); break;
    case OPT_DETAIL: strcat(option," -detail"); break;
    case OPT_DEBUG:  strcat(option," -debug");  break;
    }
    
    if (OptExpandP) strcat(option," -expand");
    if (!OptIgnoreChar) {
	sprintf(buf," -i %c",OptIgnoreChar);
	strcat(option,buf);
    }

    /* これから動作じゃ */
    sprintf(buf,"RUN%s\n",option);
    if (send_string(buf) != 200) {
	fprintf(stderr,"argument error OK? [%s]\n",option);
	close(fd);
	exit(1);
    }

    /* あとは LOOP */
    strnum = 0;
    while (fgets(buf,sizeof(buf),stdin) != NULL) {
	if (strncmp(buf,"EOS",3) == 0) {
	    if (strnum != 0) {
		fwrite(buf,sizeof(char),strlen(buf),fo);
		fflush(fo);
		strnum = 0;
		while (fgets(buf,sizeof(buf),fi) != NULL) {
		    fwrite(buf,sizeof(char),strlen(buf),stdout);
		    fflush(stdout);
		    if (strncmp(buf,"EOS",3) == 0)  break;
		}
	    }
	} else {
	    fwrite(buf,sizeof(char),strlen(buf),fo);
	    fflush(fo);
	    strnum++;
	}
    }

    /* 終了処理 */
    fprintf(fo,"\n%c\nQUIT\n", EOf);
    fclose(fo);
    fclose(fi);
    close(fd);
    exit(0);
}


/*==================================================================*/
		   int main(int argc, char **argv)
/*==================================================================*/
{
    option_proc(argc, argv);

    Infp  = stdin;
    Outfp = stdout;

    /* モードによって処理を分岐 */
    if (OptMode == STAND_ALONE_MODE) {
	init_all();
	knp_main();
    } else if (OptMode == SERVER_MODE) {
	init_all();
	server_mode();
    } else if (OptMode == CLIENT_MODE) {
	client_mode();
    }
    exit(0);
}

/*====================================================================
                               END
====================================================================*/
