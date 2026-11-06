// meowfb: a framebuffer that maps the panel's physical memory directly, the
// same way the MEOW OS shim draws on it. The panel is already set up by the
// bootloader at 0x5c000000, so all this driver has to do is map it and expose
// it to fbcon -- no clock, no PHY, no reinitialisation.
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fb.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#define MEOW_FB_PHYS	0x5c000000UL
#define MEOW_FB_W	1080
#define MEOW_FB_H	2400
#define MEOW_FB_STRIDE	(MEOW_FB_W * 4)

static void __iomem *meow_fb_base;
static struct fb_info *meow_info;

/* the kernel keeps this as an anonymous struct inside fb_info */
struct meow_apertures {
	unsigned int count;
	struct {
		resource_size_t base;
		resource_size_t size;
	} ranges[1];
};

/*
 * A plain character device: userspace mmaps the panel's physical memory and
 * draws on it directly, exactly the way the MEOW OS shim does. This is the
 * reliable path while the framebuffer-console side is being sorted out.
 */
static int meowfb_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long size = vma->vm_end - vma->vm_start;

	if (size > (unsigned long)MEOW_FB_STRIDE * MEOW_FB_H)
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

static struct fb_ops meowfb_ops = {
	.owner		= THIS_MODULE,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
};

static int __init meowfb_init(void)
{
	int ret;

	ret = misc_register(&meowfb_miscdev);
	if (ret) {
		pr_err("meowfb: misc_register: %d\n", ret);
		return ret;
	}

	meow_fb_base = ioremap_wc(MEOW_FB_PHYS,
				  (resource_size_t)MEOW_FB_STRIDE * MEOW_FB_H);
	if (!meow_fb_base) {
		pr_err("meowfb: ioremap_wc(0x%lx) failed\n", MEOW_FB_PHYS);
		misc_deregister(&meowfb_miscdev);
		return -ENOMEM;
	}

	pr_info("meowfb: panel at 0x%lx, %dx%d stride %d, /dev/meowfb ready\n",
		MEOW_FB_PHYS, MEOW_FB_W, MEOW_FB_H, MEOW_FB_STRIDE);
	return 0;
}

static void __exit meowfb_exit(void)
{
	if (meow_fb_base)
		iounmap(meow_fb_base);
	misc_deregister(&meowfb_miscdev);
}

module_init(meowfb_init);
module_exit(meowfb_exit);

MODULE_DESCRIPTION("Direct panel framebuffer (the MEOW OS way)");
MODULE_LICENSE("GPL");
