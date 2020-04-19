/*
==============================================================================
	juman_lib.c
        				Last Update: 2012/9/28 Sadao Kurohashi
==============================================================================
*/
 
/*
  連接表に基づく基本的処理を越える処理について，アルゴリズムを簡単に説
  明する．

  【連語処理】

  連語はdefaultの重みを0.5とし，連語内の形態素および連接のコストを重み
  倍する．

  m_buffer 及び p_buffer の構造は，以前の juman と何ら変わりない．例と
  して，現在のラティスが次のような状態であるとする．

  発表 − する
  (T)     (T)

  連語「ことになる」が追加される場合，一気に 3つの m_buffer と 3つの 
  p_bufferが新規作成される．

  発表 − する − こと − に − なる
   (T)     (T)     (F)   (F)     (T)
                ^^^^^^^^^^^^^^^^^^^^ 一括追加

  但し，p_buffer の構造体に connect というメンバが追加されている．(上
  図において (T) が connect = TRUE，(F) が connect = FALSE を表してい
  る)これは，連語の途中に別の形態素へのつながりが侵入するのを防ぐため
  である．具体的には，連語内の形態素のうち，最後の形態素に対応する部分
  以外の p_bufferの connect を FALSE とする．これによって，次のような
  ラティスの生成を禁止できる．

  発表 − する − こと − に − なる
                       ＼
                          にな − ....  (連語内への割り込み)

  【透過処理】

  括弧や空白などが来た場合，その前の形態素の連接属性を透過させる処理．
  (例) 『ひとこと』で………   という文が入力された場合
  まず，次のようなラティスが生成される

  ひとこと　−  』(con_tbl は括弧閉のもの)
            ／
      こと

  その直後に through_word が呼ばれ，新たに 2つの m_buffer と 2つの 
  p_buffer が新規作成される．新規作成された m_bufferは，con_tbl だけが
  異なっていて，その他はオリジナルと全く同じである．

  ひとこと　−  』(「ひとこと」と同じ con_tbl)
            ＼
                』(con_tbl は括弧閉のもの)
            ／ 
      こと　−  』(「こと」と同じ con_tbl)

  以上のように，様々な連接属性(con_tbl)を持った m_buffer 及び p_buffer 
  が増植することによって，透過処理を実現している．

  (ただし，このままでは大量の空白などが続く場合にm_buffer, p_buffer が
  溢れてしまうので，重複するものを出来るだけ再利用し，枝刈りを適宜行う
  ことによって，爆発を防いでいる．)

  【数詞処理】

  数詞の直後に数詞が来た場合に，その2つを連結した新たな形態素を追加する．

  今月 − の − ２

  この次に数詞「０」が来た場合，まず次のようなラティスが生成される．

  今月 − の − ２ − ０

  その直後に suusi_word が呼ばれ，新たに数詞「２０」が追加される．

  今月 − の − ２ − ０
             ＼
                ２０

  ただし，この「２０」の形態素コストは後ろの「０」の方の形態素コストが
  コピーされる．
  */

/*
------------------------------------------------------------------------------
	inclusion of header files
------------------------------------------------------------------------------ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include	"juman.h"
#include	"const.h"
#include	"darts_for_juman.h"

/*
------------------------------------------------------------------------------
	definition of macros
------------------------------------------------------------------------------
*/

#define         DEF_CLASS_C             "品詞コスト"
#define         DEF_RENSETSU_W          "連接コスト重み"
#define         DEF_KEITAISO_W          "形態素コスト重み"
#define         DEF_COST_HABA           "コスト幅"
#define         DEF_DIC_FILE            "辞書ファイル"
#define         DEF_GRAM_FILE           "文法ファイル"

#define 	DEF_SUUSI_HINSI		"名詞"
#define 	DEF_SUUSI_BUNRUI	"数詞"
#define 	DEF_KAKKO_HINSI		"特殊"
#define 	DEF_KAKKO_BUNRUI1	"括弧始"
#define 	DEF_KAKKO_BUNRUI2	"括弧終"
#define 	DEF_KUUHAKU_HINSI	"特殊"
#define 	DEF_KUUHAKU_BUNRUI	"空白"

#define 	DEF_UNDEF		"未定義語"
#define 	DEF_UNDEF_KATA		"カタカナ"
#define 	DEF_UNDEF_ALPH		"アルファベット"
#define 	DEF_UNDEF_ETC		"その他"

#define 	RENGO_BUFSIZE		20

#define my_fprintf fprintf

/*
------------------------------------------------------------------------------
	GLOBAL:
	definition of global variables          >>> changed by T.Nakamura <<<
------------------------------------------------------------------------------
*/

char			*ProgName;
FILE			*db;
extern CLASS		Class[CLASSIFY_NO + 1][CLASSIFY_NO + 1];
extern TYPE		Type[TYPE_NO];
extern FORM		Form[TYPE_NO][FORM_NO];
extern RENSETU_PAIR     *rensetu_tbl;
extern U_CHAR           *rensetu_mtr;

DIC_FILES               DicFile;

COST_OMOMI       	cost_omomi;    
char             	Jumangram_Dirname[FILENAME_MAX]; 
extern FILE		*Jumanrc_Fileptr;
int              	LineNo = 0;
int     		LineNoForError;       /* k.n */

char			pat_buffer[500000];

/*
------------------------------------------------------------------------------
	LOCAL:
	definition of global variables          >>> changed by T.Nakamura <<<
------------------------------------------------------------------------------
*/
int             Show_Opt1;
int             Show_Opt2;
char		Show_Opt_tag[MIDASI_MAX];
int		Show_Opt_jumanrc;
int		Show_Opt_debug;
int		Rendaku_Opt;
int		Repetition_Opt;
int             Onomatopoeia_Opt;
int		LowercaseRep_Opt;
int		LowercaseDel_Opt;
int		LongSoundRep_Opt;
int		LongSoundDel_Opt;
int		UseGivenSegmentation_Opt;

U_CHAR	        String[LENMAX];
U_CHAR	        NormalizedString[LENMAX];
int		String2Length[LENMAX];
int		CharLatticeUsedFlag;
CHAR_NODE	CharLattice[MAX_LATTICES];
CHAR_NODE	CharRootNode;
size_t		CharNum;
int		MostDistantPosition;
int		Unkword_Pat_Num;
int             pre_m_buffer_num;
int             m_buffer_num;
int             Jiritsu_buffer[CLASSIFY_NO + 1];
int             undef_hinsi;
int		undef_kata_bunrui, undef_alph_bunrui, undef_etc_bunrui;
int		undef_kata_con_tbl, undef_alph_con_tbl, undef_etc_con_tbl;
int             suusi_hinsi, suusi_bunrui;
int             kakko_hinsi, kakko_bunrui1, kakko_bunrui2;
int		kuuhaku_hinsi, kuuhaku_bunrui, kuuhaku_con_tbl;
int		onomatopoeia_hinsi, onomatopoeia_bunrui, onomatopoeia_con_tbl;
int             rendaku_hinsi1, rendaku_hinsi2, rendaku_hinsi3, rendaku_hinsi4;
int             rendaku_renyou, rendaku_bunrui2_1, rendaku_bunrui2_2, rendaku_bunrui2_3;
int             rendaku_bunrui4_1, rendaku_bunrui4_2, rendaku_bunrui4_3, rendaku_bunrui4_4;
int             prolong_interjection, prolong_copula;
int             prolong_ng_hinsi1, prolong_ng_hinsi2, prolong_ng_hinsi3, prolong_ng_hinsi4;
int             prolong_ng_bunrui4_1, prolong_ng_bunrui4_2, prolong_ng_bunrui4_3;
int             jiritsu_num;
int             p_buffer_num;
CONNECT_COST	connect_cache[CONNECT_MATRIX_MAX];

/* MRPH_BUFFER_MAX の制限を撤廃，動的にメモリ確保 */
int		 mrph_buffer_max = 0;
MRPH *           m_buffer;
int *            m_check_buffer;

/* PROCESS_BUFFER_MAX の制限を撤廃，動的にメモリ確保 */
int		 process_buffer_max = 0;
PROCESS_BUFFER * p_buffer;
int *            path_buffer;
int *		 match_pbuf;

U_CHAR		kigou[MIDASI_MAX];   /* 先頭の記号(@) */
U_CHAR		midasi1[LENMAX];     /* 活用 */
U_CHAR		midasi2[MIDASI_MAX]; /* 原形 */
U_CHAR		yomi[MIDASI_MAX];    /* 活用の読み */

extern COST_OMOMI       cost_omomi;     /*k.n*/
extern char             Jumangram_Dirname[FILENAME_MAX];  /*k.n*/

/*
------------------------------------------------------------------------------
	prototype definition of functions       >>> changed by T.Nakamura <<<
------------------------------------------------------------------------------
*/

BOOL	juman_init_rc(FILE *fp);
int	juman_close(void);
void	realloc_mrph_buffer(void);
void	realloc_process_buffer(void);
void    read_class_cost(CELL *cell); /* k.n */
int     search_all(int position, int count);
int     take_data(int pos, int pos_in_char, char **pbuf, char opt);
char *	_take_data(char *s, MRPH *mrph, int deleted_bytes, char *opt);
int 	numeral_decode(char **str);
void 	string_decode(char **str, char *out);
void    check_rc(void);
void    changeDictionary(int number);

int     trim_space(int pos);
int	undef_word(int pos);
int	check_code(U_CHAR *cp, int position);
void 	juman_init_etc(void);
int 	suusi_word(int pos , int m_num);
int	through_word(int pos , int m_num);
int 	is_through(MRPH *mrph_p);

/*
  結果の出力関数については、サーバーモード対応のため、出力先を引数として取る
  ように変更
  NACSIS 吉岡
*/   
void	print_path_mrph(FILE* output, int path_num , int para_flag);
char	**print_best_path(FILE* output);
char	**print_all_mrph(FILE* output);
void	_print_all_mrph(FILE* output, int path_num);
char	**print_all_path(FILE* output);
void	_print_all_path(FILE* output, int path_num, int pathes);
char	**print_homograph_path(FILE* output);
int	_print_homograph_path(FILE* output, int pbuf_start, int new_p);

int	pos_match_process(int pos, int p_start);
int	pos_right_process(int position);
int	check_connect(int pos_start, int m_num, char opt);
int	juman_sent(void);

/*
------------------------------------------------------------------------------
	PROCEDURE: <changeDictionary>          >>> changed by T.Nakamura <<<
------------------------------------------------------------------------------
*/

void changeDictionary(int number)
{
    db = DicFile.dic[number];
    DicFile.now = number;
}

/*
------------------------------------------------------------------------------
	PROCEDURE: <push_dic_file_for_win>
------------------------------------------------------------------------------
*/

int push_dic_file_for_win(char *dic_file_name, int num)
{
    char full_file_name[BUFSIZE];

    /* open and set a dictionary for Windows */

    if ((endchar(dic_file_name)) != '\\')
	strcat(dic_file_name, "\\");
    
    sprintf(full_file_name, "%s%s", dic_file_name, PATFILE);
    strcat(dic_file_name, DICFILE);

    /* if ((DicFile.dic[num] = fopen(dic_file_name , "rb")) == NULL)
       return FALSE; */
    DicFile.dic[num] = my_fopen(dic_file_name , "rb");
    pat_init_tree_top(&DicFile.tree_top[num]);
    com_l(full_file_name, &DicFile.tree_top[num]);
    return TRUE;
}

/*
------------------------------------------------------------------------------
	PROCEDURE: <juman_init_rc>
------------------------------------------------------------------------------
*/

