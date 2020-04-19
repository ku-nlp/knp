/*
 * server.c - ChaSen server
 *
 * Copyright (c) 1996,1997 Nara Institute of Science and Technology
 *
 * Author: M.Izumo <masana-i@is.aist-nara.ac.jp>, Tue Feb 11 1997
 *         A.Kitauchi <akira-k@is.aist-nara.ac.jp>, Apr 1997
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <signal.h>
#include <errno.h>

#if defined _WIN32 && ! defined __CYGWIN__

#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif

#else

#define closesocket(s)  close(s)

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#endif

#include <juman.h>

extern int errno;

#ifndef SOMAXCONN
#define SOMAXCONN 5
#endif /* SOMAXCONN */

static int client_fd = -1;
static FILE *client_ofp = NULL;
static FILE *client_ifp = NULL;
static char *data_buff = NULL;
static int data_column = 0;
static int data_buff_cur = 0;


/* Parameter Definition from juman.c
   NACSIS 吉岡
*/
extern FILE	*Jumanrc_Fileptr;
extern FILE	*Cha_stderr;

extern int              Show_Opt1;
extern int              Show_Opt2;
extern char		Show_Opt_tag[MIDASI_MAX];
extern int		Show_Opt_jumanrc;
extern int		Show_Opt_debug;
extern int		Rendaku_Opt;
extern int		LowercaseRep_Opt;
extern int		LowercaseDel_Opt;
extern int		Repetition_Opt;
extern int              Onomatopoeia_Opt;
extern int              LongSoundRep_Opt;
extern int              LongSoundDel_Opt;

extern U_CHAR	        String[LENMAX];

#define JUMAN_INPUT_SIZE  8192
   
/*
 * skip_until_EOf()
 */
static int skip_until_EOf(fp)
    FILE *fp;
{
    char buff[BUFSIZ];
    while(fgets(buff, BUFSIZ, fp)) {
	if (buff[0] == EOf &&
	    (buff[1] == '\n' || (buff[1] == '\r' && buff[2] == '\n')))
	    break;
    }
}

/*
 * chomp()
 */
static char *chomp(s)
    char *s;
{
    int len = strlen(s);

    if(len < 2)
    {
	if(len == 0)
	    return s;
	if(s[0] == '\n' || s[0] == '\r')
	    s[0] = '\0';
	return s;
    }
    if(s[len - 1] == '\n')
	s[--len] = '\0';
    if(s[len - 1] == '\r')
	s[--len] = '\0';
    return s;
}

/***********************************************************************
 * read_chasenrc_server()
 ***********************************************************************/
/*
  read_chasenrc_server()をjuman_init_rc_server()に改名
  rcfileの処理手法をJUMAN用に変更
  NACSIS 吉岡
*/
static int juman_init_rc_server()
{
    set_cha_getc();
    if (juman_init_rc(client_ifp))	/* rcfileの処理 */
	fputs("200 OK\n", client_ofp);
    else
	fputs("500 Error in parsing .jumanrc\n", client_ofp);
    unset_cha_getc();
    
    fflush(client_ofp);
    return 1;
}

/***********************************************************************
 * do_chasen()
 ***********************************************************************/
/*
  do_chasen()をdo_juman()に改名
  入出力部の関数をJUMAN用に変更
  NACSIS 吉岡
*/
static int do_juman()
{
    char *end_of_line;
    int ret;

    /* fgets() のチェックのための番人 */
    end_of_line = String + sizeof(String) - 1;
    *end_of_line = '\n';

    while (fgets(String, sizeof(String), client_ifp) != NULL) {
        if (*end_of_line != '\n') {
	    fputs("line too long\n", client_ofp);
	    skip_until_EOf(client_ifp);
	    *end_of_line = '\n';
	    return 1;
	}

#if 0
	printf("## : %s", String);
	printf("## : %x:%x:%x\n", String[0], String[1], String[2]);
	fflush(stdout);
#endif

	/* 04\r\n なら終了 (telnetでは\nが\r\nに変換される) */
	if (String[0] == EOf &&
	    (String[1] == '\n' || (String[1] == '\r' && String[2] == '\n')))
	    return 1;

	/* -i オプションで指定した文字からの行は解析しない */
	if (Show_Opt_tag[0])
	    if (!strncmp(String , Show_Opt_tag , strlen(Show_Opt_tag))) {
		fprintf(client_ofp, "%s", String);
		fflush(client_ofp);
		continue;
	    }

	chomp(String);

        /* juman_sentは解析失敗に加え、空行の時にFALSEを返す。
           空行の時は解析をスキップする必要がある。 NACSIS 吉岡 */
        if ((ret = juman_sent()) == FALSE && strlen(String) != 0)
          return 0;
        if (strlen(String) != 0)
          {
            switch(Show_Opt1) {
            case Op_B:  
              print_best_path(client_ofp); fprintf(client_ofp, "EOS\n"); fflush(client_ofp); break;
            case Op_M: 
              print_all_mrph(client_ofp); fprintf(client_ofp, "EOS\n"); fflush(client_ofp); break;
            case Op_P: 
              print_all_path(client_ofp); fprintf(client_ofp, "EOS\n"); fflush(client_ofp); break;
            case Op_BB: 
            case Op_PP: 
              print_homograph_path(client_ofp); fprintf(client_ofp, "EOS\n"); fflush(client_ofp); break;
            default:   break;
            }
          }
#if 0
	fflush(client_ofp);
#endif
    }

    return 0; /* Connection Closed */
}

