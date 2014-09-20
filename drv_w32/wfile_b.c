/* for w32 */
/* ���ǽ�����win32API�����ϵĺ�����
  * ������win32��CreateFileA, GetStdHandle, WriteFile, and CloseHandle��ЩAPI������
  * ���������Ŀ���ļ���Ҫ��win32�Ŀ��ļ����Ӻ�ſ�ִ�С�
  *
  */

int GOLD_write_b(const UCHAR *name, int len, const UCHAR *p0)
/* ���ֽ�Ϊ��λ��nameָ����ļ�����д��len���ȵ����� */
{
	HANDLE h;
	int ll = 0;
	if (name)
		/* ���ָ�����ļ������͸����ļ��������ļ��� */
		h = CreateFileA((char *) name, GENERIC_WRITE, 0, NULL,
			CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, 0);
	else
		/* �ļ���ΪNULLֵ����򿪱�׼�������ȡ������ */
		h = GetStdHandle(STD_OUTPUT_HANDLE);

	/* �ж��Ƿ��ȡ���ʧ�� */
	if (h == INVALID_HANDLE_VALUE)
		goto err;

	/* ���ָ���ĳ���len�����������ļ�д�����ݡ� */
	if (len)
		WriteFile(h, p0, len, &ll, NULL);

	/* �д������ļ��Ļ������˳�֮ǰӦ�ùرո��ļ��ľ���� */
	if (name)
		CloseHandle(h);

	/* ʵ��д������ݺ�Ҫ������ݴ�С��һ���Ǵ��� */
	if (ll != len)
		goto err;
	return 0;
err:
	return 1;
}

