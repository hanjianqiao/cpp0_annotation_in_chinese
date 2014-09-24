/* Utility to update paths from internal to external forms.
Copyright (C) 1997, 1998, 1999, 2000, 2001 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU Library General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with GCC; see the file COPYING.  If not, write to the Free
Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* This file contains routines to update a path, both to canonicalize
the directory format and to handle any prefix translation.

This file must be compiled with -DPREFIX= to specify the "prefix"
value used by configure.  If a filename does not begin with this
prefix, it will not be affected other than by directory canonicalization.

Each caller of 'update_path' may specify both a filename and
a translation prefix and consist of the name of the package that contains
the file ("@GCC", "@BINUTIL", "@GNU", etc).

If the prefix is not specified, the filename will only undergo
directory canonicalization.

If it is specified, the string given by PREFIX will be replaced
by the specified prefix (with a '@' in front unless the prefix begins
with a '$') and further translation will be done as follows
until none of the two conditions below are met:

1) If the filename begins with '@', the string between the '@' and
the end of the name or the first '/' or directory separator will
be considered a "key" and looked up as follows:

-- If this is a Win32 OS, then the Registry will be examined for
an entry of "key" in

HKEY_LOCAL_MACHINE\SOFTWARE\Free Software Foundation\<KEY>

if found, that value will be used. <KEY> defaults to GCC version
string, but can be overridden at configuration time.

-- If not found (or not a Win32 OS), the environment variable
key_ROOT (the value of "key" concatenated with the constant "_ROOT")
is tried.  If that fails, then PREFIX (see above) is used.

2) If the filename begins with a '$', the rest of the string up
to the end or the first '/' or directory separator will be used
as an environment variable, whose value will be returned.

Once all this is done, any '/' will be converted to DIR_SEPARATOR,
if they are different.

NOTE:  using resolve_keyed_path under Win32 requires linking with
advapi32.dll.  */

/*���ļ��������³���·�������̣���Ҫ�淶��
Ŀ¼��ʽ��ҲҪ�����κ�ǰ׺�ķ��롣

���ļ�������-DPREFIX=ָ��һ��ʹ�ñ�����ָ�����ò���ʹ�õġ�prefix��
��ֵ������ļ�������������
ǰ׺��ʼ����ֻ���ܵ�Ŀ¼�淶����Ӱ�졣

��update_path����ÿһ�������߶�����ָ��һ���ļ�����
����ǰ׺���Ұ��������ļ��İ�������
����@GCC������@BINUTIL������@GNU�����ȵȣ���

���û��ָ��ǰ׺�����ļ���ֻ��ͨ��
Ŀ¼�淶����

���ָ������PREFIX�������ַ���������ָ����ǰ׺�滻
����ǰ����һ��'@'������ǰ׺��ʼ
��'$'��������һ�����뽫�����µ�����
ֱ���������������������㣺

1������ļ����Ŀ�ʼ��'@'����@����
���Ƶ������һ'/'��Ŀ¼�ָ��м���ַ���
�ᱻ��Ϊ��"key"���������£�

-- �������һ��Win32�Ĳ���ϵͳ��Ȼ����ע���Ѱ��
�������·���еġ�key����Ŀ

HKEY_LOCAL_MACHINE\SOFTWARE\Free Software Foundation\<KEY>

������֣����ֵ����ʹ�á� <key>Ĭ��ΪGCC�汾
�ַ�������������ʱ���Ա����ǡ�

-- ���û���ҵ������߲���һ��Win32����ϵͳ������������
key_ROOT����key���롰_ROOT�����Ӻ��ֵ��
�����ԡ����ʧ�ܣ���PREFIX�������ģ���ʹ�á�

2������ļ�����'$'��ʼ�����ಿ��
�������ߵ�һ��'/'��Ŀ¼�ָ���֮����ַ���������
��Ϊһ����������������ֵ�������ء�

һ����һ����ɺ����еġ�/������ת��ΪDIR_SEPARATOR��
���������ǲ�ͬ������Ż�������

ע�⣺ʹ��Win32��resolve_keyed_path��Ҫ����
ADVAPI32.DLL��*/
#undef _WIN32
#define PREFIX	""

