# 
# Master Makefile for testfs
# Edited for ECE344 - Lab 6
#
# Author: Kuei Sun, Ashvin Goel
# E-mail: kuei.sun@mail.utoronto.ca
#         ashvin@eecg.toronto.edu
#
# University of Toronto
# 2014

PROGS := testfs mktestfs
COMMON_OBJECTS := bitmap.o block.o super.o inode.o read_write.o dir.o file.o common.o
COMMON_SOURCES := $(COMMON_OBJECTS:.o=.c)
INCLUDES := 
LOADLIBS := -lpopt
#CFLAGS := -O2 -Wall -Wextra -Werror $(DEFINES) $(INCLUDES)
CFLAGS := -g -Wall -Wextra -Werror -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 $(DEFINES) $(INCLUDES)
SOURCES := testfs.c mktestfs.c $(COMMON_SOURCES)

all: depend $(PROGS)

testfs: testfs.o $(COMMON_OBJECTS)
	$(CC) -o $@ $(CFLAGS) $^ $(LOADLIBS) 

mktestfs: mktestfs.o $(COMMON_OBJECTS)
	$(CC) -o $@ $(CFLAGS) $^ $(LOADLIBS)     

.PHONY: zip clean

depend:
	$(CC) -MM $(INCLUDES) $(SOURCES) > depend.mk

clean:
	rm -f *.o depend.mk $(PROGS) *.exe *.stackdump *~

realclean: clean
	rm -f *.img *.log *.out TAGS

ifeq (depend.mk,$(wildcard depend.mk))
include depend.mk
endif