BOOL juman_init_rc(FILE *fp)
{
    int  num, win32_decided = 0;
    char dic_file_name[BUFSIZE], full_file_name[BUFSIZE];
    CELL *cell1, *cell2;
#ifdef _WIN32
    char ExecFilePath[MAX_PATH + 1], ExecFileDrive[MAX_PATH + 1], ExecFileDir[MAX_PATH + 1];
#endif

    LineNo = 0 ;
    
    /* DEFAULT VALUE*/
    
    cost_omomi.keitaiso = KEITAISO_WEIGHT_DEFAULT;
    cost_omomi.rensetsu = RENSETSU_WEIGHT_DEFAULT * MRPH_DEFAULT_WEIGHT;
    cost_omomi.cost_haba = COST_WIDTH_DEFAULT * MRPH_DEFAULT_WEIGHT;

    /* 
     *  MS Windows は辞書のパスを,juman.ini から取得する (辞書Dir も1つのみ (やや手抜))
     *  しかも gramfile == dicfile
     *  Changed by Taku Kudoh (taku@pine.kuee.kyoto-u.ac.jp)
     */
#ifdef _WIN32
    /* 文法ファイル */
    num = 0;
#ifdef WIN_AZURE
    GetPrivateProfileString("juman","dicfile",WIN_AZURE_DICFILE_DEFAULT,Jumangram_Dirname,sizeof(Jumangram_Dirname),"juman.ini");
#else
    GetModuleFileName(NULL, ExecFilePath, MAX_PATH); /* 実行ファイルのfullpathを取得 */
    _splitpath(ExecFilePath, ExecFileDrive, ExecFileDir, NULL, NULL); /* 実行ファイルのfullpathからdrive名、ディレクトリを取得 */
    sprintf(Jumangram_Dirname, "%s%s\\%s", ExecFileDrive, ExecFileDir, WIN_AZURE_DICFILE_DEFAULT);
#endif
    if (Jumangram_Dirname[0]) {
	grammar(NULL);
	katuyou(NULL);
	connect_table(NULL);
	connect_matrix(NULL);

	/* 辞書ファイル */
#ifdef WIN_AZURE
	/* use dic, autodic and wikipediadic for Azure */
	GetPrivateProfileString("juman","dicfile",WIN_AZURE_DICFILE_DEFAULT,dic_file_name,sizeof(dic_file_name),"juman.ini");
	push_dic_file_for_win(dic_file_name, num++);
	GetPrivateProfileString("juman","autodicfile",WIN_AZURE_AUTODICFILE_DEFAULT,dic_file_name,sizeof(dic_file_name),"juman.ini");
	push_dic_file_for_win(dic_file_name, num++);
	GetPrivateProfileString("juman","wikipediadicfile",WIN_AZURE_WIKIPEDIADICFILE_DEFAULT,dic_file_name,sizeof(dic_file_name),"juman.ini");
	push_dic_file_for_win(dic_file_name, num++);
#else
        strcpy(dic_file_name, Jumangram_Dirname);
	push_dic_file_for_win(dic_file_name, num++);
#endif
	DicFile.number = num;
	changeDictionary(0);
	win32_decided = 1;
    }
    /* juman.iniが利用できなければ、jumanrcから読む */
#endif
    
    while (!s_feof(fp)) { 
	LineNoForError = LineNo ;
	cell1 = s_read(fp);

	/* 文法ファイル */
	if (!win32_decided && !strcmp(DEF_GRAM_FILE, _Atom(car(cell1)))) {
	    if (!Atomp(cell2 = car(cdr(cell1)))) {
		return FALSE;
	    } else {
		strcpy(Jumangram_Dirname , _Atom(cell2));
		grammar(NULL);
		katuyou(NULL);
		connect_table(NULL);
		connect_matrix(NULL);
	    }
	}
	  
	/* 辞書ファイル */
	else if (!win32_decided && !strcmp(DEF_DIC_FILE, _Atom(car(cell1)))) {
	    cell2 = cdr(cell1) ;
	    for(num = 0 ; ; num++) {
		if( Null(car(cell2)) )  break;
		else if ( !Atomp( car(cell2) ) ) {
		    return FALSE;
		}
		else if(num >= MAX_DIC_NUMBER)
		    error(ConfigError, "Too many dictionary files.", EOA);
		else {
		    strcpy(dic_file_name, _Atom(car(cell2)));
		    if ((endchar(dic_file_name)) != '/')
			strcat(dic_file_name, "/");
		    cell2 = cdr(cell2);
		      
		    /* 辞書のオープン */
                    push_darts_file(dic_file_name);
		    sprintf(full_file_name, "%s%s", dic_file_name, PATFILE);
		    strcat(dic_file_name, DICFILE);
		    DicFile.dic[num] = my_fopen(dic_file_name , "r");
                    if (check_filesize(DicFile.dic[num]) == 0) { /* ファイルサイズが0でないことをチェック */
                        warning(OpenError, "filesize is 0", dic_file_name, ".", EOA);
                        num--;
                        continue;
                    }
		    /* pat_init_tree_top(&DicFile.tree_top[num]);
                       com_l(full_file_name, &DicFile.tree_top[num]); */
		}
	    }
	    DicFile.number = num;
	    changeDictionary(0);
	}
	/* 連接コスト重み */
	else if (!strcmp(DEF_RENSETSU_W, _Atom(car(cell1)))) { 
	    if (!Atomp(cell2 = car(cdr(cell1)))) {
		return FALSE;
	    } else 
		cost_omomi.rensetsu = 
		    (int) atoi(_Atom(cell2)) * MRPH_DEFAULT_WEIGHT;
	}
	  
	/* 形態素コスト重み */
	else if (!strcmp(DEF_KEITAISO_W, _Atom(car(cell1)))) { 
	    if (!Atomp(cell2 = car(cdr(cell1)))) {
		return FALSE;
	    } else 
		cost_omomi.keitaiso = (int) atoi(_Atom(cell2));
	}
	  
	/* コスト幅 */
	else if (!strcmp(DEF_COST_HABA, _Atom(car(cell1)))) { 
	    if (!Atomp(cell2 = car(cdr(cell1)))) {
		return FALSE;
	    } else 
		cost_omomi.cost_haba = 
		    (int) atoi(_Atom(cell2)) * MRPH_DEFAULT_WEIGHT;
	}
	  
	/* 品詞コスト*/
	else if (!strcmp(DEF_CLASS_C, _Atom(car(cell1)))) { 
	    read_class_cost(cdr(cell1));
	}

	/* 未定義語コスト (3.4以降不要)
	else if (!strcmp("未定義語品詞", _Atom(car(cell1))));
	*/
    }
    return TRUE;
}

/*
------------------------------------------------------------------------------
	PROCEDURE: <compile_patterns>
------------------------------------------------------------------------------
*/

#ifdef HAVE_REGEX_H
int compile_unkword_patterns() {
    int i, j, flag;

    /* malloc m_pattern */
    for (i = 0; *mrph_pattern[i]; i++);    
    m_pattern = (MRPH_PATTERN *)(malloc(sizeof(MRPH_PATTERN) * i));

    /* make m_pattern[i].preg */
    for (i = 0; *mrph_pattern[i]; i++) {

	/* read mrph_pattern */
	flag = 0;
	m_pattern[i].weight = DefaultWeight;
	sprintf(m_pattern[i].regex, "^");
	for (j = 0; *(mrph_pattern[i] + j); j += BYTES4CHAR) {
	    if ((mrph_pattern[i] + j)[0] == ' ' ||
		(mrph_pattern[i] + j)[0] == '\t') {
		flag = 1;
		j -= 1;
		continue;
	    }

	    /* read weight */
	    if (flag) {
		m_pattern[i].weight = atof(mrph_pattern[i] + j);
		break;
	    }

	    /* read pattern */
	    if (strlen(m_pattern[i].regex) >= PATTERN_MAX - 3) {
		printf("too long pattern: \"%s\"\n", mrph_pattern[i]);
		exit(1);
	    }
	    if (!strncmp(mrph_pattern[i] + j, Hkey, BYTES4CHAR)) {
		strcat(m_pattern[i].regex, Hcode);
	    }
	    else if (!strncmp(mrph_pattern[i] + j, Kkey, BYTES4CHAR)) {
		strcat(m_pattern[i].regex, Kcode);
	    }
	    else if (!strncmp(mrph_pattern[i] + j, Ykey, BYTES4CHAR)) {
		strcat(m_pattern[i].regex, Ycode);
	    }
	    else {
		strncat(m_pattern[i].regex, mrph_pattern[i] + j, BYTES4CHAR);
	    }
	}

	/* compile mrph_pattern */
	if (regcomp(&(m_pattern[i].preg), m_pattern[i].regex, REG_EXTENDED) != 0) {
	    printf("regex compile failed\n");
	}
    }
    return i;
}
#endif

/*
------------------------------------------------------------------------------
        PROCEDURE: <juman_close>             >>> changed by T.Nakamura <<<
------------------------------------------------------------------------------
*/
BOOL juman_close(void)
{
  int i;

  for(i = 0 ; i < DicFile.number ; i++)
    fclose(DicFile.dic[i]);

  close_darts();

  free(rensetu_tbl);
  free(rensetu_mtr);

  return TRUE;
}

/*
------------------------------------------------------------------------------
        PROCEDURE: <realloc_mrph_buffer>
------------------------------------------------------------------------------
*/
void	realloc_mrph_buffer(void)
{
    mrph_buffer_max += BUFFER_BLOCK_SIZE;
    m_buffer = (MRPH *)my_realloc(m_buffer, sizeof(MRPH)*mrph_buffer_max);
    m_check_buffer = (int *)my_realloc(m_check_buffer,
				       sizeof(int)*mrph_buffer_max);
}

/*
------------------------------------------------------------------------------
        PROCEDURE: <realloc_process_buffer>
------------------------------------------------------------------------------
*/
void	realloc_process_buffer(void)
{
    process_buffer_max += BUFFER_BLOCK_SIZE;
    p_buffer = (PROCESS_BUFFER *)my_realloc(p_buffer,
				    sizeof(PROCESS_BUFFER)*process_buffer_max);
    path_buffer = (int *)my_realloc(path_buffer,
				       sizeof(int)*process_buffer_max);
    match_pbuf = (int *)my_realloc(match_pbuf, 
				       sizeof(int)*process_buffer_max);
}

/*
------------------------------------------------------------------------------
        PROCEDURE: <read_class_cost>
------------------------------------------------------------------------------
*/
void read_class_cost(CELL *cell)
{
    /* 品詞コストの読み込み (後ろの設定を優先) */

    CELL *pos_cell;
    int hinsi, bunrui, cost;

    while (!Null(car(cell))) {

	pos_cell = car(car(cell)) ;
	cost = (int) atoi(_Atom(car(cdr(car(cell)))));

	if (!strcmp(_Atom(car(pos_cell)), "*")) {
	    /* 品詞が * なら全体に */
	    for (hinsi = 1 ; Class[hinsi][0].id; hinsi++)
		for (bunrui = 0 ; Class[hinsi][bunrui].id ; bunrui++)
		    Class[hinsi][bunrui].cost = cost;
	}
	else {
	    hinsi = get_hinsi_id(_Atom(car(pos_cell)));
	    if (Null(car(cdr(pos_cell))) || 
		!strcmp(_Atom(car(cdr(pos_cell))), "*")) {
		/* 細分類が * または無しなら品詞全体に */
		for (bunrui = 0; Class[hinsi][bunrui].id ; bunrui++)
		    Class[hinsi][bunrui].cost = cost;
	    }
	    else {
		/* 品詞，細分類ともに指定された場合 */
		bunrui = get_bunrui_id(_Atom(car(cdr(pos_cell))), hinsi);
		Class[hinsi][bunrui].cost = cost;
	    }
	}
	cell = cdr(cell);		
    }

    /* default */
  
    for (hinsi = 1; Class[hinsi][0].id; hinsi++) 
	for (bunrui = 0; Class[hinsi][bunrui].id; bunrui++)
	    if (Class[hinsi][bunrui].cost == 0) 
		Class[hinsi][bunrui].cost = CLASS_COST_DEFAULT;
  
    /* For 文頭 文末 added by S.Kurohashi */

    Class[0][0].cost = 0;
}

/*
------------------------------------------------------------------------------
	PROCEDURE: <katuyou_process>       >>> changed by T.Nakamura <<<
------------------------------------------------------------------------------
*/
static BOOL katuyou_process(int position, int *k, MRPH *mrph, int *length, char opt)
{
    U_CHAR      buf[LENMAX];
    int deleted_length, del_rep_position = 0;

     while (Form[mrph->katuyou1][*k].name) {
	 if (compare_top_str1(Form[mrph->katuyou1][*k].gobi, 
			      String + position + mrph->length)) {
	     *length = mrph->length + strlen(Form[mrph->katuyou1][*k].gobi);
	     return TRUE;
	 } else {
	     (*k)++;
	 }
     }
     return FALSE;
}

/*
------------------------------------------------------------------------------
	PROCEDURE: <register_nodes_by_deletion>
------------------------------------------------------------------------------
*/
void register_nodes_by_deletion(char *src_buf, char *pat_buf, char node_type, char deleted_bytes) {
    int length;
    char *start_buf = src_buf, *current_pat_buf;
    deleted_bytes++;

    while (*src_buf) {
        if (*src_buf == '\n') {
            current_pat_buf = pat_buf + strlen(pat_buf); /* 次のノードの最初 */
            length = src_buf - start_buf + 1;
            strncat(pat_buf, start_buf, length); /* 次のノードをコピー */
            *current_pat_buf = node_type + PAT_BUF_INFO_BASE; /* node_typeの書き換え */
            *(current_pat_buf + 1) = deleted_bytes + PAT_BUF_INFO_BASE; /* 削除バイト数 (取り出すときに1引く必要がある) */
            *(current_pat_buf + length) = '\0';
            start_buf = src_buf + 1;
        }
        src_buf++;
    }
}

/*
------------------------------------------------------------------------------
	PROCEDURE: <da_search_one_step>
------------------------------------------------------------------------------
*/
void da_search_one_step(int dic_no, int left_position, int right_position, char *pat_buf) {
    int i, status;
    size_t current_da_node_pos;
    CHAR_NODE *left_char_node, *right_char_node;
    char *current_pat_buf, current_node_type;

#ifdef DEBUG
    printf(";; S from node=%d, chr=%d, most_distant=%d\n", left_position, right_position, MostDistantPosition);
#endif

    if (left_position < 0) {
        left_char_node = &CharRootNode;
    }
    else
        left_char_node = &(CharLattice[left_position]);

    while (left_char_node) {
        for (i = 0; i < left_char_node->da_node_pos_num; i++) { /* 現在位置のtrieのノード群から */
            right_char_node = &(CharLattice[right_position]);
            while (right_char_node) { /* 次の文字の集合 */
                if (right_char_node->chr[0] == '\0') { /* 削除ノード */
                    if (left_position >= 0) {
                        // printf(";; D from node=%d, chr=%d\n", left_position, right_position + 1);
                        right_char_node->node_type[right_char_node->da_node_pos_num] = left_char_node->node_type[i] | right_char_node->type;
                        right_char_node->deleted_bytes[right_char_node->da_node_pos_num] = left_char_node->deleted_bytes[i] + strlen(CharLattice[right_position].chr);
                        if (left_char_node->p_buffer[i]) {
                            if (right_char_node->p_buffer[right_char_node->da_node_pos_num] == NULL) {
                                right_char_node->p_buffer[right_char_node->da_node_pos_num] = (char *)malloc(50000);
                                right_char_node->p_buffer[right_char_node->da_node_pos_num][0] = '\0';
                            }
                            strcat(right_char_node->p_buffer[right_char_node->da_node_pos_num], left_char_node->p_buffer[i]);

                            /* 各ノードのp_bufferの先頭(node_type)は未更新 -> pat_bufに入れるときに書き換える */
                            register_nodes_by_deletion(left_char_node->p_buffer[i], pat_buf, 
                                                       right_char_node->node_type[right_char_node->da_node_pos_num] | OPT_PROLONG_DEL_LAST, 
                                                       right_char_node->deleted_bytes[right_char_node->da_node_pos_num]);
#ifdef DEBUG
                            printf("*** D p_buf=%d p_buffer=%d at %d,", strlen(pat_buf), strlen(right_char_node->p_buffer[right_char_node->da_node_pos_num]), right_position);
                            int j;
                            for (j = 0; j < right_char_node->da_node_pos_num; j++)
                                printf("%d,", right_char_node->da_node_pos[j]);
                            printf("%d\n", left_char_node->da_node_pos[i]);
#endif
                        }
                        right_char_node->da_node_pos[right_char_node->da_node_pos_num++] = left_char_node->da_node_pos[i];
                        if (MostDistantPosition < right_position)
                            MostDistantPosition = right_position;
                    }
                }
                else { /* 通常ノード */
                    if (!(right_char_node->type & OPT_DEVOICE || right_char_node->type & OPT_PROLONG_REPLACE) || 
                        (right_char_node->type & OPT_DEVOICE && left_position < 0) || /* 連濁ノードは先頭である必要がある */
                        (right_char_node->type & OPT_PROLONG_REPLACE && left_position >= 0)) { /* 長音置換ノードは先頭以外である必要がある */
                        current_node_type = left_char_node->node_type[i] | right_char_node->type;
                        current_da_node_pos = left_char_node->da_node_pos[i];
#ifdef DEBUG
                        printf(";; T <%s> from %d ", right_char_node->chr, current_da_node_pos);
#endif
                        current_pat_buf = pat_buf + strlen(pat_buf);
                        status = da_traverse(dic_no, right_char_node->chr, &current_da_node_pos, 0, strlen(right_char_node->chr), 
                                             current_node_type, left_char_node->deleted_bytes[i], pat_buf);
                        if (status > 0) { /* マッチしたら結果に登録するとともに、次回のノード開始位置とする */
                            if (right_char_node->p_buffer[right_char_node->da_node_pos_num] == NULL) {
                                right_char_node->p_buffer[right_char_node->da_node_pos_num] = (char *)malloc(50000);
                                right_char_node->p_buffer[right_char_node->da_node_pos_num][0] = '\0';
                            }
                            strcat(right_char_node->p_buffer[right_char_node->da_node_pos_num], current_pat_buf);
#ifdef DEBUG
                            printf("OK (position=%d, value exists(p_buffer=%d).)", current_da_node_pos, strlen(right_char_node->p_buffer[right_char_node->da_node_pos_num]));
#endif
                        }
                        else if (status == -1) {
#ifdef DEBUG
                            printf("OK (position=%d, cont..)", current_da_node_pos);
#endif
                            ;
                        }
                        if (status > 0 || status == -1) { /* マッチした場合と、ここではマッチしていないが、続きがある場合 */
                            right_char_node->node_type[right_char_node->da_node_pos_num] = current_node_type; /* ノードタイプの伝播 */
                            right_char_node->deleted_bytes[right_char_node->da_node_pos_num] = left_char_node->deleted_bytes[i]; /* 削除文字数の伝播 */
                            right_char_node->da_node_pos[right_char_node->da_node_pos_num++] = current_da_node_pos;
                            if (MostDistantPosition < right_position)
                                MostDistantPosition = right_position;
                        }
#ifdef DEBUG
                        printf("\n");
#endif
                    }
                }
                if (right_char_node->da_node_pos_num >= MAX_NODE_POS_NUM) {
#ifdef DEBUG
                    fprintf(stderr, ";; exceeds MAX_NODE_POS_NUM in %s\n", String);
#endif
                    right_char_node->da_node_pos_num = MAX_NODE_POS_NUM - 1;
                }
                right_char_node = right_char_node->next;
            }
        }
        left_char_node = left_char_node->next;
    }
}

