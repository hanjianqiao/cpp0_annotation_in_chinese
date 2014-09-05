/* cc1sub.c: auxiliary driver for cc1, cpp0, cc1plus */
/* w32, stdc common. osa is different */

struct GOL_STR_DIR;

typedef struct GOL_STR_FILE {
	unsigned int size;
	UCHAR *p0;
	unsigned int linkcount;
	void *p_sys;
} GOL_FILE;

int GOL_retcode = 0;
UCHAR *GOL_outname = NULL;
void *GOL_sysmalloc(unsigned int size);
int main1(int argc, UCHAR **argv);
int GO_fputs(const char *s, GO_FILE *stream);

void GOLD_eixt(int s);
int GOLD_getsize(const UCHAR *name);
int GOLD_read(const UCHAR *name, int len, UCHAR *b0);
int GOLD_write_t(const UCHAR *name, int len, const UCHAR *p0);
void GOL_sysabort(unsigned char termcode);


/* 这个函数的功能是指定输出文件名 */
void GOL_callmain(int argc, UCHAR **argv)
{
	UCHAR **argv1, **p;
	p = argv1 = GOL_sysmalloc((argc + 1) * sizeof (char *));
	for (;;) {
		if ((*p = *argv) == NULL)
			break;

		/* 如果是-o参数，将输出文件名设为该值，并除去该参数 */
		if ((*argv)[0] == '-' && (*argv)[1] == 'o') {
			GOL_outname = &((*argv)[2]);
			if ((*argv)[2] == '\0') {
				if (argv[1] != NULL)
					GOL_outname = (argv++)[1];
			}
			p--;  /* delete -o */
		}
		p++;
		argv++;
	}

	/* 执行无-o选项的main1
	  * main1就是CPP0核心代码的开始了
	  *
	  *
	 */
	GOL_retcode = main1(p - argv1, argv1);


	
	/* 下面的函数就是将处理过的数据输出到文件或者标准输出 */
	GOL_sysabort(0);
}

/* 以下的代码是文件驱动类似，我们暂时忽略 */

void GOL_close(GOL_FILE *gfp)
{
	if (--gfp->linkcount == 0) {
		GOL_sysfree(gfp->p0);
		GOL_sysfree(gfp);
	}
	return;
}

GOL_FILE *GOL_open(struct GOL_STR_DIR *dir, const UCHAR *name)
{
	GOL_FILE *gfp;
	int bytes;
	UCHAR *p;
	bytes = GOLD_getsize(name);
	if (bytes == -1)
		goto err;
	gfp = (struct GOL_STR_FILE *) GOL_sysmalloc(sizeof (struct GOL_STR_FILE));
	p = gfp->p0 = (UCHAR *) GOL_sysmalloc(bytes);
	gfp->linkcount = 1;
	if (GOLD_read(name, bytes, p)) {
		GOL_close(gfp);
		goto err;
	}
	gfp->size = bytes;
	return gfp;
err:
	return (GOL_FILE *) ~0;
}

UCHAR *GOL_stepdir(struct GOL_STR_DIR **dir, const UCHAR *path)
{
	return (UCHAR *) path;
}
