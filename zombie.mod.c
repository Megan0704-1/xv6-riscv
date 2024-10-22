#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

#ifdef CONFIG_UNWINDER_ORC
#include <asm/orc_header.h>
ORC_HEADER;
#endif

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_MITIGATION_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x64ab2ee9, "kthread_stop" },
	{ 0x37a0cba, "kfree" },
	{ 0x122c3a7e, "_printk" },
	{ 0x246e77b3, "init_task" },
	{ 0xb3f7646e, "kthread_should_stop" },
	{ 0x8d522714, "__rcu_read_lock" },
	{ 0x4dfa8d4b, "mutex_lock" },
	{ 0xa65c6def, "alt_cb_patch_nops" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0xe2964344, "__wake_up" },
	{ 0x2469810f, "__rcu_read_unlock" },
	{ 0xf9a482f9, "msleep" },
	{ 0x296695f, "refcount_warn_saturate" },
	{ 0x6ff3a485, "dynamic_might_resched" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0x1000e51, "schedule" },
	{ 0x8c26d495, "prepare_to_wait_event" },
	{ 0x92540fbf, "finish_wait" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x52c5c991, "__kmalloc_noprof" },
	{ 0xcefb0c9f, "__mutex_init" },
	{ 0xd9a5ea54, "__init_waitqueue_head" },
	{ 0x80e6e7d6, "kthread_create_on_node" },
	{ 0x32710744, "wake_up_process" },
	{ 0x3efc6d60, "param_ops_int" },
	{ 0x7d439289, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "B3E98C6B6F27F1A172B28DC");
