/* Multibyte Character Functions.
Copyright (C) 1998 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.  */

/* Note regarding cross compilation:

	In general, translation of multibyte characters to wide characters can
	only work in a native compiler since the translation function (mbtowc)
	needs to know about both the source and target character encoding.  However,
	this particular implementation for JIS, SJIS and EUCJP source characters
	will work for any compiler with a newlib target.  Other targets may also
	work provided that their wchar_t implementation is 2 bytes and the encoding
	leaves the source character values unchanged (except for removing the
	state shifting markers).  */
	
/* 跨平台移植说明：

	由于转换函数（mbtowc）需要知道目标和源的字符编码方式，所以一般情况下
	多字节（multibyte）的转换只能在本地的编译器中完成。然而，通过一种newlib目标，
	对于JIS，SJIS和EUCJP源字符的实现可以在任何编译器中实现。其他目标也可能可行，
	它们wchar_t的实现是通过两个字节并且编码不会改变源字符的值（除了状态移动标志）。
*/

#include "config.h"
#ifdef MULTIBYTE_CHARS
#include "system.h"
#include "mbchar.h"
#include <locale.h>

/* JIS字符类型 */
typedef enum {
	ESCAPE, DOLLAR, BRACKET, AT, B, J, NUL, JIS_CHAR, OTHER,
	JIS_C_NUM
} 从;

/* JIS状态 */
typedef enum {
	ASCII, A_ESC, A_ESC_DL, JIS, JIS_1, JIS_2, J_ESC, J_ESC_BR,
	J2_ESC, J2_ESC_BR, INV, JIS_S_NUM
} JIS_STATE;

/* JIS动作 */
typedef enum {
	COPYA, COPYJ, COPYJ2, MAKE_A, MAKE_J, NOOP,
	EMPTY, ERROR
} JIS_ACTION;

/* State/action tables for processing JIS encoding:

Where possible, switches to JIS are grouped with proceding JIS characters
and switches to ASCII are grouped with preceding JIS characters.
Thus, maximum returned length is:
2 (switch to JIS) + 2 (JIS characters) + 2 (switch back to ASCII) = 6.  */

/* 处理JIS编码的状态/动作表：
	
	
*/
static JIS_STATE JIS_state_table[JIS_S_NUM][JIS_C_NUM] = {
	/*					ESCAPE	DOLLAR		BRACKET		AT		B		J		NUL		JIS_CHAR	OTH		*/
	/*ASCII*/		{	A_ESC,	ASCII,		ASCII,		ASCII,	ASCII,	ASCII,	ASCII,	ASCII,		ASCII	},
	/*A_ESC*/		{	ASCII,	A_ESC_DL,	ASCII,		ASCII,	ASCII,	ASCII,	ASCII,	ASCII,		ASCII	},
	/*A_ESC_DL*/	{	ASCII,	ASCII,		ASCII,		JIS,	JIS,	ASCII,	ASCII,	ASCII,		ASCII	},
	/*JIS*/			{	J_ESC,	JIS_1,		JIS_1,		JIS_1,	JIS_1,	JIS_1,	INV,	JIS_1,		INV		},
	/*JIS_1*/		{	INV,	JIS_2,		JIS_2,		JIS_2,	JIS_2,	JIS_2,	INV,	JIS_2,		INV		},
	/*JIS_2*/		{	J2_ESC,	JIS,		JIS,		JIS,	JIS,	JIS,	INV,	JIS,		JIS		},
	/*J_ESC*/		{	INV,	INV,		J_ESC_BR,	INV,	INV,	INV,	INV,	INV,		INV		},
	/*J_ESC_BR*/	{	INV,	INV,		INV,		INV,	ASCII,	ASCII,	INV,	INV,		INV		},
	/*J2_ESC*/		{	INV,	INV,		J2_ESC_BR,	INV,	INV,	INV,	INV,	INV,		INV		},
	/*J2_ESC_BR*/	{	INV,	INV,		INV,		INV,	ASCII,	ASCII,	INV,	INV,		INV		},
};

