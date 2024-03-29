/* Map logical line numbers to (source file, line number) pairs.
Copyright (C) 2001
Free Software Foundation, Inc.

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
#include "line-map.h"
#include "intl.h"

static void trace_include
PARAMS((const struct line_maps *, const struct line_map *));

/* Initialize a line map set.  */
/* 初始化行图集合 */
void
init_line_maps(set)
struct line_maps *set;
{
	set->maps = 0;
	set->allocated = 0;
	set->used = 0;
	set->last_listed = -1;
	set->trace_includes = false;
	set->depth = 0;
}

/* Free a line map set.  */
/* 释放一个行图集合 */
void
free_line_maps(set)
struct line_maps *set;
{
	if (set->maps)
	{
		struct line_map *map;

		/* Depending upon whether we are handling preprocessed input or
		not, this can be a user error or an ICE.  */
		/* 如果释放行图时还有map存在 */
		for (map = CURRENT_LINE_MAP(set); !MAIN_FILE_P(map);
			map = INCLUDED_FROM(set, map))
			fprintf(stderr, "line-map.c: file \"%s\" entered but not left\n",
			map->to_file);

		free(set->maps);
	}
}

/* Add a mapping of logical source line to physical source file and
line number.  Ther text pointed to by TO_FILE must have a lifetime
at least as long as the final call to lookup_line ().

FROM_LINE should be monotonic increasing across calls to this
function.  */
/* 添加引用联系图 */
const struct line_map *
add_line_map(set, reason, sysp, from_line, to_file, to_line)
struct line_maps *set;
enum lc_reason reason;
unsigned int sysp;
unsigned int from_line;
const char *to_file;
unsigned int to_line;
{
	struct line_map *map;

	if (set->used && from_line < set->maps[set->used - 1].from_line)
		abort();  //如果set已被使用并且当前的from_line比前一个set的used的from_line还小，退出

	if (set->used == set->allocated)
	/* 全用光了 */
	{
		/* 扩展map空间 */
		set->allocated = 2 * set->allocated + 256;
		set->maps = (struct line_map *)
			xrealloc(set->maps, set->allocated * sizeof (struct line_map));
	}

	/* 指针后移 */
	map = &set->maps[set->used++];

	/* If we don't keep our line maps consistent, we can easily
	segfault.  Don't rely on the client to do it for us.  */
	/* 维护行图的一致性 */
	if (set->depth == 0)
		reason = LC_ENTER;
	else if (reason == LC_LEAVE)
	{
		struct line_map *from;
		bool error;

		if (MAIN_FILE_P(map - 1))
		/* 是主文件的话 */
		{
			error = true;
			reason = LC_RENAME;
			from = map - 1;
		}
		else
		/* 被引用的文件 */
		{
			from = INCLUDED_FROM(set, map - 1);
			/* 如果当前的离开前一个map不是该文件的进入，错误 */
			error = to_file && strcmp(from->to_file, to_file);
		}

		/* Depending upon whether we are handling preprocessed input or
		not, this can be a user error or an ICE.  */
		/* 输出错误信息 */
		if (error)
			fprintf(stderr, "line-map.c: file \"%s\" left but not entered\n",
			to_file);

		/* A TO_FILE of NULL is special - we use the natural values.  */
		/* 给空指针赋值 */
		if (error || to_file == NULL)
		{
			to_file = from->to_file;
			to_line = LAST_SOURCE_LINE(from) + 1;
			sysp = from->sysp;
		}
	}

	/* 赋值数据 */
	map->reason = reason;
	map->sysp = sysp;
	map->from_line = from_line;
	map->to_file = to_file;
	map->to_line = to_line;

	if (reason == LC_ENTER)
	/* 进入 */
	{
		set->depth++;
		map->included_from = set->used - 2;
		/* 输出引用的深度 */
		if (set->trace_includes)
			trace_include(set, map);
	}
	else if (reason == LC_RENAME)
		/* 重命名 */
		map->included_from = map[-1].included_from;
	else if (reason == LC_LEAVE)
	/* 离开 */
	{
		/* 回溯到前一个引用 */
		set->depth--;
		map->included_from = INCLUDED_FROM(set, map - 1)->included_from;
	}

	return map;
}

/* Given a logical line, returns the map from which the corresponding
(source file, line) pair can be deduced.  Since the set is built
chronologically, the logical lines are monotonic increasing, and so
the list is sorted and we can use a binary search.  */
/* 二分法查找line，获取其指针。 */
const struct line_map *
lookup_line(set, line)
struct line_maps *set;
unsigned int line;
{
	unsigned int md, mn = 0, mx = set->used;

	if (mx == 0)
		abort();

	while (mx - mn > 1)
	{
		md = (mn + mx) / 2;
		if (set->maps[md].from_line > line)
			mx = md;
		else
			mn = md;
	}

	return &set->maps[mn];
}

/* Print the file names and line numbers of the #include commands
which led to the map MAP, if any, to stderr.  Nothing is output if
the most recently listed stack is the same as the current one.  */
/* 输出#include指定的文件名和行号 */
void
print_containing_files(set, map)
struct line_maps *set;
const struct line_map *map;
{
	if (MAIN_FILE_P(map) || set->last_listed == map->included_from)
		return;

	set->last_listed = map->included_from;
	map = INCLUDED_FROM(set, map);

	fprintf(stderr, _("In file included from %s:%u"),
		map->to_file, LAST_SOURCE_LINE(map));

	while (!MAIN_FILE_P(map))
	/* 逆序输出引用路径 */
	{
		map = INCLUDED_FROM(set, map);
		/* Translators note: this message is used in conjunction
		with "In file included from %s:%ld" and some other
		tricks.  We want something like this:

		| In file included from sys/select.h:123,
		|                  from sys/types.h:234,
		|                  from userfile.c:31:
		| bits/select.h:45: <error message here>

		with all the "from"s lined up.
		The trailing comma is at the beginning of this message,
		and the trailing colon is not translated.  */
		fprintf(stderr, _(",\n                 from %s:%u"),
			map->to_file, LAST_SOURCE_LINE(map));
	}

	fputs(":\n", stderr);
}

/* Print an include trace, for e.g. the -H option of the preprocessor.  */
/* 输出递归引用的顺序 */
static void
trace_include(set, map)
const struct line_maps *set;
const struct line_map *map;
{
	unsigned int i = set->depth;

	/*点代表深度*/
	while (--i)
		putc('.', stderr);
	fprintf(stderr, " %s\n", map->to_file);
}
