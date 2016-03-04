#include "thread.h"
#include "interrupt.h"
#include "test_thread.h"

int
main(int argc, char **argv)
{
	thread_init();
	register_interrupt_handler(0);
	/* Test sleep and wakeup */
	test_wakeup(0);
	return 0;
}