/*
------------------------------------------------------------------------------
	PROCEDURE: <da_search_from_position>
------------------------------------------------------------------------------
*/
void da_search_from_position(int dic_no, int position, char *pat_buf)
{
    int i, j;
    CHAR_NODE *char_node;

#ifdef DEBUG
    printf(";; SS from start_position=%d\n", position);
#endif

    /* initialization */
    MostDistantPosition = position - 1;
    for (i = position; i < CharNum; i++) {
        char_node = &(CharLattice[i]);
        while (char_node) {
            for (j = 0; j < char_node->da_node_pos_num; j++) {
                if (char_node->p_buffer[j])
                    free(char_node->p_buffer[j]);
                char_node->p_buffer[j] = NULL;
            }
            char_node->da_node_pos_num = 0;
            char_node = char_node->next;
        }
    }

    /* search double array by traverse */
    da_search_one_step(dic_no, -1, position, pat_buf); // the second -1 means the search from the root of double array
    for (i = position + 1; i < CharNum; i++) {
        if (MostDistantPosition < i - 1)
            break;
        da_search_one_step(dic_no, i - 1, i, pat_buf);
    }
}

/*
------------------------------------------------------------------------------
	PROCEDURE: <search_all>       >>> changed by T.Nakamura <<<
------------------------------------------------------------------------------
*/
int search_all(int position, int position_in_char)
{
    int         dic_no;
    char	*pbuf;

    for(dic_no = 0 ; dic_no < DicFile.number ; dic_no++) {
	changeDictionary(dic_no);

	/* TRIE(double array)から形態素を検索 */
	pat_buffer[0] = '\0';
        if (CharLatticeUsedFlag) /* traverse on character lattice (if input is a lattice) */
            da_search_from_position(dic_no, position_in_char, pat_buffer);
        else /* normal search of double array (for linear input) */
            da_search(dic_no, String + position, pat_buffer);
	pbuf = pat_buffer;
	while (*pbuf != '\0') {
	    if (take_data(position, position_in_char, &pbuf, 0) == FALSE) return FALSE;
	}
    }

    return TRUE;
}

/*
------------------------------------------------------------------------------
        PROCEDURE: <recognize_repetition>   >>> Added by Ryohei Sasano <<<
------------------------------------------------------------------------------
*/
int recognize_onomatopoeia(int pos)
{
    int i, len, code, next_code;
    int key_length = strlen(String + pos); /* キーの文字数を数えておく */
#ifdef HAVE_REGEX_H
    regmatch_t pmatch[1];
#endif

    /* 通常の平仮名、片仮名以外から始まるものは不可 */
    code = check_code(String, pos);
    if (code != HIRAGANA && code != KATAKANA) return FALSE;
    for (i = 0; *lowercase[i]; i++) {
	if (!strncmp(String + pos, lowercase[i], BYTES4CHAR)) return FALSE;
    }

#ifdef HAVE_REGEX_H
    /* 非反復型オノマトペ */
    if (Onomatopoeia_Opt &&
	/* 1文字目と2文字目の文字種が異なるものは不可 */
	code == check_code(String, pos + BYTES4CHAR)) {
	
	for (i = 0; i < Unkword_Pat_Num; i++) {
	    
	    /* マッチング */
	    if (regexec(&(m_pattern[i].preg), String + pos, 1, pmatch, 0) == 0) {
		
		m_buffer[m_buffer_num].hinsi = onomatopoeia_hinsi;
		m_buffer[m_buffer_num].bunrui = onomatopoeia_bunrui;
		m_buffer[m_buffer_num].con_tbl = onomatopoeia_con_tbl;
		
		m_buffer[m_buffer_num].katuyou1 = 0;
		m_buffer[m_buffer_num].katuyou2 = 0;
		m_buffer[m_buffer_num].length = (int)(pmatch[0].rm_eo - pmatch[0].rm_so);    
		
		strncpy(m_buffer[m_buffer_num].midasi, String+pos, m_buffer[m_buffer_num].length);
		m_buffer[m_buffer_num].midasi[m_buffer[m_buffer_num].length] = '\0';
		strncpy(m_buffer[m_buffer_num].yomi, String+pos, m_buffer[m_buffer_num].length);
		m_buffer[m_buffer_num].yomi[m_buffer[m_buffer_num].length] = '\0';
		
		m_buffer[m_buffer_num].weight = m_pattern[i].weight;
		
		strcpy(m_buffer[m_buffer_num].midasi2, m_buffer[m_buffer_num].midasi);
		strcpy(m_buffer[m_buffer_num].imis, "\"");
		strcat(m_buffer[m_buffer_num].imis, DEF_ONOMATOPOEIA_IMIS);
		strcat(m_buffer[m_buffer_num].imis, "\"");
		
		check_connect(pos, m_buffer_num, 0);
		if (++m_buffer_num == mrph_buffer_max) realloc_mrph_buffer();	
		break; /* 最初にマッチしたもののみ採用 */
	    }
	}
    }
#endif

    /* 反復型オノマトペ */
    if (Repetition_Opt) {
	for (len = 2; len < 5; len++) {
	    /* 途中で文字種が変わるものは不可 */
	    next_code = check_code(String, pos + len * BYTES4CHAR - BYTES4CHAR);
	    if (next_code == CHOON) next_code = code; /* 長音は直前の文字種として扱う */
	    if (key_length < len * 2 * BYTES4CHAR || code != next_code) break;
	    code = next_code;
	    
	    /* 反復があるか判定 */
	    if (strncmp(String + pos, String + pos + len * BYTES4CHAR, len * BYTES4CHAR)) continue;
	    /* ただし3文字が同じものは不可 */
	    if (!strncmp(String + pos, String + pos + BYTES4CHAR, BYTES4CHAR) &&
		!strncmp(String + pos, String + pos + 2 * BYTES4CHAR, BYTES4CHAR)) continue;
	    
	    m_buffer[m_buffer_num].hinsi = onomatopoeia_hinsi;
	    m_buffer[m_buffer_num].bunrui = onomatopoeia_bunrui;
	    m_buffer[m_buffer_num].con_tbl = onomatopoeia_con_tbl;
	    
	    m_buffer[m_buffer_num].katuyou1 = 0;
	    m_buffer[m_buffer_num].katuyou2 = 0;
	    m_buffer[m_buffer_num].length = len * 2 * BYTES4CHAR;
	    
	    strncpy(m_buffer[m_buffer_num].midasi, String+pos, len * 2 * BYTES4CHAR);
	    m_buffer[m_buffer_num].midasi[len * 2 * BYTES4CHAR] = '\0';
	    strncpy(m_buffer[m_buffer_num].yomi, String+pos, len * 2 * BYTES4CHAR);
	    m_buffer[m_buffer_num].yomi[len * 2 * BYTES4CHAR] = '\0';
	    
	    /* weightの設定 */
	    m_buffer[m_buffer_num].weight = REPETITION_COST * len;
	    /* 拗音を含む場合 */
	    for (i = CONTRACTED_LOWERCASE_S; i < CONTRACTED_LOWERCASE_E; i++) {
		if (strstr(m_buffer[m_buffer_num].midasi, lowercase[i])) break;
	    }
	    if (i < CONTRACTED_LOWERCASE_E) {
		if (len == 2) continue; /* 1音の繰り返しは禁止 */		
		/* 1文字分マイナス+ボーナス */
		m_buffer[m_buffer_num].weight -= REPETITION_COST + CONTRACTED_BONUS;
	    }
	    /* 濁音・半濁音を含む場合 */
	    for (i = 0; *dakuon[i]; i++) {
		if (strstr(m_buffer[m_buffer_num].midasi, dakuon[i])) break;
	    }
	    if (*dakuon[i]) {
		m_buffer[m_buffer_num].weight -= DAKUON_BONUS; /* ボーナス */
		/* 先頭が濁音の場合はさらにボーナス */
		if (!strncmp(m_buffer[m_buffer_num].midasi, dakuon[i], BYTES4CHAR)) 
		    m_buffer[m_buffer_num].weight -= DAKUON_BONUS;
	    }
	    /* カタカナである場合 */
	    if (code == KATAKANA) 
		m_buffer[m_buffer_num].weight -= KATAKANA_BONUS;
	    
	    strcpy(m_buffer[m_buffer_num].midasi2, m_buffer[m_buffer_num].midasi);
	    strcpy(m_buffer[m_buffer_num].imis, "\"");
	    strcat(m_buffer[m_buffer_num].imis, DEF_ONOMATOPOEIA_IMIS);
	    strcat(m_buffer[m_buffer_num].imis, "\"");
	    
	    check_connect(pos, m_buffer_num, 0);
	    if (++m_buffer_num == mrph_buffer_max) realloc_mrph_buffer();	
	    break; /* 最初にマッチしたもののみ採用 */
	}
    }
}