#include "config.h"
#include "system.h"
#if defined(_WIN32) && defined(ENABLE_WIN32_REGISTRY)
/* #include <windows.h> */
#endif
#include "prefix.h"

static const char *std_prefix = PREFIX;

static const char *get_key_value	PARAMS((char *));
static char *translate_name		PARAMS((char *));
static char *save_string		PARAMS((const char *, int));
static void tr				PARAMS((char *, int, int));

#if defined(_WIN32) && defined(ENABLE_WIN32_REGISTRY)
static char *lookup_key		PARAMS((char *));
static HKEY reg_key = (HKEY)INVALID_HANDLE_VALUE;
#endif

/* Given KEY, as above, return its value.  */
/* ��ȡkey��Ӧ��ֵ */
static const char *
get_key_value(key)
char *key;
{
	const char *prefix = 0;
	char *temp = 0;

#if defined(_WIN32) && defined(ENABLE_WIN32_REGISTRY)
	prefix = lookup_key(key);  //��win32ע����в���key
#endif

	/* ��ȡprefix */
	if (prefix == 0)
		prefix = getenv(temp = concat(key, "_ROOT", NULL));

	if (prefix == 0)
		prefix = std_prefix;

	/* �ͷſռ� */
	if (temp)
		free(temp);

	return prefix;
}

/* Return a copy of a string that has been placed in the heap.  */
/* �����ڶ��е��ַ����ı��� */
static char *
save_string(s, len)
const char *s;
int len;
{
	/* ����ռ� */
	char *result = xmalloc(len + 1);

	/* �����ַ��� */
	memcpy(result, s, len);

	result[len] = 0;
	return result;
}

#if defined(_WIN32) && defined(ENABLE_WIN32_REGISTRY)
/* win32���� */
/* Look up "key" in the registry, as above.  */

/* ����win32API������ע����ֵ����������ò�����������ģ�libmingw.lib����Ҳû���⼸��API�� */
static char *
lookup_key(key)
char *key;
{
	char *dst;
	DWORD size;
	DWORD type;
	LONG res;

	if (reg_key == (HKEY)INVALID_HANDLE_VALUE)
	{
		res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE", 0,
			KEY_READ, &reg_key);

		if (res == ERROR_SUCCESS)
			res = RegOpenKeyExA(reg_key, "Free Software Foundation", 0,
			KEY_READ, &reg_key);

		if (res == ERROR_SUCCESS)
			res = RegOpenKeyExA(reg_key, WIN32_REGISTRY_KEY, 0,
			KEY_READ, &reg_key);

		if (res != ERROR_SUCCESS)
		{
			reg_key = (HKEY)INVALID_HANDLE_VALUE;
			return 0;
		}
	}

	size = 32;
	dst = (char *)xmalloc(size);

	res = RegQueryValueExA(reg_key, key, 0, &type, dst, &size);
	if (res == ERROR_MORE_DATA && type == REG_SZ)
	{
		dst = (char *)xrealloc(dst, size);
		res = RegQueryValueExA(reg_key, key, 0, &type, dst, &size);
	}

	if (type != REG_SZ || res != ERROR_SUCCESS)
	{
		free(dst);
		dst = 0;
	}

	return dst;
}
#endif

/* If NAME, a malloc-ed string, starts with a '@' or '$', apply the
translation rules above and return a newly malloc-ed name.
Otherwise, return the given name.  */

/* �����malloc������ַ������ԡ�@�����ߡ�$����ͷ�Ļ���������ַ�����������ļ���ͷ��˵��
���������ת�����������µ���malloc���������ַ���������ֱ�ӷ��ظ��ַ����� */

