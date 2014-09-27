/* Hash tables.
Copyright (C) 2000, 2001 Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

In other words, you are welcome to use, share and improve this program.
You are forbidden to forbid anyone else to use, share and improve
what you give them.   Help stamp out software-hoarding!  */

#include "config.h"
#include "system.h"
#include "hashtable.h"

/* The code below is a specialization of Vladimir Makarov's expandable
hash tables (see libiberty/hashtab.c).  The abstraction penalty was
too high to continue using the generic form.  This code knows
intrinsically how to calculate a hash value, and how to compare an
existing entry with a potential new one.  Also, the ability to
delete members from the table has been removed.  */

/* 下面的代码是对弗拉基米尔·马卡罗夫可扩展哈希表（完全版见libiberty/ hashtab.c）
的特性化。使用其通用形式的代价太大，所以这代码进行的删减。
这部分代码能够计算散列值，并且能将现有条目与一个潜在的新条目进行比较。
此外，从表中删除的成员的功能已被移除。 */

static unsigned int calc_hash PARAMS((const unsigned char *, unsigned int));
static void ht_expand PARAMS((hash_table *));

/* Let particular systems override the size of a chunk.  */
/* 如果某些操作系统已经有自己对这些宏的定义 */
#ifndef OBSTACK_CHUNK_SIZE
#define OBSTACK_CHUNK_SIZE 0
#endif
/* Let them override the alloc and free routines too.  */
/* 可以自己定义内存管理函数 */
#ifndef OBSTACK_CHUNK_ALLOC
#define OBSTACK_CHUNK_ALLOC xmalloc
#endif
#ifndef OBSTACK_CHUNK_FREE
#define OBSTACK_CHUNK_FREE free
#endif

/* Initialise an obstack.  */
/* 初始化obstack（对象栈） */
void
gcc_obstack_init(obstack)
struct obstack *obstack;
{
	/* 初始化obstack结构，设置默认大小和内存获取函数和内存释放函数 */
	_obstack_begin(obstack, OBSTACK_CHUNK_SIZE, 0,
		(void *(*) PARAMS((long))) OBSTACK_CHUNK_ALLOC,
		(void(*) PARAMS((void *))) OBSTACK_CHUNK_FREE);
}

/* Calculate the hash of the string STR of length LEN.  */
/* 计算长度为len的字符串的哈希值 */
static unsigned int
calc_hash(str, len)
const unsigned char *str;
unsigned int len;
{
	unsigned int n = len;
	unsigned int r = 0;
	/* 哈希计算方式：hash = hash × 67 +　char - 113 */
#define HASHSTEP(r, c) ((r) * 67 + ((c) - 113));

	while (n--)
		r = HASHSTEP(r, *str++);

	return r + len;
#undef HASHSTEP
}

/* Initialize an identifier hashtable.  */
/* 初始化标识符哈希表，用来存放标识符 */
hash_table *
ht_create(order)
unsigned int order;
{
	/* order指的是2的幂 */
	unsigned int nslots = 1 << order;
	hash_table *table;

	/* 申请哈希表指针空间 */
	table = (hash_table *)xmalloc(sizeof (hash_table));
	memset(table, 0, sizeof (hash_table));

	/* Strings need no alignment.  */
	/* 初始化结构和obstack */
	gcc_obstack_init(&table->stack);
	obstack_alignment_mask(&table->stack) = 0;

	/* 分配哈希表存储空间，大小为节点数目×节点大小 */
	table->entries = (hashnode *)xcalloc(nslots, sizeof (hashnode));
	table->nslots = nslots;
	return table;
}

/* Frees all memory associated with a hash table.  */
/* 释放哈希表所有相关空间 */
void
ht_destroy(table)
hash_table *table;
{
	/* obstack的释放函数 */
	obstack_free(&table->stack, NULL);
	/* 释放xcalloc得到的空间 */
	free(table->entries);
	free(table);
}

