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

/*该文件包含更新程序路径的例程，既要规范化
目录格式，也要处理任何前缀的翻译。

此文件必须与-DPREFIX=指令一起使用编译来指定配置部分使用的“prefix”
的值。如果文件名不是以这种
前缀开始，它只会受到目录规范化的影响。

“update_path”的每一个调用者都可以指定一个文件名和
翻译前缀并且包含包含文件的包的名称
（“@GCC”，“@BINUTIL”，“@GNU”，等等）。

如果没有指定前缀，则文件名只会通过
目录规范化。

如果指定，按PREFIX给出的字符串将被由指定的前缀替换
（在前面有一个'@'，除非前缀开始
是'$'），并进一步翻译将做如下的事情
直到下面两个条件都不满足：

1）如果文件名的开始是'@'，“@”到
名称的最后或第一'/'或目录分隔中间的字符串
会被认为是"key"，查找如下：

-- 如果这是一个Win32的操作系统，然后检查注册表寻找
在下面的路径中的“key”条目

HKEY_LOCAL_MACHINE\SOFTWARE\Free Software Foundation\<KEY>

如果发现，则该值将被使用。 <key>默认为GCC版本
字符串，但在配置时可以被覆盖。

-- 如果没有找到（或者不是一个Win32操作系统），环境变量
key_ROOT（“key”与“_ROOT”链接后的值）
被尝试。如果失败，则PREFIX（见上文）被使用。

2）如果文件名以'$'开始，其余部分
到最后或者第一个'/'或目录分隔符之间的字符串将用于
作为一个环境变量，它的值将被返回。

一旦这一切完成后，所有的“/”将被转换为DIR_SEPARATOR，
仅当它们是不同的情况才会这样。

注意：使用Win32下resolve_keyed_path需要链接
ADVAPI32.DLL。*/
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
/* 获取key对应的值 */
static const char *
get_key_value(key)
char *key;
{
	const char *prefix = 0;
	char *temp = 0;

#if defined(_WIN32) && defined(ENABLE_WIN32_REGISTRY)
	prefix = lookup_key(key);  //从win32注册表中查找key
#endif

	/* 获取prefix */
	if (prefix == 0)
		prefix = getenv(temp = concat(key, "_ROOT", NULL));

	if (prefix == 0)
		prefix = std_prefix;

	/* 释放空间 */
	if (temp)
		free(temp);

	return prefix;
}

/* Return a copy of a string that has been placed in the heap.  */
/* 返回在堆中的字符串的备份 */
static char *
save_string(s, len)
const char *s;
int len;
{
	/* 分配空间 */
	char *result = xmalloc(len + 1);

	/* 复制字符串 */
	memcpy(result, s, len);

	result[len] = 0;
	return result;
}

#if defined(_WIN32) && defined(ENABLE_WIN32_REGISTRY)
/* win32下用 */
/* Look up "key" in the registry, as above.  */

/* 利用win32API来查找注册表键值，这个程序用不到这个函数的，libmingw.lib里面也没有这几个API。 */
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

/* 如果由malloc分配的字符串是以‘@’或者‘$’开头的话，将这个字符串按照这个文件开头所说的
规则将其进行转换，并返回新的有malloc分配来的字符串。否则，直接返回该字符串。 */

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
		/* 如果是以'@'或者'$'开头的字符串，直接返回 */
			break;

		/* 遍历每一个字符，直到结束符或者分隔符，并记录扫描过的字符个数 */
		for (keylen = 0;
			(name[keylen + 1] != 0 && !IS_DIR_SEPARATOR(name[keylen + 1]));
			keylen++)
			;

		/* 重新分配字符串空间 */
		key = (char *)alloca(keylen + 1);
		/* 复制字符串数据 */
		strncpy(key, &name[1], keylen);
		/* 添加终结符 */
		key[keylen] = 0;

		if (code == '@')
		/* '@'开头的转换 */
		{
			prefix = get_key_value(key);
			if (prefix == 0)
				prefix = std_prefix;
		}
		else
		/* '$'开头的转换 */
			prefix = getenv(key);

		if (prefix == 0)
			prefix = PREFIX;

		/* We used to strip trailing DIR_SEPARATORs here, but that can
		sometimes yield a result with no separator when one was coded
		and intended by the user, causing two path components to run
		together.  */

		old_name = name;
		/* 给名称添加前缀 */
		name = concat(prefix, &name[keylen + 1], NULL);
		/* 释放old空间 */
		free(old_name);
	}

	return name;
}

/* In a NUL-terminated STRING, replace character C1 with C2 in-place.  */
/* 将一个以NULL终结的字符串中的所有C1字符替换为C2 */
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
/* 如果path是以prefix开始，则用key将path更新。
返回的字符串是malloc得来的，所以调用者应该负责将其空间释放。 */
char *
update_path(path, key)
const char *path;
const char *key;
{
	char *result;

	/* path以标准前缀开始，并且key不是NULL */
	if (!strncmp(path, std_prefix, strlen(std_prefix)) && key != 0)
	{
		bool free_key = false;

		/* key第一个字符不是'$' */
		if (key[0] != '$')
		{
			/* '@'链接到key开头 */
			key = concat("@", key, NULL);
			free_key = true;
		}

		/* 用key将path的前缀更新 */
		result = concat(key, &path[strlen(std_prefix)], NULL);
		if (free_key)
			/* 需要释放key的空间 */
			free((char *)key);
		/* 翻译结果 */
		result = translate_name(result);
	}
	else
		/* 返回path的一个有xmalloc得来的备份 */
		result = xstrdup(path);

#ifdef UPDATE_PATH_HOST_CANONICALIZE
	/* Perform host dependent canonicalization when needed.  */
	/* 如果需要的话，进行一些标准化操作 */
	UPDATE_PATH_HOST_CANONICALIZE(path);
#endif

#ifdef DIR_SEPARATOR_2
	/* Convert DIR_SEPARATOR_2 to DIR_SEPARATOR.  */
	/* 将第二种分隔符替换成第一种分隔符 */
	if (DIR_SEPARATOR_2 != DIR_SEPARATOR)
		tr(result, DIR_SEPARATOR_2, DIR_SEPARATOR);
#endif

#if defined (DIR_SEPARATOR) && !defined (DIR_SEPARATOR_2)
	/* 只定义了一种分隔符，处理成标准分隔符。 */
	if (DIR_SEPARATOR != '/')
		tr(result, '/', DIR_SEPARATOR);
#endif

	return result;
}

/* Reset the standard prefix */
/* 用prefix来重置standard prefix */
void
set_std_prefix(prefix, len)
const char *prefix;
int len;
{
	/* 返回xmalloc的新字符串备份 */
	std_prefix = save_string(prefix, len);
}