/*
------------------------------------------------------------------------------
        PROCEDURE: <take_data>                  >>> Changed by yamaji <<<
------------------------------------------------------------------------------
*/
int take_data(int pos, int pos_in_char, char **pbuf, char opt)
{
    unsigned char    *s;
    int     i, k, f, component_num;
    int	    rengo_con_tbl, rengo_weight;
    MRPH    mrph, c_mrph_buf[RENGO_BUFSIZE];
    MRPH    *c_mrph_p;
    int	    con_tbl_bak, pnum_bak, c_weight;
    int	    new_mrph_num, deleted_bytes;

    s = *pbuf;
    opt = *s++ - PAT_BUF_INFO_BASE;
    deleted_bytes = *s++ - 1 - PAT_BUF_INFO_BASE;
    s = _take_data(s, &mrph, deleted_bytes, &opt);

    /* 連語の場合 */
    if (mrph.hinsi == RENGO_ID) {

        if (Show_Opt_debug) printf("\\begin{連語} %s(%s)\n", mrph.midasi, mrph.midasi2);
	component_num = mrph.bunrui;	/* bunruiの場所に要素数 */
	rengo_con_tbl = mrph.con_tbl;
	rengo_weight  = mrph.weight;

        /* まず連語全体をバッファに読み込む */
	for (i = 0; i < component_num; i++) { 
	    s = _take_data(s, &c_mrph_buf[i], 0, &opt);
	    if (opt != OPT_NORMAL) c_mrph_buf[i].weight = STOP_MRPH_WEIGHT;
	}

        /* 前と連接すれば，順に各要素をm_buffer,p_bufferに入れる
           ※ 連接しない場合にbreakするので，上のループとはマージできない
              （マージするとbreak時にpbufからの読み出しが途中のままに）
        */
        for (i = 0; i < component_num; i++) {
            m_buffer[m_buffer_num] = c_mrph_buf[i];
            c_mrph_p = &m_buffer[m_buffer_num];

            /* 与えられたsegmentationと長さが一致しない場合 */
            if (UseGivenSegmentation_Opt && c_mrph_p->length != String2Length[pos]) {
                s++; /* 項目間の\n */
                *pbuf = s;
                return TRUE;
            }

            /* 連語の先頭の要素 */
            if (i == 0) {

                /* 連語特有の左連接規則を調べる */
                con_tbl_bak = c_mrph_p->con_tbl;
                if (rengo_con_tbl != -1 && check_matrix_left(rengo_con_tbl) == TRUE)
                    c_mrph_p->con_tbl = rengo_con_tbl;

                /* check_connectで，前との連接を調べ，連接する場合はコストが計算され，
                   p_buffer_numが 1 増える */
                pnum_bak = p_buffer_num;
                check_connect(pos, m_buffer_num, opt);
                if (p_buffer_num == pnum_bak) break;	/* 連接しない場合 */

                c_mrph_p->con_tbl = con_tbl_bak;
                /* 次の要素との接続で使うので，もとの接続情報にもどす */

                p_buffer[pnum_bak].end = pos + c_mrph_p->length;
                p_buffer[pnum_bak].connect = FALSE;
                p_buffer[pnum_bak].score = p_buffer[pnum_bak].score +
                    (Class[c_mrph_p->hinsi][c_mrph_p->bunrui].cost
                     * c_mrph_p->weight * cost_omomi.keitaiso 
                     * (rengo_weight-10)/10);
                /* 先頭要素の形態素scoreが普通に足されているので，連語内のscoreに修正 */

                if (Show_Opt_debug)
                    printf("----- 連語内 %s %d\n", c_mrph_p->midasi, p_buffer[pnum_bak].score);
            } 

            /* 連語の先頭以外の要素 */
            else {
                /* 連語内では単純に前後の要素をつなぐ */
                p_buffer[p_buffer_num].mrph_p = m_buffer_num;
                p_buffer[p_buffer_num].start = pos;
                p_buffer[p_buffer_num].end = pos + c_mrph_p->length;
                p_buffer[p_buffer_num].path[0] = p_buffer_num - 1;
                p_buffer[p_buffer_num].path[1] = -1;

                /* 連語内の接続コストは，接続表で定義されていない場合，および，
                   デフォルト値より大きいコストの場合，デフォルト値とする． */
                c_weight = check_matrix(m_buffer[p_buffer[p_buffer_num-1].mrph_p].con_tbl, 
                                        c_mrph_p->con_tbl);
                if (c_weight == 0 || c_weight > DEFAULT_C_WEIGHT) 
                    c_weight = DEFAULT_C_WEIGHT;
                p_buffer[p_buffer_num].score =
                    p_buffer[p_buffer_num-1].score +
                    (Class[c_mrph_p->hinsi][c_mrph_p->bunrui].cost * c_mrph_p->weight * cost_omomi.keitaiso +
                     c_weight * cost_omomi.rensetsu)
                    * rengo_weight/10;
                
                /* 連語の中程の要素 */
                if (i < component_num - 1) {
                    p_buffer[p_buffer_num].connect = FALSE;
                } 
                /* 連語の末尾の要素 */
                else { 
                    p_buffer[p_buffer_num].connect = TRUE;

                    /* 連語特有の右連接規則を調べる */
                    if (rengo_con_tbl != -1 && check_matrix_right(rengo_con_tbl) == TRUE) 
                        c_mrph_p->con_tbl = rengo_con_tbl;
                }

                if (Show_Opt_debug)
                    printf("----- 連語内 %s %d\n", c_mrph_p->midasi, p_buffer[p_buffer_num].score);
                
                if (++p_buffer_num == process_buffer_max)
                    realloc_process_buffer();
            }
            pos += c_mrph_p->length;

            /* fixed by kuro on 01/01/19
               以下がreallocが上のif-elseの中だたったので，c_mrph_pへの
               アクセスで segmentation fault となっていた */
            if (++m_buffer_num == mrph_buffer_max)
                realloc_mrph_buffer();
        }

        if (Show_Opt_debug) printf("\\end{連語}\n");
    } 

    /* 形態素の場合 */
    else {
        /* 与えられたsegmentationと長さが一致しない場合 */
        if (UseGivenSegmentation_Opt && mrph.length != String2Length[pos]) {
            mrph.weight = STOP_MRPH_WEIGHT;
        }


	/* 重みがSTOP_MRPH_WEIGHTとなっているノードは生成しない */
	if (mrph.weight == STOP_MRPH_WEIGHT) {
            s++; /* 項目間の\n */
	    *pbuf = s;
	    return TRUE;
	}

        /* 連濁に関する制限 */
        if (opt & OPT_DEVOICE) {
            int code = check_code(String, pos);
            if (/* 連濁は文の先頭は不可 */
                pos < BYTES4CHAR || 
                /* カタカナ複合語の連濁は認めない */
                code == KATAKANA &&
                (check_code(String, pos - BYTES4CHAR) == KATAKANA ||
                 check_code(String, pos - BYTES4CHAR) == CHOON)) {
                s++; /* 項目間の\n */
                *pbuf = s;
                return TRUE;
            }
        }

        if (!(opt & OPT_NORMALIZE) ||
            /* 正規化ノードは2字以上("ヵ"は例外)の場合のみ作成 */
            (strlen(mrph.midasi) > BYTES4CHAR || !strncmp(mrph.midasi, uppercase[NORMALIZED_LOWERCASE_KA], BYTES4CHAR))) {
            m_buffer[m_buffer_num] = mrph;
            check_connect(pos, m_buffer_num, opt);
            new_mrph_num = m_buffer_num;
            if (++m_buffer_num == mrph_buffer_max)
                realloc_mrph_buffer();	    

#ifdef NUMERIC_P
            if (suusi_word(pos, new_mrph_num) == FALSE)
                return FALSE;
#endif
#ifdef THROUGH_P
            if (through_word(pos, new_mrph_num) == FALSE)
                return FALSE;
#endif
	}
    }
    s++; /* 項目間の\n */
    *pbuf = s;
    return TRUE;
}

/*
------------------------------------------------------------------------------
        PROCEDURE: <_take_data>                 >>> added by T.Nakamura <<<
------------------------------------------------------------------------------
*/

char *_take_data(char *s, MRPH *mrph, int deleted_bytes, char *opt)
{
    int		i, imi_length;
    char	c, *rep;

    string_decode(&s, mrph->midasi);
    mrph->hinsi    = numeral_decode(&s);
    mrph->bunrui   = numeral_decode(&s);
    mrph->katuyou1 = numeral_decode(&s);
    mrph->katuyou2 = numeral_decode(&s);
    mrph->weight   = numeral_decode(&s);
    mrph->con_tbl  = numeral_decode(&s);
    string_decode(&s, mrph->yomi);
    string_decode(&s, mrph->midasi2);

    if (!mrph->midasi2[0]) strcpy(mrph->midasi2, mrph->midasi);
    mrph->length = strlen(mrph->midasi);

    imi_length = numeral_decode(&s);
    if (imi_length) {
        for (i = 0; i < imi_length; i++) mrph->imis[i] = *(s++);
        mrph->imis[i] = '\0';
    } else {
        strcpy(mrph->imis, NILSYMBOL);
    }

    /* 連濁処理 */
    if (imi_length > 0 && mrph->imis[imi_length - 2] == 'D') {
        mrph->imis[imi_length - 2] = '"'; /* 末尾のDを削除 */
        mrph->imis[imi_length - 1] = '\0';
        if (*opt != OPT_NORMAL)
            mrph->weight = STOP_MRPH_WEIGHT;
        else
            *opt = OPT_DEVOICE;
    }

    /* 正規化したものにペナルティ(小書き文字・長音記号置換された表現用) */
    if ((*opt & OPT_NORMALIZE) && mrph->weight != STOP_MRPH_WEIGHT) {
	mrph->weight += NORMALIZED_COST;
	if (imi_length == 0) {
	    strcpy(mrph->imis, "\"");
	}
	else {
	    mrph->imis[strlen(mrph->imis) - 1] = ' ';
	}
	strcat(mrph->imis, DEF_ABNORMAL_IMIS);
	strcat(mrph->imis, "\"");
        imi_length = strlen(mrph->imis);
    }

    /* 長音挿入したものにペナルティ */
    if ((*opt & OPT_PROLONG_DEL) && mrph->weight != STOP_MRPH_WEIGHT ) {

    /* 元々末尾が長音でないカタカナ語は 末尾の長音削除を認める */
    if ((*opt & OPT_PROLONG_DEL_LAST) && mrph->hinsi == prolong_ng_hinsi2 && 
            deleted_bytes==3 && check_code(mrph->midasi, 0) == KATAKANA && check_code(mrph->midasi, mrph->length -3) == KATAKANA) { 
        /* ただし，フィルターに対し，フィルター，フィルタの両方を出力することを避けるため，
         * ・形態素の開始地点が同じ
         * ・長さが同一, かつ原形の末尾が長音(長音削除を行っていない語)
         * ・意味情報が同じ
         * を満たす形態素がすでに m_buffer にある場合には，認めない 
         */
        int m;
        /* 代表表記が漢字で始まる語は，長音削除を認めない 
         * アリ(蟻)，スキ(隙)，カラ(殻), カバ(河馬), アワ(泡，粟) など */
        char* rep_pos = strstr(mrph->imis,"代表表記:");
        if( rep_pos && check_code((rep_pos + 13), 0) == KANJI ){
            mrph->weight = STOP_MRPH_WEIGHT;
        }else{
            /* 文字ラティスで，長音削除ノードは通常ノードよりも後に追加されるため，OPT_PROLONG_DEL_LAST が設定された形態素を辞書から
             * 引いた時に，長音削除前の形態素が既に引かれて m_buffer に登録されていないか調べる */
            for(m = pre_m_buffer_num;m<m_buffer_num;m++){
                if( m_buffer[m].length == mrph->length+3 && 
                        check_code(m_buffer[m].midasi2, m_buffer[m].length -3) == CHOON &&
                        strcmp(m_buffer[m].imis, mrph->imis) == 0 ){
                    mrph->weight = STOP_MRPH_WEIGHT;
                    break;
                }
            }
        }
    } /* 動詞、名詞、接頭辞、格助詞、1文字の接続助詞・副助詞は長音削除を認めない */ 
    else if ((mrph->hinsi == prolong_ng_hinsi1 || /* 動詞 */
	     mrph->hinsi == prolong_ng_hinsi2 || /* 名詞 */
	     mrph->hinsi == prolong_ng_hinsi3 || /* 接頭辞 */
	     mrph->hinsi == prolong_ng_hinsi4 && mrph->bunrui == prolong_ng_bunrui4_1 || /* 格助詞 */
	     mrph->hinsi == prolong_ng_hinsi4 && mrph->bunrui == prolong_ng_bunrui4_2 && mrph->length == BYTES4CHAR || /* 接続助詞 */
	     mrph->hinsi == prolong_ng_hinsi4 && mrph->bunrui == prolong_ng_bunrui4_3 && mrph->length == BYTES4CHAR) && /* 副助詞 */
	    /* 上記の品詞でも長音挿入可という意味素があれば削除を認める */
	    !strstr(mrph->imis, DEF_PROLONG_OK_FEATURE)) {
	    mrph->weight = STOP_MRPH_WEIGHT;
	}
	else {
	    mrph->weight += (mrph->hinsi == prolong_interjection) ? PROLONG_DEL_COST1 : 
	      (mrph->hinsi == prolong_copula) ? PROLONG_DEL_COST2 : PROLONG_DEL_COST3;

	    if (imi_length == 0) {
		strcpy(mrph->imis, "\"");
	    }
	    else {
		mrph->imis[strlen(mrph->imis) - 1] = ' ';
	    }
	    strcat(mrph->imis, DEF_PROLONG_IMIS);
	    strcat(mrph->imis, "\"");
	}
    }

    mrph->length += deleted_bytes; /* 削除分を足す */
    if (mrph->length >= MIDASI_MAX) { /* MIDASI_MAXを越えるmidasiは作成できない (あとでprepare_path_mrph()で書き換えるときに問題となる) */
        mrph->weight = STOP_MRPH_WEIGHT;
    }

    return(s);
}

int numeral_decode(char **str)
{
    unsigned char *s;

    s = *str;
    *str = s+2;
    return((*s-0x20)*(0x100-0x20)+*(s+1)-0x20-1);
}

void string_decode(char **str, char *out)
{
    while (**str != 0x20 && **str != '\t')
        *(out++) = *(*str)++;
    (*str)++;
    *out = '\0';
}

/*
------------------------------------------------------------------------------
  PROCEDURE: <trim_space> スペースの削除 (透過処理導入後使用せず)
------------------------------------------------------------------------------
*/
int trim_space(int pos)
{
    while (1) {
 	/* 全角スペース */
	if (String[pos] == 0xa1 && String[pos+1] == 0xa1)
	  pos += 2;
	else
	  break;
    }
    return pos;
}
    
/*
------------------------------------------------------------------------------
  PROCEDURE: <undef_word> 未定義語処理
------------------------------------------------------------------------------
*/
    