/***********************************************************************
 * chasen_run()
 ***********************************************************************/
/*
  juman.cのoption_procをサーバー用のoption_proc_for_serverに修正。
  サーバーモード用のオプションの取扱いの削除とエラーハンドリングの部分を修正 
  NACSIS 吉岡
*/

int option_proc_for_server(int argc, char **argv)
{
    int 	i;
    
    Show_Opt1 = Op_BB;
    Show_Opt2 = Op_E2;
    Show_Opt_jumanrc = 0;
    Show_Opt_tag[0] = '\0';
    Show_Opt_debug = 0;
    Rendaku_Opt = 1;
    LowercaseRep_Opt = 1;
    LowercaseDel_Opt = 1;
    Repetition_Opt = 1;
    Onomatopoeia_Opt = 1;
    LongSoundRep_Opt = 1;
    LongSoundDel_Opt = 1;
    
    for (i = 1; i < argc; i++ ) {
	if (argv[i][0] != '-') {
	    fprintf(client_ofp, "Invalid Option !!\nHELP command for more detail.\n");
	    return FALSE;
	}
	else {
	    if ( argv[i][1] == 'b' ) Show_Opt1 = Op_B;
	    else if ( argv[i][1] == 'm' ) Show_Opt1 = Op_M;
	    else if ( argv[i][1] == 'p' ) Show_Opt1 = Op_P;
	    else if ( argv[i][1] == 'B' ) Show_Opt1 = Op_BB;
	    else if ( argv[i][1] == 'P' ) Show_Opt1 = Op_PP;
	    else if ( argv[i][1] == 'f' ) Show_Opt2 = Op_F;
	    else if ( argv[i][1] == 'e' && argv[i][2] == '\0' ) 
					  Show_Opt2 = Op_E;
	    else if ( argv[i][1] == 'e' && argv[i][2] == '2' ) 
					  Show_Opt2 = Op_E2;
	    else if ( argv[i][1] == 'c' ) Show_Opt2 = Op_C;
	    else if ( argv[i][1] == 'E' ) Show_Opt2 = Op_EE;
	    else if ( argv[i][1] == 'i' ) strcpy(Show_Opt_tag, argv[i+1]), i++;
            else if ( argv[i][1] == 'r' ) i++;
	    else if ( argv[i][1] == 'u' ) { /* 未知語処理用のオプションの取扱い */
	        if (i == argc - 1 || argv[i+1][0] == '-' ) { /* no argument */
		    Rendaku_Opt = LowercaseRep_Opt = LowercaseDel_Opt = LongSoundRep_Opt = 
			LongSoundDel_Opt = Repetition_Opt = Onomatopoeia_Opt = 0;
		}
		else { /* 先頭3文字をチェック */
		    i++;
		    if ( argv[i][0] == 'r' && argv[i][1] == 'e' && argv[i][2] == 'n') {
			Rendaku_Opt = 0; /* rendaku */
		    }
		    else if ( argv[i][0] == 'l' && argv[i][1] == 'o' && argv[i][2] == 'w') {
			LowercaseRep_Opt = LowercaseDel_Opt = 0; /* lowercase */
		    }
		    else if ( argv[i][0] == 'l' && argv[i][1] == 'o' && argv[i][2] == 'n') {
			LongSoundRep_Opt = LongSoundDel_Opt = 0; /* long-sound */
		    }
		    else if ( argv[i][0] == 'o' && argv[i][1] == 'n' && argv[i][2] == 'o') {
			Repetition_Opt = Onomatopoeia_Opt = 0; /* onomatopoeia */
		    }
		}
	    }
	    else {
	      fprintf(client_ofp, "Invalid Option !!\nHELP command for more detail.\n");
	      return FALSE;
	    }
	}
    }

    return TRUE;
}

