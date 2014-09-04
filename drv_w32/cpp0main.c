/* copyright(C) 2003 H.Kawai (under KL-01). */

#define SIZ_STDOUT			(2 * 1024 * 1024)	//标准输出缓冲大小
#define SIZ_STDERR			(16 * 1024)			//标准错误缓冲大小
#define SIZ_WORK			(8 * 1024 * 1024)	//标准工作空间大小
#define SIZ_SYSWRK			(2 * 1024 * 1024)	//系统工作空间大小

/*
 * 下面是引用cc1drv.c中的代码框架来初始化程序。
 * 第一次接触的人可能会对函数的入口不是main有些疑问，这个问题我们得先弄清楚。
 * 其实对main还是mainCRTStartup是函数入口的不同理解在于角度不同。
 * 对于普通编程人员来说，main函数就是他们关注的起点。
 * 但是有没有想过，程序运行是需要内存的，这些内存是怎么得到的？
 * 这就是mainCRTStartup的作用。
 * 在Windows或者Linux等操作系统上运行main函数前，系统需要执行mainCRTStartup来为main函数分配系统资源。
 * 由于这个工具应该是原作者将用于自制操作系统的原因，尽量避免使用系统自带的功能。
 *
 */
#include "cc1drv.c"
