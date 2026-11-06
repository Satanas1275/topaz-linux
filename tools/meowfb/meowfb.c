// meowfb: expose the panel's physical memory (0x5c000000) to userspace and to
// the kernel console. The MEOW OS shim and the bootloader already set the
// panel up; all this driver has to do is map it -- no clock, no PHY, no
// reinitialisation -- and register it as a framebuffer so fbcon can draw on it.
//
// The mapping is done with remap_pfn_range (like /dev/mem): the kernel refuses
// to ioremap the RAM / reserved-memory ranges (pfn_is_map_memory), but a plain
// PFN mapping works.
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fb.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>

#define MEOW_FB_PHYS	0x5c000000UL
#define MEOW_FB_W	1080
#define MEOW_FB_H	2400
#define MEOW_FB_STRIDE	(MEOW_FB_W * 4)

static struct fb_info *meow_info;

static int meowfb_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long size = vma->vm_end - vma->vm_start;

	if (size > PAGE_ALIGN((unsigned long)MEOW_FB_STRIDE * MEOW_FB_H))
		return -EINVAL;
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	if (io_remap_pfn_range(vma, vma->vm_start, MEOW_FB_PHYS >> PAGE_SHIFT,
			       size, vma->vm_page_prot))
		return -EAGAIN;
	return 0;
}

static const struct file_operations meowfb_fops = {
	.owner	= THIS_MODULE,
	.mmap	= meowfb_mmap,
};

static struct miscdevice meowfb_miscdev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "meowfb",
	.fops	= &meowfb_fops,
};

/* kernel console access: a framebuffer over the same memory */
static int meowfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
			    u_int transp, struct fb_info *info)
{
	u32 *pal = info->pseudo_palette;
	u32 value;

	if (regno >= 16)
		return -EINVAL;

	value = ((red >> (16 - info->var.red.length)) << info->var.red.offset) |
		((green >> (16 - info->var.green.length)) << info->var.green.offset) |
		((blue >> (16 - info->var.blue.length)) << info->var.blue.offset);
	if (info->var.transp.length > 0) {
		u32 mask = (1 << info->var.transp.length) - 1;

		mask <<= info->var.transp.offset;
		value |= mask;
	}
	pal[regno] = value;
	return 0;
}

static struct fb_ops meowfb_ops = {
	.owner		= THIS_MODULE,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
	.fb_setcolreg	= meowfb_setcolreg,
};

static int __init meowfb_init(void)
{
	struct fb_info *info;
	int ret;

	ret = misc_register(&meowfb_miscdev);
	if (ret) {
		pr_err("meowfb: misc_register: %d\n", ret);
		return ret;
	}

	info = framebuffer_alloc(0, NULL);
	if (!info) {
		pr_err("meowfb: framebuffer_alloc failed\n");
		misc_deregister(&meowfb_miscdev);
		return -ENOMEM;
	}
	meow_info = info;

	info->fbops = &meowfb_ops;
	info->flags = FBINFO_DEFAULT;
	/* fbcon's software ops read the pseudo palette */
	info->pseudo_palette = kcalloc(16, sizeof(u32), GFP_KERNEL);
	if (!info->pseudo_palette) {
		framebuffer_release(info);
		misc_deregister(&meowfb_miscdev);
		return -ENOMEM;
	}
	strlcpy(info->fix.id, "meowfb", sizeof(info->fix.id));
	info->fix.smem_start = MEOW_FB_PHYS;
	info->fix.smem_len = MEOW_FB_STRIDE * MEOW_FB_H;
	info->fix.type = FB_TYPE_PACKED_PIXELS;
	info->fix.visual = FB_VISUAL_TRUECOLOR;
	info->fix.line_length = MEOW_FB_STRIDE;

	info->var.xres = MEOW_FB_W;
	info->var.yres = MEOW_FB_H;
	info->var.xres_virtual = MEOW_FB_W;
	info->var.yres_virtual = MEOW_FB_H;
	info->var.bits_per_pixel = 32;
	info->var.red.offset = 16;
	info->var.red.length = 8;
	info->var.green.offset = 8;
	info->var.green.length = 8;
	info->var.blue.offset = 0;
	info->var.blue.length = 8;
	info->var.transp.offset = 24;
	info->var.transp.length = 8;
	info->var.activate = FB_ACTIVATE_NOW;

	/*
	 * fbcon's software ops (and the software cursor) draw through
	 * screen_base, so it needs a kernel virtual address. The ioremap path is
	 * refused by pfn_is_map_memory for the panel's reserved memory, but the
	 * pages are valid, so vmap them.
	 */
	{
		struct page **pages;
		unsigned long i, nr;

		nr = PAGE_ALIGN((unsigned long)MEOW_FB_STRIDE * MEOW_FB_H) >> PAGE_SHIFT;
		pages = kmalloc_array(nr, sizeof(*pages), GFP_KERNEL);
		if (!pages) {
			framebuffer_release(info);
			misc_deregister(&meowfb_miscdev);
			return -ENOMEM;
		}
		for (i = 0; i < nr; i++)
			pages[i] = pfn_to_page((MEOW_FB_PHYS >> PAGE_SHIFT) + i);
		info->screen_base = vmap(pages, nr, VM_MAP,
					 pgprot_writecombine(PAGE_KERNEL));
		info->screen_size = (resource_size_t)MEOW_FB_STRIDE * MEOW_FB_H;
		kfree(pages);
		if (!info->screen_base) {
			pr_err("meowfb: vmap of the panel failed\n");
			framebuffer_release(info);
			misc_deregister(&meowfb_miscdev);
			return -ENOMEM;
		}
	}

	ret = register_framebuffer(info);
	if (ret) {
		pr_err("meowfb: register_framebuffer: %d\n", ret);
		framebuffer_release(info);
		meow_info = NULL;
		misc_deregister(&meowfb_miscdev);
		return ret;
	}

	pr_info("meowfb: panel at 0x%lx, %dx%d stride %d, fb%d, /dev/meowfb\n",
		MEOW_FB_PHYS, MEOW_FB_W, MEOW_FB_H, MEOW_FB_STRIDE, info->node);
	return 0;
}

static void __exit meowfb_exit(void)
{
	if (meow_info) {
		unregister_framebuffer(meow_info);
		framebuffer_release(meow_info);
		meow_info = NULL;
	}
	misc_deregister(&meowfb_miscdev);
}

module_init(meowfb_init);
module_exit(meowfb_exit);

MODULE_DESCRIPTION("Direct panel framebuffer (the MEOW OS way)");
MODULE_LICENSE("GPL");
