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
int		OptHelpsys;
char		OptIgnoreChar;

/* Server Client Extention */
int		OptMode = STAND_ALONE_MODE;
int		OptPort = DEFAULT_PORT;
char		OptHostname[256];

FILE		*Infp  = stdin;
FILE		*Outfp = stdout;

char *Case_name[] = {
    		"ガ格", "ヲ格", "ニ格", "デ格", "カラ格", 
		"ト格", "ヨリ格", "ヘ格", "マデ格", "ノ格",
		"ガ２", ""};

char 		*ProgName;
extern int 	Case_frame_num;
extern CLASS    Class[CLASSIFY_NO + 1][CLASSIFY_NO + 1];
extern TYPE     Type[TYPE_NO];
extern FORM     Form[TYPE_NO][FORM_NO];
int CLASS_num;

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
	    "[-case|dpnd|bnst|-disc] [-tree|sexp] [-normal|detail|debug] [-expand]\n"
		"           [-C host:port] [-S] [-N port] \n");
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
    OptHelpsys = FALSE;
/*    OptIgnoreChar = (char)NULL;*/
	OptIgnoreChar = '\0';

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
	else if (str_eq(argv[0], "-S"))      OptMode    = SERVER_MODE;
	else if (str_eq(argv[0], "-check"))  OptCheck = TRUE;
	else if (str_eq(argv[0], "-nesm"))   OptNE = OPT_NESM;
	else if (str_eq(argv[0], "-ne"))     OptNE = OPT_NE;
	else if (str_eq(argv[0], "-helpsys")) OptHelpsys = TRUE;
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
	} else if (str_eq(argv[0], "-N")) {
	  argv++; argc--;	  
	  OptPort = atol(argv[0]);
	} else if (str_eq(argv[0], "-C")) {
	  OptMode = CLIENT_MODE;
	  argv++; argc--;	  
	  strcpy(OptHostname,argv[0]);
	}else {
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

    set_jumanrc_fileptr(NULL, TRUE);
    set_jumangram_dirname();
    /*read_jumanpathrc();*/			/* JUMAN辞書のパス */
    grammar(NULL);				/* 文法辞書 */
    katuyou(NULL);				/* 活用辞書 */

    for (i = 1; Class[i][0].id; i++);
    CLASS_num = i;
}

/*==================================================================*/
			void read_rules(void)
/*==================================================================*/
{
    read_homo_rule(HOMO_FILE);			/* 同形異義語ルール */
    read_mrph_rule(MRPH_FILE, MrphRuleArray, &CurMrphRuleSize, MrphRule_MAX);
						/* 形態素ルール */
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
	read_NE_rule(NE_FILE, NERuleArray, &CurNERuleSize, NERule_MAX);
						/* 固有名詞ルール */
	read_NE_rule(CN_PRE_FILE, CNpreRuleArray, &CurCNpreRuleSize, CNRule_MAX);
						/* 複合名詞準備ルール */
	read_NE_rule(CN_FILE, CNRuleArray, &CurCNRuleSize, CNRule_MAX);
						/* 複合名詞ルール */
	read_NE_rule(CN_AUX_FILE, CNauxRuleArray, &CurCNauxRuleSize, CNRule_MAX);
						/* 複合名詞補助ルール */
    }
    else {
	read_NE_rule(NE_JUMAN_FILE, CNRuleArray, &CurCNRuleSize, CNRule_MAX);
						/* 固有名詞ルール */
    }

    if (OptHelpsys == TRUE) {
	read_mrph_rule(HELPSYS_FILE, HelpsysArray,
		       &CurHelpsysSize, Helpsys_MAX);
    						/* 対話的ヘルプシステム用ルール */
    }

    read_bnst_rule(CONT_FILE, ContRuleArray,	/* 文脈処理のルール */
		   &ContRuleSize, ContRule_MAX);
}

/*==================================================================*/
	       static void timeout_function(int signal)
/*==================================================================*/
{
    longjmp(timeout, 1);
}

/* すべての初期化をひき受ける とってもせこい関数 */
void init_all()
{
    /* 初期化 */

	init_juman();	/* JUMAN関係 */
	init_ipal();	/* 格フレームオープン */
	init_bgh();		/* シソーラスオープン */
	init_sm();		/* NTT 辞書オープン */
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
}

