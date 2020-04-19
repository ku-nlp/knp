#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#ifdef HAVE_FCNTL_H
#include 	<fcntl.h>
#endif

#ifdef HAVE_STDLIB_H
#include        <stdlib.h>
#endif

#include "juman_pat.h"
#ifndef _WIN32
#define O_BINARY (0)
#endif

int fd_pat; /* パト木のセーブ/ロード用ファイルディスクリプタ */

/******************************************************
* com_l --- 木のロード
*  by 米沢恵司(keiji-y@is.aist-nara.ac.jp)
*
* パラメータ、返し値
*   なし
******************************************************/
void com_l(char *fname_pat, pat_node *ptr){
/*  fprintf(stderr, "# Loading pat-tree \"%s\" ... ",fname_pat); */
  if ((fd_pat = open(fname_pat, O_RDONLY|O_BINARY)) == -1) {
    fprintf(stderr, "ファイル %s がオープン出来ません。\n",fname_pat);
    exit(1);
  }
  OL(fd_pat)OI(fd_pat)
  ptr->right = load_anode(ptr);
  close(fd_pat);
/*  fprintf(stderr,"done.\n"); */
}


/****************************************************
* load_anode --- パトリシア木をロード
*  by 米沢恵司(keiji-y@is.aist-nara.ac.jp)
*
* パラメータ
*   in --- 入力先ファイル
*   p_ptr --- このノードが外部接点であった時にインデックスを格納する場所
*             内部接点であったときは、このポインタは右の子に渡される。
*
* アルゴリズム
*   チェックビットを読み込んだら、それは内部接点だから新しくノードを作る
*     左部分木、右部分木の順に再帰する
*     左再帰の時は新しく作ったこの接点のポインタを、
*     右再帰の時は p_ptr をインデックスの格納場所として渡す。
*   インデックスを読み込んだら、それは外部接点だから、p_ptr->index に格納
*
* メモ
*   インデックスの格納場所が元と違うが、特に問題ない。
*************************************************************************/
pat_index_list *malloc_pat_index_list()
{
    static int  idx = 1024;
    static char *ptr;

    if (idx == 1024) {
	ptr = malloc(sizeof(pat_index_list) * idx);
	idx = 0;
    }

    return (pat_index_list *)(ptr + sizeof(pat_index_list) * idx++);
}

pat_node *malloc_pat_node()
{
    static int  idx = 1024;
    static char *ptr;

    if (idx == 1024) {
	ptr = malloc(sizeof(pat_node) * idx);
	idx = 0;
    }

    return (pat_node *)(ptr + sizeof(pat_node) * idx++);
}

pat_node *load_anode(pat_node *p_ptr){
  unsigned char c;
  pat_node *new_ptr; /* 新しく作ったノード(==このノード)を指すポインタ */
  long tmp_idx;
  pat_index_list *new_l_ptr,*t_ptr=NULL;

  if ((c = egetc(fd_pat)) & 0x80) { /* 葉っぱの処理、インデックスの読み込み */
    while(c & 0x80) {
      tmp_idx = (c & 0x3f) << 24;
      tmp_idx |= egetc(fd_pat) << 16;
      tmp_idx |= egetc(fd_pat) << 8;
      tmp_idx |= egetc(fd_pat);

      if((p_ptr->il).index < 0)
	new_l_ptr = &(p_ptr->il);
      else {
	new_l_ptr = malloc_pat_index_list();
	t_ptr->next = new_l_ptr;
      }
      new_l_ptr->index = tmp_idx;
      new_l_ptr->next = NULL;
      t_ptr = new_l_ptr;

      if(c & 0x40) break;
      c = egetc(fd_pat);
    }

    return (p_ptr);
  }
  else { /* 内部接点の処理、再帰する */
    new_ptr = malloc_pat_node();
    new_ptr->checkbit = ((c << 8) | egetc(fd_pat)) - 1; /* チェックビット */
/*    printf("#cb %d\n",new_ptr->checkbit);*/
    (new_ptr->il).index = -1;
    new_ptr->left = load_anode(new_ptr);
    new_ptr->right = load_anode(p_ptr);
    return (new_ptr);
  }
}

