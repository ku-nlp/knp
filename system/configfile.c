#include "knp.h"
#include <sys/types.h>
#include <sys/stat.h>

/* $Id$ */

extern FILE *Jumanrc_Fileptr;
extern char Jumangram_Dirname[];
extern int LineNoForError, LineNo;
char Knprule_Dirname[FILENAME_MAX];

RuleVector *RULE;
int CurrentRuleNum = 0;
int RuleNumMax = 0;

/*==================================================================*/
			   void init_knp()
/*==================================================================*/
{
#ifdef  _WIN32
    /* MS Windws のばあいは,juman.ini を見に行くように変更 
     dicfile == gramfile */
    GetPrivateProfileString("juman","dicfile","",Jumangram_Dirname,sizeof(Jumangram_Dirname),"juman.ini");
#else
    CELL *cell1,*cell2;

    LineNo = 0 ;
    Jumangram_Dirname[0]='\0';

    while (!s_feof(Jumanrc_Fileptr))  {
	LineNoForError = LineNo;
	cell1 = s_read(Jumanrc_Fileptr);

	if (!strcmp(DEF_GRAM_FILE, _Atom(car(cell1)))) {
	    if (!Atomp(cell2 = car(cdr(cell1)))) {
		fprintf(stderr, "error in .jumanrc");
		exit(0);
	    } else 
		strcpy(Jumangram_Dirname, _Atom(cell2));
	}
	/* KNP ルールディレクトリ */
	else if (!strcmp(DEF_KNP_DIR, _Atom(car(cell1)))) {
	    if (!Atomp(cell2 = car(cdr(cell1)))) {
		fprintf(stderr, "error in .jumanrc");
		exit(0);
	    }
	    else
		strcpy(Knprule_Dirname, _Atom(cell2));
	}
	/* KNP ルールファイル */
	else if (!strcmp(DEF_KNP_FILE, _Atom(car(cell1)))) {
	    cell1 = cdr(cell1);

	    while (!Null(car(cell1))) {
		if (CurrentRuleNum >= RuleNumMax) {
		    RuleNumMax += RuleIncrementStep;
		    RULE = (RuleVector *)realloc(RULE, sizeof(RuleVector)*RuleNumMax);
		}

		/* デフォルト値設定 */
		(RULE+CurrentRuleNum)->file = (char *)strdup(_Atom(car(car(cell1))));
		(RULE+CurrentRuleNum)->mode = RLOOP_MRM;
		(RULE+CurrentRuleNum)->breakmode = RLOOP_BREAK_NONE;
		(RULE+CurrentRuleNum)->direction = 0;

		cell2 = cdr(car(cell1));

		while (!Null(car(cell2))) {
		    if (!strcmp(_Atom(car(cell2)), "同形多義語")) {
			(RULE+CurrentRuleNum)->type = HomoRuleType;
		    }
		    else if (!strcmp(_Atom(car(cell2)), "形態素")) {
			(RULE+CurrentRuleNum)->type = MorphRuleType;
		    }
		    else if (!strcmp(_Atom(car(cell2)), "文節")) {
			(RULE+CurrentRuleNum)->type = BnstRuleType;
		    }
		    else if (!strcmp(_Atom(car(cell2)), "係り受け")) {
			(RULE+CurrentRuleNum)->type = DpndRuleType;
		    }
		    else if (!strcmp(_Atom(car(cell2)), "呼応")) {
			(RULE+CurrentRuleNum)->type = KoouRuleType;
		    }
		    else if (!strcmp(_Atom(car(cell2)), "固有表現形態素")) {
			(RULE+CurrentRuleNum)->type = NeMorphRuleType;
		    }
		    else if (!strcmp(_Atom(car(cell2)), "固有表現句-PRE")) {
			(RULE+CurrentRuleNum)->type = NePhrasePreRuleType;
		    }
		    else if (!strcmp(_Atom(car(cell2)), "固有表現句")) {
			(RULE+CurrentRuleNum)->type = NePhraseRuleType;
		    }
		    else if (!strcmp(_Atom(car(cell2)), "固有表現句-AUX")) {
			(RULE+CurrentRuleNum)->type = NePhraseAuxRuleType;
		    }
		    else if (!strcmp(_Atom(car(cell2)), "文脈")) {
			(RULE+CurrentRuleNum)->type = ContextRuleType;
		    }
		    else if (!strcmp(_Atom(car(cell2)), "ルールループ先行")) {
			(RULE+CurrentRuleNum)->mode = RLOOP_RMM;
		    }
		    else if (!strcmp(_Atom(car(cell2)), "BREAK")) {
			(RULE+CurrentRuleNum)->breakmode = RLOOP_BREAK_NORMAL;
		    }
		    else {
			fprintf(stderr, "%s is invalid in .jumanrc\n", _Atom(car(cell2)));
			exit(0);
		    }
		    cell2 = cdr(cell2);
		}
		CurrentRuleNum++;
		cell1 = cdr(cell1);
	    }
	}
    }
#endif
}

/*==================================================================*/
		   char *check_filename(char *file)
/*==================================================================*/
{
    /* ルールファイル (*.data) の fullpath を返す関数 */

    char *fullname, *rulename;
    int status;
    time_t data;
    struct stat sb;

    fullname = (char *)malloc_data(strlen(Knprule_Dirname)+strlen(file)+7, "check_filename");
    sprintf(fullname, "%s/%s.data", Knprule_Dirname, file);
    /* dir + filename + ".data" */
    status = stat(fullname, &sb);

    if (status < 0) {
	*(fullname+strlen(fullname)-5) = '\0';
	/* dir + filename */
	status = stat(fullname, &sb);
	if (status < 0) {
	    sprintf(fullname, "%s.data", file);
	    /* filename + ".data" */
	    status = stat(fullname, &sb);
	    if (status < 0) {
		*(fullname+strlen(fullname)-5) = '\0';
		/* filename */
		status = stat(fullname, &sb);
		if (status < 0) {
		    fprintf(stderr, "%s: No such file.\n", fullname);
		    exit(1);
		}
	    }
	}
    }

    data = sb.st_mtime;

    /* ルールファイルとの時間チェック */
    rulename = strdup(fullname);
    *(rulename+strlen(rulename)-5) = '\0';
    strcat(rulename, ".rule");
    status = stat(rulename, &sb);
    if (!status) {
	if (data < sb.st_mtime) {
	    fprintf(stderr, "%s: rule file is newer!\n", fullname);
	}
    }
    free(rulename);

    return fullname;
}

/*====================================================================
				 END
====================================================================*/
