/*====================================================================

			      PATHS

                                               S.Kurohashi 94. 8.25

    $Id$
====================================================================*/

#define HOMO_FILE	KNP_RULE "/mrph_homo.data"
#define MRPH_FILE	KNP_RULE "/mrph_basic.data"
#define BNST1_FILE	KNP_RULE "/bnst_basic.data"
#define UKE_FILE    	KNP_RULE "/bnst_uke.data"
#define BNST2_FILE	KNP_RULE "/bnst_uke_ex.data"
#define KAKARI_FILE 	KNP_RULE "/bnst_kakari.data"
#define BNST3_FILE	KNP_RULE "/bnst_etc.data"
#define CONT_FILE	KNP_RULE "/context.data"
#define KOOU_FILE  	KNP_RULE "/koou.data"
#define DPND_FILE  	KNP_RULE "/kakari_uke.data"
#define NE_FILE  	KNP_RULE "/ne.data"
#define NE_JUMAN_FILE  	KNP_RULE "/ne-juman.data"
#define CN_PRE_FILE  	KNP_RULE "/compoundnoun-preliminary.data"
#define CN_FILE  	KNP_RULE "/compoundnoun.data"
#define CN_AUX_FILE  	KNP_RULE "/compoundnoun-auxiliary.data"
#define HELPSYS_FILE    KNP_RULE "/mrph_helpsys.data"

#define BGH_DB_NAME	KNP_DICT "/bgh/bgh"
#define SM_DB_NAME	KNP_DICT "/sm/word2code"
#define SM2CODE_DB_NAME	KNP_DICT "/sm/sm2code"
#define SMP2SMG_DB_NAME	KNP_DICT "/sm/smp2smg"
#define SCASE_DB_NAME	KNP_DICT "/scase/scase"
#define IPAL_DB_NAME	KNP_DICT "/ipal/ipal"
#define IPAL_DAT_NAME	KNP_DICT "/ipal/ipal.dat"
#define PROPER_DB_NAME	KNP_DICT "/proper/word"
#define PROPERC_DB_NAME	KNP_DICT "/proper/class"
#define PROPERCASE_DB_NAME	KNP_DICT "/proper/case"
#define RULEV_DIC_FILE	KNP_DICT "/rule/rulev.dic"
#define RULEV_DB_NAME	KNP_DICT "/rule/rulev"
#define RULEP_DIC_FILE	KNP_DICT "/rule/rulep.dic"
#define RULEP_DB_NAME	KNP_DICT "/rule/rulep"

#ifdef GDBM
#define CLAUSE_DB_NAME	KNP_DICT "/clause/clause.gdbm"
#define CLAUSE_CDB_NAME	KNP_DICT "/clause/c_clause.gdbm"
#define CASE_PRED_DB_NAME	KNP_DICT "/case_pred/case_pred.gdbm"
#define OP_DB_NAME	KNP_DICT "/optional_case/optional_case.gdbm"
#define WC_DB_NAME	KNP_DICT "/optional_case/wordcount.gdbm"
#else
#define CLAUSE_DB_NAME	KNP_DICT "/clause/clause.db"
#define CLAUSE_CDB_NAME	KNP_DICT "/clause/c_clause.db"
#define CASE_PRED_DB_NAME	KNP_DICT "/case_pred/case_pred.db"
#define OP_DB_NAME	KNP_DICT "/optional_case/optional_case.db"
#define WC_DB_NAME	KNP_DICT "/optional_case/wordcount.db"
#endif
