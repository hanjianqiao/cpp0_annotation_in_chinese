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
	
/* ��ƽ̨��ֲ˵����

	����ת��������mbtowc����Ҫ֪��Ŀ���Դ���ַ����뷽ʽ������һ�������
	���ֽڣ�multibyte����ת��ֻ���ڱ��صı���������ɡ�Ȼ����ͨ��һ��newlibĿ�꣬
	����JIS��SJIS��EUCJPԴ�ַ���ʵ�ֿ������κα�������ʵ�֡�����Ŀ��Ҳ���ܿ��У�
	����wchar_t��ʵ����ͨ�������ֽڲ��ұ��벻��ı�Դ�ַ���ֵ������״̬�ƶ���־����
*/

#include "config.h"
#ifdef MULTIBYTE_CHARS
#include "system.h"
#include "mbchar.h"
#include <locale.h>

/* JIS�ַ����� */
typedef enum {
	ESCAPE, DOLLAR, BRACKET, AT, B, J, NUL, JIS_CHAR, OTHER,
	JIS_C_NUM
} ��;

/* JIS״̬ */
typedef enum {
	ASCII, A_ESC, A_ESC_DL, JIS, JIS_1, JIS_2, J_ESC, J_ESC_BR,
	J2_ESC, J2_ESC_BR, INV, JIS_S_NUM
} JIS_STATE;

/* JIS���� */
typedef enum {
	COPYA, COPYJ, COPYJ2, MAKE_A, MAKE_J, NOOP,
	EMPTY, ERROR
} JIS_ACTION;

/* State/action tables for processing JIS encoding:

Where possible, switches to JIS are grouped with proceding JIS characters
and switches to ASCII are grouped with preceding JIS characters.
Thus, maximum returned length is:
2 (switch to JIS) + 2 (JIS characters) + 2 (switch back to ASCII) = 6.  */

/* ����JIS�����״̬/������
	
	
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

/*	����ӦN��С�Ļ���S��ͷ�Ķ��ֽ��ַ��Ŀ��ַ����浽*PWC�У�PWC��Ϊ�գ���
	���ض��ֽ��ַ��е��ֽ���������ֽڲ���������Ч���ַ�������-1��
	SΪ�ջ���ָ��һ�����ֽڵĻ�������0��
	
	���������C��׼����mbtowc���ƣ����˶��ڸ�ʽΪ"C-..."���������Ĵ����ر�
*/

