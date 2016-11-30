#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("Dual BSD/GPL");
#define DEBUG_DR6 0xffff0ff1
#define DEBUG_DR7 0xb0602

#define write_debugreg(reg, val) do {                       \
	unsigned long __val = val;                              \
	asm volatile ( "mov %0,%%db" #reg : : "r" (__val) );    \
} while (0)

static int hello_init(void)
{
	unsigned long addr = 0;
	unsigned long mem = 5;
	write_debugreg(0, 0);
	write_debugreg(6, DEBUG_DR6);
	write_debugreg(7, DEBUG_DR7);
	printk(KERN_ALERT "hello,I am edsionte/n");
	return 0;
}

static void hello_exit(void)
{
	printk(KERN_ALERT "goodbye,kernel/n");
}

module_init(hello_init);
module_exit(hello_exit);
// 可选 
MODULE_AUTHOR("Tiger-John");
MODULE_DESCRIPTION("This is a simple example!/n");
MODULE_ALIAS("A simplest example");
