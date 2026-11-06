// meowfb: expose the panel's physical memory (0x5c000000) to userspace.
// The MEOW OS shim and the bootloader already set the panel up; userspace
// (splash, console, diagnostic) only needs a mapping it can write to, exactly
// the way the shim does. No ioremap: the kernel refuses to ioremap the RAM /
// reserved-memory ranges (pfn_is_map_memory), but remap_pfn_range is fine.
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>

#define MEOW_FB_PHYS	0x5c000000UL
#define MEOW_FB_W	1080
#define MEOW_FB_H	2400
#define MEOW_FB_STRIDE	(MEOW_FB_W * 4)

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

static int __init meowfb_init(void)
{
	int ret;

	ret = misc_register(&meowfb_miscdev);
	if (ret) {
		pr_err("meowfb: misc_register: %d\n", ret);
		return ret;
	}

	pr_info("meowfb: panel at 0x%lx, %dx%d stride %d, /dev/meowfb ready\n",
		MEOW_FB_PHYS, MEOW_FB_W, MEOW_FB_H, MEOW_FB_STRIDE);
	return 0;
}

static void __exit meowfb_exit(void)
{
	misc_deregister(&meowfb_miscdev);
}

module_init(meowfb_init);
module_exit(meowfb_exit);

MODULE_DESCRIPTION("Direct panel memory mapping (the MEOW OS way)");
MODULE_LICENSE("GPL");