int
local_mbtowc(pwc, s, n)
wchar_t *pwc;
const char *s;
size_t n;
{
	/* ��¼�뱣��״̬ */
	static JIS_STATE save_state = ASCII;
	JIS_STATE curr_state = save_state;
	
	const unsigned char *t = (const unsigned char *)s;

	/* ���sָ����ַ�����СΪ�㣬����-1�� */
	if (s != NULL && n == 0)
		return -1;

	/* ����literal_codeset�ж�����һ�ֱ��뷽ʽ�� */
	if (literal_codeset == NULL || strlen(literal_codeset) <= 1)
		/* This must be the "C" locale or unknown locale -- fall thru */
		/* ���������"C"�����������δ֪������������ */
		;
	else if (!strcmp(literal_codeset, "C-SJIS"))
	/* literal_codeset��C-SJIS */
	{
		int char1;
		if (s == NULL)
			/* Not state-dependent.  */
			/* sΪ�գ�״̬�޹ء� */
			return 0;

		char1 = *t;
		if (ISSJIS1(char1))
		/* �����һ���ֽ���SJIS1���͵� */
		{
			/* �ڶ����ֽڵ�ֵ */
			int char2 = t[1];

			/* ���sָ����ַ�����С������1������-1�� */
			if (n <= 1)
				return -1;

			/* �ڶ����ֽ���SJIS2���͵Ļ� */
			if (ISSJIS2(char2))
			{
				/* �ϲ���ͷ�������ֽڵ�һ��wchar_t�� */
				if (pwc != NULL)
					*pwc = (((wchar_t)*t) << 8) + (wchar_t)(*(t + 1));
				/* multibyte��СΪ2 */
				return 2;
			}

			/* C-SJISģʽ�£���һ���ַ���SJIS1���͵ģ��ڶ����ַ��ͱ�����SJIS2���͵ġ� */
			return -1;
		}

		/* ��һ���ַ�����SJIS1���͵ģ�ֱ�ӽ�����ַ�����wchar_t���͵ġ� */
		if (pwc != NULL)
			*pwc = (wchar_t)*t;

		/* ���з��Ļ�����СΪ0. */
		if (*t == '\0')
			return 0;

		/* ����Ļ�����СΪ1�� */
		return 1;
	}
	else if (!strcmp(literal_codeset, "C-EUCJP"))
	/* ���뷽ʽ��"C-EUCJP"�Ļ���
		��������Ĵ����ǰһ�ֻ�����ͬ�Ĵ���
		�ϲ������ֽڻ��߾�һ���ֽڡ�
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
	/* ���뷽ʽ��"C-JIS" */
	{
		JIS_ACTION action;
		JIS_CHAR_TYPE ch;
		const unsigned char *ptr;
		size_t i, curr_ch;

		if (s == NULL)
		/* sΪ�գ�Ĭ�ϱ��뷽ʽΪASCII������1�� */
		{
			save_state = ASCII;
			/* State-dependent.  */
			return 1;
		}

		ptr = t;

		/* ��sָ����ַ����е�ÿһ���ַ����д��� */
		for (i = 0; i < n; i++)
		{
			/* ȡһ���ֽ��ַ� */
			curr_ch = t[i];
			switch (curr_ch)
			/* ȷ����ǰ���ַ������� */
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
			default:/* �����ַ���JIS_CHAR��OTHER���� */
				if (ISJIS(curr_ch))
					ch = JIS_CHAR;
				else
					ch = OTHER;
			}

			/* �����ַ����ͣ�ch���͵�ǰ״̬�ҵ���Ӧ�Ķ�����״̬ת��Ŀ�� */
			action = JIS_action_table[curr_state][ch];
			curr_state = JIS_state_table[curr_state][ch];

			switch (action)
			/* ���ݶ������� */
			{
			case NOOP:/* �ղ�����ʲô�������� */
				break;

			case EMPTY:/* ���ַ� */
				if (pwc != NULL)
					*pwc = (wchar_t)0;

				save_state = curr_state;/* OK�����浱ǰ״̬�� */
				return i;

			case COPYA:/* �����ַ�ASCII */
				if (pwc != NULL)
					*pwc = (wchar_t)*ptr;
				save_state = curr_state;/* OK�����浱ǰ״̬�� */
				return i + 1;

			case COPYJ:/* �����ַ�SJIS1 */
				if (pwc != NULL)
					*pwc = (((wchar_t)*ptr) << 8) + (wchar_t)(*(ptr + 1));

				save_state = curr_state;/* OK�����浱ǰ״̬�� */
				return i + 1;

			case COPYJ2:/* �����ַ�SJIS2 */
				if (pwc != NULL)
					*pwc = (((wchar_t)*ptr) << 8) + (wchar_t)(*(ptr + 1));

				save_state = curr_state;/* OK�����浱ǰ״̬�� */
				return ptr - t + 2;

			case MAKE_A:
			case MAKE_J:
				/* �ƶ�ptrָ�� */
				ptr = (const unsigned char *)(t + i + 1);
				break;

			case ERROR:/* ���������Ķ����� */
			default:
				return -1;
			}
		}

		/* More than n bytes needed.  */
		return -1;
	}

/* ����ʶ���˵���� */
#ifdef CROSS_COMPILE /* ��ƽ̨�� */
	if (s == NULL)
		/* Not state-dependent.  */
		return 0;

	if (pwc != NULL)
		*pwc = *s;
	return 1;
#else /* ƽ̨������ */

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
