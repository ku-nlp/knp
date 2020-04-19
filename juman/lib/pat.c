#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

/*#define HAVE_MMAP*/
#ifdef HAVE_MMAP
#undef USE_HASH
#include <sys/mman.h>

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#endif

#if defined _WIN32 && ! defined __CYGWIN__
typedef char *	caddr_t;
#endif

#include "juman_pat.h"

/* ハッシュテーブルの宣言 */
#ifdef USE_HASH
th_hash_node hash_array[HASH_SIZE];
#endif

pat_node tree_top[MAX_DIC_NUMBER]; /* 木のねっこ♪ 辞書の数だけ使う */
FILE *dic_file[MAX_DIC_NUMBER]; /* 木のもとデータ(辞書ファイル) */

static struct _dic_t {
  int used;
  int fd;
  off_t size;
  caddr_t addr;
} dicinfo[MAX_DIC_NUMBER];

/******************************************************
* pat_strcmp_prefix --- プレフィクスマッチ
*
* パラメータ
*   s1 --- Prefix String
*   s2 --- 調べられる String
*
* 返し値  成功 1、失敗 0
******************************************************/
static int pat_strcmp_prefix(char *s1, char *s2)
{
  for (;;) {
    if (*s2 == '\t') return 1;
    if (*s1++ != *s2++) return 0;
  }
}

/******************************************************
* pat_strcpy --- 文字列コピー
*
* パラメータ
*   s1, s2
*
* 返し値
******************************************************/
static char *pat_strcpy(char *s1, char *s2)
{
    while (*s1++ = *s2++);
    return s1 - 1;
}


/******************************************************
* pat_init_tree_top --- パトリシア木の根の初期化
*
* パラメータ
*   ptr --- 初期化する木の根へのポインタ
******************************************************/
void pat_init_tree_top(pat_node *ptr) {
  (ptr->il).index = -1; /* インデックスのリスト */
  ptr->checkbit = -1;
  ptr->right = ptr;
  ptr->left = ptr;
}

/****************************************************
* pat_search --- パトリシア木を検索
* 
* パラメータ
*   key --- 検索キー
*   x_ptr --- 検索開始位置(ポインタ)
*   rslt --- 結果を入れる．
* 
* 返し値
*   検索終了位置(ポインタ)
*
****************************************************/
pat_node *pat_search(FILE *f, char *key, pat_node *x_ptr, char *rslt)
{
  pat_node *ptr,*tmp_ptr,*top_ptr = x_ptr,*tmp_x_ptr = x_ptr;
  int in_hash = 0;
  pat_index_list *tmp_l_ptr;
  int i;
  int key_length = strlen(key); /* キーの文字数を数えておく */
  char buffer[50000]; /* 汎用バッファ */
  int totyu_match_len = 0; /* 途中でマッチしたPrefixの文字数 */
  char *r;

  rslt += strlen(rslt);
  r = rslt;

/*  rslt[0] = '\0';*/

  /* OL(pat_search:\n); */
  do {
    ptr = x_ptr;
    /* 敷居ビットならば */
    OL(checkbit:)OI(ptr->checkbit);
    if(ptr->checkbit%SIKII_BIT==0 && ptr->checkbit!=0){ /* 途中単語を探す */
      tmp_x_ptr = ptr;
      do { /* 左部分木の一番左のノードを調べる． */
	tmp_ptr = tmp_x_ptr;
	tmp_x_ptr = tmp_x_ptr->left;
      } while(tmp_ptr->checkbit < tmp_x_ptr->checkbit);

      /* ハッシュをチェック */
      in_hash = hash_check_proc(f,(tmp_x_ptr->il).index,buffer);
      strtok(buffer,"\t"); /* 最初の '\t' を '\0' にする．*/
      /* bufferの先頭の「見出し語」部分だけでマッチングを行なう */
      if(strncmp(key,buffer,ptr->checkbit/8) == 0) { /* 見つけた */
	totyu_match_len = ptr->checkbit/8; /* 途中でマッチしたPrefixの文字数 */
	tmp_l_ptr = &(tmp_x_ptr->il); /* 全リスト要素の取り出し */
	while(tmp_l_ptr != NULL){
	  in_hash = hash_check_proc(f,tmp_l_ptr->index,buffer);
	  r = pat_strcpy(r, buffer);
	  *r++ = '\n';
	  *r = '\0';

	  tmp_l_ptr = tmp_l_ptr->next;
	}
      } else { /* 途中で失敗を発見 */
	return x_ptr;
      }
    }

    /* key の checkbitビット目で左右に振り分け */
    if(pat_bits(key,x_ptr->checkbit,key_length)==1){x_ptr = x_ptr->right;}
    else {x_ptr = x_ptr->left;}

  } while(ptr->checkbit < x_ptr->checkbit);
  

  if(tmp_x_ptr != x_ptr || top_ptr == x_ptr) { /* 終了ノードをチェックする */
    char *s;
    int tmp_len;
    /* ハッシュをチェック */
    in_hash = hash_check_proc(f,(x_ptr->il).index,buffer);

    s = strchr(buffer,'\t'); /* 最初の '\t' を '\0' にする．*/
    *s = '\0';
    tmp_len = s - buffer;/*開始 */

    /* bufferの先頭の「見出し語」部分だけでマッチングを行なう */
    if(strncmp(key,buffer,tmp_len) == 0){ /* いきどまり単語のPrefixチェック */
      if(totyu_match_len != key_length){ /* 新登場の単語か否かのチェック */
	tmp_l_ptr = &(x_ptr->il); /* 全リスト要素の取り出し */
	while(tmp_l_ptr != NULL){
	  in_hash = hash_check_proc(f,tmp_l_ptr->index,buffer);
	  r = pat_strcpy(r, buffer);
	  *r++ = '\n';
	  *r = '\0';

	  tmp_l_ptr = tmp_l_ptr->next;
	}
      }
    }
  }

  return x_ptr;
}


