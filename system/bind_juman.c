/*====================================================================

			   JUMAN の呼び出し

                                               S.Kurohashi 2000.10.13

    $Id$
====================================================================*/
#ifdef INTEGRATE_JUMAN
#include "knp.h"

extern U_CHAR String[];
extern FILE *Jumanrc_Fileptr;
extern int Show_Opt1;
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
    int status;
    if (JumanAlive) {
	fclose(rfc);
	fclose(w2c);
	wait(&status);
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
			 void PerformJuman()
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
	print_homograph_path(w2p);
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
	Show_Opt1 = Op_BB;
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
		    FILE *JumanSentence(FILE *fp)
/*==================================================================*/
{
    char buffer[LENMAX];

    if (!JumanAlive) {
	ForkJuman();
    }

    buffer[LENMAX-1] = GUARD;
    if (fgets(buffer, LENMAX, fp) == NULL) return NULL;
    if (buffer[LENMAX-1] != GUARD) {
	buffer[LENMAX-1] = '\0';
	fprintf(stderr, "Too long input string (%s).\n", String);
	return NULL;
    }

    fputs(buffer, w2c);
    fflush(w2c);
    return rfc;
}

/*==================================================================*/
	   int ParseSentence(SENTENCE_DATA *s, char *input)
/*==================================================================*/
{
    /* この関数を使うときには s を初期化しておく必要がある */

    int OptInputBackup;

    if (!JumanAlive) {
	ForkJuman();
    }

    fputs(input, w2c);
    fputc('\n', w2c);
    fflush(w2c);

    OptInputBackup = OptInput;
    OptInput = OPT_RAW;

    /* 形態素解析結果を rfc から読んで s に入れる
       EOS で TRUE がかえる */
    one_sentence_analysis(s, rfc);
#ifdef DEBUGMORE
    print_result(s);
    fputc('\n', Outfp);
#endif

    OptInput = OptInputBackup;

    return 1;
}

#endif

/*====================================================================
                               END
====================================================================*/
