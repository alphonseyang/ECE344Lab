#ifndef _POINT_H_
#define _POINT_H_

/* DO NOT CHANGE THIS FILE */

/* simplistic definition of point and operations on a point for 2D
 * double-precision space. */
struct point {
	double x;
	double y;
};

/* You will be writing these three functions */

/* update *p by increasing p->x by x and p->y by y */
void point_translate(struct point *p, double x, double y);

/* return the cartesian distance between p1 and p2 */
double point_distance(const struct point *p1, const struct point *p2);

/* this function compares the Euclidean lengths of p1 and p2. The Euclidean
 * length of a point is the distance of the point from the origin (0, 0). The
 * function should return -1, 0, or 1, depending on whether p1 has smaller
 * length, equal length, or larger length, than p2. */
int point_compare(const struct point *p1, const struct point *p2);

/* the rest are inlined functions */

static inline double
point_X(const struct point *p)
{
	return p->x;
}

static inline double
point_Y(const struct point *p)
{
	return p->y;
}

static inline struct point *
point_set(struct point *p, double x, double y)
{
	p->x = x;
	p->y = y;
	return p;
}

#endif /* _POINT_H_ */