/*
  chasen_run()をjuman_run()に改名
  オプションの取扱いの変更とJUMAN用に変更した関数名への対応
  NACSIS 吉岡
*/
static int juman_run(argc, argv)
     int argc;
     char **argv;
{
    int ret;
  
    if (!option_proc_for_server(argc, argv)) {
 	skip_until_EOf(client_ifp);
	fputs("500 Option Error\n", client_ofp);
	fflush(client_ofp);
	return 1;
    }

    fputs("200 OK\n", client_ofp);
    fflush(client_ofp);

    ret = do_juman();
    fprintf(client_ofp, "%c\n", EOf);
    fflush(client_ofp);

    return ret;
}
/*
 * expand_string()
 */
static void expand_string(str)
    char *str;
{
    char *in, *out;

    for(out = in = str; *in; in++)
    {
	/* quotation */
	if (*in == '"' || *in == '\'')
	  continue;
	if (*in != '\\')
	  *out++ = *in;
	else
	{
	    switch(*++in)
	    {
	      case 'n': *out++ = '\n'; break;
	      case 't': *out++ = '\t'; break;
	      case 'v': *out++ = '\v'; break;
	      case 'b': *out++ = '\b'; break;
	      case 'r': *out++ = '\r'; break;
	      case 'f': *out++ = '\f'; break;
	      case 'a': *out++ = 0x07; break;
	      case '\0': break;
	      default:
		*out++ = *in;
	    }
	}
    }
    *out = '\0';
}

/*
 * split_args()
 */
static int split_args(argbuff, maxargc, argv)
    char *argbuff, **argv;
    int maxargc;
{
    char *arg;
    int argc, i;

    arg = argbuff;
    maxargc--;

    for(argc = 0; argc < maxargc; argc++)
    {
	/* skip space */
	while(*arg == ' ')
	  arg++;
	if(*arg == '\0')
	  break;

	argv[argc] = arg;

	/* find end of arg. */
	while (*arg && *arg != ' ') {
	    /* quoted string */
	    if (*arg == '"' || *arg == '\'') {
		char *s = strchr(arg + 1, *arg);
		if (s != NULL)
		  arg = s + 1;
		else
		  arg += strlen(arg);
	    }
	    /* escaped character */
	    else if (*arg++ == '\\' && *arg)
	      arg++;
	}

	if(*arg == '\0')
	{
	    argc++;
	    break;
	}
	*arg++ = '\0';
    }
    argv[argc] = NULL;

    for(i = 0; i < argc; i++)
      expand_string(argv[i]);

    return argc;
}

/*
 * do_cmd()
 *
 * return:
 * 1 - continue
 * 0 - connection closed
 */
/*
  HELPメッセージの変更とJUMAN用に変更した関数名への対応
  NACSIS 吉岡
*/
static int do_cmd(line)
    char *line;
{
    char *argv[64];
    int argc;

#if 0
    printf("## %s\n",line);
#endif
    argc = split_args(line, 64, argv);

#if 0
    { int i;
      for(i = 0; i < argc; i++)
	printf("##[%d] <%s>\n", i, argv[i]);
      fflush(stdout); }
#endif

    if (!strcasecmp(argv[0], "RUN"))
      return juman_run(argc, argv);
    if (!strcasecmp(argv[0], "RC"))
      return juman_init_rc_server(); 
    if (!strcasecmp(argv[0], "QUIT"))
      return 0;
    if (!strcasecmp(argv[0], "HELP")) {
	static char *message[] = {
	    "200 OK\n",
	    "RUN [options]\n",
	    "  -b          show best path (default)\n",
	    "  -B          show best path including homographs\n",
	    "  -m          show all morphemes\n",
	    "  -p          show all pathes\n",
	    "  -P          show all pathes by -B style\n",
	    "\n",
	    "  -f          show formatted morpheme data (default)\n",
	    "  -c          show coded morpheme data\n",
	    "  -e          show entire morpheme data\n",
	    "  -e2         -e plus semantics data\n",
	    "  -E          -e plus location and semantics data\n",
	    "\n",
	    "  -u          disable unknown word processing\n",
	    "  -i          ignore an input line startig with 'string'\n",
	    "\n",
	    "RC\n",
	    "QUIT\n",
	    "HELP\n",
	    NULL
	};
	char **mes;

	for (mes = message; *mes; mes++)
	  fputs(*mes, client_ofp);
	fflush(client_ofp);

	return 1;
    }

    fprintf(client_ofp, "500 What ?\n");
    fflush(client_ofp);
    return 1;
}

static void cmd_loop()
{
    static char buff[BUFSIZ];

    while(fgets(buff, BUFSIZ, client_ifp))
    {
	chomp(buff);
	if(!buff[0])
	    continue;
	if(!do_cmd(buff))
	    break;
    }

    fprintf(client_ofp, "205 Quit\n");
    fflush(client_ofp);
}

