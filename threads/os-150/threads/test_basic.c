#include "thread.h"
#include "test_thread.h"

int
main(int argc, char **argv)
{
	thread_init();
	test_basic();
	return 0;
}
