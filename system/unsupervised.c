/*====================================================================

		      unsupervised learning 関連

                                             S.Kurohashi 1999. 4. 1

    $Id$
====================================================================*/
#include "knp.h"

/* Server Client extention */
extern FILE  *Infp;
extern FILE  *Outfp;
extern int   OptMode;

void CheckCandidates() {
    int i, j;
    TOTAL_MGR *tm = &Best_mgr;
    char buffer[DATA_LEN], buffer2[256];

    /* 各文節ごとにチェック用の feature を与える */
    for (i = 0; i < Bnst_num; i++)
	if (tm->dpnd.check[i].num != -1) {
	    /* 係り側 -> 係り先 */
	    sprintf(buffer, "Check:%2d %2d", i, tm->dpnd.head[i]);
	    for (j = 0; j < tm->dpnd.check[i].num; j++) {
		/* 候補ども */
		sprintf(buffer2, ":%d", tm->dpnd.check[i].pos[j]);
		if (strlen(buffer)+strlen(buffer2) >= DATA_LEN) {
		    fprintf(stderr, "Too long string <$s> in CheckCandidates.\n", buffer);
		    return;
		}
		strcat(buffer, buffer2);
	    }
	    assign_cfeature(&(bnst_data[i].f), buffer);
	}
}