int undef_word(int pos)
{
    int i, end, code, next_code, cur_bytes;
    
    /* 字種による切りだし
       平仮名，漢字，半角空白，記号 → 一文字
       半角(空白以外)，片仮名(「ー」も)，アルファベット(「．」も) → 連続 */

#if defined(IO_ENCODING_EUC) || defined(IO_ENCODING_SJIS)
    cur_bytes = String[pos]&0x80 ? 2 : 1;
#else
    cur_bytes = utf8_bytes(String + pos);
#endif
    code = check_code(String, pos);
    if (code == HIRAGANA || code == KANJI || code == KIGOU) {
	end = (pos + cur_bytes);
    } else if (code == KUUHAKU) {
	end = (pos + 1);
    } else {
	end = pos;
	while(1) {
	    /* MIDASI_MAXを越える未定義語は作成しない */
	    if (end - pos >= MIDASI_MAX - cur_bytes) break;

	    end += cur_bytes;
	    next_code = check_code(String, end);
#if defined(IO_ENCODING_EUC) || defined(IO_ENCODING_SJIS)
            cur_bytes = String[end]&0x80 ? 2 : 1;
#else
	    cur_bytes = utf8_bytes(String + end);
#endif
	    if (next_code == code ||
		(code == KATAKANA && next_code == CHOON) ||
		(code == ALPH     && next_code == PRIOD)) continue;
	    else break;
	}
    }

    if (UseGivenSegmentation_Opt)
        end = pos + String2Length[pos];

    switch (code) {
    case KUUHAKU:
	m_buffer[m_buffer_num].hinsi = kuuhaku_hinsi;
	m_buffer[m_buffer_num].bunrui = kuuhaku_bunrui;
	m_buffer[m_buffer_num].con_tbl = kuuhaku_con_tbl;
	break;
    case KATAKANA:
	m_buffer[m_buffer_num].hinsi = undef_hinsi;
	m_buffer[m_buffer_num].bunrui = undef_kata_bunrui;
	m_buffer[m_buffer_num].con_tbl = undef_kata_con_tbl;
	break;
    case ALPH:
	m_buffer[m_buffer_num].hinsi = undef_hinsi;
	m_buffer[m_buffer_num].bunrui = undef_alph_bunrui;
	m_buffer[m_buffer_num].con_tbl = undef_alph_con_tbl;
	break;
    default:
	m_buffer[m_buffer_num].hinsi = undef_hinsi;
	m_buffer[m_buffer_num].bunrui = undef_etc_bunrui;
	m_buffer[m_buffer_num].con_tbl = undef_etc_con_tbl;
	break;
    }

    m_buffer[m_buffer_num].katuyou1 = 0;
    m_buffer[m_buffer_num].katuyou2 = 0;
    m_buffer[m_buffer_num].length = end - pos;
    if (end - pos >= MIDASI_MAX) {
	fprintf(stderr, "Too long undef_word<%s>\n", String);
	return FALSE;
    }
    if (code == KUUHAKU) {
	strcpy(m_buffer[m_buffer_num].midasi, "\\ ");
	strcpy(m_buffer[m_buffer_num].yomi, "\\ ");
    } else {
	strncpy(m_buffer[m_buffer_num].midasi, String+pos, end - pos);
	m_buffer[m_buffer_num].midasi[end - pos] = '\0';
	strncpy(m_buffer[m_buffer_num].yomi, String+pos, end - pos);
	m_buffer[m_buffer_num].yomi[end - pos] = '\0';
    }
    m_buffer[m_buffer_num].weight = MRPH_DEFAULT_WEIGHT;
    strcpy(m_buffer[m_buffer_num].midasi2, m_buffer[m_buffer_num].midasi);
    strcpy(m_buffer[m_buffer_num].imis, NILSYMBOL);

    check_connect(pos, m_buffer_num, 0);
    if (++m_buffer_num == mrph_buffer_max)
	realloc_mrph_buffer();

    /* オノマトペの自動認識 */
    if (Repetition_Opt || Onomatopoeia_Opt) {
	recognize_onomatopoeia(pos);
    }

#ifdef THROUGH_P				/* 半角スペース透過 */
    if (code == KUUHAKU) {
	return (through_word(pos, m_buffer_num-1));
    }
#endif

    return TRUE;
}

int check_unicode_char_type(int code)
{
    /* HIRAGANA */
    if (code > 0x303f && code < 0x30a0) {
	return HIRAGANA;
    }
    /* KATAKANA */
    else if (code > 0x309f && code < 0x30fb) {
	return KATAKANA;
    }
    /* CHOON ("ー") */
    else if (code == 0x30fc) {
	return CHOON;
    }
    /* PRIOD */
    else if (code == 0xff0e) {
	return PRIOD;
    }
    /* FIGURE (０-９, 0-9) */
    else if ((code > 0xff0f && code < 0xff1a) || 
             (code > 0x2f && code < 0x3a)) {
	return SUJI;
    }
    /* ALPHABET (A-Z, a-z, ウムラウトなど, Ａ-Ｚ, ａ-ｚ) */
    else if ((code > 0x40 && code < 0x5b) || 
             (code > 0x60 && code < 0x7b) || 
             (code > 0xbf && code < 0x0100) || 
             (code > 0xff20 && code < 0xff3b) || 
	     (code > 0xff40 && code < 0xff5b)) {
	return ALPH;
    }
    /* CJK Unified Ideographs and "々" */
    else if ((code > 0x4dff && code < 0xa000) || code == 0x3005) {
	return KANJI;
    }
    /* α, β, ... */
    else if (code > 0x036f && code < 0x0400) {
	return GR;
    }
    else {
	return KIGOU;
    }
}

/* check the code of a char */
int check_utf8_char_type(unsigned char *ucp)
{
    int code = 0;
    int length = strlen(ucp);
    int unicode;

    unsigned char c = *ucp;
    if (c > 0xfb) { /* 6 bytes */
	code = 0;
    }
    else if (c > 0xf7) { /* 5 bytes */
	code = 0;
    }
    else if (c > 0xef) { /* 4 bytes */
	code = 0;
    }
    else if (c > 0xdf) { /* 3 bytes */
	unicode = (c & 0x0f) << 12;
	c = *(ucp + 1);
	unicode += (c & 0x3f) << 6;
	c = *(ucp + 2);
	unicode += c & 0x3f;
	code = check_unicode_char_type(unicode);
    }
    else if (c > 0x7f) { /* 2 bytes */
	unicode = (c & 0x1f) << 6;
	c = *(ucp + 1);
	unicode += c & 0x3f;
	code = check_unicode_char_type(unicode);
    }
    else { /* 1 byte */
	code = check_unicode_char_type(c);
    }

    return code;
}

int check_code(U_CHAR *cp, int pos)
{
    int	code;
    
    if ( cp[pos] == '\0')			return 0;
    else if ( cp[pos] == KUUHAKU )		return KUUHAKU;
    
#ifdef IO_ENCODING_EUC
    else if ( cp[pos] < HANKAKU )		return HANKAKU;

    code = cp[pos]*256 + cp[pos+1];
    
    if ( code == PRIOD ) 			return PRIOD;
    else if ( code == CHOON ) 			return CHOON;
    else if ( code < KIGOU ) 			return KIGOU;
    else if ( code < SUJI ) 			return SUJI;
    else if ( code < ALPH )			return ALPH;
    else if ( code < HIRAGANA ) 		return HIRAGANA;
    else if ( code < KATAKANA ) 		return KATAKANA;
    else if ( code < GR ) 			return GR;
    else return KANJI;
#else
#ifdef IO_ENCODING_SJIS
    else if ( cp[pos] < HANKAKU )		return HANKAKU;

    code = cp[pos]*256 + cp[pos+1];
    
    if ( code == 0x8144 ) 			return PRIOD;
    else if ( code == 0x815b ) 			return CHOON;
    else if ( code < 0x8200 ) 			return KIGOU;
    else if ( code < 0x8260 ) 			return SUJI;
    else if ( code < 0x829f )			return ALPH;
    else if ( code < 0x8300 ) 			return HIRAGANA;
    else if ( code < 0x839f ) 			return KATAKANA;
    else if ( code < 0x8800 ) 			return GR;
    else return KANJI;
#else /* UTF-8 */
    return check_utf8_char_type(cp + pos);
#endif
#endif
}

/* return the bytes of a char */
int utf8_bytes(unsigned char *ucp)
{
    unsigned char c = *ucp;

    if (c > 0xfb) { /* 6 bytes */
	return 6;
    }
    else if (c > 0xf7) { /* 5 bytes */
	return 5;
    }
    else if (c > 0xef) { /* 4 bytes */
	return 4;
    }
    else if (c > 0xdf) { /* 3 bytes */
	return 3;
    }
    else if (c > 0x7f) { /* 2 bytes */
	return 2;
    }
    else { /* 1 byte */
	return 1;
    }
}

/*
------------------------------------------------------------------------------
  PROCEDURE: <juman_init_etc> 未定義語処理，数詞処理，透過処理等の準備
------------------------------------------------------------------------------
*/
void juman_init_etc(void)
{
    int i, flag;

    /* 未定義語処理の準備 */
    undef_hinsi = get_hinsi_id(DEF_UNDEF);
    undef_kata_bunrui = get_bunrui_id(DEF_UNDEF_KATA, undef_hinsi);
    undef_alph_bunrui = get_bunrui_id(DEF_UNDEF_ALPH, undef_hinsi);
    undef_etc_bunrui = get_bunrui_id(DEF_UNDEF_ETC, undef_hinsi);
    undef_kata_con_tbl = check_table_for_undef(undef_hinsi, undef_kata_bunrui);
    undef_alph_con_tbl = check_table_for_undef(undef_hinsi, undef_alph_bunrui);
    undef_etc_con_tbl = check_table_for_undef(undef_hinsi, undef_etc_bunrui);

    /* 数詞処理の準備 */
    suusi_hinsi = get_hinsi_id(DEF_SUUSI_HINSI);
    suusi_bunrui = get_bunrui_id(DEF_SUUSI_BUNRUI, suusi_hinsi);

    /* 透過処理の準備 */
    kakko_hinsi = get_hinsi_id(DEF_KAKKO_HINSI);
    kakko_bunrui1 = get_bunrui_id(DEF_KAKKO_BUNRUI1, kakko_hinsi);
    kakko_bunrui2 = get_bunrui_id(DEF_KAKKO_BUNRUI2, kakko_hinsi);

    kuuhaku_hinsi = get_hinsi_id(DEF_KUUHAKU_HINSI);
    kuuhaku_bunrui = get_bunrui_id(DEF_KUUHAKU_BUNRUI, kuuhaku_hinsi);
    kuuhaku_con_tbl = check_table_for_undef(kuuhaku_hinsi, kuuhaku_bunrui);

    /* オノマトペ自動認識の準備 */
    onomatopoeia_hinsi = get_hinsi_id(DEF_ONOMATOPOEIA_HINSI);
    onomatopoeia_bunrui = 0;
    onomatopoeia_con_tbl = check_table_for_undef(onomatopoeia_hinsi, onomatopoeia_bunrui);

    /* 連濁処理・小書き文字・長音記号処理の準備 */
    rendaku_hinsi1 = get_hinsi_id(DEF_RENDAKU_HINSI1);
    rendaku_renyou = get_form_id(DEF_RENDAKU_RENYOU, 1); /* 母音動詞(type=1)の基本連用形のform_idを基本連用形の汎用idとみなす */
    rendaku_hinsi2 = get_hinsi_id(DEF_RENDAKU_HINSI2);
    rendaku_bunrui2_1 = get_bunrui_id(DEF_RENDAKU_BUNRUI2_1, rendaku_hinsi2);
    rendaku_bunrui2_2 = get_bunrui_id(DEF_RENDAKU_BUNRUI2_2, rendaku_hinsi2);
    rendaku_bunrui2_3 = get_bunrui_id(DEF_RENDAKU_BUNRUI2_3, rendaku_hinsi2); /* 直前が形式名詞は不可 */
    rendaku_hinsi3 = get_hinsi_id(DEF_RENDAKU_HINSI3);
    rendaku_hinsi4 = get_hinsi_id(DEF_RENDAKU_HINSI4);
    rendaku_bunrui4_1 = get_bunrui_id(DEF_RENDAKU_BUNRUI4_1, rendaku_hinsi4);
    rendaku_bunrui4_2 = get_bunrui_id(DEF_RENDAKU_BUNRUI4_2, rendaku_hinsi4);
    rendaku_bunrui4_3 = get_bunrui_id(DEF_RENDAKU_BUNRUI4_3, rendaku_hinsi4);
    rendaku_bunrui4_4 = get_bunrui_id(DEF_RENDAKU_BUNRUI4_4, rendaku_hinsi4);
    prolong_interjection = get_hinsi_id(DEF_PROLONG_INTERJECTION);
    prolong_copula = get_hinsi_id(DEF_PROLONG_COPULA);
    prolong_ng_hinsi1 = get_hinsi_id(DEF_PROLONG_NG_HINSI1);
    prolong_ng_hinsi2 = get_hinsi_id(DEF_PROLONG_NG_HINSI2);
    prolong_ng_hinsi3 = get_hinsi_id(DEF_PROLONG_NG_HINSI3);
    prolong_ng_hinsi4 = get_hinsi_id(DEF_PROLONG_NG_HINSI4);
    prolong_ng_bunrui4_1 = get_bunrui_id(DEF_PROLONG_NG_BUNRUI4_1, prolong_ng_hinsi4);
    prolong_ng_bunrui4_2 = get_bunrui_id(DEF_PROLONG_NG_BUNRUI4_2, prolong_ng_hinsi4);
    prolong_ng_bunrui4_3 = get_bunrui_id(DEF_PROLONG_NG_BUNRUI4_3, prolong_ng_hinsi4);
}

/*
------------------------------------------------------------------------------
  PROCEDURE: <suusi_word> 数詞処理
------------------------------------------------------------------------------
*/
    
int 	suusi_word(int pos , int m_num)
{
    int i, j;
    MRPH *new_mrph , *pre_mrph;

    new_mrph = &m_buffer[m_num];
    if (new_mrph->hinsi != suusi_hinsi || new_mrph->bunrui != suusi_bunrui)
        return TRUE;

    for (j = 0; (i = match_pbuf[j]) >= 0; j++) {
	pre_mrph = &m_buffer[p_buffer[i].mrph_p];
	if (pre_mrph->hinsi == suusi_hinsi &&
	    pre_mrph->bunrui == suusi_bunrui &&
	    check_matrix(pre_mrph->con_tbl, new_mrph->con_tbl) != 0) {

	    if (strlen(pre_mrph->midasi)+strlen(new_mrph->midasi) >= SUUSI_MIDASI_MAX || 
		strlen(pre_mrph->yomi)+strlen(new_mrph->yomi) >= SUUSI_YOMI_MAX) {
        /* 零が連続した際に読みの候補数が爆発するため，より短い SUUSI_MIDASI_MAX, SUUSI_YOMI_MAX を設定 2014/08/14 */
		/* MIDASI_MAX、YOMI_MAXを越える数詞は分割するように変更 08/01/15 */
		/* fprintf(stderr, "Too long suusi<%s>\n", String);
		   return FALSE; */
		return TRUE;
	    }
	    m_buffer[m_buffer_num] = *pre_mrph;
	    strcat(m_buffer[m_buffer_num].midasi, new_mrph->midasi);
	    strcat(m_buffer[m_buffer_num].yomi, new_mrph->yomi);
	    strcat(m_buffer[m_buffer_num].midasi2, new_mrph->midasi2);
	    m_buffer[m_buffer_num].length += strlen(new_mrph->midasi);
	    /* コストは後ろ側の方を継承 */
	    m_buffer[m_buffer_num].weight = new_mrph->weight;
	    m_buffer[m_buffer_num].con_tbl = new_mrph->con_tbl;

	    p_buffer[p_buffer_num] = p_buffer[i];
	    p_buffer[p_buffer_num].end = pos+strlen(new_mrph->midasi);
	    p_buffer[p_buffer_num].mrph_p = m_buffer_num;
	    p_buffer[p_buffer_num].score += (new_mrph->weight-pre_mrph->weight)
		*Class[new_mrph->hinsi][new_mrph->bunrui].cost
		*cost_omomi.keitaiso;

	    if (++m_buffer_num == mrph_buffer_max) {
		realloc_mrph_buffer();
	        new_mrph = &m_buffer[m_num];	/* fixed by kuro 99/09/01 */
	    }
	    if (++p_buffer_num == process_buffer_max)
		realloc_process_buffer();
	}
    }
    return TRUE;
}

