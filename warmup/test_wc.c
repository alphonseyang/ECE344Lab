#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <malloc.h>
#include <unistd.h>
#include <assert.h>
#include "wc.h"

int
main(int argc, char *argv[])
{
	int fd;
	void *addr;
	struct stat sb;
	struct wc *wc;
	struct mallinfo minfo;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s filename\n", argv[0]);
		exit(1);
	}
	/* open file */
	if ((fd = open(argv[1], O_RDONLY)) < 0) {
		fprintf(stderr, "open: %s: %s\n", argv[1], strerror(errno));
		exit(1);
	}
	/* obtain file size */
	if (fstat(fd, &sb) < 0) {
		fprintf(stderr, "fstat: %s: %s\n", argv[1], strerror(errno));
		exit(1);
	}
	/* map the whole file, sb.st_size is the file size. after this call, the
	   file can be accessed as an array of characters, at address addr. */
	addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (addr == MAP_FAILED) {
		fprintf(stderr, "mmap: %s: %s\n", argv[1], strerror(errno));
		exit(1);
	}
	/* close the file */
	close(fd);


	/* note that the array addr is read only, and cannot be modified. */
	wc = wc_init(addr, sb.st_size);
	/* unmap the word array so it is no longer accessible */
	munmap(addr, sb.st_size);
	/* output the words and their counts */
	wc_output(wc);
	/* destroy any data structures created previously */
	wc_destroy(wc);

	/* check for memory leaks */
	minfo = mallinfo();
	assert(minfo.uordblks == 0);
	assert(minfo.hblks == 0);

	exit(0);
}
