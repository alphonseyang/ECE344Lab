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
main()
{
    printf("guale\n");
	struct wc *wc;
	struct mallinfo minfo;

   
	/* note that the array addr is read only, and cannot be modified. */
        char *temp = "a b c d e f g";
	wc = wc_init(temp, strlen(temp));
	/* unmap the word array so it is no longer accessible */
//	munmap(addr, sb.st_size);
//	/* output the words and their counts */
	wc_output(wc);
//	/* destroy any data structures created previously */
//	wc_destroy(wc);

	/* check for memory leaks */
	minfo = mallinfo();
	assert(minfo.uordblks == 0);
	assert(minfo.hblks == 0);

	exit(0);
}