/*
------------------------------------------------------------------------------
  PROCEDURE: <through_word> 括弧，空白の透過処理
------------------------------------------------------------------------------
*/
    
int 	through_word(int pos , int m_num)
{
    int i, j, k, l, n, nn, sc, scmin, tmp_path;
    MRPH *now_mrph, *mrph_p;

    now_mrph = &m_buffer[m_num];
    if (!is_through(now_mrph)) return TRUE;

    for (l = 0; (i = match_pbuf[l]) >= 0; l++) {
	for (j = 0 ; j < m_buffer_num ; j++) {
	    mrph_p = &m_buffer[j];
	    if (mrph_p->hinsi   == now_mrph->hinsi   &&
		mrph_p->bunrui  == now_mrph->bunrui  &&
		mrph_p->con_tbl == m_buffer[p_buffer[i].mrph_p].con_tbl &&
		mrph_p->weight  == now_mrph->weight  &&
		strcmp(mrph_p->midasi, now_mrph->midasi) == 0 &&
		strcmp(mrph_p->yomi,   now_mrph->yomi  ) == 0) break;
	}
	if ((n = j) == m_buffer_num) {
	    /* その文の中で初めて出現したタイプの括弧であれば，直前の
	       形態素のcon_tblを透過させるようにcon_tblを変化させて，
	       m_bufferに追加する */
	    m_buffer[m_buffer_num] = *now_mrph;
	    m_buffer[m_buffer_num].con_tbl
	      = m_buffer[p_buffer[i].mrph_p].con_tbl;
	    if (++m_buffer_num == mrph_buffer_max) {
		realloc_mrph_buffer();
		now_mrph = &m_buffer[m_num];	/* fixed by kuro 99/09/01 */
	    }
	}

	/* 透過規則に基づく連接 */
	sc = (now_mrph->weight*cost_omomi.keitaiso*
	      Class[now_mrph->hinsi][now_mrph->bunrui].cost);
	for (j = 0 ; j < p_buffer_num ; j++)
	  if (p_buffer[j].mrph_p == n && p_buffer[j].start == pos) break;
	if ((nn = j) == p_buffer_num) {
	    /* 初めて現われるような透過なら，p_bufferに新たに追加する */
	    p_buffer[p_buffer_num].score = p_buffer[i].score+sc;
	    p_buffer[p_buffer_num].mrph_p = n;
	    p_buffer[p_buffer_num].start = pos;
	    p_buffer[p_buffer_num].end = pos + now_mrph->length;
	    p_buffer[p_buffer_num].path[0] = i;
	    p_buffer[p_buffer_num].path[1] = -1;
	    p_buffer[p_buffer_num].connect = TRUE;
	    if (++p_buffer_num == process_buffer_max)
		realloc_process_buffer();
	} else {
	    /* p_bufferの爆発を防ぐため，以前のp_bufferの使い回しをする */
	    for (j = 0 ; p_buffer[nn].path[j] != -1 ; j++) {}
	    p_buffer[nn].path[j]   = i;
	    p_buffer[nn].path[j+1] = -1;

	    /* コストが高いものは枝刈りする */
	    scmin = INT_MAX;
	    for (j = 0 ; p_buffer[nn].path[j] != -1 ; j++)
	      if (scmin > p_buffer[p_buffer[nn].path[j]].score)
		scmin = p_buffer[p_buffer[nn].path[j]].score;
	    for (j = 0 ; p_buffer[nn].path[j] != -1 ; j++)
	      if (p_buffer[p_buffer[nn].path[j]].score
		  > scmin + cost_omomi.cost_haba) {
		  for (k = j ; p_buffer[nn].path[k] != -1 ; k++)
		    p_buffer[nn].path[k] = p_buffer[nn].path[k+1];
		  j--;
	      }

            /* もっともコストが小さいpathを0番目にもってくる */
	    for (j = 1 ; p_buffer[nn].path[j] != -1 ; j++) {
                if (p_buffer[p_buffer[nn].path[j]].score == scmin) {
                    tmp_path = p_buffer[nn].path[0];
                    p_buffer[nn].path[0] = p_buffer[nn].path[j];
                    p_buffer[nn].path[j] = tmp_path;
                    break;
                }
            }

	    p_buffer[nn].score = scmin+sc;
	}
    }
    return TRUE;
}

/*
------------------------------------------------------------------------------
  PROCEDURE: <is_through>
------------------------------------------------------------------------------
*/
    
int 	is_through(MRPH *mrph_p)
{
    if (
	/* 括弧始は透過処理をしない 2007/12/06
	(mrph_p->hinsi == kakko_hinsi && mrph_p->bunrui == kakko_bunrui1) ||
	*/
	(mrph_p->hinsi == kakko_hinsi && mrph_p->bunrui == kakko_bunrui2) ||
	(mrph_p->hinsi == kuuhaku_hinsi && mrph_p->bunrui == kuuhaku_bunrui))
	return TRUE;
    else 
	return FALSE;
}

char **OutputAV;
int OutputAVnum;
int OutputAVmax;

MRPH *prepare_path_mrph(int path_num , int para_flag)
{
    MRPH       	*mrph_p;
    int        	j;

    mrph_p = &(m_buffer[p_buffer[path_num].mrph_p]);

    if (para_flag != 0 && is_through(mrph_p) == TRUE) return NULL;
    
    if (para_flag)
	strcpy(kigou, "@ ");
    else
	kigou[0] = '\0';
    strcpy(midasi1, mrph_p->midasi);
    strcpy(midasi2, *mrph_p->midasi2 ? mrph_p->midasi2 : mrph_p->midasi);
    strcpy(yomi, mrph_p->yomi);

    /* 連濁、小書き文字、長音記号処理用 */
    if (strcmp(midasi1, "\\ ") && /* 空白は入力と異なるが、そのまま出力 */
        strncmp(midasi1, String + p_buffer[path_num].start, mrph_p->length)) {
	strncpy(midasi1, String + p_buffer[path_num].start, mrph_p->length);
	midasi1[mrph_p->length] = '\0';
    }

    return mrph_p;
}

char *get_path_mrph(int path_num , int para_flag)
{
    int len = 0;
    MRPH *mrph_p;
    char *ret;

    if ((mrph_p = prepare_path_mrph(path_num, para_flag)) == NULL) return NULL;
    len = strlen(kigou)+strlen(midasi1)+strlen(yomi)+strlen(midasi2)+strlen(Class[mrph_p->hinsi][0].id)+mrph_p->hinsi/10+1;

    if ( mrph_p->bunrui ) {
	len += strlen(Class[mrph_p->hinsi][mrph_p->bunrui].id);
    }
    else {
	len += 1;
    }

    len += mrph_p->bunrui/10+1;
	
    if ( mrph_p->katuyou1 ) { 
	len += strlen(Type[mrph_p->katuyou1].name);
    }
    else {
	len += 1;
    }

    len += mrph_p->katuyou1/10+1;
	
    if ( mrph_p->katuyou2 ) {
	len += strlen(Form[mrph_p->katuyou1][mrph_p->katuyou2].name);
    }
    else {
	len += 1;
    }

    len += mrph_p->katuyou2/10+1;

    len += 12;	/* 隙間 10, 改行 1, 終端 1 */

    switch (Show_Opt2) {
    case Op_E2:
	len += strlen(mrph_p->imis) + 1;
	ret = (char *)malloc(len);
	sprintf(ret, "%s%s %s %s %s %d %s %d %s %d %s %d %s\n", kigou, midasi1, yomi, midasi2, 
		Class[mrph_p->hinsi][0].id, mrph_p->hinsi, 
		mrph_p->bunrui ? Class[mrph_p->hinsi][mrph_p->bunrui].id : (U_CHAR *)"*", mrph_p->bunrui, 
		mrph_p->katuyou1 ? Type[mrph_p->katuyou1].name : (U_CHAR *)"*", mrph_p->katuyou1, 
		mrph_p->katuyou2 ? Form[mrph_p->katuyou1][mrph_p->katuyou2].name : (U_CHAR *)"*", mrph_p->katuyou2, 
		mrph_p->imis);
	break;
    case Op_E:
	ret = (char *)malloc(len);
	sprintf(ret, "%s%s %s %s %s %d %s %d %s %d %s %d\n", kigou, midasi1, yomi, midasi2, 
		Class[mrph_p->hinsi][0].id, mrph_p->hinsi, 
		mrph_p->bunrui ? Class[mrph_p->hinsi][mrph_p->bunrui].id : (U_CHAR *)"*", mrph_p->bunrui, 
		mrph_p->katuyou1 ? Type[mrph_p->katuyou1].name : (U_CHAR *)"*", mrph_p->katuyou1, 
		mrph_p->katuyou2 ? Form[mrph_p->katuyou1][mrph_p->katuyou2].name : (U_CHAR *)"*", mrph_p->katuyou2);
	break;
    }
    return ret;
}

int get_best_path_num()
{
    int j, last;

    j = 0;
    last = p_buffer_num-1;
    do {
	last = p_buffer[last].path[0];
	path_buffer[j] = last;
	j++;
    } while ( p_buffer[last].path[0] );

    return j;
}

/*
------------------------------------------------------------------------------
  PROCEDURE: <print_path_mrph> 形態素の表示      >>> changed by yamaji <<<
------------------------------------------------------------------------------
*/
/*
  サーバーモード対応のため、引数outputを増やして出力先の変更を可能にする。
  NACSIS 吉岡
*/
/* para_flag != 0 なら @出力 */
void print_path_mrph(FILE* output, int path_num , int para_flag)
{
    PROCESS_BUFFER	*proc_p;
    MRPH       	*mrph_p;
    int		newDicNo;
    int         now_r_buffer_num;
    MRPH        r_last_mrph;
    int		pos;
    int        	i, j, k, len;

    if ((mrph_p = prepare_path_mrph(path_num, para_flag)) == NULL) return;
    proc_p = &(p_buffer[path_num]);
    pos = proc_p->start;

    fputs(kigou, output);

    switch (Show_Opt2) {
    case Op_F: 
	len = strlen(yomi); yomi[len] = ')'; yomi[len+1] = '\0';
	my_fprintf(output, "%-12.12s(%-12.12s%-10.10s %-14.14s",
		midasi1, yomi, midasi2,
		Class[mrph_p->hinsi][mrph_p->bunrui].id);
	if (mrph_p->katuyou1)
	    my_fprintf(output, " %-14.14s %-12.12s",
		    Type[mrph_p->katuyou1].name,
		    Form[mrph_p->katuyou1][mrph_p->katuyou2].name);
	fputc('\n', output);
	break;
	
    case Op_C:
	my_fprintf(output, "%s %s %s %d %d %d %d\n", midasi1, yomi, midasi2,
		mrph_p->hinsi, mrph_p->bunrui,
		mrph_p->katuyou1, mrph_p->katuyou2);
	break;

    case Op_EE:
	/* ラティスのつながりを表示 */
	fprintf(output, "%d ", path_num);
	for (i = 0; proc_p->path[i] != -1; i++) {
	    if (i) fprintf(output, ";");
	    fprintf(output, "%d", proc_p->path[i]);
	}
	fprintf(output, " ");

	fprintf(output, "%d ", pos);
	if (!strcmp(midasi1, "\\ ")) pos++; else pos += strlen(midasi1);
	fprintf(output, "%d ", pos);
	/* -E オプションはこの部分だけ追加,
	   その後は -e -e2 オプションと同様の出力をするので break しない */

    case Op_E:
    case Op_E2:
	my_fprintf(output, "%s %s %s ", midasi1, yomi, midasi2);
	
	my_fprintf(output, "%s ", Class[mrph_p->hinsi][0].id);
	fprintf(output, "%d ", mrph_p->hinsi);
	
	if ( mrph_p->bunrui ) 
	  my_fprintf(output, "%s ", Class[mrph_p->hinsi][mrph_p->bunrui].id);
	else
	  my_fprintf(output, "* ");
	fprintf(output, "%d ", mrph_p->bunrui);
	
	if ( mrph_p->katuyou1 ) 
	  my_fprintf(output, "%s ", Type[mrph_p->katuyou1].name);
	else                    
	  fprintf(output, "* ");
	fprintf(output, "%d ", mrph_p->katuyou1);
	
	if ( mrph_p->katuyou2 ) 
	  my_fprintf(output, "%s ", Form[mrph_p->katuyou1][mrph_p->katuyou2].name);
	else 
	  fprintf(output, "* ");
	fprintf(output, "%d", mrph_p->katuyou2);

	if (Show_Opt2 == Op_E) {
	    fprintf(output, "\n");	/* -e では imis は出力しない */
	    break;
	}

	/* for SRI
	   fprintf(stdout, "\n");
	   if (para_flag) fprintf(stdout , "@ ");
	   */

	my_fprintf(output, " %s\n", mrph_p->imis);
	break;
    
    case Op_G:
	my_fprintf(output, "%s ", midasi2);
    }

}

void process_path_mrph(FILE* output, int path_num , int para_flag) {
    if (output) {
	print_path_mrph(output, path_num, para_flag);
    }
    else {
	char *p;
	if (OutputAVnum == 0) {
	    OutputAVmax = 10;
	    OutputAV = (char **)malloc(sizeof(char *)*OutputAVmax);
	}
	else if (OutputAVnum >= OutputAVmax-1) {
	    OutputAV = (char **)realloc(OutputAV, sizeof(char *)*(OutputAVmax <<= 1));
	}
	p =  get_path_mrph(path_num, para_flag);
	if (p != NULL) {
	    *(OutputAV+OutputAVnum++) = p;
	    *(OutputAV+OutputAVnum) =  NULL;
	}
    }
}

