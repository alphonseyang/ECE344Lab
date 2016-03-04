#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include "point.h"
#include "sorted_points.h"

static void basic_test();
static void stress_test();
static void random_test();

int
main(int argc, char **argv)
{
	struct mallinfo end_info;

	srand(0);
	basic_test();
	stress_test();
	random_test();
	end_info = mallinfo();

	assert(end_info.uordblks == 0);
	assert(end_info.hblks == 0);

	printf("OK\n");
	return 0;
}

static struct sorted_points *
alloc_random_list(int num)
{
	struct sorted_points *sp = sp_init();
	int i;
	int ret;

	assert(sp);
	for (i = 0; i < num; i++) {
		ret = sp_add_point(sp, (double)(rand() % 10),
				   (double)(rand() % 10));
		assert(ret);
	}
	
	return sp;
}

static void
random_test()
{
	struct sorted_points *sp;
	static const int NRUNS = 512;
	static const int MAXSIZE = 1024;
	int i;
	int ret;


	printf("this test may also take a minute.\n");

	for (i = 0; i < NRUNS; i++) {
		struct point p1;
		int size = (int)(rand() % MAXSIZE);

		sp = alloc_random_list(size);		
		ret = sp_delete_duplicates(sp);
		assert(ret >= 0);
		size -= ret;
		while (size > 0) {
			switch (rand() % 3) {
			case 0:
				ret = sp_remove_by_index(sp, 
							 (int)(rand() % size),
							 &p1);
				assert(ret);
				size--;
				break;
			case 1:
				ret = sp_remove_first(sp, &p1);
				assert(ret);
				size--;
				break;
			case 2:
				ret = sp_remove_last(sp, &p1);
				assert(ret);
				size--;
				break;
			default:
				assert(0);
			}
		}
		sp_destroy(sp);
	}
}

static void
basic_test()
{
	int ret;
	struct sorted_points *sp1;
	struct sorted_points *sp2;
	struct point p1;

	srand(0);
	sp1 = sp_init();
	sp2 = sp_init();

	// empty list checks
	ret = sp_remove_by_index(sp1, 1, &p1);
	assert(!ret);
	ret = sp_remove_by_index(sp1, 0, &p1);
	assert(!ret);
	ret = sp_remove_by_index(sp1, -1, &p1);
	assert(!ret);
	ret = sp_remove_first(sp1, &p1);
	assert(!ret);
	ret = sp_remove_last(sp1, &p1);
	assert(!ret);

	ret = sp_add_point(sp1, 1.0, 1.0);
	assert(ret);
	ret = sp_add_point(sp1, 1.0, 1.0);
	assert(ret);
	ret = sp_add_point(sp1, 1.0, 1.0);
	assert(ret);

	ret = sp_add_point(sp2, 3.0, 1.0);
	assert(ret);
	ret = sp_add_point(sp2, 2.0, 1.0);
	assert(ret);
	ret = sp_add_point(sp2, 1.0, 1.0);
	assert(ret);

	ret = sp_add_point(sp1, 2.0, 2.0);
	assert(ret);
	ret = sp_add_point(sp1, 2.0, 2.0);
	assert(ret);
	ret = sp_add_point(sp1, 4.0, 3.0);
	assert(ret);
	ret = sp_add_point(sp1, 2.0, 2.0);
	assert(ret);

	ret = sp_remove_last(sp1, &p1);
	assert(ret);
	assert(point_X(&p1) == 4.0);
	assert(point_Y(&p1) == 3.0);

	ret = sp_remove_first(sp1, &p1);
	assert(ret);
	assert(point_X(&p1) == 1.0);
	assert(point_Y(&p1) == 1.0);

	ret = sp_remove_by_index(sp1, 2, &p1);
	assert(ret);
	assert(point_X(&p1) == 2.0);
	assert(point_Y(&p1) == 2.0);

	ret = sp_add_point(sp1, 2.0, 2.0);
	assert(ret);
	ret = sp_add_point(sp1, 2.0, 2.0);
	assert(ret);
	ret = sp_add_point(sp1, 2.0, 2.0);
	assert(ret);

	ret = sp_remove_first(sp1, &p1);
	assert(ret);
	assert(point_X(&p1) == 1.0);
	assert(point_Y(&p1) == 1.0);
	
	ret = sp_delete_duplicates(sp1);
	assert(ret == 4);

	ret = sp_remove_first(sp1, &p1);
	assert(ret);
	assert(point_X(&p1) == 1.0);
	assert(point_Y(&p1) == 1.0);

	ret = sp_remove_first(sp1, &p1);
	assert(ret);
	assert(point_X(&p1) == 2.0);
	assert(point_Y(&p1) == 2.0);

	ret = sp_remove_first(sp1, &p1);
	assert(!ret);

	ret = sp_delete_duplicates(sp2);
	assert(ret == 0);

	ret = sp_add_point(sp2, 5.0, 2.0);
	assert(ret);
	ret = sp_add_point(sp2, 6.0, 3.0);
	assert(ret);
	ret = sp_add_point(sp2, 7.0, 4.0);
	assert(ret);
	ret = sp_add_point(sp2, 0.0, 1.0);
	assert(ret);

	ret = sp_remove_by_index(sp2, 5, &p1);
	assert(ret);
	assert(point_X(&p1) == 6.0);
	assert(point_Y(&p1) == 3.0);
	
	ret = sp_remove_by_index(sp2, 5, &p1);
	assert(ret);
	assert(point_X(&p1) == 7.0);
	assert(point_Y(&p1) == 4.0);
	
	ret = sp_remove_by_index(sp2, 5, &p1);
	assert(!ret);

	ret = sp_remove_by_index(sp2, 0, &p1);
	assert(ret);
	assert(point_X(&p1) == 0.0);
	assert(point_Y(&p1) == 1.0);

	sp_destroy(sp1);
	sp_destroy(sp2);
}

/* rudimentary check for memory leaks */
static void
stress_test()
{
	struct sorted_points *sp;
	struct point dummy;
	static const int NINSERT = 1024;
	static const int NITER = 20 * 1024;
	int ii;
	int jj;
	int ret;

	sp = sp_init();
	printf("this test may take a minute.\n");

	for (ii = 0; ii < NITER; ii++) {	
		for (jj = 0; jj < NINSERT; jj++) {
			sp_add_point(sp, (double)(NINSERT - jj), 1.0);
		}
		ret = sp_remove_by_index(sp, NINSERT, &dummy);
		assert(0 == ret);
		ret = sp_remove_by_index(sp, NINSERT - 1, &dummy);
		assert(1 == ret);
		for (jj = 0; jj < NINSERT - 1; jj++) {
			ret = sp_remove_first(sp, &dummy);
			assert(1 == ret);
		}
		ret = sp_remove_first(sp, &dummy);
		assert(0 == ret);
	}
	sp_destroy(sp);
}