/****************************************************
* pat_search_exact --- パトリシア木を検索(exact match)
* 
* パラメータ
*   key --- 検索キー
*   x_ptr --- 検索開始位置(ポインタ)
*   rslt --- 結果を入れる．
* 
* 返し値
*   検索終了位置(ポインタ)
****************************************************/
pat_node *pat_search_exact(FILE *f, char *key, pat_node *x_ptr, char *rslt)
{
  pat_node *ptr,*tmp_ptr,*top_ptr = x_ptr,*tmp_x_ptr = x_ptr;
  pat_index_list *tmp_l_ptr;
  int in_hash;
  int i;
  int key_length = strlen(key); /* キーの文字数を数えておく */
  char buffer[50000]; /* 汎用バッファ */
  char *r;

  rslt += strlen(rslt);
  r = rslt;

  /*  printf("##");*/
  do {
    ptr = x_ptr;
    /* key の checkbitビット目で左右に振り分け */
    if(pat_bits(key,x_ptr->checkbit,key_length)==1){x_ptr = x_ptr->right;}
    else {x_ptr = x_ptr->left;}

  } while(ptr->checkbit < x_ptr->checkbit);

  /* ファイルから取って来る */
  in_hash = hash_check_proc(f,(x_ptr->il).index,buffer);
  /*buffer = get_line(f,x_ptr->il_ptr->index);*/

  strtok(buffer,"\t"); /* 最初の '\t' を '\0' にする．*/

  /* bufferの先頭の「見出し語」部分だけでマッチングを行なう */
  if(strcmp(key,buffer) == 0){ /* いきどまり単語のチェック */
    tmp_l_ptr = &(x_ptr->il); /* 全リスト要素の取り出し */
    while(tmp_l_ptr != NULL){
      in_hash = hash_check_proc(f,tmp_l_ptr->index,buffer);
      r = pat_strcpy(r, buffer);
      *r++ = '\n';
      *r = '\0';

      tmp_l_ptr = tmp_l_ptr->next;
    }
  }

  return x_ptr;
}

/****************************************************
* pat_search4insert --- 挿入用に検索
* 
* パラメータ
*   key --- 検索キー
*   x_ptr --- 検索開始位置(ポインタ)
* 
* 返し値
*   検索終了位置(ポインタ)
*
* メモ
*   大域変数 prefix_str の指す先にプレフィックス文字列を入れる。
****************************************************/
pat_node *pat_search4insert(char *key, pat_node *x_ptr)
{
  pat_node *ptr,*tmp_ptr,*tmp_x_ptr;
  int checked_char = 0; /* 何文字目までチェックしたか patrie 960919 */
  int key_length = strlen(key); /* キーの文字数を数えておく */

  do {
    ptr = x_ptr;
    /* key の checkbitビット目で左右に振り分け */
    if(pat_bits(key,x_ptr->checkbit,key_length)==1){
      x_ptr = x_ptr->right; OL(R);}
    else {x_ptr = x_ptr->left; OL(L);}
  } while(ptr->checkbit < x_ptr->checkbit);
  OL(\n);
  return x_ptr;
}


