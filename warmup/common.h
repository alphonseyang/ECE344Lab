#ifndef __COMMON_H
#define __COMMON_H

/* DO NOT CHANGE THIS FILE */

#include <stdio.h>
#include <stdlib.h>

#define TBD() do {							\
		printf("%s:%d: %s: please implement this functionality\n", \
		       __FILE__, __LINE__, __FUNCTION__);		\
		exit(1);						\
	} while (0)

#endif /* __COMMON_H */
