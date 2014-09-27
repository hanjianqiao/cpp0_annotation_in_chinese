/* Dependency generator for Makefile fragments.
Copyright (C) 2000, 2001 Free Software Foundation, Inc.
Contributed by Zack Weinberg, Mar 2000

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

In other words, you are welcome to use, share and improve this program.
You are forbidden to forbid anyone else to use, share and improve
what you give them.   Help stamp out software-hoarding!  */

#include "config.h"
#include "system.h"
#include "mkdeps.h"

/* Keep this structure local to this file, so clients don't find it
easy to start making assumptions.  */
/* 将这个结构体保存在本文件内，这样用户就不容易使用断言了 */
struct deps
{
	const char **targetv;
	/* 实际占用的槽数 */
	unsigned int ntargets;	/* number of slots actually occupied */
	/* 总的空间大小，以字为单位 */
	unsigned int targets_size;	/* amt of allocated space - in words */

	const char **depv;
	unsigned int ndeps;
	unsigned int deps_size;
};

static const char *munge	PARAMS((const char *));

/* Given a filename, quote characters in that filename which are
significant to Make.  Note that it's not possible to quote all such
characters - e.g. \n, %, *, ?, [, \ (in some contexts), and ~ are
not properly handled.  It isn't possible to get this right in any
current version of Make.  (??? Still true?  Old comment referred to
3.76.1.)  */
/* 将给定的文件名中需要处理的字符引用起来。由于环境的不同，一些特殊的字符可能是
无法处理的。
*/
static const char *
munge(filename)
const char *filename;
{
	int len;
	const char *p, *q;
	char *dst, *buffer;

	/* 遍历文件名中的字符，计算需要的空间大小 */
	for (p = filename, len = 0; *p; p++, len++)
	{
		/* 处理空格、制表符和dollar */
		switch (*p)
		{
		case ' ':
		case '\t':
			/* GNU make uses a weird quoting scheme for white space.
			A space or tab preceded by 2N+1 backslashes represents
			N backslashes followed by space; a space or tab
			preceded by 2N backslashes represents N backslashes at
			the end of a file name; and backslashes in other
			contexts should not be doubled.  */
			/* GNU make 使用奇怪的引用模式来处理空白符。一个空格或者制表符前面
			有2N+1个反斜杠表示N个反斜杠后跟一个空白符；一个空格或者制表符前面
			有2N个反斜杠表示文件名末尾的N个反斜杠；其他环境下的反斜杠数量不应该是偶数的 */
			
			/* 计算反斜杠个数， 2N+1 */
			for (q = p - 1; filename <= q && *q == '\\'; q--)
				len++;
			len++;
			break;

		case '$':
			/* '$' is quoted by doubling it.  */
			/* 两个$表示一个$ */
			len++;
			break;
		}
	}

	/* Now we know how big to make the buffer.  */
	/* 申请缓冲空间 */
	buffer = xmalloc(len + 1);

	/* 格式转换 */
	for (p = filename, dst = buffer; *p; p++, dst++)
	{
		switch (*p)
		/* 按照规则添加填充符号 */
		{
		case ' ':
		case '\t':
			/* 添加一倍加一的反斜杠 */
			for (q = p - 1; filename <= q && *q == '\\'; q--)
				*dst++ = '\\';
			*dst++ = '\\';
			break;

		case '$':
			/* 添加一个dollar */
			*dst++ = '$';
			break;

		default:
			/* nothing */;
		}
		*dst = *p;
	}

	/* 添加终结符 */
	*dst = '\0';
	return buffer;
}

/* Public routines.  */

struct deps *
	deps_init()
{
		struct deps *d = (struct deps *) xmalloc(sizeof (struct deps));

		/* Allocate space for the vectors only if we need it.  */

		d->targetv = 0;
		d->depv = 0;

		d->ntargets = 0;
		d->targets_size = 0;
		d->ndeps = 0;
		d->deps_size = 0;

		return d;
	}

void
deps_free(d)
struct deps *d;
{
	unsigned int i;

	if (d->targetv)
	{
		for (i = 0; i < d->ntargets; i++)
			free((PTR)d->targetv[i]);
		free(d->targetv);
	}

	if (d->depv)
	{
		for (i = 0; i < d->ndeps; i++)
			free((PTR)d->depv[i]);
		free(d->depv);
	}

	free(d);
}

/* Adds a target T.  We make a copy, so it need not be a permanent
string.  QUOTE is true if the string should be quoted.  */
void
deps_add_target(d, t, quote)
struct deps *d;
const char *t;
int quote;
{
	if (d->ntargets == d->targets_size)
	{
		d->targets_size = d->targets_size * 2 + 4;
		d->targetv = (const char **)xrealloc(d->targetv,
			d->targets_size * sizeof (const char *));
	}

	if (quote)
		t = munge(t);  /* Also makes permanent copy.  */
	else
		t = xstrdup(t);

	d->targetv[d->ntargets++] = t;
}

/* Sets the default target if none has been given already.  An empty
string as the default target in interpreted as stdin.  The string
is quoted for MAKE.  */
void
deps_add_default_target(d, tgt)
struct deps *d;
const char *tgt;
{
	/* Only if we have no targets.  */
	if (d->ntargets)
		return;

	if (tgt[0] == '\0')
		deps_add_target(d, "-", 1);
	else
	{
#ifndef TARGET_OBJECT_SUFFIX
# define TARGET_OBJECT_SUFFIX ".o"
#endif
		const char *start = lbasename(tgt);
		char *o = (char *)alloca(strlen(start) + strlen(TARGET_OBJECT_SUFFIX) + 1);
		char *suffix;

		strcpy(o, start);

		suffix = strrchr(o, '.');
		if (!suffix)
			suffix = o + strlen(o);
		strcpy(suffix, TARGET_OBJECT_SUFFIX);

		deps_add_target(d, o, 1);
	}
}

void
deps_add_dep(d, t)
struct deps *d;
const char *t;
{
	t = munge(t);  /* Also makes permanent copy.  */

	if (d->ndeps == d->deps_size)
	{
		d->deps_size = d->deps_size * 2 + 8;
		d->depv = (const char **)
			xrealloc(d->depv, d->deps_size * sizeof (const char *));
	}
	d->depv[d->ndeps++] = t;
}

void
deps_write(d, fp, colmax)
const struct deps *d;
FILE *fp;
unsigned int colmax;
{
	unsigned int size, i, column;

	column = 0;
	if (colmax && colmax < 34)
		colmax = 34;

	for (i = 0; i < d->ntargets; i++)
	{
		size = strlen(d->targetv[i]);
		column += size;
		if (colmax && column > colmax)
		{
			fputs(" \\\n ", fp);
			column = 1 + size;
		}
		if (i)
		{
			putc(' ', fp);
			column++;
		}
		fputs(d->targetv[i], fp);
	}

	putc(':', fp);
	putc(' ', fp);
	column += 2;

	for (i = 0; i < d->ndeps; i++)
	{
		size = strlen(d->depv[i]);
		column += size;
		if (colmax && column > colmax)
		{
			fputs(" \\\n ", fp);
			column = 1 + size;
		}
		if (i)
		{
			putc(' ', fp);
			column++;
		}
		fputs(d->depv[i], fp);
	}
	putc('\n', fp);
}

void
deps_phony_targets(d, fp)
const struct deps *d;
FILE *fp;
{
	unsigned int i;

	for (i = 1; i < d->ndeps; i++)
	{
		putc('\n', fp);
		fputs(d->depv[i], fp);
		putc(':', fp);
		putc('\n', fp);
	}
}
