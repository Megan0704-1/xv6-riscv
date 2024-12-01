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
	{ 0x21ac03cb, "bio_associate_blkg" },
	{ 0x6fbc6a00, "radix_tree_insert" },
	{ 0x86ac3eec, "bio_copy_data" },
	{ 0x51019eb4, "dm_put_device" },
	{ 0x70ad75fb, "radix_tree_lookup" },
	{ 0x4829a47e, "memcpy" },
	{ 0x37a0cba, "kfree" },
	{ 0x601f665f, "dm_io_client_create" },
	{ 0xba8fbd64, "_raw_spin_lock" },
	{ 0x3ad9d5b2, "kvmalloc_node_noprof" },
	{ 0x122c3a7e, "_printk" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0xc7c5f1d8, "bio_put" },
	{ 0x859368a6, "submit_bio_noacct" },
	{ 0xc05f582d, "dm_unregister_target" },
	{ 0xd4be4c91, "bio_add_page" },
	{ 0xbcab6ee6, "sscanf" },
	{ 0x75ca79b5, "__fortify_panic" },
	{ 0x9e4faeef, "dm_io_client_destroy" },
	{ 0x5b667666, "bio_free_pages" },
	{ 0xa648e561, "__ubsan_handle_shift_out_of_bounds" },
	{ 0x8af1d478, "dm_table_device_name" },
	{ 0x5f731dbf, "dm_register_target" },
	{ 0x793b97c0, "dm_table_get_mode" },
	{ 0x231f026a, "alloc_pages_noprof" },
	{ 0xa65c6def, "alt_cb_patch_nops" },
	{ 0x99759f2e, "dm_get_device" },
	{ 0x98cf60b3, "strlen" },
	{ 0x7aa1756e, "kvfree" },
	{ 0xb5b54b34, "_raw_spin_unlock" },
	{ 0x4eadb341, "bio_alloc_bioset" },
	{ 0x73f12f89, "fs_bio_set" },
	{ 0x7d439289, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "84805A472D2DD1781D2BDC9");
