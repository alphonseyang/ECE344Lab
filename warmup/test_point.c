#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "point.h"

int
main(int argc, char **argv)
{
	struct point p1, *p2;

	point_set(&p1, 1.0, 1.0);

	p2 = malloc(sizeof(struct point));
	assert(p2);

	point_set(p2, 1.0, 1.0);
	assert(point_distance(&p1, p2) == 0.0);

	point_translate(&p1, 1.0, 0.0);
	assert(point_distance(&p1, p2) == 1.0);

	point_set(&p1, 0.0, 0.0);
	point_set(p2, 3.0, 4.0);
	assert(point_distance(&p1, p2) == 5.0);


	assert(point_compare(&p1, p2) < 0);

	point_set(&p1, 1, 2);
	point_set(p2, 2, 1);
	assert(point_compare(&p1, p2) == 0);
	
	point_set(&p1, 4, 2);
	point_set(p2, 3, 3);
	assert(point_compare(&p1, p2) > 0);
	
	free(p2);
	p2 = NULL;

	printf("OK\n");
	return 0;
}
