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
	{ 0x244c44a4, "platform_driver_unregister" },
	{ 0xb2549451, "__platform_driver_register" },
	{ 0xe9a2b3af, "_dev_info" },
	{ 0xe56496d3, "framebuffer_release" },
	{ 0x773c6a94, "_dev_err" },
	{ 0x99c64543, "register_framebuffer" },
	{ 0xc6766124, "kmem_cache_alloc_trace" },
	{ 0xecaf9da0, "kmalloc_caches" },
	{ 0x5792f848, "strlcpy" },
	{ 0x5dc5f1ad, "framebuffer_alloc" },
	{ 0x9a707e4e, "registered_fb" },
	{ 0x6b4b2933, "__ioremap" },
	{ 0xaf56600a, "arm64_use_ng_mappings" },
	{ 0x2c5d14a, "misc_register" },
	{ 0x7c339477, "remap_pfn_range" },
	{ 0xedc03953, "iounmap" },
	{ 0xa00f8fa7, "unregister_framebuffer" },
	{ 0x210563cc, "misc_deregister" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Cmeow,fb");
MODULE_ALIAS("of:N*T*Cmeow,fbC*");

MODULE_INFO(scmversion, "g6574c211592b-dirty");