/****************************************************
* pat_insert --- パトリシア木にデータを挿入
* 
* パラメータ
*   f --- ファイル
*   line --- データ(挿入キーと内容が区切り文字で区切られている構造)
*   index --- データのファイル上のインデックス
*   x_ptr --- 挿入のための検索の開始位置
*   kugiri --- キーと内容の区切り文字
* 
* 返し値
*   無し!
****************************************************/
void pat_insert(FILE *f,char *line, long index, pat_node *x_ptr, char *kugiri)
{
  pat_node *t_ptr, *p_ptr, *new_ptr;
  int diff_bit;
  int i;
  pat_index_list *new_l_ptr, *tmp_l_ptr, *mae_wo_sasu_ptr = NULL;
  int in_hash;
  int buffer_length;
  int key_length;
  char key[1000];
  char buffer[50000]; /* 汎用バッファ */

  OL(line:)OS(line);
  strcpy(key,line);
  strtok(key,kugiri);  /* 最初の区切り文字を '\0' にする．*/
  key_length = strlen(key); /* キーの文字数を数えておく */

  OL(key:)OS(key);

  /* キーの探索 */
  t_ptr = (pat_node*)pat_search4insert(key,x_ptr);

  if((t_ptr->il).index >= 0) {
    /* ハッシュをチェック */
    in_hash = hash_check_proc(f,(t_ptr->il).index,buffer);

    if(strncmp(key,buffer,strlen(key)) == 0){ /* キーが一致 */
      /* printf("%s: キーが一致するものがある\n",buffer);fflush(stdout); */

      tmp_l_ptr = &(t_ptr->il);

      while(tmp_l_ptr !=NULL){
	in_hash = hash_check_proc(f,tmp_l_ptr->index,buffer);
	if(strcmp(buffer,line)==0){
	  /* 全く同じのがあるので挿入せずにリターン */
/*	  printf("%s: 全く同じのがあるので無視\n",buffer);*/
	  return;
	}
	mae_wo_sasu_ptr = tmp_l_ptr;
	tmp_l_ptr = tmp_l_ptr->next;
      }  /* この時点で tmp_l_ptr はリストの末尾を指す */

      /* 既にあるキーに内容をさらに挿入する */
      new_l_ptr = (pat_index_list*)malloc_pat_index_list(); /* indexのlist */
      new_l_ptr->index = index;
      new_l_ptr->next = NULL;
      mae_wo_sasu_ptr->next = new_l_ptr;

      return;
    } else { /* キーが一致しなかった場合 buffer にその一致しなかったキー */
    }
  } else { /* データの無いノードに落ちた場合: 最初にデータをいれたとき */
    *(buffer) = 0;*(buffer+1) = '\0';
  }


  /* 挿入キーと衝突するキーとの間で最初に異なる bit の位置(diff_bit)を求める */
  buffer_length = strlen(buffer);
  for(diff_bit=0; pat_bits(key,diff_bit,key_length) == pat_bits(buffer,diff_bit,buffer_length); diff_bit++)
    ;/* 空文 */

  OL(diff_bit:)OI(diff_bit);

  /* キーを置く位置(x_ptr)を求める。 */
  do {
    p_ptr = x_ptr;
    /* key の checkbitビット目で左右に振り分け */
    if(pat_bits(key,x_ptr->checkbit,key_length)==1) {x_ptr = x_ptr->right;}
    else {x_ptr = x_ptr->left;}
  } while((x_ptr->checkbit < diff_bit)&&(p_ptr->checkbit < x_ptr->checkbit));

  /* 挿入するノードを生成しキー・検査ビット等を設定する。 */
  new_ptr = (pat_node*)malloc_pat_node(); /* ノード本体 */
  new_ptr->checkbit = diff_bit; /* チェックビット */
  (new_ptr->il).index = index;
  (new_ptr->il).next = NULL;

  /* 子節と親節を設定する。 */
  /* ビットが1なら右リンクがキーのある位置を指す。0なら左リンク。 */
  if(pat_bits(key,new_ptr->checkbit,key_length)==1){
    new_ptr->right = new_ptr; new_ptr->left = x_ptr;
  } else {new_ptr->left = new_ptr; new_ptr->right = x_ptr;}
  /* ビットが1なら、親の右につなぐ。0なら左。 */
  if(pat_bits(key,p_ptr->checkbit,key_length)==1) p_ptr->right = new_ptr;
  else p_ptr->left = new_ptr;

  return;
}


