/* CPP main program, using CPP Library.
Copyright (C) 1995, 1997, 1998, 1999, 2000, 2001, 2002
Free Software Foundation, Inc.
Written by Per Bothner, 1994-95.

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
#include "cpplib.h"
#include "intl.h"

/* Encapsulates state used to convert the stream of tokens coming from
cpp_get_token back into a text file.  */
/* 这是一个封装的结构体，用来将从cpp_get_token获得的token转换为目标形式的token并输出到文件。 */
struct printer
{
	FILE *outf;			/* Stream to write to. 目标文件 */
	const struct line_map *map;	/* Logical to physical line mappings. 逻辑到物理行关系图 */
	const cpp_token *prev;	/* Previous token. 前一个token */
	const cpp_token *source;	/* Source token for spacing. 为空格的token */
	unsigned int line;		/* Line currently being written. 当前行 */
	unsigned char printed;	/* Nonzero if something output at line. 非零代表当前行还有数据要输出 */
};

/* !kawai! */
int main1		PARAMS((int, char **));
static void general_init PARAMS((const char *));
static void do_preprocessing PARAMS((int, char **));
static void setup_callbacks PARAMS((void));
/* end of !kawai! */

/* General output routines.  */
/* 输出的例行程序 */
static void scan_translation_unit PARAMS((cpp_reader *));
static void check_multiline_token PARAMS((const cpp_string *));
static int dump_macro PARAMS((cpp_reader *, cpp_hashnode *, void *));

static void print_line PARAMS((const struct line_map *, unsigned int,
	const char *));
static void maybe_print_line PARAMS((const struct line_map *, unsigned int));

/* Callback routines for the parser.   Most of these are active only
in specific modes.  */
/* 回调函数们 */
static void cb_line_change PARAMS((cpp_reader *, const cpp_token *, int));
static void cb_define	PARAMS((cpp_reader *, unsigned int, cpp_hashnode *));
static void cb_undef	PARAMS((cpp_reader *, unsigned int, cpp_hashnode *));
static void cb_include	PARAMS((cpp_reader *, unsigned int,
	const unsigned char *, const cpp_token *));
static void cb_ident	  PARAMS((cpp_reader *, unsigned int,
	const cpp_string *));
static void cb_file_change PARAMS((cpp_reader *, const struct line_map *));
static void cb_def_pragma PARAMS((cpp_reader *, unsigned int));


const char *progname;		/* Needs to be global. 该程序名 */
static cpp_reader *pfile;	/* An opaque handle. 不透明句柄 */
static cpp_options *options;	/* Options of pfile. 选项 */
static struct printer print;  /* 用来输出 */

/* !kawai! */
/* 主函数1 */
int main1(argc, argv)
/* end of !kawai! */
int argc;
char **argv;
{
	/* 记录程序名 */
	general_init(argv[0]);

	/* 初始化pfile结构，申请内存、初始化树结构等 */
	/* Construct a reader with default language GNU C89.  */
	/* 获取符合C89标准的读取器 */
	pfile = cpp_create_reader(CLK_GNUC89);
	/* 获取选项的指针 */
	options = cpp_get_options(pfile);


	/* 重头戏，处理选项、处理代码、输出到缓冲区 */
	do_preprocessing(argc, argv);




	/* 释放之前申请的空间 */
	if (cpp_destroy(pfile))
		return FATAL_EXIT_CODE;

	return SUCCESS_EXIT_CODE;
}

/* Store the program name, and set the locale.  */
/* 保存程序名称，设置字符集 */
static void
general_init(argv0)
const char *argv0;
{
	progname = argv0 + strlen(argv0);

	/* IS_DIR_SEPARATOR 是判断字符是不是'/' 或者'\\' */
	while (progname != argv0 && !IS_DIR_SEPARATOR(progname[-1]))
		--progname;  //progname指向的是不带路径的程序名称

	/* !kawai! */
	/* xmalloc_set_program_name (progname); */
	/* end of !kawai! */

	/* 这是个空函数 */
	hex_init();

	/* 这也是个空函数 */
	gcc_init_libintl();
}