unsigned char egetc(int file_discripter){
  static int fd_pat_check = -1;
  static char buf[BUFSIZ];
  static int ctr = sizeof(buf) - 1;

  if(file_discripter != fd_pat_check) { /* バッファの初期化 */
    fd_pat_check = file_discripter;
    ctr = sizeof(buf) - 1;
  }

  if(++ctr == sizeof(buf)){
    ctr = 0;
    read(file_discripter, buf, sizeof(buf));
/* OL(.);fflush(stdout);*/
  }

  return(buf[ctr]);
}


/*****************************************************
* com_s --- 木のセーブ 
*  by 米沢恵司(keiji-y@is.aist-nara.ac.jp)
*
* パラメータ、返し値
*   なし
*****************************************************/
void com_s(char *fname_pat, pat_node *ptr){
  int i;

  printf("Saving pat-tree \"%s\" ...\n",fname_pat);
  if ((fd_pat = open(fname_pat, O_WRONLY|O_CREAT|O_BINARY, 0644)) == -1) {
    fprintf(stderr, "ファイル %s がオープン出来ません。\n", fname_pat);
    exit(1);
  }; 
  save_pat(ptr->right); /* ファイル出力 */
  for(i = 0; i < BUFSIZ; i++)
    eputc(0, fd_pat); /* flush */
  close(fd_pat);
}


/****************************************************
* save_pat --- パトリシア木データをセーブ 
*  by 米沢恵司(keiji-y@is.aist-nara.ac.jp)
*
* パラメータ
*   top_ptr --- 検索開始ノードの位置(ポインタ)
*   out_to --- 出力先(stdoutやファイル)
* 
* 返し値
*   無し。パトリシア木データを出力。
*
* 出力フォーマット --- 8ビットに区切ってバイナリ出力
*   左優先探索で内部接点はチェックビット、外部接点はインデックスを出力
*   チェックビット --- 基本的にそのまま (第 0 ビットが 0)
*     ただし -1 のとき困るので 1 を足す
*   インデックス --- 第 0 ビットを 1 にする
****************************************************/
void save_pat(pat_node *top_ptr)
{
  pat_index_list *ptr;
  long out_idx;
  /* 内部接点の処理、チェックビットを出力 */
  eputc (((top_ptr->checkbit + 1)>> 8) & 0x7f, fd_pat);
  eputc ((top_ptr->checkbit + 1)& 0xff, fd_pat);

  /* 左右の Subtree の処理。葉っぱならインデックスを出力、
     葉っぱでなければ再帰。*/
  if(top_ptr->checkbit < top_ptr->left->checkbit)
    save_pat(top_ptr->left);
  else {
    ptr = &(top_ptr->left->il);
    if(ptr->index < 0) dummy();
    else {
      while(ptr != NULL) {
	if(ptr->next == NULL) eputc (((ptr->index >> 24) & 0x3f) | 0xc0, fd_pat);
	else eputc (((ptr->index >> 24) & 0x3f) | 0x80, fd_pat);
	eputc ((ptr->index >> 16) & 0xff, fd_pat);
	eputc ((ptr->index >> 8) & 0xff, fd_pat);
	eputc ((ptr->index) & 0xff, fd_pat);
	ptr = ptr->next;
      }
    }
  }
  if(top_ptr->checkbit < top_ptr->right->checkbit)
    save_pat(top_ptr->right);
  else {
    ptr = &(top_ptr->right->il);
    if(ptr->index < 0) dummy();
    else {
      while(ptr != NULL) {
	if(ptr->next == NULL) eputc (((ptr->index >> 24) & 0x3f) | 0xc0, fd_pat);
	else eputc (((ptr->index >> 24) & 0x3f) | 0x80, fd_pat);
	eputc ((ptr->index >> 16) & 0xff, fd_pat);
	eputc ((ptr->index >> 8) & 0xff, fd_pat);
	eputc ((ptr->index) & 0xff, fd_pat);
	ptr = ptr->next;
      }
    }
  }

  return;
}

void dummy() {
  eputc(0xff,fd_pat);eputc(0xff,fd_pat);eputc(0xff,fd_pat);eputc(0xff,fd_pat);
}

void eputc(unsigned char c, int file_discripter){
  static int ctr = 0;
  static unsigned char buf[BUFSIZ];

  buf[ctr] = (char) c;
  ctr++;

  if(ctr == BUFSIZ){
    ctr = 0;
    write(file_discripter, buf, BUFSIZ);
  }

  return;
}
