/* for w32 */
/* ������win32�ĺ��� */

//	void GOLD_exit(int s);		/* Exit */
int GOLD_getsize(const UCHAR *name); /* File size acquisition */
int GOLD_read(const UCHAR *name, int len, UCHAR *b0);
	/* Read file, binary mode,
		You have been file check just before the caller the size,
		I come to request the file size perfect */

/* ��ȡ�ļ���С */
int GOLD_getsize(const UCHAR *name)
{
	HANDLE h;
	int len = -1;
	/* �Դ򿪵ķ�ʽ�����ļ�������� */
	h = CreateFileA((char *) name, GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (h == INVALID_HANDLE_VALUE)
		goto err;
	/* ��ȡ�ļ���С */
	len = GetFileSize(h, NULL);
	/* �ر��ļ���� */
	CloseHandle(h);
err:
	return len;
}

/* ���ļ��ж�ȡlen���ȵ����ݵ�b0*/
int GOLD_read(const UCHAR *name, int len, UCHAR *b0)
{
	HANDLE h;
	int i;
	/* �Դ򿪵ķ�ʽ�����ļ���� */
	h = CreateFileA((char *) name, GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (h == INVALID_HANDLE_VALUE)
		goto err;
	/* ���ļ��ж�ȡ���� */
	ReadFile(h, b0, len, &i, NULL);
	/* �ر��ļ���� */
	CloseHandle(h);
	/* �����������ݺ��������ݴ�С��ͬ�����Ǵ��� */
	if (len != i)
		goto err;
	return 0;
err:
	return 1;
}