/* Handle switches, preprocess and output.  */
/* 处理特性开关、处理代码、输出 */
static void
do_preprocessing(argc, argv)
int argc;
char **argv;
{
	int argi = 1;  /* Next argument to handle.  */

	/* 处理命令行参数，根据选项及参数将pfile的变量赋值 */
	argi += cpp_handle_options(pfile, argc - argi, argv + argi);
	if (CPP_FATAL_ERRORS(pfile))
		return;

	if (argi < argc)
	{
		/* 这个函数应该就是输出fatal error xxx的函数了，
		  * 每次调用会使pfile的error成员增加1
		  */
		cpp_fatal(pfile, "invalid option %s", argv[argi]);
		return;
	}


	/* 已经将字符串选项转换成可处理的数值信息
	  * 继续处理额外的信息，和之前的转换差不多
	  */
	cpp_post_options(pfile);

	/* 判定错误是否超过CPP_FATAL_LIMIT(1000) 
	  * 
	  *
	  */
	if (CPP_FATAL_ERRORS(pfile))
		return;

	/* If cpp_handle_options saw --help or --version on the command
	line, it will have set pfile->help_only to indicate this.  Exit
	successfully.  [The library does not exit itself, because
	e.g. cc1 needs to print its own --help message at this point.]  */
	/* 如果选项中有--help或者--version，就直接返回 */
	if (options->help_only)
		return;

	/* Initialize the printer structure.  Setting print.line to -1 here
	is a trick to guarantee that the first token of the file will
	cause a linemarker to be output by maybe_print_line.  */
	/* 初始化printer，设置line为-1来确保文件的第一个token会使linemarker被输出 */
	print.line = (unsigned int)-1;
	print.printed = 0;
	print.prev = 0;
	print.map = 0;

	/* Open the output now.  We must do so even if no_output is on,
	because there may be other output than from the actual
	preprocessing (e.g. from -dM).  */

	/* 先打开输出文件，为输出做准备 */
	if (options->out_fname[0] == '\0')
		print.outf = stdout;
	else
	{
		print.outf = fopen(options->out_fname, "w");
		if (print.outf == NULL)
		{
			cpp_notice_from_errno(pfile, options->out_fname);
			return;
		}
	}


	/* 设置pfile的回调函数，
	  * 就是设置当遇到#include之类的代码时调用哪个函数
	  */
	setup_callbacks();


	/* 下面的这行函数是将要处理的目标文件读入，
	  * 并向目标流输出读入的文件名
	  */
	if (cpp_read_main_file(pfile, options->in_fname, NULL))
	{

		/* 下面的函数会输出编译器默认的引用文件名和使用
		  * -include选项指定的文件名
		  */
		cpp_finish_options(pfile);


		/* A successful cpp_read_main_file guarantees that we can call
		cpp_scan_nooutput or cpp_get_token next.  */
		/* 这里将预处理好的代码输出
		  * 执行的应该是false的函数
		  */
		if (options->no_output)
		{
			cpp_scan_nooutput(pfile);
		}
		else
		{
			/* 我们要找的输出函数
			  * 这个函数会调用此法分析函数
			  * 这个函数会输出Token
			  */
			scan_translation_unit(pfile);
		}

		/* -dM command line option.  Should this be in cpp_finish?  */
		if (options->dump_macros == dump_only)
			cpp_forall_identifiers(pfile, dump_macro, NULL);


		/* 结束 */
		cpp_finish(pfile);
	}


	/* 清空缓冲区 */
	/* Flush any pending output.  */
	if (print.printed)
		putc('\n', print.outf);

	if (ferror(print.outf) || fclose(print.outf))
		cpp_notice_from_errno(pfile, options->out_fname);
}

/* Set up the callbacks as appropriate.  */
/* 设置回调函数 */
static void
setup_callbacks()
{
	/* 获取pfile的option指针 */
	cpp_callbacks *cb = cpp_get_callbacks(pfile);

	if (!options->no_output)
	{
		cb->line_change = cb_line_change;
		/* Don't emit #pragma or #ident directives if we are processing
		assembly language; the assembler may choke on them.  */

		/* 处理#pragma和#ident */
		if (options->lang != CLK_ASM)
		{
			cb->ident = cb_ident;
			cb->def_pragma = cb_def_pragma;
		}

		/* 当文件发生变化，
		  * 即引用了其他的文件
		  */
		if (!options->no_line_commands)
			cb->file_change = cb_file_change;
	}

	/* 好像下面的函数没有被用到 */
	if (options->dump_includes)
		cb->include = cb_include;


	/* 下面的两个函数好像也没有用到 */
	if (options->dump_macros == dump_names
		|| options->dump_macros == dump_definitions)
	{
		cb->define = cb_define;
		cb->undef = cb_undef;
	}
}

