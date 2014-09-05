/* for w32 */

/* 以下的代码是文件驱动类似，我们暂时忽略 */

//	void GOLD_exit(int s);		/* Exit */
int GOLD_getsize(const UCHAR *name); /* File size acquisition */
int GOLD_read(const UCHAR *name, int len, UCHAR *b0);
	/* Read file, binary mode,
		You have been file check just before the caller the size,
		I come to request the file size perfect */

int GOLD_getsize(const UCHAR *name)
{
	HANDLE h;
	int len = -1;
	h = CreateFileA((char *) name, GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (h == INVALID_HANDLE_VALUE)
		goto err;
	len = GetFileSize(h, NULL);
	CloseHandle(h);
err:
	return len;
}

int GOLD_read(const UCHAR *name, int len, UCHAR *b0)
{
	HANDLE h;
	int i;
	h = CreateFileA((char *) name, GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (h == INVALID_HANDLE_VALUE)
		goto err;
	ReadFile(h, b0, len, &i, NULL);
	CloseHandle(h);
	if (len != i)
		goto err;
	return 0;
err:
	return 1;
}
