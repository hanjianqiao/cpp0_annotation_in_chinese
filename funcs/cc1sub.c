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
void *GOL_sysmalloc(unsigned int size);  //��̬�ڴ�����
int main1(int argc, UCHAR **argv);  //������
int GO_fputs(const char *s, GO_FILE *stream);  //������д������

void GOLD_eixt(int s);  //�˳�����
int GOLD_getsize(const UCHAR *name);  //��ȡ�ļ���С
int GOLD_read(const UCHAR *name, int len, UCHAR *b0);  //���ļ��ж�ȡ����
int GOLD_write_t(const UCHAR *name, int len, const UCHAR *p0); //���ļ���д������
void GOL_sysabort(unsigned char termcode); //ϵͳ�˳�


/* ��������Ĺ�����ָ������ļ��� */
void GOL_callmain(int argc, UCHAR **argv)
{
	UCHAR **argv1, **p;
	p = argv1 = GOL_sysmalloc((argc + 1) * sizeof (char *));
	for (;;) {
		if ((*p = *argv) == NULL)
			break;

		/* �����-o������������ļ�����Ϊ��ֵ������ȥ�ò��� */
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

	/* ִ����-oѡ���main1
	  * main1����CPP0���Ĵ���Ŀ�ʼ��
	  *
	  *
	 */
	GOL_retcode = main1(p - argv1, argv1);


	
	/* ����ĺ������ǽ������������������ļ����߱�׼��� */
	GOL_sysabort(0);
}


/* �ر�GOL�ļ���ʵ��������ļ����ܻ��ڱ���������ʹ�ã�
  * ����ʵ����û�йرա�
  */
void GOL_close(GOL_FILE *gfp)
{
	if (--gfp->linkcount == 0)
		/* ������˵�ǰ�ĺ��������ļ�û�б���������ʹ���� */
	{
		GOL_sysfree(gfp->p0);  //�ͷ��ļ��Ļ����ڴ�
		GOL_sysfree(gfp);      //�ͷ�GOL�ļ��ṹ�屾��ռ�õ��ڴ�
	}
	return;
}

/* ���ļ���dir������ֻ��һ����ʽ�ԵĲ��� */
GOL_FILE *GOL_open(struct GOL_STR_DIR *dir, const UCHAR *name)
{
	GOL_FILE *gfp;
	int bytes;
	UCHAR *p;
	
	/* ��ȡ�ļ���С */
	bytes = GOLD_getsize(name);
	if (bytes == -1)
		goto err;
	
	/* ��gfp����ռ� */
	gfp = (struct GOL_STR_FILE *) GOL_sysmalloc(sizeof (struct GOL_STR_FILE));
	/* �����ļ��洢�ռ� */
	p = gfp->p0 = (UCHAR *) GOL_sysmalloc(bytes);
	/* ���ļ���һ��ʹ���� */
	gfp->linkcount = 1;
	/* ���ļ����뵽�ڴ��� */
	if (GOLD_read(name, bytes, p)) {
		/* ������ļ�ʧ�ܣ��ر��ļ����ͷſռ䡣 */
		GOL_close(gfp);
		goto err;
	}
	/* ��¼�ļ���С */
	gfp->size = bytes;
	return gfp;
err:
	return (GOL_FILE *) ~0;
}

/* ֱ�ӷ���path? */
UCHAR *GOL_stepdir(struct GOL_STR_DIR **dir, const UCHAR *path)
{
	return (UCHAR *) path;
}
