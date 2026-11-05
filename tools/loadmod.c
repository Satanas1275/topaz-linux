/*
 * loadmod - load kernel modules with the version checks bypassed.
 *
 * The vendor modules are built from the same 5.15.41 tree but a different
 * config, so their symbol CRCs do not match the running kernel.  The kernel
 * is built with CONFIG_MODULE_FORCE_LOAD=y, which lets finit_module() accept
 * them once both ignore flags are passed.  busybox insmod only passes flags
 * for 2.4-style modules, hence this helper.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>

#ifndef __NR_finit_module
#define __NR_finit_module 273
#endif

#ifndef MODULE_INIT_IGNORE_MODVERSIONS
#define MODULE_INIT_IGNORE_MODVERSIONS 1
#endif
#ifndef MODULE_INIT_IGNORE_VERMAGIC
#define MODULE_INIT_IGNORE_VERMAGIC 2
#endif

int main(int argc, char **argv)
{
	int i;

	if (argc < 2) {
		fprintf(stderr, "usage: %s module.ko [module.ko ...]\n", argv[0]);
		return 2;
	}
	for (i = 1; i < argc; i++) {
		int fd = open(argv[i], O_RDONLY);
		long rc;

		if (fd < 0) {
			perror(argv[i]);
			continue;
		}
		rc = syscall(__NR_finit_module, fd, "",
			    MODULE_INIT_IGNORE_VERMAGIC |
			    MODULE_INIT_IGNORE_MODVERSIONS);
		if (rc)
			fprintf(stderr, "%s: %s\n", argv[i], strerror(errno));
		close(fd);
	}
	return 0;
}
