/*
==============================================================================
	zentohan.c
		1991/01/08/Tue	Yutaka MYOKI(Nagao Lab., KUEE)
		1991/01/08/Tue	Last Modified
==============================================================================
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STDIO_H
#include	<stdio.h>
#endif

#ifdef HAVE_STRING_H
#include	<string.h>
#endif

/*
------------------------------------------------------------------------------
	LOCAL:
	definition of macros
------------------------------------------------------------------------------
*/

#define		U_CHAR		unsigned char
#define		iskanji(x)	((x)&&0x80)

/*
------------------------------------------------------------------------------
	LOCAL:
	definition of global variables
------------------------------------------------------------------------------
*/

static	U_CHAR	hankaku_table[]=
     "!\"#$%&()*+,-.'/0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^abcdefghijklmnopqrstuvwxyz{|}^~_ --" ;

static U_CHAR	zenkaku_table[]=
     "！”＃＄％＆（）＊＋，−．’／０１２３４５６７８９：；＜＝＞？＠ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺ［￥］＾ａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ｛｜｝＾￣＿　‐―" ;

static U_CHAR	str_buffer[100000];

/*
------------------------------------------------------------------------------
	FUNCTION:
	<zentohan>: convert (zenkaku)str -> (hankaku)str_out
------------------------------------------------------------------------------
*/

U_CHAR *zentohan(U_CHAR *str) 
{
     U_CHAR	*str_out = str_buffer;
  
     while (*str) {
	  if (iskanji(*str) && iskanji(*(str+1))) {
	       int		ptr;

	       for (ptr = 0; *(zenkaku_table + ptr); ptr += 2) {
		    if (zenkaku_table[ptr]   == *(str) &&
			zenkaku_table[ptr+1] == *(str+1)) {
			 *str_out++ = hankaku_table[ptr/2];
			 break;
		    }
	       }
	       if (*(zenkaku_table + ptr) == 0) {
		    *str_out++ = *(str  );
		    *str_out++ = *(str+1);
	       }
	       str += 2;
	  } else {
	       *str_out++ = *str++;
	  }
     }
     *str_out = 0;
     return str_buffer;
}

/*
------------------------------------------------------------------------------
	FUNCTION
	<hantozen>: convert (hankaku)str -> (zenkaku)str_out
------------------------------------------------------------------------------
*/

U_CHAR *hantozen(U_CHAR *str)
{
     U_CHAR	 *str_out = str_buffer;

     while (*str) {
	  U_CHAR	*str_tmp;

	  if ((str_tmp = (U_CHAR *)strchr(hankaku_table, *str)) != NULL) {
	       int	ptr = str_tmp-hankaku_table;

	       *str_out++ = zenkaku_table[ptr<<1];
	       *str_out++ = zenkaku_table[(ptr<<1)+1];
	  } else {
	       *str_out++ = *str;
	  }
	  str++;
     }
     *str_out = 0;
     return str_buffer;
}
