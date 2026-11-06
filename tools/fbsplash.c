/*
 * Draw on the panel to find out whether the kernel can reach the display at
 * all. Every path is tried in turn, whatever the previous one did:
 *
 *   1. /dev/fb0     the framebuffer (ioctl + mmap)
 *   2. /dev/meowfb  the meowfb misc device (direct mmap of 0x5c000000)
 *   3. /dev/mem     the physical memory (direct mmap of 0x5c000000)
 *
 * The panel is already programmed by the bootloader, so a direct mapping is
 * enough. Exit status (reported through the heartbeat delay by /init):
 *   0  drawn through /dev/fb0
 *   7  drawn through /dev/meowfb
 *   6  drawn through /dev/mem
 *   otherwise the errno of the last attempt
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

	for (i = 0; i + 3 < stride * h; i += 4) {
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

static int draw_via_fb0(void)
{
	struct fb_var_screeninfo vi;
	struct fb_fix_screeninfo fi;
	unsigned char *p;
	int fd;

	fd = open("/dev/fb0", O_RDWR);
	if (fd < 0) {
		printf("fbsplash: /dev/fb0 open: %s\n", strerror(errno));
		return -errno;
	}
	if (ioctl(fd, FBIOGET_VSCREENINFO, &vi) ||
	    ioctl(fd, FBIOGET_FSCREENINFO, &fi)) {
		printf("fbsplash: fb0 ioctl: %s\n", strerror(errno));
		close(fd);
		return -errno;
	}
	printf("fbsplash: fb0 %dx%d bpp %d stride %ld len %ld\n",
	       vi.xres, vi.yres, vi.bits_per_pixel, fi.line_length, fi.smem_len);
	p = mmap(NULL, fi.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED) {
		printf("fbsplash: fb0 mmap: %s\n", strerror(errno));
		close(fd);
		return -errno;
	}
	draw(p, fi.line_length, vi.xres, vi.yres);
	printf("fbsplash: drawn through /dev/fb0\n");
	return 0;
}

static int draw_via_dev(const char *path, unsigned long phys)
{
	unsigned char *p;
	int fd;

	fd = open(path, phys ? O_RDWR | O_SYNC : O_RDWR);
	if (fd < 0) {
		printf("fbsplash: %s open: %s\n", path, strerror(errno));
		return -errno;
	}
	p = mmap(NULL, (size_t)FB_STRIDE * FB_H, PROT_READ | PROT_WRITE,
		 MAP_SHARED, fd, phys);
	if (p == MAP_FAILED) {
		printf("fbsplash: %s mmap: %s\n", path, strerror(errno));
		close(fd);
		return -errno;
	}
	draw(p, FB_STRIDE, FB_W, FB_H);
	printf("fbsplash: drawn through %s\n", path);
	return 0;
}

int main(void)
{
	int rc;

	setvbuf(stdout, NULL, _IONBF, 0);

	rc = draw_via_fb0();
	if (rc == 0)
		return 0;

	rc = draw_via_dev("/dev/meowfb", 0);
	if (rc == 0)
		return 7;

	rc = draw_via_dev("/dev/mem", FB_PHYS);
	if (rc == 0)
		return 6;

	return rc < 0 ? -rc : rc;
}