/* Writes out the preprocessed file, handling spacing and paste
avoidance issues.  */
/* 将处理过的文件输出 */
static void
scan_translation_unit(pfile)
cpp_reader *pfile;
{
	bool avoid_paste = false;

	print.source = NULL;
	for (;;)
	{
		/* 调用这个函数会向输出输出一个TOKEN */
		const cpp_token *token = cpp_get_token(pfile);

		if (token->type == CPP_PADDING)
		/* 如果是填充token */
		{
			avoid_paste = true;
			if (print.source == NULL
				|| (!(print.source->flags & PREV_WHITE)
				&& token->val.source == NULL))
				print.source = token->val.source;
			continue;
		}

		if (token->type == CPP_EOF)
			break;  //如果文件结束了，退出循环

		/* Subtle logic to output a space if and only if necessary.  */
		/* 如果需要的话输出一个空格 */
		if (avoid_paste)
		{
			if (print.source == NULL)
				print.source = token;
			if (print.source->flags & PREV_WHITE
				|| (print.prev && cpp_avoid_paste(pfile, print.prev, token))
				|| (print.prev == NULL && token->type == CPP_HASH))
				putc(' ', print.outf);
		}
		else if (token->flags & PREV_WHITE)
			putc(' ', print.outf);

		avoid_paste = false;
		print.source = NULL;
		print.prev = token;
		/* 输出token */
		cpp_output_token(token, print.outf);

		if (token->type == CPP_STRING || token->type == CPP_WSTRING
			|| token->type == CPP_COMMENT)
			check_multiline_token(&token->val.str);  //检查多行的token
	}
}

/* Adjust print.line for newlines embedded in tokens.  */
/* 调整print的line变量 */
static void
check_multiline_token(str)
const cpp_string *str;
{
	unsigned int i;

	for (i = 0; i < str->len; i++)
	if (str->text[i] == '\n')
		print.line++;   //换行加一
}

/* If the token read on logical line LINE needs to be output on a
different line to the current one, output the required newlines or
a line marker, and return 1.  Otherwise return 0.  */
/* 如果token所在的逻辑行和实际的行不相同，
  * 输出它应该在的行号
  */
static void
maybe_print_line(map, line)
const struct line_map *map;
unsigned int line;
{
	/* End the previous line of text.  */
	/* 前一行的东西都输出完毕了，输出换行符开始新行 */
	if (print.printed)
	{
		putc('\n', print.outf);
		print.line++;
		print.printed = 0;
	}

	/* 目标行在当前行之后并且是8行之内，输出换行直到目标行。
	  * 否则输出行号标识
	  */
	if (line >= print.line && line < print.line + 8)
	{
		while (line > print.line)
		{
			putc('\n', print.outf);
			print.line++;
		}
	}
	else
		print_line(map, line, "");
}

/* Output a line marker for logical line LINE.  Special flags are "1"
or "2" indicating entering or leaving a file.  */
/* 这个函数是输出逻辑行号的
  *
  */
static void
print_line(map, line, special_flags)
const struct line_map *map;
unsigned int line;
const char *special_flags;
{
	/* End any previous line of text.  */
	/* 终结前一行的输出 */
	if (print.printed)
		putc('\n', print.outf);
	print.printed = 0;

	print.line = line;
	/* 如果没有禁止输出行号 */
	if (!options->no_line_commands)
	{
		size_t to_file_len = strlen(map->to_file);
		/* 可能文件名里含有需要转义型的字符 */
		unsigned char *to_file_quoted = alloca(to_file_len * 4 + 1);
		unsigned char *p;

		/* cpp_quote_string does not nul-terminate, so we have to do it
		ourselves.  */
		/* 给字符串加上双引号，最坏的可能会使长度变为原来的4被，
		  * 下面这个函数不会给字符串加上终结符，所以要留一个\0的空间
		  */
		p = cpp_quote_string(to_file_quoted,
			(unsigned char *)map->to_file, to_file_len);
		*p = '\0';
		fprintf(print.outf, "# %u \"%s\"%s",
			SOURCE_LINE(map, print.line), to_file_quoted, special_flags);

		/* 输出系统头文件或者C 系统头文件的标志 */
		if (map->sysp == 2)
			fputs(" 3 4", print.outf);
		else if (map->sysp == 1)
			fputs(" 3", print.outf);

		putc('\n', print.outf);
	}
}

