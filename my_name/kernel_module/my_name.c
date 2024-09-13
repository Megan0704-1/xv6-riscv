#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Megan Kuo");

static int intParameter = 0;
module_param(intParameter, int, 0);

static char *charParameter = "default";
module_param(charParameter, charp, 0);

static int __init example_init(void) {
    printk(KERN_INFO "Hello, I am Megan Kuo, a student of CSE330 %s %d\n", charParameter, intParameter);
    return 0;
}

static void __exit example_exit(void) {
    printk(KERN_INFO, "Exiting...\n");
}

module_init(example_init);
module_exit(example_exit);