void stand_alone_mode()
{
	int i, j, flag, success = 1;
	int relation_error, d_struct_error;

	while ( 1 ) {

		/* Server Mode の場合 前回の出力が成功してない場合は 
		   ERROR とはく Server/Client モードの場合は,出力の同期をこれで行う */
		if (!success && OptMode == SERVER_MODE) {
		    fprintf(Outfp,"EOS ERROR\n");
		    fflush(Outfp);
		}

		/* この段階では成功していない */
		success = 0;

		if (setjmp(timeout)) {
		    fprintf(stderr, "Parse timeout.\n(");
		    for (i = 0; i < Mrph_num; i++)
			fprintf(stderr, "%s", mrph_data[i].Goi);
		    fprintf(stderr, ")\n");
		}

		for (i = 0; i < Mrph_num; i++) clear_feature(&(mrph_data[i].f));
		for (i = 0; i < Bnst_num; i++) clear_feature(&(bnst_data[i].f));
		for (i = Bnst_num; i < Bnst_num + New_Bnst_num; i++)
		    bnst_data[i].f = NULL;

		if ((flag = read_mrph(Infp)) == EOF) break;

		Sen_num++;

		if (flag == FALSE) continue;

		/* 形態素に意味素を与える */
		for (i = 0; i < Mrph_num; i++) {
		    strcpy(mrph_data[i].SM, (char *)get_sm(mrph_data[i].Goi));
		    assign_ntt_dict(i);
		}

		/* 形態素への情報付与 --> 文節 */

		assign_cfeature(&(mrph_data[0].f), "文頭");
		assign_cfeature(&(mrph_data[Mrph_num-1].f), "文末");
		assign_mrph_feature(MrphRuleArray, CurMrphRuleSize);
		if (OptHelpsys == TRUE)
		    assign_mrph_feature(HelpsysArray, CurHelpsysSize);

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
		if (Bnst_num > 0)
		    assign_cfeature(&(bnst_data[Bnst_num-1].f), "文末");
		else
		    assign_cfeature(&(bnst_data[0].f), "文末");

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
		    dpnd_info_to_bnst(&(Best_mgr.dpnd)); /* 係り受け情報を bnst 構造体に記憶 */
		    print_result();
		    fflush(Outfp);
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
		    fprintf(Outfp, "Relaxation ... \n");
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
					relation_error++;
					continue;
				}
				if (OptDisplay == OPT_DEBUG) print_para_relation();
				/* 並列構造内の依存構造チェック */
				if (check_dpnd_in_para() == FALSE) {
					d_struct_error++;
					continue;
				}
				if (OptDisplay == OPT_DEBUG) print_matrix(PRINT_MASK, 0);
				goto ParaOK;		/* 並列構造解析成功 */
			} while (relation_error <= 3 &&
					 d_struct_error <= 3 &&
					 detect_para_scope(Revised_para_num, TRUE) == TRUE);
			fprintf(Outfp, ";; Cannot detect consistent CS scopes.\n");
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
			fprintf(Outfp, ";; Cannot detect dependency structure.\n");
			when_no_dpnd_struct();	/* 係り受け構造が求まらない場合
									   すべて文節が隣に係ると扱う */
		}

		alarm(0);

		dpnd_info_to_bnst(&(Best_mgr.dpnd)); /* 係り受け情報を bnst 構造体に記憶 */

		/* 固有名詞認識処理 */
		if (OptNE != OPT_NORMAL)
		    NE_analysis();
		else
		    assign_mrph_feature(CNRuleArray, CurCNRuleSize);

		memo_by_program();	/* メモへの書き込み */

		/* チェック用 */
		if (OptCheck == TRUE)
		    CheckCandidates();

		/* 結果表示 */
		if (OptAnalysis != OPT_DISC) print_result();
		fflush(Outfp);

		/* OK 成功 */
		success = 1;

		/* 認識した固有名詞を保存しておく */
		if (OptNE != OPT_NORMAL) {
			preserveNE();
			if (OptDisplay == OPT_DEBUG)
				printNE();
		}

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

/* サーバモード */
void server_mode()
{
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
			fprintf(stderr,"Frok Error\n");
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

				fgets(buf,sizeof(buf),Infp);

				/* QUIT */
				if (strncasecmp(buf,"QUIT",4) == 0) {
					fprintf(Outfp, "200 OK Quit\n");
					fflush(Outfp);
					exit(0);
				}

				/* RUN */
				/* Option 解析は strstr なんかでかなりええかげん 
				   つまり間違ったオプションはエラーにならん... */
				if (strncasecmp(buf,"RUN",3) == 0) {
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
					if (strstr(buf, "-helpsys")) OptHelpsys = TRUE;
					/* うーん 引数とるのは困るんだなぁ..
					   とおもいつつかなり強引... */
					if ((p = strstr(buf, "-i")) != NULL) {
						p += 3;
						while(*p != '\0' && (*p == ' ' || *p == '\t')) p++;
						if (*p != '\0') OptIgnoreChar = *p;
					} 
					fprintf(Outfp,"200 OK option=[Analysis=%d Express=%d"
							" Display=%d IgnoreChar=%c Helpsys=%d]\n",
							OptAnalysis,OptExpress,OptDisplay,OptIgnoreChar, OptHelpsys);
					fflush(Outfp);
					break;
				} else {
					fprintf(Outfp,"500 What?\n");
					fflush(Outfp);
				}
			}

			/* 解析 */
			stand_alone_mode();

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

/* クライアントモード */
void client_mode()
{
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
		   void main(int argc, char **argv)
/*==================================================================*/
{
	option_proc(argc, argv);

    /* モードによって処理を分岐 */
	if (OptMode == STAND_ALONE_MODE) {
		init_all();
		stand_alone_mode();
	} else if (OptMode == SERVER_MODE) {
		init_all();
		server_mode();
	} else if (OptMode == CLIENT_MODE) {
		client_mode();
	}
}

/*====================================================================
                               END
====================================================================*/
