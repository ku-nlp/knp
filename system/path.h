/*====================================================================

			      PATHS

                                               S.Kurohashi 94. 8.25

    $Id$
====================================================================*/

#define IPAL_DAT_NAME	KNP_DICT "/cf/ipal.dat"
#define RULEV_DIC_FILE	KNP_DICT "/rule/rulev.dic"
#define RULEP_DIC_FILE	KNP_DICT "/rule/rulep.dic"

#ifdef GDBM
#define BGH_DB_NAME	KNP_DICT "/bgh/bgh"
#define SM_DB_NAME	KNP_DICT "/sm/word2code"
#define SM2CODE_DB_NAME	KNP_DICT "/sm/sm2code"
#define SMP2SMG_DB_NAME	KNP_DICT "/sm/smp2smg"
#define SCASE_DB_NAME	KNP_DICT "/scase/scase"
#define IPAL_DB_NAME	KNP_DICT "/cf/ipal"
#define PROPER_DB_NAME	KNP_DICT "/proper/word"
#define PROPERC_DB_NAME	KNP_DICT "/proper/class"
#define PROPERCASE_DB_NAME	KNP_DICT "/proper/case"
#define RULEV_DB_NAME	KNP_DICT "/rule/rulev"
#define RULEP_DB_NAME	KNP_DICT "/rule/rulep"

#define CLAUSE_DB_NAME	KNP_DICT "/clause/clause.gdbm"
#define CLAUSE_CDB_NAME	KNP_DICT "/clause/c_clause.gdbm"
#define CASE_PRED_DB_NAME	KNP_DICT "/case_pred/case_pred.gdbm"
#define OP_DB_NAME	KNP_DICT "/optional_case/optional_case.gdbm"
#define OP_SM_DB_NAME	KNP_DICT "/optional_case/optional_case_sm.gdbm"
#define WC_DB_NAME	KNP_DICT "/optional_case/wordcount.gdbm"
#else
#define BGH_DB_NAME	KNP_DICT "/bgh/bgh.db"
#define SM_DB_NAME	KNP_DICT "/sm/word2code.db"
#define SM2CODE_DB_NAME	KNP_DICT "/sm/sm2code.db"
#define SMP2SMG_DB_NAME	KNP_DICT "/sm/smp2smg.db"
#define SCASE_DB_NAME	KNP_DICT "/scase/scase.db"
#define IPAL_DB_NAME	KNP_DICT "/cf/ipal.db"
#define PROPER_DB_NAME	KNP_DICT "/proper/word.db"
#define PROPERC_DB_NAME	KNP_DICT "/proper/class.db"
#define PROPERCASE_DB_NAME	KNP_DICT "/proper/case.db"
#define RULEV_DB_NAME	KNP_DICT "/rule/rulev.db"
#define RULEP_DB_NAME	KNP_DICT "/rule/rulep.db"

#define CLAUSE_DB_NAME	KNP_DICT "/clause/clause.db"
#define CLAUSE_CDB_NAME	KNP_DICT "/clause/c_clause.db"
#define CASE_PRED_DB_NAME	KNP_DICT "/case_pred/case_pred.db"
#define OP_DB_NAME	KNP_DICT "/optional_case/optional_case.db"
#define OP_SM_DB_NAME	KNP_DICT "/optional_case/optional_case_sm.db"
#define WC_DB_NAME	KNP_DICT "/optional_case/wordcount.db"
#endif