/* Returns the hash entry for the a STR of length LEN.  If that string
already exists in the table, returns the existing entry, and, if
INSERT is CPP_ALLOCED, frees the last obstack object.  If the
identifier hasn't been seen before, and INSERT is CPP_NO_INSERT,
returns NULL.  Otherwise insert and returns a new entry.  A new
string is alloced if INSERT is CPP_ALLOC, otherwise INSERT is
CPP_ALLOCED and the item is assumed to be at the top of the
obstack.  */
/* 查找哈希节点，找不到的话根据insert的值来确定是否新建该节点 */
hashnode
ht_lookup(table, str, len, insert)
hash_table *table;
const unsigned char *str;
unsigned int len;
enum ht_lookup_option insert;
{
	/* 计算出目标哈希值 */
	unsigned int hash = calc_hash(str, len);
	unsigned int hash2;
	unsigned int index;
	size_t sizemask;
	hashnode node;

	/* 将哈希数值转换成索引 */
	sizemask = table->nslots - 1;
	index = hash & sizemask;

	/* hash2 must be odd, so we're guaranteed to visit every possible
	location in the table during rehashing.  */
	/* hash2 必须是个奇数，才能确保在重新hash的时候能访问所有可能的位置 */
	hash2 = ((hash * 17) & sizemask) | 1;
	/* 记录查找次数 */
	table->searches++;

	for (;;)
	{
		/* 获取索引相同的节点 */
		node = table->entries[index];

		/* 节点为空，未使用过，目标一定不在哈希表中 */
		if (node == NULL)
			break;

		/* 比较目标与结果的实际值，确定搜索结果 */
		if (HT_LEN(node) == len && !memcmp(HT_STR(node), str, len))
		{
			if (insert == HT_ALLOCED)
				/* The string we search for was placed at the end of the
				obstack.  Release it.  */
				/* 搜索的目标字符串分配在obstack的最后，找到哈希表中已经存在该字符串，为避免冗余进行释放 */
				obstack_free(&table->stack, (PTR)str);
			/* 返回结果 */
			return node;
		}

		/* 再次哈希 */
		index = (index + hash2) & sizemask;
		/* 记录冲突次数 */
		table->collisions++;
	}

	/* 未找到节点 */
	
	/* 指明不要插入，直接返回NULL */
	if (insert == HT_NO_INSERT)
		return NULL;

	/* 默认为插入，将搜索的目标作为新节点放入哈希表 */
	node = (*table->alloc_node) (table);
	table->entries[index] = node;

	/* 设置属性 */
	HT_LEN(node) = len;
	/* 是否需要分配空间 */
	if (insert == HT_ALLOC)
		/* 用栈增长的方式将字符串保存到栈空间的尾部 */
		HT_STR(node) = obstack_copy0(&table->stack, str, len);
	else
		HT_STR(node) = str;

	/* 当已用空间达到一定比例时要扩展哈希表 */
	if (++table->nelements * 4 >= table->nslots * 3)
		/* Must expand the string table.  */
		ht_expand(table);

	/* 返回结果 */
	return node;
}

/* Double the size of a hash table, re-hashing existing entries.  */
/* 扩展哈希表空间大小至原大小的两倍，重新哈希已存在的哈希表项 */
static void
ht_expand(table)
hash_table *table;
{
	hashnode *nentries, *p, *limit;
	unsigned int size, sizemask;

	/* 两倍大小 */
	size = table->nslots * 2;
	nentries = (hashnode *)xcalloc(size, sizeof (hashnode));
	sizemask = size - 1;

	p = table->entries;
	limit = p + table->nslots;
	/* 遍历，将所有已存在项目重新哈希并存放新的哈希表中 */
	do
	if (*p)  /* 该项存在... */
	{
		unsigned int index, hash, hash2;

		/* 字符串哈希值 */
		hash = calc_hash(HT_STR(*p), HT_LEN(*p));
		hash2 = ((hash * 17) & sizemask) | 1;
		index = hash & sizemask;

		/* 找到哈希表项存放 */
		for (;;)
		{
			if (!nentries[index])
			{
				nentries[index] = *p;
				break;
			}
			/* 再哈希找到可用位置 */
			index = (index + hash2) & sizemask;
		}
	}
	while (++p < limit);

	/* 释放原表空间 */
	free(table->entries);
	table->entries = nentries;
	table->nslots = size;
}

