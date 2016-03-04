#include "thread.h"
#include "interrupt.h"
#include "test_thread.h"

int
main(int argc, char **argv)
{
	thread_init();
	register_interrupt_handler(0);
	/* Test locking */
	test_lock();
	return 0;
}
