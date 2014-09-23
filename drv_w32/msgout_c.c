
/* 为win32API设计的函数 */

/* 向标准输出输出信息 */
void msgout(UCHAR *s)
{
	GOLD_write_t(NULL, GO_strlen(s), s);
	return;
}

/* 输出错误信息并退出 */
void errout(UCHAR *s)
{
	msgout(s);
	GOLD_exit(1);
}

/* 输出错误信息，带有换行 */
void errout_s_NL(UCHAR *s, UCHAR *t)
{
	msgout(s);
	msgout(t);
	msgout(NL);
	GOLD_exit(1);
}