/* For all nodes in TABLE, callback CB with parameters TABLE->PFILE,
the node, and V.  */
/* 对所有表节点进行指定参数的函数回调 */
void
ht_forall(table, cb, v)
hash_table *table;
ht_cb cb;
const PTR v;
{
	hashnode *p, *limit;

	p = table->entries;
	limit = p + table->nslots;
	do
	if (*p)
	{
		/* 回调函数，等于零终止 */
		if ((*cb) (table->pfile, *p, v) == 0)
			break;
	}
	while (++p < limit);
}

/* Dump allocation statistics to stderr.  */
/* 输出统计信息到标准错误输出 */
void
ht_dump_statistics(table)
hash_table *table;
{
	size_t nelts, nids, overhead, headers;
	size_t total_bytes, longest, sum_of_squares;
	double exp_len, exp_len2, exp2_len;
	hashnode *p, *limit;

#define SCALE(x) ((unsigned long) ((x) < 1024*10 \
	? (x) \
	: ((x) < 1024 * 1024 * 10 \
	? (x) / 1024 \
	: (x) / (1024 * 1024))))
#define LABEL(x) ((x) < 1024*10 ? ' ' : ((x) < 1024*1024*10 ? 'k' : 'M'))

	total_bytes = longest = sum_of_squares = nids = 0;
	p = table->entries;
	limit = p + table->nslots;
	
	/* 统计哈希表中的信息 */
	do
	if (*p)
	{
		size_t n = HT_LEN(*p);

		total_bytes += n;
		sum_of_squares += n * n;
		if (n > longest)
			longest = n;
		nids++;
	}
	while (++p < limit);

	nelts = table->nelements;
	overhead = obstack_memory_used(&table->stack) - total_bytes;
	headers = table->nslots * sizeof (hashnode);

	/* 进行输出 */
	fprintf(stderr, "\nString pool\nentries\t\t%lu\n",
		(unsigned long)nelts);
	fprintf(stderr, "identifiers\t%lu (%.2f%%)\n",
		(unsigned long)nids, nids * 100.0 / nelts);
	fprintf(stderr, "slots\t\t%lu\n",
		(unsigned long)table->nslots);
	fprintf(stderr, "bytes\t\t%lu%c (%lu%c overhead)\n",
		SCALE(total_bytes), LABEL(total_bytes),
		SCALE(overhead), LABEL(overhead));
	fprintf(stderr, "table size\t%lu%c\n",
		SCALE(headers), LABEL(headers));

	exp_len = (double)total_bytes / (double)nelts;
	exp2_len = exp_len * exp_len;
	exp_len2 = (double)sum_of_squares / (double)nelts;

	fprintf(stderr, "coll/search\t%.4f\n",
		(double)table->collisions / (double)table->searches);
	fprintf(stderr, "ins/search\t%.4f\n",
		(double)nelts / (double)table->searches);
	fprintf(stderr, "avg. entry\t%.2f bytes (+/- %.2f)\n",
		exp_len, approx_sqrt(exp_len2 - exp2_len));
	fprintf(stderr, "longest entry\t%lu\n",
		(unsigned long)longest);
#undef SCALE
#undef LABEL
}

/* Return the approximate positive square root of a number N.  This is for
statistical reports, not code generation.  */
/* 二分法逼近求一个数的算术平方根 */
double
approx_sqrt(x)
double x;
{
	double s, d;

	if (x < 0)
		abort();
	if (x == 0)
		return 0;

	s = x;
	do
	{
		d = (s * s - x) / (2 * s);
		s -= d;
	} while (d > .0001);
	return s;
}