/****************************************************
* pat_bits --- 文字列中の指定された位置のビットを返す
* 
* パラメータ
*   string --- 文字列
*   cbit --- 指定された位置。文字列全体を一つのビット列と考え、
*           先頭(左)bitから 0,1,2,3... で指定する。
*   len --- 文字列の長さ．strlenをいちいちやってたんじゃ大変だから 900918
*
* 返し値
*   0,1(ビット),2(文字列の長さが指定された位置より大きいとき)
****************************************************/
int pat_bits(char *string, int cbit, int len)
{
  int moji_idx = cbit / 8; /* 指定された位置が何文字目か (for DEBUG)*/
  char moji = *(string+moji_idx); /* その文字 */
  int idx_in_moji = cbit % 8; /* その文字の何ビット目か */
  if(cbit == -1) return 1; /* トップノードのときは1を返す(topからは必ず右) */
  if(len-1 < moji_idx) return 0;  /* 文字列の長さ < 指定された位置のチェック */
  return(((moji << idx_in_moji) & 0x80) >> 7); /* 0 or 1 を返す。 */
}



/****************************************************
* hash_check_proc --- インデックスでハッシュを引く
* 
* パラメータ
*   index --- インデックス
* 
* 返し値  ハッシュになければファイルから取る．
*         あったら文字列先頭ポインタ，なければ NULL ( 不要か? )
****************************************************/
int hash_check_proc(FILE *f, long index, char *buf) {
  char *data,key[40];
  long num_of_deleted = 0; /* 消された数 */
  int i;

  /* キャッシュ無しの場合 */
#ifndef USE_HASH
  strcpy(buf, get_line(f,index));
  return(0);
#else
  if((data = th_hash_out( hash_array, HASH_SIZE, index, f)) == NULL) {
    strcpy(buf, get_line(f,index)); /* なければファイルから取る */

    th_hash_in(hash_array,HASH_SIZE,index,buf,f);

    return(0);
  } else {
    strcpy(buf,data); /* あれば用いる */
    return(1);
  }
#endif
}


/****************************************************
* get_line --- ファイルの pos 文字目から \n まで読む
* 
* パラメータ
*   f --- 読むファイル
*   pos --- 読み込み始める位置
*   buf --- 読み込むバッファ
* 
* 返し値
*   文字数(strlen方式) 
*   -1 : 失敗
****************************************************/
char *get_line(FILE *f, long pos){
  int i = 0, j = 0, ch, ffd = fileno(f);
#ifdef HAVE_MMAP
  static int oldf = -1;
  static caddr_t addr;
  static off_t size;
  struct stat st;
#endif

#ifdef HAVE_MMAP
  if (oldf != ffd){
    for (i = 0; i < MAX_DIC_NUMBER; i++){
      if (ffd == dicinfo[i].fd && dicinfo[i].used){
	oldf = dicinfo[i].fd;
	addr = dicinfo[i].addr;
	size = dicinfo[i].size;
	break;
      }
      if (dicinfo[i].used == 0){
	dicinfo[i].fd   = ffd;
	dicinfo[i].used = 1;
	fstat(dicinfo[i].fd, &st);
	dicinfo[i].size = size = st.st_size;
	dicinfo[i].addr = addr = mmap(NULL, dicinfo[i].size, PROT_READ,
				      MAP_PRIVATE, dicinfo[i].fd, 0);
	break;
      }
    }
    if (i == MAX_DIC_NUMBER){
      exit(1);
    }
    oldf = ffd;
  }

  if (pos >= size)
    return NULL;

#if 1
  return addr + pos;
#else
#if 1
  {
      char *b = buf;
      char *a = addr + pos;
      i = 0;
      while (*a && *a != '\n') {
	  *b++ = *a++;
	  i++;
      }
      *b = '\0';
  }
#else
  for (i = 0; addr[pos+i] && addr[pos+i] != '\n'; i++)
    buf[i] = addr[pos+i];
  buf[i] = 0;
#endif

  return i+1;
#endif

#else
    if(fseek(f, pos, 0) == 0){
      static char buf[2000];
      if(NULL == fgets(buf,sizeof(buf),f))
	return NULL;
      return buf;
    }
    else return NULL; /* seek 失敗 */
#endif
}


