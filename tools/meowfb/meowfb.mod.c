#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

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

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x15f7a249, "module_layout" },
	{ 0x3fff2fe8, "cfb_imageblit" },
	{ 0x1ff5351c, "cfb_copyarea" },
	{ 0x17710c9d, "cfb_fillrect" },
	{ 0xa00f8fa7, "unregister_framebuffer" },
	{ 0x99c64543, "register_framebuffer" },
	{ 0x37a0cba, "kfree" },
	{ 0x64d1d3c3, "vmap" },
	{ 0xaf56600a, "arm64_use_ng_mappings" },
	{ 0x9688de8b, "memstart_addr" },
	{ 0xb8b9f817, "kmalloc_order_trace" },
	{ 0x5792f848, "strlcpy" },
	{ 0xe56496d3, "framebuffer_release" },
	{ 0xc6766124, "kmem_cache_alloc_trace" },
	{ 0xecaf9da0, "kmalloc_caches" },
	{ 0x210563cc, "misc_deregister" },
	{ 0x5dc5f1ad, "framebuffer_alloc" },
	{ 0x92997ed8, "_printk" },
	{ 0x2c5d14a, "misc_register" },
	{ 0x7c339477, "remap_pfn_range" },
};

MODULE_INFO(depends, "");


MODULE_INFO(scmversion, "gde1cd93b9cd8-dirty");
