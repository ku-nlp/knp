/*====================================================================

			   JUMAN の呼び出し

                                               S.Kurohashi 2000.10.13

    $Id$
====================================================================*/
#include "knp.h"

extern U_CHAR String[];
extern FILE *Jumanrc_Fileptr;
extern int Show_Opt2;

int JumanAlive = 0;
FILE *w2c, *w2p, *rfc, *rfp;

/*==================================================================*/
	       int ReadSentence(char **str, char **ret)
/*==================================================================*/
{
    char *start, *cp;

    if (**str == '\0') return EOF;

    for (cp = start = *str; ; cp++) {
	if (*cp == '\n') {
	    *cp = '\0';
	    *str = cp+1;
	}
	else if (*cp == '\0') {
	    *str = cp;
	}
	else {
	    continue;
	}
	*ret = start;
	/* strcpy(buffer, start); */
	return TRUE;
    }
    return FALSE;
}

/*==================================================================*/
			  void CloseJuman()
/*==================================================================*/
{
    if (JumanAlive) {
	fclose(rfc);
	fclose(w2c);
	wait();
	JumanAlive = 0;
    }
}

/*==================================================================*/
			void CloseJumanChild()
/*==================================================================*/
{
    fclose(rfp);
    fclose(w2p);
}

/*==================================================================*/
			  int PerformJuman()
/*==================================================================*/
{
    int length;

    /* 入力文 (pipe) を読み込むループ */
    while (fgets(String, LENMAX, rfp) != NULL) {
	length = strlen(String);
	if (String[length-1] == '\n') {
	    String[length-1] = '\0';
	}
	juman_sent();
	print_best_path(w2p);
	fputs("EOS\n", w2p);
	fflush(w2p);
    }
}

/*==================================================================*/
			   int ForkJuman()
/*==================================================================*/
{
    int pid;
    int pipei[2], pipeo[2];

    if (pipe(pipei) < 0) {
	return 0;
    }
    if (pipe(pipeo) < 0) {
	close(pipei[0]);
	close(pipei[1]);
	return 0;
    }

    if ((pid = fork()) > 0) {
	/* parent */
	close(pipei[0]);
	close(pipeo[1]);
	/* pipei[1] に書き込み、pipeo[0] から結果を読む */
	w2c = fdopen(pipei[1], "w");
	rfc = fdopen(pipeo[0], "r");
	JumanAlive = 1;
	return pid;
    }
    else if (pid == 0) {
	/* child */
	close(pipei[1]);
	close(pipeo[0]);

	/* JUMAN の初期化 */
	Show_Opt2 = Op_E;
	rewind(Jumanrc_Fileptr);
	if (!juman_init_rc(Jumanrc_Fileptr)) {
	    fprintf(stderr, "error in .jumanrc\n");
	    exit(0);
	}
	juman_init_etc();

	rfp = fdopen(pipei[0], "r");
	w2p = fdopen(pipeo[1], "w");
	PerformJuman();
	CloseJumanChild();
	_exit(0);
    }
    return 0;
}

/*==================================================================*/
	  int GetJumanResult(SENTENCE_DATA *s, char *input)
/*==================================================================*/
{
    char *cp, buffer[DATA_LEN];
    int flag;

    if (!JumanAlive) {
	ForkJuman();
    }

    fputs(input, w2c);
    fputc('\n', w2c);
    fflush(w2c);
    /* 形態素解析結果を rfc から読んで s に入れる
       EOS で TRUE がかえる */
    /* main_analysis(s, rfc); */
    return 1;
}

/*====================================================================
                               END
====================================================================*/
