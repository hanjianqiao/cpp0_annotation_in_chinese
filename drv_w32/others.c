/* for w32 */
/* 适用于win32的函数 */

//	void GOLD_exit(int s);		/* Exit */
int GOLD_getsize(const UCHAR *name); /* File size acquisition */
int GOLD_read(const UCHAR *name, int len, UCHAR *b0);
	/* Read file, binary mode,
		You have been file check just before the caller the size,
		I come to request the file size perfect */

/* 获取文件大小 */
int GOLD_getsize(const UCHAR *name)
{
	HANDLE h;
	int len = -1;
	/* 以打开的方式创建文件操作句柄 */
	h = CreateFileA((char *) name, GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (h == INVALID_HANDLE_VALUE)
		goto err;
	/* 获取文件大小 */
	len = GetFileSize(h, NULL);
	/* 关闭文件句柄 */
	CloseHandle(h);
err:
	return len;
}

/* 从文件中读取len长度的数据到b0*/
int GOLD_read(const UCHAR *name, int len, UCHAR *b0)
{
	HANDLE h;
	int i;
	/* 以打开的方式创建文件句柄 */
	h = CreateFileA((char *) name, GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (h == INVALID_HANDLE_VALUE)
		goto err;
	/* 从文件中读取数据 */
	ReadFile(h, b0, len, &i, NULL);
	/* 关闭文件句柄 */
	CloseHandle(h);
	/* 若读出的数据和期望数据大小不同，则是错误 */
	if (len != i)
		goto err;
	return 0;
err:
	return 1;
}
