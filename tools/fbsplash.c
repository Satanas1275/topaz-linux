/*
 * Draw on the panel to find out why the kernel console is not visible.
 *
 * The ABL splash framebuffer is at 0x5c000000 and the shim writes there with
 * the MMU off, so the address is known good. If /dev/fb0 cannot be used, fall
 * back to /dev/mem and map that physical address directly: if the panel changes
 * then the kernel's memory mapping works and the problem is the fb driver.
 *
 * Exit status (reported through the heartbeat delay by /init):
 *   0  drawn through /dev/fb0
 *   6  drawn through /dev/mem
 *   10 + errno  /dev/mem could not be opened
 *   20 + errno  /dev/fb0 could not be opened
 *   30 + errno  fb0 ioctl failed
 *   40 + errno  fb0 mmap failed
 *   50 + errno  /dev/mem mmap failed
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

#define FB_PHYS		0x5c000000UL
#define FB_W		1080
#define FB_H		2400
#define FB_STRIDE	(FB_W * 4)

static void draw(unsigned char *p, long stride, int w, int h)
{
	long i;
	int x, y;

	for (i = 0; i + 3 < (long)stride * h; i += 4) {
		p[i + 0] = 0x00;
		p[i + 1] = 0xff;
		p[i + 2] = 0x00;
		p[i + 3] = 0xff;
	}
	/* white bars every 120 lines: a pattern that cannot be mistaken for text */
	for (y = 0; y < h; y += 120) {
		for (x = 0; x < w; x++) {
			long off = (long)y * stride + (long)x * 4;

			p[off + 0] = 0xff;
			p[off + 1] = 0xff;
			p[off + 2] = 0xff;
			p[off + 3] = 0xff;
		}
	}
}

int main(void)
{
	int fd, rc = 0;
	struct fb_var_screeninfo vi;
	struct fb_fix_screeninfo fi;
	unsigned char *p;

	fd = open("/dev/fb0", O_RDWR);
	if (fd < 0) {
		printf("fbsplash: no /dev/fb0: %s\n", strerror(errno));
		return errno == ENOENT ? 20 : errno == EACCES ? 21 : 29;
	}
	if (ioctl(fd, FBIOGET_VSCREENINFO, &vi) < 0 ||
	    ioctl(fd, FBIOGET_FSCREENINFO, &fi) < 0) {
		printf("fbsplash: fb0 ioctl: %s\n", strerror(errno));
		rc = errno == ENOTTY ? 30 : errno == EINVAL ? 31 :
		     errno == EACCES ? 32 : 39;
	} else {
		printf("fbsplash: fb0 %dx%d bpp %d stride %ld len %ld\n",
		       vi.xres, vi.yres, vi.bits_per_pixel,
		       fi.line_length, fi.smem_len);
		p = mmap(NULL, fi.smem_len, PROT_READ | PROT_WRITE,
			 MAP_SHARED, fd, 0);
		if (p == MAP_FAILED) {
			printf("fbsplash: fb0 mmap: %s\n", strerror(errno));
			rc = errno == ENODEV ? 40 : errno == EINVAL ? 41 :
			     errno == EACCES ? 42 : 49;
		} else {
			draw(p, fi.line_length, vi.xres, vi.yres);
			printf("fbsplash: drawn through /dev/fb0\n");
			return 0;
		}
	}

	/* the meowfb module: the panel's physical memory, mmaped directly */
	fd = open("/dev/meowfb", O_RDWR);
	if (fd >= 0) {
		p = mmap(NULL, (size_t)FB_STRIDE * FB_H, PROT_READ | PROT_WRITE,
			 MAP_SHARED, fd, 0);
		if (p == MAP_FAILED) {
			printf("fbsplash: /dev/meowfb mmap: %s\n", strerror(errno));
			return errno == EACCES ? 60 : errno == EPERM ? 61 :
			       errno == EINVAL ? 62 : 69;
		}
		draw(p, FB_STRIDE, FB_W, FB_H);
		printf("fbsplash: drawn through /dev/meowfb\n");
		return 7;
	}
	if (!rc) {
		printf("fbsplash: no /dev/meowfb: %s\n", strerror(errno));
		rc = errno == ENOENT ? 50 : errno == EACCES ? 51 : 59;
	}

	fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd < 0) {
		printf("fbsplash: no /dev/mem: %s (rc %d)\n", strerror(errno), rc);
		if (rc)
			return rc;
		return errno == ENOENT ? 70 : errno == EACCES ? 71 :
		       errno == EPERM ? 72 : 79;
	}
	p = mmap(NULL, (size_t)FB_STRIDE * FB_H, PROT_READ | PROT_WRITE,
		 MAP_SHARED, fd, FB_PHYS);
	if (p == MAP_FAILED) {
		printf("fbsplash: /dev/mem mmap: %s\n", strerror(errno));
		return errno == EACCES ? 80 : errno == EPERM ? 81 :
		       errno == EINVAL ? 82 : errno == EBUSY ? 83 : 89;
	}
	draw(p, FB_STRIDE, FB_W, FB_H);
	printf("fbsplash: drawn through /dev/mem\n");
	return 6;
}
