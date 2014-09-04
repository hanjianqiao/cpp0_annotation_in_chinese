#include "windows.h"

typedef unsigned char UCHAR;

typedef struct GO_STR_FILE {
	UCHAR *p0, *p1, *p;
	int dummy;
} GO_FILE;

extern GO_FILE GO_stdin, GO_stdout, GO_stderr;

void GOL_callmain(int argc, UCHAR **argv);
unsigned int GO_strlen(UCHAR *s);
void *GOL_sysmalloc(unsigned int size);

/* 这个函数的功能是将一串以空格分隔的命令行参数转换为标准的数组参数 */
void GOL_callmain0()
{
	int argc = 0, i;
	//GetCommandLineA是libmingw中的一个函数，获取当前进程的命令参数缓冲区指针
	UCHAR *p = GetCommandLineA(), *q, *q0, **argv;
	//因此有必要将命令拷贝到程序的空间里面来
	q = q0 = GOL_sysmalloc(GO_strlen(p) + 1);
	do {
		while ((*q++ = *p++) > ' ');
		argc++;		//遇到空格表示一个参数结束，因此argc加1
		p--;		//由于p是后增式，所以现在的p已经指向空格下一个字符了，所以要向前移动一个
		*(q - 1) = '\0';	//将参数放到数组中时参数以\0分隔
		while ('\0' < *p && *p <= ' ')	//跳过无效字符
			p++;
	} while (*p);						//处理所有的参数

	/* 生成标准的argv参数 */
	argv = GOL_sysmalloc((argc + 1) * sizeof (char *));
	argv[0] = q = q0;
	i = 1;
	/* 将argv和每个参数对应起来。 */
	while (i < argc) {
		while (*q++);
		argv[i++] = q;
	}
	argv[i] = NULL;

	/* 标准方式调用传到下一个函数 */
	GOL_callmain(argc, argv);
}

#include "others.h"
#include "wfile_b.c"
#include "others.c"
#include "../funcs/cc1sub.c"

extern UCHAR *GOL_work0;

static int writefile(const UCHAR *name, const UCHAR *p0, const UCHAR *p1)
{
	UCHAR *q, c;
	int len;
	q = GOL_work0;
	while (p0 < p1) {
		c = *p0++;
		if (c == '\n')
			*q++ = '\r';
		if (c == '\r') {
			if (p0 < p1 && *p0 == '\n') {
				*q++ = '\r';
				c = '\n';
				p0++;
			}
		}
		*q++ = c;
	}
	return GOLD_write_b(name, q - GOL_work0, GOL_work0);
}

void GOL_sysabort(UCHAR termcode)
{
	static const UCHAR *termmsg[] = {
		"",
		"[TERM_WORKOVER]\n",
		"[TERM_OUTOVER]\n",
		"[TERM_ERROVER]\n",
		"[TERM_BUGTRAP]\n",
		"[TERM_SYSRESOVER]\n",
		"[TERM_ABORT]\n"
	};
	GO_stderr.p1 += 128; /* Resurrect minute that was set aside in reserve */
	/* The output buffer */
	if (writefile(GOL_outname, GO_stdout.p0, GO_stdout.p)) {
		GO_fputs("GOL_sysabort:output error!\n", &GO_stderr);
		termcode = 6;
	}
	if (termcode <= 6)
		GO_fputs(termmsg[termcode], &GO_stderr);
	if (GO_stderr.p > GO_stderr.p0)
		writefile(NULL, GO_stderr.p0, GO_stderr.p);
	GOLD_exit((termcode == 0) ? GOL_retcode : 1);
}
