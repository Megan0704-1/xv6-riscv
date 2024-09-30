// File: kernel_module/my_syscall.c

#include <linux/kernel.h>
#include <linux/syscalls.h>

/**
 * @brief Implement syscall for linux. There are 2 ways:
 * 1. asmlinkage is a calling convention modifier. It basically tells the compiler that the func args will be passed on the stack (!registers).
 * @note The syscall handler expects args on the stack. Useful in architecture like x86, as args are typically passed from the user space on the stack.
 *
 * 2. SYSCALL_DEFINE`N` is a family of macros to define a sys call with N arguments.
 * @note High level syscall defs. In ARM64 arguments are usually passed via registers.
 * @param 1) the name of the syscall, 2) pairs of (type and name)
 * @return void
 */
asmlinkage long __arm64_sys_my_syscall(void) {

	printk(KERN_INFO "This is the new system call Megan Kuo implemented.\n");
	return 0;
}