static JIS_ACTION JIS_action_table[JIS_S_NUM][JIS_C_NUM] = {
	/*            		ESCAPE	DOLLAR	BRACKET	AT		B		J		NUL		JIS_CHAR	OTH		*/
	/*ASCII */		{	NOOP,	COPYA,	COPYA,	COPYA,	COPYA,	COPYA,	EMPTY,	COPYA,		COPYA	},
	/*A_ESC */		{	COPYA,	NOOP,	COPYA,	COPYA,	COPYA,	COPYA,	COPYA,	COPYA,		COPYA	},
	/*A_ESC_DL */	{	COPYA,	COPYA,	COPYA,	MAKE_J,	MAKE_J,	COPYA,	COPYA,	COPYA,		COPYA	},
	/*JIS */		{	NOOP,	NOOP,	NOOP,	NOOP,	NOOP,	NOOP,	ERROR,	NOOP,		ERROR	},
	/*JIS_1 */		{	ERROR,	NOOP,	NOOP,	NOOP,	NOOP,	NOOP,	ERROR,	NOOP,		ERROR	},
	/*JIS_2 */		{	NOOP,	COPYJ2,	COPYJ2,	COPYJ2,	COPYJ2,	COPYJ2,	ERROR,	COPYJ2,		COPYJ2	},
	/*J_ESC */		{	ERROR,	ERROR,	NOOP,	ERROR,	ERROR,	ERROR,	ERROR,	ERROR,		ERROR	},
	/*J_ESC_BR */	{	ERROR,	ERROR,	ERROR,	ERROR,	NOOP,	NOOP,	ERROR,	ERROR,		ERROR	},
	/*J2_ESC */		{	ERROR,	ERROR,	NOOP,	ERROR,	ERROR,	ERROR,	ERROR,	ERROR,		ERROR	},
	/*J2_ESC_BR*/	{	ERROR,	ERROR,	ERROR,	ERROR,	COPYJ,	COPYJ,	ERROR,	ERROR,		ERROR	},
};


const char *literal_codeset = NULL;

/* 	Store into *PWC (if PWC is not null) the wide character
	corresponding to the multibyte character at the start of the
	buffer S of size N.  Return the number of bytes in the multibyte
	character.  Return -1 if the bytes do not form a valid character,
	or 0 if S is null or points to a null byte.

	This function behaves like the Standard C function mbtowc, except
	it treats locale names of the form "C-..." specially.
*/

/*	将对应N大小的缓冲S开头的多字节字符的宽字符保存到*PWC中（PWC不为空）。
	返回多字节字符中的字节数。如果字节不是来自有效的字符，返回-1；
	S为空或者指向一个空字节的话，返回0。
	
	这个函数和C标准函数mbtowc相似，除了对于格式为"C-..."的区域名的处理特别。
*/

