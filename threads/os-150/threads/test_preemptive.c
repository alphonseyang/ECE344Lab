#include "thread.h"
#include "interrupt.h"
#include "test_thread.h"

int
main(int argc, char **argv)
{
	thread_init();
	/* show interrupt handler output, we will turn it off later */
	register_interrupt_handler(1);
	/* Test preemptive threads */
	test_preemptive();
	return 0;
}
