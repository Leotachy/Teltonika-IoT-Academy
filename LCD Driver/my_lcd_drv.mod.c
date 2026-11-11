#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

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

MODULE_INFO(intree, "Y");



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0xc93d7961, "class_destroy" },
	{ 0x037a0cba, "kfree" },
	{ 0x6774026c, "i2c_transfer_buffer_flags" },
	{ 0xc3055d20, "usleep_range_state" },
	{ 0x122c3a7e, "_printk" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0xb2fcb56d, "queue_delayed_work_on" },
	{ 0x6cbbfc54, "__arch_copy_to_user" },
	{ 0x01883fd3, "cdev_add" },
	{ 0x7ba352b0, "i2c_register_driver" },
	{ 0xb6c4379f, "system_percpu_wq" },
	{ 0xda17a5ae, "device_create" },
	{ 0x08b870d9, "krealloc_node_align_noprof" },
	{ 0x61415b05, "class_create" },
	{ 0xdcb764ad, "memset" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x9fa7184a, "cancel_delayed_work_sync" },
	{ 0x1b11d831, "device_destroy" },
	{ 0x091c67af, "__kmalloc_cache_noprof" },
	{ 0xffeedf6a, "delayed_work_timer_fn" },
	{ 0x12a4e128, "__arch_copy_from_user" },
	{ 0xf9ddb5d9, "timer_init_key" },
	{ 0xe96233b4, "i2c_del_driver" },
	{ 0x98cf60b3, "strlen" },
	{ 0x97adb487, "utf8s_to_utf16s" },
	{ 0xf39640e8, "cdev_init" },
	{ 0x79845717, "kmalloc_caches" },
	{ 0xd7597cd6, "cdev_del" },
	{ 0x150028cc, "module_layout" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Cmy_lcd");
MODULE_ALIAS("of:N*T*Cmy_lcdC*");

MODULE_INFO(srcversion, "A39EC6C613A9A0646EDC1A8");