static char *
translate_name(name)
char *name;
{
	char code;
	char *key, *old_name;
	const char *prefix;
	int keylen;

	for (;;)
	{
		code = name[0];
		if (code != '@' && code != '$')
		/* �������'@'����'$'��ͷ���ַ�����ֱ�ӷ��� */
			break;

		/* ����ÿһ���ַ���ֱ�����������߷ָ���������¼ɨ������ַ����� */
		for (keylen = 0;
			(name[keylen + 1] != 0 && !IS_DIR_SEPARATOR(name[keylen + 1]));
			keylen++)
			;

		/* ���·����ַ����ռ� */
		key = (char *)alloca(keylen + 1);
		/* �����ַ������� */
		strncpy(key, &name[1], keylen);
		/* ����ս�� */
		key[keylen] = 0;

		if (code == '@')
		/* '@'��ͷ��ת�� */
		{
			prefix = get_key_value(key);
			if (prefix == 0)
				prefix = std_prefix;
		}
		else
		/* '$'��ͷ��ת�� */
			prefix = getenv(key);

		if (prefix == 0)
			prefix = PREFIX;

		/* We used to strip trailing DIR_SEPARATORs here, but that can
		sometimes yield a result with no separator when one was coded
		and intended by the user, causing two path components to run
		together.  */

		old_name = name;
		/* ���������ǰ׺ */
		name = concat(prefix, &name[keylen + 1], NULL);
		/* �ͷ�old�ռ� */
		free(old_name);
	}

	return name;
}

/* In a NUL-terminated STRING, replace character C1 with C2 in-place.  */
/* ��һ����NULL�ս���ַ����е�����C1�ַ��滻ΪC2 */
static void
tr(string, c1, c2)
char *string;
int c1, c2;
{
	do
	{
		if (*string == c1)
			*string = c2;
	} while (*string++);
}

/* Update PATH using KEY if PATH starts with PREFIX.  The returned
string is always malloc-ed, and the caller is responsible for
freeing it.  */
/* ���path����prefix��ʼ������key��path���¡�
���ص��ַ�����malloc�����ģ����Ե�����Ӧ�ø�����ռ��ͷš� */
char *
update_path(path, key)
const char *path;
const char *key;
{
	char *result;

	/* path�Ա�׼ǰ׺��ʼ������key����NULL */
	if (!strncmp(path, std_prefix, strlen(std_prefix)) && key != 0)
	{
		bool free_key = false;

		/* key��һ���ַ�����'$' */
		if (key[0] != '$')
		{
			/* '@'���ӵ�key��ͷ */
			key = concat("@", key, NULL);
			free_key = true;
		}

		/* ��key��path��ǰ׺���� */
		result = concat(key, &path[strlen(std_prefix)], NULL);
		if (free_key)
			/* ��Ҫ�ͷ�key�Ŀռ� */
			free((char *)key);
		/* ������ */
		result = translate_name(result);
	}
	else
		/* ����path��һ����xmalloc�����ı��� */
		result = xstrdup(path);

#ifdef UPDATE_PATH_HOST_CANONICALIZE
	/* Perform host dependent canonicalization when needed.  */
	/* �����Ҫ�Ļ�������һЩ��׼������ */
	UPDATE_PATH_HOST_CANONICALIZE(path);
#endif

#ifdef DIR_SEPARATOR_2
	/* Convert DIR_SEPARATOR_2 to DIR_SEPARATOR.  */
	/* ���ڶ��ַָ����滻�ɵ�һ�ַָ��� */
	if (DIR_SEPARATOR_2 != DIR_SEPARATOR)
		tr(result, DIR_SEPARATOR_2, DIR_SEPARATOR);
#endif

#if defined (DIR_SEPARATOR) && !defined (DIR_SEPARATOR_2)
	/* ֻ������һ�ַָ���������ɱ�׼�ָ����� */
	if (DIR_SEPARATOR != '/')
		tr(result, '/', DIR_SEPARATOR);
#endif

	return result;
}

/* Reset the standard prefix */
/* ��prefix������standard prefix */
void
set_std_prefix(prefix, len)
const char *prefix;
int len;
{
	/* ����xmalloc�����ַ������� */
	std_prefix = save_string(prefix, len);
}
