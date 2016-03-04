#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

#include <stdio.h>
#include <signal.h>

#define TBD() do {							\
		printf("%s:%d: %s: please implement this functionality\n", \
		       __FILE__, __LINE__, __FUNCTION__);		\
		exit(1);						\
	} while (0)

/* we will use this signal type for delivering "interrupts". */
#define SIG_TYPE SIGALRM
/* the interrupt will be delivered every 200 usec */
#define SIG_INTERVAL 200

void register_interrupt_handler(int verbose);
int interrupts_on(void);
int interrupts_off(void);
int interrupts_set(int enabled);
int interrupts_enabled();
void interrupts_quiet();

void spin(int msecs);
/* turn off interrupts while printing */
int unintr_printf(const char *fmt, ...);
#endif
