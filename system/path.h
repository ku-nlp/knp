/*====================================================================

			      PATHS

                                               S.Kurohashi 94. 8.25

    $Id$
====================================================================*/

#define CF_DAT_NAME	"cf/cf.dat"
#define RULEV_DIC_FILE	"rule/rulev.dic"
#define RULEP_DIC_FILE	"rule/rulep.dic"

#define BGH_DB_NAME	"bgh/bgh"
#define SM2BGHCODE_DB_NAME	"bgh/sm2code"
#define SM_DB_NAME	"sm/word2code"
#define SM2CODE_DB_NAME	"sm/sm2code"
#define CODE2SM_DB_NAME	"sm/code2sm"
#define SMP2SMG_DB_NAME	"sm/smp2smg"
#define SCASE_DB_NAME	"scase/scase"
#define CF_DB_NAME	"cf/cf"
#define PROPER_DB_NAME	"proper/word"
#define PROPERC_DB_NAME	"proper/class"
#define PROPERCASE_DB_NAME	"proper/case"
#define RULEV_DB_NAME	"rule/rulev"
#define RULEP_DB_NAME	"rule/rulep"
#define NOUN_DB_NAME	"rsk/rsk"

#ifdef GDBM
#define CLAUSE_DB_NAME	"clause/clause.gdbm"
#define CLAUSE_CDB_NAME	"clause/c_clause.gdbm"
#define CASE_PRED_DB_NAME	"case_pred/case_pred.gdbm"
#define OP_DB_NAME	"optional_case/optional_case.gdbm"
#define OP_SM_DB_NAME	"optional_case/optional_case_sm.gdbm"
#define WC_DB_NAME	"optional_case/wordcount.gdbm"
#else
#define CLAUSE_DB_NAME	"clause/clause"
#define CLAUSE_CDB_NAME	"clause/c_clause"
#define CASE_PRED_DB_NAME	"case_pred/case_pred"
#define OP_DB_NAME	"optional_case/optional_case"
#define OP_SM_DB_NAME	"optional_case/optional_case_sm"
#define WC_DB_NAME	"optional_case/wordcount"
#endif
