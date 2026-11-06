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
 *   1  no /dev/fb0 and no /dev/mem
 *   2  fb0 ioctl failed, and no /dev/mem
 *   3  fb0 mmap failed, and no /dev/mem
 *   4  /dev/mem missing
 *   5  /dev/mem mmap failed
 */
#include <stdio.h>
#include <string.h>
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
	int fd, rc = 1;
	struct fb_var_screeninfo vi;
	struct fb_fix_screeninfo fi;
	unsigned char *p;

	fd = open("/dev/fb0", O_RDWR);
	if (fd >= 0) {
		if (ioctl(fd, FBIOGET_VSCREENINFO, &vi) < 0 ||
		    ioctl(fd, FBIOGET_FSCREENINFO, &fi) < 0) {
			printf("fbsplash: fb0 ioctl failed\n");
			rc = 2;
		} else {
			printf("fbsplash: fb0 %dx%d bpp %d stride %ld len %ld\n",
			       vi.xres, vi.yres, vi.bits_per_pixel,
			       fi.line_length, fi.smem_len);
			p = mmap(NULL, fi.smem_len, PROT_READ | PROT_WRITE,
				 MAP_SHARED, fd, 0);
			if (p == MAP_FAILED) {
				printf("fbsplash: fb0 mmap failed\n");
				rc = 3;
			} else {
				draw(p, fi.line_length, vi.xres, vi.yres);
				printf("fbsplash: drawn through /dev/fb0\n");
				return 0;
			}
		}
	} else {
		printf("fbsplash: no /dev/fb0\n");
	}

	fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd < 0) {
		printf("fbsplash: no /dev/mem (rc %d)\n", rc);
		return rc ? rc : 4;
	}
	p = mmap(NULL, (size_t)FB_STRIDE * FB_H, PROT_READ | PROT_WRITE,
		 MAP_SHARED, fd, FB_PHYS);
	if (p == MAP_FAILED) {
		printf("fbsplash: /dev/mem mmap failed\n");
		return 5;
	}
	draw(p, FB_STRIDE, FB_W, FB_H);
	printf("fbsplash: drawn through /dev/mem\n");
	return 6;
}