/*
------------------------------------------------------------------------------
  PROCEDURE: <print_best_path> 後方最長一致のPATHを調べる
------------------------------------------------------------------------------
*/
/*
  サーバーモード対応のため、引数outputを増やして出力先の変更を可能にする。
  NACSIS 吉岡
*/
char **print_best_path(FILE* output)
{
    int i, j, last;
    MRPH *mrph_p,*mrph_p1;

    j = 0;
    last = p_buffer_num-1;
    do {
	last = p_buffer[last].path[0];
	path_buffer[j] = last;
	j++;
    } while ( p_buffer[last].path[0] );

    /* 結果を buffer に入れる場合 */
    if (!output) {
	OutputAVnum = 0;
	OutputAVmax = 0;
    }

    for ( i=j-1; i>=0; i-- ) {
	process_path_mrph(output, path_buffer[i] , 0);
    }
    return OutputAV;	/* 後でちゃんと free すること */
}

/*
------------------------------------------------------------------------------
  PROCEDURE: <print_all_mrph> 正しい解析結果に含まれる全ての形態素
------------------------------------------------------------------------------
*/
/*
  サーバーモード対応のため、引数outputを増やして出力先の変更を可能にする。
  NACSIS 吉岡
*/
char **print_all_mrph(FILE* output)
{
    int  i, j, k;
    MRPH mrph;

    for (i = 0 ; i < m_buffer_num ; i++)
	m_check_buffer[i] = 0;
    
    _print_all_mrph(output, p_buffer_num-1);
    m_check_buffer[0] = 0;

    /* 結果を buffer に入れる場合 */
    if (!output) {
	OutputAVnum = 0;
	OutputAVmax = 0;
    }

    for (i = 0 ; i < m_buffer_num ; i++)
	if (m_check_buffer[i])
	    process_path_mrph(output, i , 0);

    return OutputAV;	/* 後でちゃんと free すること */
}

void _print_all_mrph(FILE* output, int path_num)
{
    int i;
    
    for (i = 0 ; p_buffer[path_num].path[i] != -1 ; i++) {
	if (!m_check_buffer[p_buffer[path_num].path[i]]) {
	    m_check_buffer[p_buffer[path_num].path[i]] = 1;
	    _print_all_mrph(output, p_buffer[path_num].path[i]);
	}
    }
}
/*
------------------------------------------------------------------------------
  PROCEDURE: <print_all_path> 正しい解析結果に含まれる全てのPATH
------------------------------------------------------------------------------
*/
/*
  サーバーモード対応のため、引数outputを増やして出力先の変更を可能にする。
  NACSIS 吉岡
*/
char **print_all_path(FILE* output)
{
/*  int i,j;
    for (i = 0 ; i < p_buffer_num ; i++) {
	printf("%d %s %d %d --- " , i , m_buffer[p_buffer[i].mrph_p].midasi ,
	       p_buffer[i].start , p_buffer[i].end);
	for (j = 0 ; p_buffer[i].path[j] != -1 ; j++)
	  printf("%d ",p_buffer[i].path[j]);
	printf("\n");
    }*/

    /* 結果を buffer に入れる場合 */
    if (!output) {
	OutputAVnum = 0;
	OutputAVmax = 0;
    }

    _print_all_path(output, p_buffer_num-1, 0);
    return OutputAV;	/* 後でちゃんと free すること */   
}

void _print_all_path(FILE* output, int path_num, int pathes)
{
    int i, j;
    
    for ( i=0; p_buffer[path_num].path[i] != -1; i++ ) {
	if ( p_buffer[path_num].path[0] == 0 ) {
	    for ( j=pathes-1; j>=0; j-- )
	      process_path_mrph(output, path_buffer[j] , 0);
	    if (output) fprintf(output, "EOP\n");
	} else {
	    path_buffer[pathes] = p_buffer[path_num].path[i];
	    _print_all_path(output, p_buffer[path_num].path[i], pathes+1);
	}
    }
}

/*
------------------------------------------------------------------------------
  PROCEDURE: <print_homograph_path> 複数の候補をとれる形態素は列挙する
------------------------------------------------------------------------------
*/
/*
  サーバーモード対応のため、引数outputを増やして出力先の変更を可能にする。
  NACSIS 吉岡
*/
char **print_homograph_path(FILE* output)
{
    path_buffer[0] = p_buffer_num-1;
    path_buffer[1] = -1;

    /* 結果を buffer に入れる場合 */
    if (!output) {
	OutputAVnum = 0;
	OutputAVmax = 0;
    }

    _print_homograph_path(output, 0, 2);
    return OutputAV;	/* 後でちゃんと free すること */
}

int _print_homograph_path(FILE* output, int pbuf_start, int new_p)
{
    int i, j, k, l, now_pos, len, ll, pt, pt2, f;

    if (p_buffer[path_buffer[pbuf_start]].path[0] == 0) {
	/* 先頭までリンクをたどり終わった */
	for (j = new_p-2; j >= 1; j--) {
	    /* 同音異義語群を一気に出力 */
	    for ( ; path_buffer[j] >= 0; j--) {}
	    for (k = j+1, l = 0; path_buffer[k] >= 0; k++) {
		process_path_mrph(output, path_buffer[k] , l++);
	    }
	}
	if (Show_Opt1 == Op_BB) return(1);
	if (output) fprintf(output, "EOP\n");
	return(0);
    }

    /* 特殊優先規則
           3文字の複合語は 2-1
	   4文字の複合語は 2-2
	   5文字の複合語は 3-2
       と分割されることが多いので，この分割を優先的に出力するようにする．
       そのため，
       ・2文字語の左に2,3,4文字語が連接している場合は2文字語を優先
       ・それ以外ならば文字数の少ない語を優先
       とする． */
    f = 0;
    now_pos = p_buffer[path_buffer[pbuf_start]].start;
    for (j = pbuf_start; path_buffer[j] >= 0; j++)
	for (i = 0; (pt = p_buffer[path_buffer[j]].path[i]) != -1; i++){
        // bestよりコストが高いパスは探索しない(コスト幅を使ったときにスコアの低い変なパスを選ばないため)
        if(p_buffer[pt].score > p_buffer[p_buffer[path_buffer[j]].path[0]].score)
            break;
	    /* 2文字語を探す */
	    if (p_buffer[pt].start == now_pos-2*BYTES4CHAR) {
		for (k = 0; (pt2 = p_buffer[pt].path[k]) != -1; k++)
		    /* 2文字語の左に連接している語が2〜4文字語か？ */
		    if (p_buffer[pt2].start <= now_pos-(2+2)*BYTES4CHAR &&
			p_buffer[pt2].start >= now_pos-(2+4)*BYTES4CHAR) f = 1;
	    }
    }

    for (ll = 1; ll <= now_pos; ll++) { /* 半角文字のため，1byteごとに処理 */
	len = ll;
	if (f)
	    /* 1文字語と2文字語の優先順位を入れ替える */
	    if (ll == BYTES4CHAR) len = BYTES4CHAR * 2; else if (ll == BYTES4CHAR * 2) len = BYTES4CHAR;

	/* 同じ長さの形態素を全てリストアップする */
	l = new_p;
	for (j = pbuf_start; path_buffer[j] >= 0; j++) {
	    for (i = 0; (pt = p_buffer[path_buffer[j]].path[i]) != -1; i++) {
        if(p_buffer[pt].score > p_buffer[p_buffer[path_buffer[j]].path[0]].score)
            break;
		if (p_buffer[pt].start == now_pos-len) {
		    /* 同音異義語群を求める(但し重複しないようにする) */
		    for (k = new_p; k < l; k++)
			if (path_buffer[k] == pt) break;
		    if (k == l) {
                        path_buffer[l++] = pt;
                        if (l >= process_buffer_max)
                            realloc_process_buffer();
                    }
		}
	    }
	}
	path_buffer[l] = -1; /* 同音異義語群のエンドマーク */
	if (l != new_p)
	    if (_print_homograph_path(output, new_p, l+1)) return(1);
    }
    return(0);
}

/*
------------------------------------------------------------------------------
  PROCEDURE: <pos_match_process> <pos_right_process>
------------------------------------------------------------------------------
*/
int pos_match_process(int pos, int p_start)
{
    int i, j;

    /* 位置がマッチする p_buffer を抽出して，match_pbuf に書き並べる */
    j = 0;
    for (i = p_start; i < p_buffer_num; i++) {
	if (p_buffer[i].end <= pos || p_buffer[i].connect == FALSE) {
	    if (i == p_start)
	      /* p_start 以前の p_buffer は十分 pos が小さいので，探索を省略 */
	      p_start++;
	    if (p_buffer[i].end == pos && p_buffer[i].connect == TRUE)
	      match_pbuf[j++] = i;
	}
    }
    match_pbuf[j] = -1;

    return p_start;
}

int pos_right_process(int pos)
{
    int	i;
    
    for ( i=0; i<p_buffer_num; i++ )
      if ( p_buffer[i].end > pos )
	return 1;
    
    return 0;
}

/*
------------------------------------------------------------------------------
  PROCEDURE: <check_connect>			Changed by yamaji
------------------------------------------------------------------------------
*/

int check_connect(int pos, int m_num, char opt)
{
    static CHK_CONNECT_WK chk_connect[MAX_PATHES_WK];
    int chk_con_num;
    int i, j, pathes;
    int	score, best_score, haba_score, best_score_num;
    int c_score, class_score;
    MRPH *new_mrph;
    MRPH *pre_mrph;
    int left_con, right_con;
    CONNECT_COST *c_cache;

    new_mrph = &m_buffer[m_num];
    best_score = INT_MAX;  /* maximam value of int */
    chk_con_num = 0;
    score = class_score = best_score_num = 0;
    class_score = Class[new_mrph->hinsi][new_mrph->bunrui].cost 
	* new_mrph->weight * cost_omomi.keitaiso;

    /* 連接キャッシュにヒットすれば，探索を行わない */
    c_cache = &connect_cache[rensetu_tbl[new_mrph->con_tbl].j_pos];
    if (Show_Opt_debug == 0) {
	if (c_cache->pos == pos && c_cache->p_no > 0 && c_cache->opt == opt) {
	    p_buffer[p_buffer_num].score = c_cache->cost + class_score;
	    p_buffer[p_buffer_num].mrph_p = m_num;
	    p_buffer[p_buffer_num].start = pos;
	    p_buffer[p_buffer_num].end = pos + new_mrph->length;
	    for (i = 0; (p_buffer[p_buffer_num].path[i] = p_buffer[c_cache->p_no].path[i]) >= 0; i++) {}
	    p_buffer[p_buffer_num].path[i] = -1;
	    p_buffer[p_buffer_num].connect = TRUE;
	    if (++p_buffer_num == process_buffer_max)
		realloc_process_buffer();
	    return TRUE;
	}
    }

    for (i = 0; (j = match_pbuf[i]) >= 0; i++) {

	/* 以前の形態素のcon_tblを取り出す */
	left_con = m_buffer[p_buffer[j].mrph_p].con_tbl;
	/* 新しい形態素のcon_tblを取り出す */
	right_con = new_mrph->con_tbl;

	c_score = check_matrix(left_con , right_con);
	
	/* 濁音化するのは直前の形態素が名詞、または動詞の連用形、名詞性接尾辞の場合のみ */
	/* 接尾辞の場合を除き直前の形態素が平仮名1文字となるものは不可 */
	if ((opt & OPT_DEVOICE) &&
	    (!(m_buffer[p_buffer[j].mrph_p].hinsi == rendaku_hinsi1 &&
	       m_buffer[p_buffer[j].mrph_p].katuyou2 == rendaku_renyou ||
	       m_buffer[p_buffer[j].mrph_p].hinsi == rendaku_hinsi2 &&
	       m_buffer[p_buffer[j].mrph_p].bunrui != rendaku_bunrui2_3 ||
	       m_buffer[p_buffer[j].mrph_p].hinsi == rendaku_hinsi4 &&
	       (m_buffer[p_buffer[j].mrph_p].bunrui == rendaku_bunrui4_1 ||
		m_buffer[p_buffer[j].mrph_p].bunrui == rendaku_bunrui4_2 ||
		m_buffer[p_buffer[j].mrph_p].bunrui == rendaku_bunrui4_3 ||
		m_buffer[p_buffer[j].mrph_p].bunrui == rendaku_bunrui4_4)) ||
	     (m_buffer[p_buffer[j].mrph_p].hinsi != rendaku_hinsi4 &&
	      check_code(m_buffer[p_buffer[j].mrph_p].midasi, 0) == HIRAGANA &&
	      m_buffer[p_buffer[j].mrph_p].length == BYTES4CHAR))) c_score = 0;

	if (c_score) {
	    chk_connect[chk_con_num].pre_p = j;

	    /* calculate the score */
	    score = p_buffer[j].score + c_score * cost_omomi.rensetsu;

	    chk_connect[chk_con_num].score = score;
	    if (score < best_score) {
		best_score = score;
		best_score_num = chk_con_num;
	    }
	    chk_con_num++;
            if (chk_con_num >= MAX_PATHES_WK) {
                /* chk_connect[] holds only MAX_PATHES_WK */
                break;
            }
	}

	/* デバグ用コスト表示 */

	if (Show_Opt_debug == 2 || (Show_Opt_debug == 1 && c_score)) {

	    fprintf(stderr, "%3d " , pos);

	    pre_mrph = &m_buffer[p_buffer[j].mrph_p];
	    fprintf(stderr, "%s" , pre_mrph->midasi);
	    if (Class[pre_mrph->hinsi][0].id) {
		fprintf(stderr, "(");
		if (strcmp(pre_mrph->midasi, pre_mrph->midasi2))
                    fprintf(stderr, "%s:" , pre_mrph->midasi2);                    
		fprintf(stderr, "%s" , Class[pre_mrph->hinsi][0].id);
		if (pre_mrph->bunrui)
		    fprintf(stderr, "-%s", Class[pre_mrph->hinsi][pre_mrph->bunrui].id);
		if (pre_mrph->katuyou1)
		    fprintf(stderr, "-%s", Type[pre_mrph->katuyou1].name);
		if (pre_mrph->katuyou2)
		    fprintf(stderr, "-%s", Form[pre_mrph->katuyou1][pre_mrph->katuyou2].name);
		fprintf(stderr, ")");
	    }
	    fprintf(stderr, "[= %d]", p_buffer[j].score);

	    if (c_score) {
		fprintf(stderr, "--[+%d*%d]--", c_score, cost_omomi.rensetsu);
	    } else {
		fprintf(stderr, "--XXX--");
	    }

	    fprintf(stderr, "%s" , new_mrph->midasi);
	    if (Class[new_mrph->hinsi][0].id) {
                fprintf(stderr, "(");
		if (strcmp(new_mrph->midasi, new_mrph->midasi2))
                    fprintf(stderr, "%s:" , new_mrph->midasi2);                    
		fprintf(stderr, "%s" , Class[new_mrph->hinsi][0].id);
		if (new_mrph->bunrui)
		    fprintf(stderr, "-%s",Class[new_mrph->hinsi][new_mrph->bunrui].id);
		if (new_mrph->katuyou1)
		    fprintf(stderr, "-%s", Type[new_mrph->katuyou1].name);
		if (new_mrph->katuyou2)
		    fprintf(stderr, "-%s", Form[new_mrph->katuyou1][new_mrph->katuyou2].name);
		fprintf(stderr, ")");
	    }

	    if (c_score == 0) {
		fprintf(stderr, "\n");
	    } else {
		fprintf(stderr, "[+%d*%d.%d*%d = %d]\n", 
			Class[new_mrph->hinsi][new_mrph->bunrui].cost,
			new_mrph->weight/10, new_mrph->weight%10, 
			cost_omomi.keitaiso*10, 
			/* = class_score */
			p_buffer[j].score + c_score * cost_omomi.rensetsu + class_score);
	    }
	}
    }

    /* return immidiately, because if best_score is
       INT_MAX then no path exists. */
    if (best_score == INT_MAX) return TRUE;

    /* 今回の連接の様子を連接キャッシュに入れる */
    c_cache->p_no = p_buffer_num;
    c_cache->cost = best_score;
    c_cache->pos = pos;
    c_cache->opt = opt;

    /* 取り敢えずベストパスの1つを0番に登録 */
    p_buffer[p_buffer_num].path[0] = chk_connect[best_score_num].pre_p;
    pathes = 1;
    haba_score = best_score + cost_omomi.cost_haba;
    for (j = 0; j < chk_con_num; j++) { /* それ以外のパスも追加 */
        if (chk_connect[j].score <= haba_score && j != best_score_num) {
            if (pathes >= MAX_PATHES - 1) /* path holds only MAX_PATHES */
                break;
            p_buffer[p_buffer_num].path[pathes++] = chk_connect[j].pre_p;
        }
    }
    p_buffer[p_buffer_num].path[pathes] = -1;

    p_buffer[p_buffer_num].score = best_score+class_score;
    p_buffer[p_buffer_num].mrph_p = m_num;
    p_buffer[p_buffer_num].start = pos;
    p_buffer[p_buffer_num].end = pos + new_mrph->length;
    p_buffer[p_buffer_num].connect = TRUE;
    if (++p_buffer_num == process_buffer_max)
	realloc_process_buffer();
    return TRUE;
}

