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

static int meowfb_probe(struct platform_device *pdev)
{
	struct fb_info *info;
	int ret;

	ret = misc_register(&meowfb_miscdev);
	if (ret) {
		dev_err(&pdev->dev, "misc_register: %d\n", ret);
		return ret;
	}

	meow_fb_base = ioremap_wc(MEOW_FB_PHYS, (resource_size_t)MEOW_FB_STRIDE * MEOW_FB_H);
	if (!meow_fb_base) {
		dev_err(&pdev->dev, "ioremap_wc(0x%lx) failed\n", MEOW_FB_PHYS);
		misc_deregister(&meowfb_miscdev);
		return -ENOMEM;
	}

	/* Another driver may already have taken fb0; the panel is ours. */
	if (registered_fb[0]) {
		dev_info(&pdev->dev, "taking over fb0 from %s\n",
			 registered_fb[0]->fix.id);
		unregister_framebuffer(registered_fb[0]);
	}

	info = framebuffer_alloc(0, &pdev->dev);
	if (!info) {
		misc_deregister(&meowfb_miscdev);
		return -ENOMEM;
	}

	strlcpy(info->fix.id, "meowfb", sizeof(info->fix.id));
	info->fix.smem_start	= MEOW_FB_PHYS;
	info->fix.smem_len	= MEOW_FB_STRIDE * MEOW_FB_H;
	info->fix.line_length	= MEOW_FB_STRIDE;
	info->fix.type		= FB_TYPE_PACKED_PIXELS;
	info->fix.visual	= FB_VISUAL_TRUECOLOR;
	info->var.xres		= MEOW_FB_W;
	info->var.yres		= MEOW_FB_H;
	info->var.xres_virtual	= MEOW_FB_W;
	info->var.yres_virtual	= MEOW_FB_H;
	info->var.bits_per_pixel = 32;
	info->var.red		= (struct fb_bitfield){ 16, 8, 0 };
	info->var.green		= (struct fb_bitfield){ 8, 8, 0 };
	info->var.blue		= (struct fb_bitfield){ 0, 8, 0 };
	info->var.transp	= (struct fb_bitfield){ 24, 8, 0 };
	info->var.activate	= FB_ACTIVATE_NOW;
	info->var.height	= -1;
	info->var.width		= -1;
	info->screen_base	= meow_fb_base;
	info->fbops		= &meowfb_ops;
	info->flags		= FBINFO_DEFAULT | FBINFO_MISC_FIRMWARE;

	struct meow_apertures *ap;

	ap = kzalloc(sizeof(*ap), GFP_KERNEL);
	if (ap) {
		ap->count = 1;
		ap->ranges[0].base = MEOW_FB_PHYS;
		ap->ranges[0].size = MEOW_FB_STRIDE * MEOW_FB_H;
		info->apertures = (void *)ap;
	}

	ret = register_framebuffer(info);
	if (ret) {
		/* not fatal: /dev/meowfb still gives userspace the panel */
		dev_err(&pdev->dev, "register_framebuffer: %d\n", ret);
		framebuffer_release(info);
		info = NULL;
	} else {
		meow_info = info;
		dev_info(&pdev->dev, "fb%d: %dx%d stride %d at 0x%lx\n",
			 info->node, MEOW_FB_W, MEOW_FB_H, MEOW_FB_STRIDE,
			 MEOW_FB_PHYS);
	}
	return 0;
}

static int meowfb_remove(struct platform_device *pdev)
{
	misc_deregister(&meowfb_miscdev);
	if (meow_info)
		unregister_framebuffer(meow_info);
	if (meow_fb_base)
		iounmap(meow_fb_base);
	return 0;
}

static const struct of_device_id meowfb_of_match[] = {
	{ .compatible = "meow,fb" },
	{ }
};
MODULE_DEVICE_TABLE(of, meowfb_of_match);

static struct platform_driver meowfb_driver = {
	.probe	= meowfb_probe,
	.remove	= meowfb_remove,
	.driver	= {
		.name		= "meowfb",
		.of_match_table	= meowfb_of_match,
	},
};
module_platform_driver(meowfb_driver);

MODULE_DESCRIPTION("Direct panel framebuffer (the MEOW OS way)");
MODULE_LICENSE("GPL");
