#include <assert.h>
#include <stdlib.h>
#include "thread.h"
#include "interrupt.h"
#include "test_thread.h"

static void
test(int enabled)
{
	int i;
	int is_enabled;

	for (i = 0; i < 16; i++) {
		spin(SIG_INTERVAL / 5);	/* spin for a short period */
		unintr_printf(".");
		fflush(stdout);
		/* check whether interrupts are enabled or not correctly */
		is_enabled = interrupts_enabled();
		assert(enabled == is_enabled);
	}
	unintr_printf("\n");
}

int
main(int argc, char **argv)
{
	int enabled;

	thread_init();
	/* show interrupt handler output */
	register_interrupt_handler(1);
	test(1);

	/*  test interrupts_on/off/set */
	enabled = interrupts_off();
	assert(enabled);
	test(0);
	enabled = interrupts_off();
	assert(!enabled);
	test(0);
	enabled = interrupts_set(1);
	assert(!enabled);
	test(1);
	enabled = interrupts_set(1);
	assert(enabled);
	test(1);
	exit(0);
}