int
local_mbtowc(pwc, s, n)
wchar_t *pwc;
const char *s;
size_t n;
{
	/* 记录与保存状态 */
	static JIS_STATE save_state = ASCII;
	JIS_STATE curr_state = save_state;
	
	const unsigned char *t = (const unsigned char *)s;

	/* 如果s指向的字符串大小为零，返回-1。 */
	if (s != NULL && n == 0)
		return -1;

	/* 根据literal_codeset判断是哪一种编码方式。 */
	if (literal_codeset == NULL || strlen(literal_codeset) <= 1)
		/* This must be the "C" locale or unknown locale -- fall thru */
		/* 这种情况是"C"的区域或者是未知的区域，跳过。 */
		;
	else if (!strcmp(literal_codeset, "C-SJIS"))
	/* literal_codeset是C-SJIS */
	{
		int char1;
		if (s == NULL)
			/* Not state-dependent.  */
			/* s为空，状态无关。 */
			return 0;

		char1 = *t;
		if (ISSJIS1(char1))
		/* 如果第一个字节是SJIS1类型的 */
		{
			/* 第二个字节的值 */
			int char2 = t[1];

			/* 如果s指向的字符串大小不大于1，返回-1。 */
			if (n <= 1)
				return -1;

			/* 第二个字节是SJIS2类型的话 */
			if (ISSJIS2(char2))
			{
				/* 合并开头的两个字节到一个wchar_t中 */
				if (pwc != NULL)
					*pwc = (((wchar_t)*t) << 8) + (wchar_t)(*(t + 1));
				/* multibyte大小为2 */
				return 2;
			}

			/* C-SJIS模式下，第一个字符是SJIS1类型的，第二个字符就必须是SJIS2类型的。 */
			return -1;
		}

		/* 第一个字符不是SJIS1类型的，直接将这个字符当作wchar_t类型的。 */
		if (pwc != NULL)
			*pwc = (wchar_t)*t;

		/* 换行符的话，大小为0. */
		if (*t == '\0')
			return 0;

		/* 这里的话，大小为1。 */
		return 1;
	}
	else if (!strcmp(literal_codeset, "C-EUCJP"))
	/* 编码方式是"C-EUCJP"的话。
		这种情况的处理和前一种基本相同的处理：
		合并两个字节或者就一个字节。
	*/
	{
		int char1;

		if (s == NULL)
			/* Not state-dependent.  */
			return 0;

		char1 = *t;
		if (ISEUCJP(char1))
		{
			int char2 = t[1];

			if (n <= 1)
				return -1;

			if (ISEUCJP(char2))
			{
				if (pwc != NULL)
					*pwc = (((wchar_t)*t) << 8) + (wchar_t)(*(t + 1));
				return 2;
			}

			return -1;
		}

		if (pwc != NULL)
			*pwc = (wchar_t)*t;

		if (*t == '\0')
			return 0;

		return 1;
	}
	else if (!strcmp(literal_codeset, "C-JIS"))
	/* 编码方式是"C-JIS" */
	{
		JIS_ACTION action;
		JIS_CHAR_TYPE ch;
		const unsigned char *ptr;
		size_t i, curr_ch;

		if (s == NULL)
		/* s为空，默认编码方式为ASCII，返回1。 */
		{
			save_state = ASCII;
			/* State-dependent.  */
			return 1;
		}

		ptr = t;

		/* 对s指向的字符串中的每一个字符进行处理。 */
		for (i = 0; i < n; i++)
		{
			/* 取一个字节字符 */
			curr_ch = t[i];
			switch (curr_ch)
			/* 确定当前的字符的类型 */
			{
			case JIS_ESC_CHAR:
				ch = ESCAPE;
				break;
			case '$':
				ch = DOLLAR;
				break;
			case '@':
				ch = AT;
				break;
			case '(':
				ch = BRACKET;
				break;
			case 'B':
				ch = B;
				break;
			case 'J':
				ch = J;
				break;
			case '\0':
				ch = NUL;
				break;
			default:/* 其他字符是JIS_CHAR和OTHER类型 */
				if (ISJIS(curr_ch))
					ch = JIS_CHAR;
				else
					ch = OTHER;
			}

			/* 根据字符类型（ch）和当前状态找到对应的动作和状态转移目标 */
			action = JIS_action_table[curr_state][ch];
			curr_state = JIS_state_table[curr_state][ch];

			switch (action)
			/* 根据动作处理 */
			{
			case NOOP:/* 空操作，什么都不做。 */
				break;

			case EMPTY:/* 空字符 */
				if (pwc != NULL)
					*pwc = (wchar_t)0;

				save_state = curr_state;/* OK，保存当前状态。 */
				return i;

			case COPYA:/* 复制字符ASCII */
				if (pwc != NULL)
					*pwc = (wchar_t)*ptr;
				save_state = curr_state;/* OK，保存当前状态。 */
				return i + 1;

			case COPYJ:/* 复制字符SJIS1 */
				if (pwc != NULL)
					*pwc = (((wchar_t)*ptr) << 8) + (wchar_t)(*(ptr + 1));

				save_state = curr_state;/* OK，保存当前状态。 */
				return i + 1;

			case COPYJ2:/* 复制字符SJIS2 */
				if (pwc != NULL)
					*pwc = (((wchar_t)*ptr) << 8) + (wchar_t)(*(ptr + 1));

				save_state = curr_state;/* OK，保存当前状态。 */
				return ptr - t + 2;

			case MAKE_A:
			case MAKE_J:
				/* 移动ptr指针 */
				ptr = (const unsigned char *)(t + i + 1);
				break;

			case ERROR:/* 错误和意外的动作。 */
			default:
				return -1;
			}
		}

		/* More than n bytes needed.  */
		return -1;
	}

/* 处理识别不了的情况 */
#ifdef CROSS_COMPILE /* 跨平台的 */
	if (s == NULL)
		/* Not state-dependent.  */
		return 0;

	if (pwc != NULL)
		*pwc = *s;
	return 1;
#else /* 平台依赖的 */

	/* This must be the "C" locale or unknown locale.  */
	return mbtowc(pwc, s, n);
#endif
}

/* Return the number of bytes in the multibyte character at the start
of the buffer S of size N.  Return -1 if the bytes do not form a
valid character, or 0 if S is null or points to a null byte.

This function behaves like the Standard C function mblen, except
it treats locale names of the form "C-..." specially.  */

int
local_mblen(s, n)
const char *s;
size_t n;
{
	return local_mbtowc(NULL, s, n);
}

/* Return the maximum mumber of bytes in a multibyte character.

This function returns the same value as the Standard C macro MB_CUR_MAX,
except it treats locale names of the form "C-..." specially.  */

int
local_mb_cur_max()
{
	if (literal_codeset == NULL || strlen(literal_codeset) <= 1)
		;
	else if (!strcmp(literal_codeset, "C-SJIS"))
		return 2;
	else if (!strcmp(literal_codeset, "C-EUCJP"))
		return 2;
	else if (!strcmp(literal_codeset, "C-JIS"))
		return 8; /* 3 + 2 + 3 */

#ifdef CROSS_COMPILE
	return 1;
#else
	if (MB_CUR_MAX > 0)
		return MB_CUR_MAX;

	return 1; /* default */
#endif
}
#else  /* MULTIBYTE_CHARS */
extern int dummy;  /* silence 'ANSI C forbids an empty source file' warning */
#endif /* MULTIBYTE_CHARS */
