#define DEBUGn

#ifdef DEBUG
#define OI(x) {printf("< %d >\n",x);fflush(stdout);} /* Output Integer */ /*デバグに便利*/
#define OS(x) {printf("[ %s ]\n",x);fflush(stdout);} /* Output String */
#define OS2(x) {printf("%s\n",x);fflush(stdout);} /* Output String */
#define OM(x) {printf("Message: " #x "\n");fflush(stdout);} /* Output Message */
#define OL(x) {printf(#x);fflush(stdout);} /* Output Label */
#else
#define OI(x)  /* */
#define OS(x)  /* */
#define OS2(x) /* */
#define OM(x)  /* */
#define OL(x)  /* */
#endif

/* たつをハッシュ */
#ifdef USE_HASH
#include "t-hash.h"
#endif

/* インデックス用のリスト型の定義 */
typedef struct pat_index_list {
  struct pat_index_list *next; /* つぎ */
  long index; /* ファイルのインデックス */
} pat_index_list;

/* ノードのデータ構造の定義 */
typedef struct pat_node {
  pat_index_list il; /* インデックスのリスト */
  short checkbit; /* チェックするビットの指定。(何番目のビット?) */
#if 0
  char *str; /* トライとしてつかうために最低限必要な文字列を保存 960919 */
#endif
  struct pat_node *right; /* 右へまいりま〜す */
  struct pat_node *left; /* 左へまいりま〜す */
} pat_node;


#define HASH_SIZE 131071 /* 107653  ハッシュテーブルのサイズ 1162213*/


#ifndef MAX_DIC_NUMBER 
#define MAX_DIC_NUMBER 5 /* 同時に使える辞書の数の上限 (JUMAN) */
#endif
extern pat_node tree_top[MAX_DIC_NUMBER]; /* 木のねっこ♪ 辞書の数だけ使う */
extern FILE *dic_file[MAX_DIC_NUMBER]; /* 木のもとデータ(辞書ファイル) */

/* 文字と文字の区切りは何ビット目? (8 or 16(EUC-JP, Shift_JIS) or 24(UTF-8) */
#if defined(IO_ENCODING_EUC) || defined(IO_ENCODING_SJIS)
#define SIKII_BIT 16
#else
#define SIKII_BIT 24
#endif

extern char line[50000]; /* 入力行 */
extern FILE *out_file, *in_file; /* セーブファイル・ロードファイル */
extern char  inkey[10000]; /* 検索・挿入キー */

/*** JUMAN辞書引き関連 ***/
extern int number_of_tree; /* 使用する辞書(パト木)の数 */

/*** get_item()用 ***/
extern char partition_char; /* 区切り文字 */
extern int column; /* 何コラム目か */

/**************************
 * 関数のプロトタイプ宣言 *
 **************************/ 
/* pat.c */
extern void pat_init_tree_top(pat_node*); /* パトリシア木の根の初期化 */
extern pat_node *pat_search(FILE*,char*,pat_node*,char*); /* パトリシア木で検索 */
extern pat_node *pat_search4insert(char*,pat_node*); /* 挿入用検索 */
extern void pat_insert(FILE*,char*,long,pat_node*,char*); /* パトリシア木に挿入 */
extern int pat_bits(char*,int,int); 
         /* 文字列中の指定された位置のビットを返す */
         /* 960918  内部でstrlrn()をやるのは無駄であることが判明 */
extern void show_pat(pat_node*,FILE*,char*); /* パトリシア木データを出力 */
extern char *get_line(FILE*,long); /* 指定された場所から'\n'まで読む */

/* file.c */
extern void com_s(char*,pat_node*); /* セーブ関連 */
extern void save_pat(pat_node*);
extern void eputc(unsigned char, int);
extern void com_l(char*,pat_node*); /* ロード関連 */
extern pat_node *load_anode(pat_node*);
extern unsigned char egetc(int);
extern void dummy(void);
extern pat_node *malloc_pat_node(void); /* Matomete malloc */
extern pat_index_list *malloc_pat_index_list(void); /* Matomete malloc */

/* morph.c */
extern void jisyohiki(char*,pat_node*); /* 辞書引き */
extern void insert_dic_data(FILE*,pat_node*,char*); /* 辞書データを挿入*/

/************************************************************************
* 
* pat --- パトリシア木の探索と挿入
* 
* 作者: たつを(tatuo-y@is.aist-nara.ac.jp)
* 
* 目的: パトリシア木の探索と挿入を行う
* 
* 参考文献: 
*   アルゴリズムの理解のために文献[1]を参照した。C言語での実装は
*   文献[2]のプログラムを参考にした。
* [1] R. Sedgewick 著 野下浩平、星守、佐藤創、田口東 共訳
*     アルゴリズム (Algorithms) 原書第2版 第2巻 探索・文字列・計算幾何
*     近代科学社,1992. (B195-2,pp.68-72)
* [2] 島内剛一、有澤誠、野下浩平、浜田穂積、伏見正則 編集委員
*     アルゴリズム辞典
*     共立出版株式会社,1994. (D74,pp.624-625)
* 
* 履歴:
*   1996/04/09  動く! (ただし扱えるデータの最大長は8bit。[2]を模倣。)
*           10  出力ルーチンを再帰に改良。文字列データ対応(最大長無制限)。
*           30  セーブ/ロード機能。ノードのデータ構造にID番号を追加(仮)。
*         5/06  部分木の全データ出力処理。
*         6/11  JUMANの辞書引き用に改造．
*           21  連想配列を導入(INDEXをキャッシュする)
*         7/01  複数の辞書ファイル(パト木)から検索できるようにした．
* 
* メモ: JUMANの辞書引きに利用する
* 
************************************************************************/
