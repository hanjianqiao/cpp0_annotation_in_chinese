
/* Ϊwin32API��Ƶĺ��� */

/* ���׼��������Ϣ */
void msgout(UCHAR *s)
{
	GOLD_write_t(NULL, GO_strlen(s), s);
	return;
}

/* ���������Ϣ���˳� */
void errout(UCHAR *s)
{
	msgout(s);
	GOLD_exit(1);
}

/* ���������Ϣ�����л��� */
void errout_s_NL(UCHAR *s, UCHAR *t)
{
	msgout(s);
	msgout(t);
	msgout(NL);
	GOLD_exit(1);
}
