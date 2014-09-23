typedef unsigned char UCHAR;

/* �����GO_FILE��ϵͳ�Ķ��󣬲��������ļ��� */
typedef struct GO_STR_FILE {
	/* ��ʼ����������ǰ */
	UCHAR *p0, *p1, *p;
	int dummy;  //���
} GO_FILE;

/* ��Щ��GO��ͷ�Ľṹ����������go_lib.lib����ġ� */
extern GO_FILE GO_stdin, GO_stdout, GO_stderr;
extern struct GOL_STR_MEMMAN GOL_memman, GOL_sysman;
UCHAR *GOL_work0;

/* ���bss_alloc�ṹ������ռ��õġ� */
struct bss_alloc {
	UCHAR _stdout[SIZ_STDOUT];
	UCHAR _stderr[SIZ_STDERR];
	UCHAR syswrk[SIZ_SYSWRK];
	UCHAR work[SIZ_WORK];
	UCHAR for_align[16];
};

void GOL_sysabort(UCHAR termcode);
void *GOL_memmaninit(struct GOL_STR_MEMMAN *man, unsigned int size, void *p);
void *GOL_sysmalloc(unsigned int size);
void GOL_callmain0();

void mainCRTStartup(void)
/* Sure, I put the -o options */
/* Here, -o option is stripped */
/* But (because the size can not be measured in the standard input) The name of the input file write */
{
	struct bss_alloc bss_image;
	struct bss_alloc *bss0 = (void *) ((((int) &bss_image) + 0x0f) & ~0x0f); /* ���� */

	/* �����Ǹ���׼�������ռ� */
	GO_stdout.p0 = GO_stdout.p = bss0->_stdout;
	GO_stdout.p1 = GO_stdout.p0 + SIZ_STDOUT;
	GO_stdout.dummy = ~0;
	GO_stderr.p0 = GO_stderr.p = bss0->_stderr;
	GO_stderr.p1 = GO_stderr.p0 + (SIZ_STDERR - 128); /* I keep a little smaller on purpose */
	GO_stderr.dummy = ~0;

	/* ����ʹ�õ����Լ���һ���ڴ���亯����������Բ���go_lib��ش��롣 */
	GOL_memmaninit(&GOL_sysman, SIZ_SYSWRK, bss0->syswrk);
	GOL_memmaninit(&GOL_memman, SIZ_WORK, GOL_work0 = bss0->work);
	
	/* ������һ���������� */
	GOL_callmain0();
}
