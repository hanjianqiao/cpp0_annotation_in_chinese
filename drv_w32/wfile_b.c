/* for w32 */
/* 这是建立在win32API基础上的函数，
  * 依赖于win32的CreateFileA, GetStdHandle, WriteFile, and CloseHandle这些API函数。
  * 编译出来的目标文件需要和win32的库文件链接后才可执行。
  *
  */

int GOLD_write_b(const UCHAR *name, int len, const UCHAR *p0)
/* 以字节为单位向name指向的文件对象写入len长度的数据 */
{
	HANDLE h;
	int ll = 0;
	if (name)
		/* 如果指定的文件名，就根据文件名创建文件。 */
		h = CreateFileA((char *) name, GENERIC_WRITE, 0, NULL,
			CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, 0);
	else
		/* 文件名为NULL值，则打开标准输出，获取其句柄。 */
		h = GetStdHandle(STD_OUTPUT_HANDLE);

	/* 判断是否获取句柄失败 */
	if (h == INVALID_HANDLE_VALUE)
		goto err;

	/* 如果指定的长度len大于零则向文件写入数据。 */
	if (len)
		WriteFile(h, p0, len, &ll, NULL);

	/* 有创建过文件的话，在退出之前应该关闭该文件的句柄。 */
	if (name)
		CloseHandle(h);

	/* 实际写入的数据和要求的数据大小不一致是错误 */
	if (ll != len)
		goto err;
	return 0;
err:
	return 1;
}

