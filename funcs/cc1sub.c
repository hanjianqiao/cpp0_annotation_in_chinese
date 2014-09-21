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
void *GOL_sysmalloc(unsigned int size);  //动态内存申请
int main1(int argc, UCHAR **argv);  //主函数
int GO_fputs(const char *s, GO_FILE *stream);  //向流中写入数据

void GOLD_eixt(int s);  //退出函数
int GOLD_getsize(const UCHAR *name);  //获取文件大小
int GOLD_read(const UCHAR *name, int len, UCHAR *b0);  //从文件中读取数据
int GOLD_write_t(const UCHAR *name, int len, const UCHAR *p0); //向文件中写入数据
void GOL_sysabort(unsigned char termcode); //系统退出


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


/* 关闭GOL文件，实际上这个文件可能还在被其他函数使用，
  * 可能实际上没有关闭。
  */
void GOL_close(GOL_FILE *gfp)
{
	if (--gfp->linkcount == 0)
		/* 如果除了当前的函数，该文件没有被其他函数使用中 */
	{
		GOL_sysfree(gfp->p0);  //释放文件的缓冲内存
		GOL_sysfree(gfp);      //释放GOL文件结构体本身占用的内存
	}
	return;
}

/* 打开文件，dir在这里只是一个形式性的参数 */
GOL_FILE *GOL_open(struct GOL_STR_DIR *dir, const UCHAR *name)
{
	GOL_FILE *gfp;
	int bytes;
	UCHAR *p;
	
	/* 获取文件大小 */
	bytes = GOLD_getsize(name);
	if (bytes == -1)
		goto err;
	
	/* 给gfp分配空间 */
	gfp = (struct GOL_STR_FILE *) GOL_sysmalloc(sizeof (struct GOL_STR_FILE));
	/* 分配文件存储空间 */
	p = gfp->p0 = (UCHAR *) GOL_sysmalloc(bytes);
	/* 该文件有一个使用者 */
	gfp->linkcount = 1;
	/* 将文件读入到内存中 */
	if (GOLD_read(name, bytes, p)) {
		/* 如果打开文件失败，关闭文件，释放空间。 */
		GOL_close(gfp);
		goto err;
	}
	/* 记录文件大小 */
	gfp->size = bytes;
	return gfp;
err:
	return (GOL_FILE *) ~0;
}

/* 直接返回path? */
UCHAR *GOL_stepdir(struct GOL_STR_DIR **dir, const UCHAR *path)
{
	return (UCHAR *) path;
}
