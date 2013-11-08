#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xb9d3067e, "module_layout" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x28f46d49, "device_create" },
	{ 0x491f0a43, "__class_create" },
	{ 0xf287ea9c, "__register_chrdev" },
	{ 0x9276b9c9, "create_proc_entry" },
	{ 0x3204ac6d, "pci_get_subsys" },
	{ 0xd6ee688f, "vmalloc" },
	{ 0xf432dd3d, "__init_waitqueue_head" },
	{ 0x3bd1b1f6, "msecs_to_jiffies" },
	{ 0x2072ee9b, "request_threaded_irq" },
	{ 0x42c8de35, "ioremap_nocache" },
	{ 0x1fedf0f4, "__request_region" },
	{ 0xa2e429a9, "kmem_cache_alloc_trace" },
	{ 0xd83ea029, "kmalloc_caches" },
	{ 0x62f9cd91, "pci_disable_link_state" },
	{ 0x8db98a2f, "pci_set_master" },
	{ 0xfa9023e1, "pci_enable_device" },
	{ 0x71e3cecb, "up" },
	{ 0x68aca4ad, "down" },
	{ 0xeae3dfd6, "__const_udelay" },
	{ 0xb2fd5ceb, "__put_user_4" },
	{ 0x25778f34, "pci_bus_write_config_byte" },
	{ 0x3074e8d4, "pci_bus_read_config_byte" },
	{ 0x466f16e, "pci_bus_read_config_word" },
	{ 0xf45b83fa, "pci_bus_read_config_dword" },
	{ 0x4f8b5ddb, "_copy_to_user" },
	{ 0x4f6b400b, "_copy_from_user" },
	{ 0xa1c76e0a, "_cond_resched" },
	{ 0x4632e3d8, "x86_dma_fallback_dev" },
	{ 0xec35ed81, "class_destroy" },
	{ 0xf2eff307, "remove_proc_entry" },
	{ 0x37a0cba, "kfree" },
	{ 0x1c044cbc, "device_destroy" },
	{ 0x7c61340c, "__release_region" },
	{ 0x69a358a6, "iomem_resource" },
	{ 0xedc03953, "iounmap" },
	{ 0x999e8297, "vfree" },
	{ 0xf20dabd8, "free_irq" },
	{ 0xfd5f002d, "dma_ops" },
	{ 0x27e1a049, "printk" },
	{ 0xfa66f77c, "finish_wait" },
	{ 0xd62c833f, "schedule_timeout" },
	{ 0x5c8b5ce8, "prepare_to_wait" },
	{ 0xc8b57c27, "autoremove_wake_function" },
	{ 0x5310fe6d, "current_task" },
	{ 0x78764f4e, "pv_irq_ops" },
	{ 0x43261dca, "_raw_spin_lock_irq" },
	{ 0x91715312, "sprintf" },
	{ 0xcf21d241, "__wake_up" },
	{ 0x69acdf38, "memcpy" },
	{ 0x95b01e0d, "try_module_get" },
	{ 0xf9a482f9, "msleep" },
	{ 0x9a1a00ea, "module_put" },
	{ 0xadaabe1b, "pv_lock_ops" },
	{ 0xd52bf1ce, "_raw_spin_lock" },
	{ 0xb4390f9a, "mcount" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "10F3CC098DC636C565A233D");
