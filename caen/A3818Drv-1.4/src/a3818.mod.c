#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
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
	{ 0x59caa4c3, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x6bc3fbc0, __VMLINUX_SYMBOL_STR(__unregister_chrdev) },
	{ 0xf0fdf6cb, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0x8f7284de, __VMLINUX_SYMBOL_STR(device_create) },
	{ 0xd7bd463f, __VMLINUX_SYMBOL_STR(__class_create) },
	{ 0xbb2c2e33, __VMLINUX_SYMBOL_STR(__register_chrdev) },
	{ 0xaed84047, __VMLINUX_SYMBOL_STR(proc_create_data) },
	{ 0xd6ee688f, __VMLINUX_SYMBOL_STR(vmalloc) },
	{ 0xf432dd3d, __VMLINUX_SYMBOL_STR(__init_waitqueue_head) },
	{ 0x3bd1b1f6, __VMLINUX_SYMBOL_STR(msecs_to_jiffies) },
	{ 0x2072ee9b, __VMLINUX_SYMBOL_STR(request_threaded_irq) },
	{ 0x42c8de35, __VMLINUX_SYMBOL_STR(ioremap_nocache) },
	{ 0x1fedf0f4, __VMLINUX_SYMBOL_STR(__request_region) },
	{ 0x68565378, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0xdab9e674, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x9ffceb28, __VMLINUX_SYMBOL_STR(pci_disable_link_state) },
	{ 0xc7df9a38, __VMLINUX_SYMBOL_STR(pci_set_master) },
	{ 0xc9336f9, __VMLINUX_SYMBOL_STR(pci_enable_device) },
	{ 0xde17de1d, __VMLINUX_SYMBOL_STR(pci_get_subsys) },
	{ 0x890ba884, __VMLINUX_SYMBOL_STR(x86_dma_fallback_dev) },
	{ 0x268f87ce, __VMLINUX_SYMBOL_STR(class_destroy) },
	{ 0x6ca47f1d, __VMLINUX_SYMBOL_STR(remove_proc_entry) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x39e12e59, __VMLINUX_SYMBOL_STR(device_destroy) },
	{ 0x7c61340c, __VMLINUX_SYMBOL_STR(__release_region) },
	{ 0x69a358a6, __VMLINUX_SYMBOL_STR(iomem_resource) },
	{ 0xedc03953, __VMLINUX_SYMBOL_STR(iounmap) },
	{ 0x999e8297, __VMLINUX_SYMBOL_STR(vfree) },
	{ 0xf20dabd8, __VMLINUX_SYMBOL_STR(free_irq) },
	{ 0x40665b55, __VMLINUX_SYMBOL_STR(dma_ops) },
	{ 0x71e3cecb, __VMLINUX_SYMBOL_STR(up) },
	{ 0x68aca4ad, __VMLINUX_SYMBOL_STR(down) },
	{ 0xeae3dfd6, __VMLINUX_SYMBOL_STR(__const_udelay) },
	{ 0xb2fd5ceb, __VMLINUX_SYMBOL_STR(__put_user_4) },
	{ 0x4f8b5ddb, __VMLINUX_SYMBOL_STR(_copy_to_user) },
	{ 0x4f6b400b, __VMLINUX_SYMBOL_STR(_copy_from_user) },
	{ 0x233d4a1a, __VMLINUX_SYMBOL_STR(pci_bus_write_config_byte) },
	{ 0x7b1ac9f6, __VMLINUX_SYMBOL_STR(pci_bus_read_config_byte) },
	{ 0xf3f751e1, __VMLINUX_SYMBOL_STR(pci_bus_read_config_word) },
	{ 0xe81292b0, __VMLINUX_SYMBOL_STR(pci_bus_read_config_dword) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x784213a6, __VMLINUX_SYMBOL_STR(pv_lock_ops) },
	{        0, __VMLINUX_SYMBOL_STR(schedule_timeout) },
	{ 0xfa66f77c, __VMLINUX_SYMBOL_STR(finish_wait) },
	{ 0x34f22f94, __VMLINUX_SYMBOL_STR(prepare_to_wait_event) },
	{ 0x78764f4e, __VMLINUX_SYMBOL_STR(pv_irq_ops) },
	{ 0xd3719d59, __VMLINUX_SYMBOL_STR(paravirt_ticketlocks_enabled) },
	{ 0x43261dca, __VMLINUX_SYMBOL_STR(_raw_spin_lock_irq) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0xcf21d241, __VMLINUX_SYMBOL_STR(__wake_up) },
	{ 0x69acdf38, __VMLINUX_SYMBOL_STR(memcpy) },
	{ 0x839fb39, __VMLINUX_SYMBOL_STR(try_module_get) },
	{ 0xf9a482f9, __VMLINUX_SYMBOL_STR(msleep) },
	{ 0x6d9da20b, __VMLINUX_SYMBOL_STR(module_put) },
	{ 0xda3e43d1, __VMLINUX_SYMBOL_STR(_raw_spin_unlock) },
	{ 0xd52bf1ce, __VMLINUX_SYMBOL_STR(_raw_spin_lock) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "1E2B4F1A7259211B3035801");