CHAR_NODE *make_new_node(CHAR_NODE **current_char_node_ptr, char *chr, int type)
{
    int i;
    CHAR_NODE *new_char_node = (CHAR_NODE *)malloc(sizeof(CHAR_NODE));
    strcpy(new_char_node->chr, chr);
    new_char_node->type = type;
    new_char_node->da_node_pos_num = 0;
    for (i = 0; i < MAX_NODE_POS_NUM; i++)
        new_char_node->p_buffer[i] = NULL;
    (*current_char_node_ptr)->next = new_char_node;
    *current_char_node_ptr = new_char_node;
    CharLatticeUsedFlag = TRUE;
    return new_char_node;
}

/*
------------------------------------------------------------------------------
  PROCEDURE: <juman_sent> 一文を形態素解析する    by T.Utsuro
------------------------------------------------------------------------------
*/
int juman_sent(void)
{
    int        i, j, pre_code, post_code;
    int        pos_end, length;
    int        pre_p_buffer_num;
    int        pos, next_pos = 0, pre_byte_length = 0, local_deleted_num = 0;
    int	       p_start = 0, count = 0;
    int        code;
    int        next_pre_is_deleted, pre_is_deleted = 0; /* 直前の文字が削除文字(長音, 小文字) */
    CHAR_NODE  *current_char_node, *new_char_node;

    if (mrph_buffer_max == 0) {
	m_buffer = (MRPH *)my_alloc(sizeof(MRPH)*BUFFER_BLOCK_SIZE);
	m_check_buffer = (int *)my_alloc(sizeof(int)*BUFFER_BLOCK_SIZE);
	mrph_buffer_max += BUFFER_BLOCK_SIZE;
    }
    if (process_buffer_max == 0) {
	p_buffer = (PROCESS_BUFFER *)
	    my_alloc(sizeof(PROCESS_BUFFER)*BUFFER_BLOCK_SIZE);
	path_buffer = (int *)my_alloc(sizeof(int)*BUFFER_BLOCK_SIZE);
	match_pbuf = (int *)my_alloc(sizeof(int)*BUFFER_BLOCK_SIZE);
	process_buffer_max += BUFFER_BLOCK_SIZE;
    }

    /* 与えられた区切りを使う場合 */
    if (UseGivenSegmentation_Opt) {
        char *token;
        char *dup_String = strdup(String);
        length = strlen(dup_String);
        String[0] = '\0';
        for (pos = 0; pos < length; pos++) String2Length[pos] = 0;
        token = strtok(dup_String, ":");
        while (token) {
            String2Length[count] = strlen(token);
            count += strlen(token);
            strcat(String, token);
            token = strtok(NULL, ":");
        }
        free(dup_String);
    }

    length = strlen(String);

    if (length == 0) return FALSE;	/* 空行はスキップ */

    for (i = 0; i < CONNECT_MATRIX_MAX; i++) connect_cache[i].p_no = 0;

    /* 文頭処理 */
    p_buffer[0].end = 0;		
    p_buffer[0].path[0] = -1;
    p_buffer[0].score = 0;	
    p_buffer[0].mrph_p = 0;
    p_buffer[0].connect = TRUE;
    m_buffer[0].hinsi = 0;
    m_buffer[0].bunrui = 0;
    m_buffer[0].con_tbl = 0;
    m_buffer[0].weight = MRPH_DEFAULT_WEIGHT;
    strcpy(m_buffer[0].midasi , "(文頭)");
    m_buffer_num = 1;
    p_buffer_num = 1;

    /* initialization for root node (starting node for looking up double array) */
    CharRootNode.next = NULL;
    CharRootNode.da_node_pos[0] = 0;
    CharRootNode.deleted_bytes[0] = 0;
    CharRootNode.p_buffer[0] = NULL;
    CharRootNode.da_node_pos_num = 1;

    CharLatticeUsedFlag = FALSE;
    CharNum = 0;
    for (pos = 0; pos < length; pos+=next_pos) {
        current_char_node = &CharLattice[CharNum];
	if (String[pos]&0x80) { /* 全角の場合 */
	    /* 長音記号による置換 */
        /* (String[pos]がーもしくは〜) && (posが文最後の文字列か次の文字が記号かひらがな) && 二文字目以降 */
        if (LongSoundRep_Opt &&
                (!strncmp(String + pos, DEF_PROLONG_SYMBOL1, BYTES4CHAR) || !strncmp(String + pos, DEF_PROLONG_SYMBOL2, BYTES4CHAR)) && 
                (!String[pos + BYTES4CHAR] || check_code(String, pos + BYTES4CHAR) == KIGOU || check_code(String, pos + BYTES4CHAR) == HIRAGANA) &&
                (pos > 0) /* 2文字目以降 */
                /* 次の文字が"ー","〜"でない */
                /*  strncmp(String + pos + BYTES4CHAR, DEF_PROLONG_SYMBOL1, BYTES4CHAR) && */
                /*  strncmp(String + pos + BYTES4CHAR, DEF_PROLONG_SYMBOL2, BYTES4CHAR)) */
                ) {
		for (i = 0; *pre_prolonged[i]; i++) {
		    if (!strncmp(String + pos - pre_byte_length, pre_prolonged[i], pre_byte_length)) {
                        new_char_node = make_new_node(&current_char_node, prolonged2chr[i], OPT_NORMALIZE | OPT_PROLONG_REPLACE);
                        break;
		    }
		}
	    }
	    /* 小書き文字による置換 */
	    else if (LowercaseRep_Opt) {
		for (i = NORMALIZED_LOWERCASE_S; i < NORMALIZED_LOWERCASE_E; i++) {
		    if (!strncmp(String + pos, lowercase[i], BYTES4CHAR)) {
                        new_char_node = make_new_node(&current_char_node, uppercase[i], OPT_NORMALIZE);
                        break;
		    }
		}
	    }

            /* 濁音化した文字の場合に、清音も文字を追加 (辞書展開によりオフに) */
            if (Rendaku_Opt) {
                code = check_code(String, pos);
                if (pos > 0 && 
                    (code == HIRAGANA || 
                     (code == KATAKANA && 
                      check_code(String, pos - pre_byte_length) != KATAKANA &&
                      check_code(String, pos - pre_byte_length) != CHOON))) {
                    for (i = VOICED_CONSONANT_S; i < VOICED_CONSONANT_E; i++) {
                        if (!strncmp(String + pos, dakuon[i], BYTES4CHAR)) {
                            new_char_node = make_new_node(&current_char_node, seion[i], OPT_DEVOICE);
                            break;
                        }
                    }
                }
            }
#if defined(IO_ENCODING_EUC) || defined(IO_ENCODING_SJIS)
            next_pos = 2;
#else
	    next_pos = utf8_bytes(String + pos);
#endif
	} else {
	    next_pos = 1;
	}
        strncpy(CharLattice[CharNum].chr, String + pos, next_pos);
        CharLattice[CharNum].chr[next_pos] = '\0';
        CharLattice[CharNum].type = OPT_NORMAL;
        next_pre_is_deleted = 0;

	/* 長音挿入 */
	if ((LongSoundDel_Opt || LowercaseDel_Opt) && next_pos == BYTES4CHAR) {
	    pre_code = (pos > 0) ? check_code(String, pos - pre_byte_length) : -1;
	    post_code = check_code(String, pos + next_pos); /* 文末の場合は0 */
	    if (LongSoundDel_Opt && pre_code > 0 && (WORD_CHAR_NUM_MAX + local_deleted_num + 1) * BYTES4CHAR < MIDASI_MAX &&
		/* 直前が削除された長音記号、平仮名、または、漢字かつ直後が平仮名 */
		((pre_is_deleted ||
		  pre_code == HIRAGANA || pre_code == KATAKANA 
          || pre_code == KANJI && post_code == HIRAGANA) &&
		 /* "ー"または"〜" */
		 (!strncmp(String + pos, DEF_PROLONG_SYMBOL1, BYTES4CHAR) ||
		  !strncmp(String + pos, DEF_PROLONG_SYMBOL2, BYTES4CHAR))) ||
		/* 直前が長音記号で、現在文字が"っ"、かつ、直後が文末、または、記号の場合も削除 */
		(pre_is_deleted && !strncmp(String + pos, DEF_PROLONG_SYMBOL3, BYTES4CHAR) &&
		 (post_code == 0 || post_code == KIGOU))) {
                local_deleted_num++;
                next_pre_is_deleted = 1;
                new_char_node = make_new_node(&current_char_node, "", OPT_PROLONG_DEL);
	    }
	    else if (LowercaseDel_Opt && pre_code > 0 && (WORD_CHAR_NUM_MAX + local_deleted_num + 1) * BYTES4CHAR < MIDASI_MAX) {
		/* 小書き文字による長音化 */
		for (i = DELETE_LOWERCASE_S; i < DELETE_LOWERCASE_E; i++) {
		    if (!strncmp(String + pos, lowercase[i], BYTES4CHAR)) {
			for (j = pre_lower_start[i]; j < pre_lower_end[i]; j++) {
			    if (!strncmp(String + pos - pre_byte_length, pre_lower[j], pre_byte_length)) break;			    
			}
			/* 直前の文字が対応する平仮名、または、削除された同一の小書き文字の場合は削除 */
			if (j < pre_lower_end[i] ||
			    pre_is_deleted && !strncmp(String + pos - pre_byte_length, String + pos, pre_byte_length)) {
                            local_deleted_num++;
                            next_pre_is_deleted = 1;
                            new_char_node = make_new_node(&current_char_node, "", OPT_PROLONG_DEL);
			    break;
			}
		    }
		}
	    }
            else {
                local_deleted_num = 0;
            }
	}
        pre_is_deleted = next_pre_is_deleted;
        pre_byte_length = next_pos;
        current_char_node->next = NULL;
        CharNum++;
    }

    count = 0;
    for (pos = 0; pos < length; pos+=next_pos) {

	p_start = pos_match_process(pos, p_start);
	if (match_pbuf[0] >= 0) {
	
	    pre_m_buffer_num = m_buffer_num;
	    pre_p_buffer_num = p_buffer_num;

            if (!UseGivenSegmentation_Opt || String2Length[pos]) {	
                if (search_all(pos, count) == FALSE) return FALSE;
                if (undef_word(pos) == FALSE) return FALSE;
            }
	}

#if defined(IO_ENCODING_EUC) || defined(IO_ENCODING_SJIS)
        next_pos = String[pos]&0x80 ? 2 : 1;
#else
	next_pos = utf8_bytes(String + pos);
#endif
        count++;
    }
    
    /* 文末処理 */
    strcpy(m_buffer[m_buffer_num].midasi , "(文末)");
    m_buffer[m_buffer_num].hinsi = 0;
    m_buffer[m_buffer_num].bunrui = 0;
    m_buffer[m_buffer_num].con_tbl = 0;
    m_buffer[m_buffer_num].weight = MRPH_DEFAULT_WEIGHT;
    if (++m_buffer_num == mrph_buffer_max)
	realloc_mrph_buffer();

    pos_match_process(pos, p_start);
    if (check_connect(length, m_buffer_num-1, 0) == FALSE)
        return FALSE;

    return TRUE;
}
