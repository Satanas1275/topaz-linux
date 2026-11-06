/*
 * Draw on /dev/fb0 directly. The kernel console is not drawing, so this is a
 * way to tell whether the framebuffer itself works: if the panel changes, the
 * framebuffer and its mapping are fine and the problem is the VT/fbcon layer.
 */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

int main(void)
{
	int fd;
	struct fb_var_screeninfo vi;
	struct fb_fix_screeninfo fi;
	unsigned char *p;
	long i;
	int x, y;

	fd = open("/dev/fb0", O_RDWR);
	if (fd < 0) {
		printf("fbsplash: no /dev/fb0\n");
		return 1;			/* heartbeat 30 s */
	}
	if (ioctl(fd, FBIOGET_VSCREENINFO, &vi) < 0 ||
	    ioctl(fd, FBIOGET_FSCREENINFO, &fi) < 0) {
		printf("fbsplash: ioctl failed\n");
		return 2;			/* heartbeat 33 s */
	}
	p = mmap(NULL, fi.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED) {
		printf("fbsplash: mmap failed\n");
		return 3;			/* heartbeat 36 s */
	}
	printf("fbsplash: %dx%d bpp %d stride %ld len %ld\n",
	       vi.xres, vi.yres, vi.bits_per_pixel, fi.line_length, fi.smem_len);

	/* green background */
	for (i = 0; i + 3 < fi.smem_len; i += 4) {
		p[i + 0] = 0x00;
		p[i + 1] = 0xff;
		p[i + 2] = 0x00;
		p[i + 3] = 0xff;
	}
	/* white bars every 120 lines: a pattern that cannot be mistaken for text */
	for (y = 0; y < (int)vi.yres; y += 120) {
		for (x = 0; x < (int)vi.xres; x++) {
			long off = (long)y * fi.line_length + (long)x * 4;

			if (off + 3 < fi.smem_len) {
				p[off + 0] = 0xff;
				p[off + 1] = 0xff;
				p[off + 2] = 0xff;
				p[off + 3] = 0xff;
			}
		}
	}
	printf("fbsplash: drew green + bars\n");
	return 0;
}
