CFLAGS := -g -Wall -Werror
LOADLIBES := -lm
TARGETS := hi hello words fact test_point test_sorted_points test_wc

# Make sure that 'all' is the first target
all: depend $(TARGETS)

clean:
	rm -rf core *.o $(TARGETS)

realclean: clean
	rm -rf *~ *.bak .depend *.log *.out

tags:
	etags *.c *.h

test_point: point.o

test_sorted_points: point.o sorted_points.o

test_wc: wc.o

depend:
	$(CC) -MM *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
