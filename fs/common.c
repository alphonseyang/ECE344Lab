/*
 * common.c
 *
 * contains common functionalities shared across files
 *
 */

#include "testfs.h"

/* convert str to a pointer to an off_t */
int
str_to_offset(const char *str, off_t *offp)
{
	char *end;
	off_t val = strtoll(str, &end, 10);
	if (end != NULL && strlen(end) > 0) {
		return -EINVAL;
	}
	*offp = val;
	return 0;
}

/* convert str to a pointer to a size_t */
int
str_to_size(const char *str, size_t *sizep)
{
	char *end;
	size_t val = strtoull(str, &end, 10);
	if (end != NULL && strlen(end) > 0) {
		return -EINVAL;
	}
	*sizep = val;
	return 0;
}
