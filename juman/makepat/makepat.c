#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <juman.h>

/* ハッシュテーブルの宣言 */
#ifdef USE_HASH
th_hash_node hash_array[HASH_SIZE];
#endif

pat_node tree_top[MAX_DIC_NUMBER]; /* 木のねっこ♪ 辞書の数だけ使う */
FILE *dic_file[MAX_DIC_NUMBER]; /* 木のもとデータ(辞書ファイル) */

char line[50000]; /* 入力行 */
FILE *out_file, *in_file; /* セーブファイル・ロードファイル */
char  inkey[10000]; /* 検索・挿入キー */
extern char	*ProgName;

/*** JUMAN辞書引き関連 ***/
int number_of_tree = 0; /* 使用する辞書(パト木)の数 */


/****************************************************
*                      メイン                       *
****************************************************/
main(int argc, char *argv[])
{
  char comm;
  int i;
  pat_node *tmp;
  char kugiri[2]; /* 区切り文字 */
  char rslt[50000];
  char	CurPath[FILENAME_MAX];
  char	JumanPath[FILENAME_MAX];

  ProgName = argv[0];

#ifdef USE_HASH
  th_hash_initialize(hash_array,HASH_SIZE);
#endif

  sprintf(kugiri,"\t"); /* 区切り文字のデフォルトはタブ */

  /* ファイルから挿入 */
  getpath(CurPath, JumanPath);
  sprintf(inkey, "%s%s", CurPath, DICFILE);

  printf("File Name \"%s\"\n",inkey);
#ifdef _WIN32   
  dic_file[number_of_tree] = fopen(inkey,"rb");
#else
  dic_file[number_of_tree] = fopen(inkey,"r");
#endif   
  OL(Tree No.);OI(number_of_tree);
  (void)pat_init_tree_top(&tree_top[number_of_tree]);
  (void)insert_dic_data(dic_file[number_of_tree],&tree_top[number_of_tree],kugiri);
  number_of_tree++;

  /* 木のセーブ */
  sprintf(inkey, "%s%s", CurPath, PATFILE);
  (void)com_s(inkey,&tree_top[0]);

  /* 終了 */
/*      th_show_hash(hash_array,HASH_SIZE);*/
  printf("QUIT\n");
  exit(0);
}

/****************************************************
* insert_dic_data
* 
* パラメータ
*   string --- 文字列
****************************************************/
void insert_dic_data(FILE *f, pat_node *x_ptr, char *kugiri)
{
  long i = 0; long entry_ctr = 0;
  int len = 0;

  char corpus_buffer[50000]; /* コーパスからのデータを格納するバッファ */
  char *c;

  while ((c = get_line(f, i)) != NULL) {
    strcpy(corpus_buffer, c);
    len = strlen(corpus_buffer) + 1;
    OL(---------------------\n)
    OL(INSERT:)OI(i)OS(corpus_buffer);
    (void)pat_insert(f,corpus_buffer, i, x_ptr, kugiri); /* 挿入*/
    i += len;
    entry_ctr++;
    if(entry_ctr % 1000 == 0){
      printf(".");
      if(entry_ctr % 20000 == 0) printf(" %ld\n",entry_ctr);
      fflush(stdout);
    }
  }
  printf("\n");
  printf("## %ld entry  %ld th char\n",entry_ctr,i);
  /* modified by S. Sato, 2010.8.12; for MacOS 10.6.x; %d -> %ld */

  return;
}
