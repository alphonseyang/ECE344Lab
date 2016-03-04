CFLAGS := -g -Wall -Werror -D_GNU_SOURCE

TARGETS := show_ucontext show_handler test_basic test_preemptive test_wakeup test_wakeup_all test_lock test_cv_signal test_cv_broadcast

# Make sure that 'all' is the first target
all: depend $(TARGETS)

clean:
	rm -rf core *.o $(TARGETS)

realclean: clean
	rm -rf *~ *.bak .depend *.log *.out

tags:
	etags *.c *.h

OBJS := test_thread.o thread.o interrupt.o

show_ucontext show_handler test_basic test_preemptive test_wakeup test_wakeup_all test_lock test_cv_signal test_cv_broadcast: $(OBJS)

depend:
	$(CC) -MM *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
