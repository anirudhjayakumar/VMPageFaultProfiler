## Compile Makefile for Kernel Module ##
EXTRA_CFLAGS = -DDEBUG_OUTPUT
APP_EXTRA_FLAGS:= -O2 -ansi -pedantic -D__DEBUG__
KERNEL_SRC:= /lib/modules/$(shell uname -r)/build
SUBDIR= $(PWD)
GCC:=gcc
RM:=rm

.PHONY : clean

all: clean modules monitor worker

obj-m += mp3_final.o
mp3_final-objs := linklist.o mem.o char_dev.o workqueu.o mp3.o

modules:
	$(MAKE) -C $(KERNEL_SRC) M=$(SUBDIR)  modules

monitor: monitor.c
	$(GCC) -o $@ $^ -g -O0

worker: work.c
	$(GCC) -o $@ $^ -g -O0

#my_factorial: my_factorial.c my_factorial.h
#	$(GCC) -o $@ $^ -g -O0
#
#factorial: factorial.c
#	$(GCC) -o $@ $^ -g -O0
#
#test: test.c
#	$(GCC) -o $@ $^ -g -O0
#
#hi_priority: hi_priority.c
#	$(GCC) -o $@ $^ -g -O0
#	
#visual: visual_proc.c
#	$(GCC) -o $@ $^ -g -O0

clean:
	$(RM) -f monitor worker *~ *.ko *.o *.mod.c Module.symvers modules.order