/****************************************************
* show_pat --- パトリシア木データを出力
*
* パラメータ
*   top_ptr --- 検索開始ノードの位置(ポインタ)
*   out_to --- 出力先(stdoutやファイル)
* 
* 返し値
*   無し。パトリシア木データを出力。
****************************************************/
void show_pat(pat_node *top_ptr, FILE *out_to, char *prefix)
{
#if 0
  long idx = -1;
  pat_index_list *t_ptr;
  char word[200];
  char pftmp[200];
  char prefix_keep[200];

  word[0] = '\0';

  strcpy(prefix_keep,prefix);

  OL(-------\n);
  OL(prefix:)OS(prefix);
  OL(<checkbit>)OI(top_ptr->checkbit);

  OL(## 左\n)
  /* 敷居ビットのとき */
  if(top_ptr->checkbit % SIKII_BIT == 0 && top_ptr->checkbit != 0){
    strcpy(word, get_line(dic_file[0],top_ptr->left->il_ptr->index));
    strtok(word,"\t");
    OL(SIKIIbitProcess\n)
    OL(SIKIIword:)OS(word);
    strcpy(pftmp,(word+strlen(prefix)));
    OL(keep:)OS(pftmp);

/*
    printf("#@# %i\n",strlen(word));
    printf("### %i\n",strlen(pftmp));

    top_ptr->left->str = (char*)malloc(strlen(word)+1);
    strcpy(top_ptr->left->str,word);
*/
    top_ptr->left->str = (char*)malloc(strlen(pftmp)+1);
    strcpy(top_ptr->left->str,pftmp);

    strcat(prefix,pftmp);

    OS(pftmp);

  } else {
    /* 左右の Subtree の処理。葉っぱでなければ再帰。*/
    if(top_ptr->checkbit < top_ptr->left->checkbit){
      show_pat(top_ptr->left,out_to,prefix);}
    else {
      if(top_ptr->left->il_ptr != NULL) {
	strcpy(word, get_line(dic_file[0],top_ptr->left->il_ptr->index));
	strtok(word,"\t");
	OL(word:)OS(word);
	strcpy(pftmp,(word+strlen(prefix)));
	OL(keep:)OS(pftmp);

/*
    printf("#@# %i\n",strlen(word));
    printf("### %i\n",strlen(pftmp));

    top_ptr->left->str = (char*)malloc(strlen(word)+1);
    strcpy(top_ptr->left->str,word);
*/
	top_ptr->left->str = (char*)malloc(strlen(pftmp)+1);
	strcpy(top_ptr->left->str,pftmp);

	OS(word);
      }
    }

  }

  OL(## 右\n)
  if(top_ptr->checkbit < top_ptr->right->checkbit){
    show_pat(top_ptr->right,out_to,prefix);}
  else {
    if(top_ptr->right->il_ptr != NULL) {
      strcpy(word, get_line(dic_file[0],top_ptr->right->il_ptr->index));
      strtok(word,"\t");
      OL(word:)OS(word);
      strcpy(pftmp,(word+strlen(prefix)));
      OL(keep:)OS(pftmp);

/*
    printf("#@# %i\n",strlen(word));
    printf("### %i\n",strlen(pftmp));

    top_ptr->left->str = (char*)malloc(strlen(word)+1);
    strcpy(top_ptr->left->str,word);
*/
      top_ptr->right->str = (char*)malloc(strlen(pftmp)+1);
      strcpy(top_ptr->right->str,pftmp);

      OS(word);
    }
  }

  OL(---------back-------\n);

  strcpy(prefix,prefix_keep);
  return;
#endif
}