/* Called when a line of output is started.  TOKEN is the first token
of the line, and at end of file will be CPP_EOF.  */
/* 当一个新行开始时调用这个函数，这个函数会输出一个换行符 */
static void
cb_line_change(pfile, token, parsing_args)
cpp_reader *pfile ATTRIBUTE_UNUSED;
const cpp_token *token;
int parsing_args;
{
	if (token->type == CPP_EOF || parsing_args)
		return;
	/* 换行可能带来行号的无规律变化 */
	maybe_print_line(print.map, token->line);
	print.printed = 1;
	print.prev = 0;
	print.source = 0;

	/* Supply enough spaces to put this token in its original column,
	one space per column greater than 2, since scan_translation_unit
	will provide a space if PREV_WHITE.  Don't bother trying to
	reconstruct tabs; we can't get it right in general, and nothing
	ought to care.  Some things do care; the fault lies with them.  */
	/* 使用空格将token对其到其逻辑列上 */
	if (token->col > 2)
	{
		unsigned int spaces = token->col - 2;

		while (spaces--)
			putc(' ', print.outf);
	}
}

/* 处理ident指令 */
static void
cb_ident(pfile, line, str)
cpp_reader *pfile ATTRIBUTE_UNUSED;
unsigned int line;
const cpp_string * str;
{
	maybe_print_line(print.map, line);
	/* 输出#ident "hello" */
	fprintf(print.outf, "#ident \"%s\"\n", str->text);
	print.line++;
}

/* 处理define指令 */
static void
cb_define(pfile, line, node)
cpp_reader *pfile;
unsigned int line;
cpp_hashnode *node;
{
	maybe_print_line(print.map, line);
	fputs("#define ", print.outf);

	/* -dD command line option.  */
	/* 如果有指定-dD */
	if (options->dump_macros == dump_definitions){
		/* 获取宏的全名和定义 */
		fputs((const char *)cpp_macro_definition(pfile, node), print.outf);
	}
	else{
		fputs((const char *)NODE_NAME(node), print.outf);
	}
	putc('\n', print.outf);
	print.line++;
}

/* 处理undef指令 */
static void
cb_undef(pfile, line, node)
cpp_reader *pfile ATTRIBUTE_UNUSED;
unsigned int line;
cpp_hashnode *node;
{
	maybe_print_line(print.map, line);
	fprintf(print.outf, "#undef %s\n", NODE_NAME(node));
	print.line++;
}

/* 处理include指令 */
static void
cb_include(pfile, line, dir, header)
cpp_reader *pfile;
unsigned int line;
const unsigned char *dir;
const cpp_token *header;
{
	maybe_print_line(print.map, line);
	fprintf(print.outf, "#%s %s\n", dir, cpp_token_as_text(pfile, header));
	print.line++;
}

/* The file name, line number or system header flags have changed, as
described in MAP.  From this point on, the old print.map might be
pointing to freed memory, and so must not be dereferenced.  */
/* 处理文件改变 */
static void
cb_file_change(pfile, map)
cpp_reader *pfile ATTRIBUTE_UNUSED;
const struct line_map *map;
{
	const char *flags = "";

	/* First time?  */
	/* 第一次执行 */
	if (print.map == NULL)
	{
		/* Avoid printing foo.i when the main file is foo.c.  */
		if (!options->preprocessed)
			print_line(map, map->from_line, flags);
	}
	else
	{
		/* 在输出文件中输出引用进来的文件名和行号信息 */
		/* Bring current file to correct line when entering a new file.  */
		if (map->reason == LC_ENTER)
			maybe_print_line(map - 1, map->from_line - 1);

		if (map->reason == LC_ENTER)
			flags = " 1";
		else if (map->reason == LC_LEAVE)
			flags = " 2";
		print_line(map, map->from_line, flags);
	}

	print.map = map;
}

/* Copy a #pragma directive to the preprocessed output.  */
/* 将#pragma复制到输出 */
static void
cb_def_pragma(pfile, line)
cpp_reader *pfile;
unsigned int line;
{
	maybe_print_line(print.map, line);
	fputs("#pragma ", print.outf);
	cpp_output_line(pfile, print.outf);
	print.line++;
}

/* Dump out the hash table.  */
/* 转储哈希表 */
static int
dump_macro(pfile, node, v)
cpp_reader *pfile;
cpp_hashnode *node;
void *v ATTRIBUTE_UNUSED;
{
	if (node->type == NT_MACRO && !(node->flags & NODE_BUILTIN))
	{
		fputs("#define ", print.outf);
		fputs((const char *)cpp_macro_definition(pfile, node), print.outf);
		putc('\n', print.outf);
		print.line++;
	}

	return 1;
}
