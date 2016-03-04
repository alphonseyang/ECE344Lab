#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdarg.h>
#include "thread.h"
#include "interrupt.h"

static void interrupt_handler(int sig, siginfo_t * sip, void *contextVP);
static void set_interrupt();
static void set_signal(sigset_t * setp);

static int loud = 0;

/* Should be called when you initialize threads package. Many of the calls won't
 * make sense at first -- study the man pages! */
void
register_interrupt_handler(int verbose)
{
	struct sigaction action;
	int error;
	static int init = 0;

	assert(!init);	/* should only register once */
	init = 1;
	loud = verbose;
	action.sa_handler = NULL;
	action.sa_sigaction = interrupt_handler;
	/* SIG_TYPE will be blocked while interrupt_handler() is running. */
	error = sigemptyset(&action.sa_mask);
	assert(!error);

	/* use sa_sigaction as handler instead of sa_handler */
	action.sa_flags = SA_SIGINFO;
	if (sigaction(SIG_TYPE, &action, NULL)) {
		perror("Setting up signal handler");
		assert(0);
	}
	set_interrupt();
}

/* enables interrupts. */
int
interrupts_on()
{
	return interrupts_set(1);
}

/* disables interrupts */
int
interrupts_off()
{
	return interrupts_set(0);
}

/* enables or disables interrupts, and returns whether interrupts were enabled
 * or not previously. */
int
interrupts_set(int enabled)
{
	int ret;
	sigset_t mask, omask;

	set_signal(&mask);
	if (enabled) {
		ret = sigprocmask(SIG_UNBLOCK, &mask, &omask);
	} else {
		ret = sigprocmask(SIG_BLOCK, &mask, &omask);
	}
	assert(!ret);
	return (sigismember(&omask, SIG_TYPE) ? 0 : 1);
}

int
interrupts_enabled()
{
	sigset_t mask;
	int ret;

	ret = sigprocmask(0, NULL, &mask);
	assert(!ret);
	return (sigismember(&mask, SIG_TYPE) ? 0 : 1);
}

void
interrupts_quiet()
{
	loud = 0;
}

void
spin(int usecs)
{
	struct timeval start, end, diff;
	int ret;

	ret = gettimeofday(&start, NULL);
	assert(!ret);
	while (1) {
		ret = gettimeofday(&end, NULL);
		timersub(&end, &start, &diff);

		if ((diff.tv_sec * 1000000 + diff.tv_usec) >= usecs) {
			break;
		}
	}
}

/* turn off interrupts while printing */
int
unintr_printf(const char *fmt, ...)
{
	int ret, enabled;
	va_list args;

	enabled = interrupts_off();
	va_start(args, fmt);
	ret = vprintf(fmt, args);
	va_end(args);
	interrupts_set(enabled);
	return ret;
}

/* static functions */

static void
set_signal(sigset_t * setp)
{
	int ret;
	ret = sigemptyset(setp);
	assert(!ret);
	ret = sigaddset(setp, SIG_TYPE);
	assert(!ret);
	return;
}

static int first = 1;
static struct timeval start, end, diff = { 0, 0 };

/*
 * STUB: once register_interrupt_handler() is called, this routine
 * gets called each time SIG_TYPE is sent to this process
 */
static void
interrupt_handler(int sig, siginfo_t * sip, void *contextVP)
{
	ucontext_t *context = (ucontext_t *) contextVP;

	/* check that SIG_TYPE is blocked on entry */
	assert(!interrupts_enabled());
	if (loud) {
		int ret;
		ret = gettimeofday(&end, NULL);
		assert(!ret);
		if (first) {
			first = 0;
		} else {
			timersub(&end, &start, &diff);
		}
		start = end;
		printf("%s: context at %10p, time diff = %ld us\n",
		       __FUNCTION__, context,
		       diff.tv_sec * 1000000 + diff.tv_usec);
	}
	set_interrupt();
	/* implement preemptive threading by calling thread_yield */
	thread_yield(THREAD_ANY);
}

/*
 * Use the setitimer() system call to set an alarm in the future. At that time,
 * this process will receive a SIGALRM signal.
 */
static void
set_interrupt()
{
	int ret;
	struct itimerval val;

	val.it_interval.tv_sec = 0;
	val.it_interval.tv_usec = 0;

	val.it_value.tv_sec = 0;
	val.it_value.tv_usec = SIG_INTERVAL;

	ret = setitimer(ITIMER_REAL, &val, NULL);
	assert(!ret);
}
