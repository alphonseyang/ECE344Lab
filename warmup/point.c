#include <assert.h>
#include "common.h"
#include "point.h"
#include <math.h>

void
point_translate(struct point *p, double x, double y)
{
	p->x = p->x + x;
	p->y = p->y + y;
}

double
point_distance(const struct point *p1, const struct point *p2)
{
	double distance;
	distance = sqrt((p1->x - p2->x)*(p1->x - p2->x) + (p1->y - p2->y)*(p1->y - p2->y));
	return distance;
}

int
point_compare(const struct point *p1, const struct point *p2)
{
	double distance1, distance2;
	distance1 = sqrt((p1->x)*(p1->x) + (p1->y)*(p1->y));
	distance2 = sqrt((p2->x)*(p2->x) + (p2->y)*(p2->y));
	if(distance1 > distance2)
		return 1;
	else if(distance1 < distance2)
		return -1;
	else 
		return 0;
}