static int open_server_socket(port)
    unsigned short port;
{
    int sfd;
    struct sockaddr_in sin;

#ifdef _WIN32
    WSADATA wsd;
    if (WSAStartup(MAKEWORD(2, 0), &wsd)) { /* use winsock2 */
	cha_perror("WSAStartup");
	return -1;
    }
    if((sfd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0)) == INVALID_SOCKET)
#else
    if((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
#endif
    {
	cha_perror("socket");
#ifdef _WIN32
	WSACleanup();
#endif
	return -1;
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_port        = htons(port);
    sin.sin_family      = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);

#ifdef SO_REUSEADDR
    {
	int on = 1;
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (caddr_t)&on, sizeof(on));
    }
#endif /* SO_REUSEADDR */

    if(bind(sfd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
	cha_perror("bind");
	closesocket(sfd);
#ifdef _WIN32
	WSACleanup();
#endif
	return -1;
    }

    /* Set it up to wait for connections. */
    if(listen(sfd, SOMAXCONN) < 0)
    {
	cha_perror("listen");
	closesocket(sfd);
#ifdef _WIN32
	WSACleanup();
#endif
	return -1;
    }

    return sfd;
}

#ifndef _WIN32
static void sigchld_handler(sig)
    int sig;
{
    int status;

    while(waitpid(-1, &status, WNOHANG) > 0)
      ;
    signal(SIGCHLD, sigchld_handler);
}
#endif

static int sfd = -1;
static void sig_term()
{
    shutdown(client_fd, 2);
    shutdown(sfd, 2);
    exit(0);
}

/***********************************************************************
 * juman_server()
 *
 * return code: exit code
 ***********************************************************************/
/*
  メッセージをJUMAN用への変更とdummy sentenseによるチェックの省略
  NACSIS 吉岡

  windowsでforegroundで走らせる必要があるので，引数foregroundを追加
  2002/06/24 Kiyota
*/
int juman_server(argv, port, foreground)
    char **argv;
    int port;
    int foreground;
{
    int pid, i;
    
    if (!juman_init_rc(Jumanrc_Fileptr)) {	/* rcfile関係の初期化 */
	fprintf(stderr, "error in .jumanrc\n");
	exit(0);
    }
    juman_init_etc(); 	/* 未定義語処理，数詞処理，透過処理等の初期化 */

    /* daemon initialization */
    umask(0);

    /* 
     * 自分自身をforkしてバックグラウンドに移行する
     * (-F オプションつきで起動した場合は移行しない)
     */
#ifndef _WIN32
    if (!foreground) {
      if((i = fork()) > 0)
	return 0;
      else if(i == -1) {
	fprintf(stderr,"jumand: unable to fork new process\n");
	cha_perror("fork");
	return 1;
      }

      if(setsid() == -1)
	cha_perror("Warning: setsid");
    }

    signal(SIGHUP,  SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, sig_term);
    signal(SIGINT,  sig_term);
    signal(SIGQUIT, sig_term);
#endif

    /* make a socket */
    if((sfd = open_server_socket(port)) < 0)
      return 1;

#if 0
    printf("## Ready to connection\n");
    printf("## Parent PID=%d\n", getpid());
#endif

#ifndef _WIN32
    signal(SIGCHLD, sigchld_handler);
#endif

    while(1)
    {
	int pid = 0;

	if((client_fd = accept(sfd, NULL, NULL)) < 0) {
	    if(errno == EINTR)
	      continue;
	    cha_perror("accept");
#ifdef _WIN32
	    WSACleanup();
#endif
	    return 1;
	}

#ifndef _WIN32
	if((pid = fork()) < 0) {
	    cha_perror("fork");
	    sleep(1);
	    continue;
	}
#endif

	if(pid == 0) { /* child */
#ifdef _WIN32
	    int client_fd_osfhandle;
	    if ((client_fd_osfhandle = _open_osfhandle(client_fd, O_RDWR | O_BINARY)) < 0) {
		fprintf(stderr, ";; _open_osfhandle error\n");
		closesocket(sfd);
		WSACleanup();
		return 1;
	    }
	    client_ofp = fdopen(client_fd_osfhandle, "wb+");
	    client_ifp = fdopen(client_fd_osfhandle, "rb+");
#else
	    client_ofp = fdopen(client_fd, "w");
	    client_ifp = fdopen(client_fd, "r");
#endif
	    Cha_stderr = client_ofp;

	    fprintf(client_ofp, "200 Running JUMAN version: %s\n", VERSION);
	    fflush(client_ofp);

	    cmd_loop();

	    shutdown(client_fd, 2);
	    fclose(client_ofp);
	    fclose(client_ifp);
#ifdef _WIN32
	    close(client_fd_osfhandle);
#else
	    closesocket(sfd);
	    close(client_fd);
	    exit(0);
#endif
	}

	/* parent */
	close(client_fd);
    }
    return 0;
}
