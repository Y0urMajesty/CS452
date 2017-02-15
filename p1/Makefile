# 1/27/17
# This is a sample Makefile for Phase 1. It provides the following targets 
# (assumes test source files are named testN.c)
# You may change this, e.g. to add new test cases, but keep in mind we will compile
# your phase1.c using our own copy of this file.
#
#	make		(makes libphase1.a and all tests)
#        make phase1     ditto
#
#	make testN 	(makes testN)
#	make testN.out	(runs testN and puts output in testN.out)
#	make tests	(makes all testN.out files, i.e. runs all tests)
#	make tests_bg	(runs all tests in the background)
#
#	make testN.v	(runs valgrind on testN and puts output in testN.v)
#	make valgrind	(makes all testN.v files, i.e. runs valgrind on all tests)
#
#	make clean	(removes all files created by this Makefile)

ifndef PREFIX
	PREFIX = $(HOME)
endif

# Set to version of USLOSS you want to use.
USLOSS_VERSION = 3.0

# All .c files in this directory are included in the Phase 1 library.
SRCS = $(wildcard *.c)

# Each .c file in the tests subdirectory is a separate test case.
TESTS = $(patsubst %.c,%,$(wildcard tests/*.c))

# Change this if you want to change the arguments to valgrind.
VGFLAGS = --track-origins=yes --leak-check=full --max-stackframe=100000

# Change this if you need to link against additional libraries (probably not).
LIBS = -lusloss$(USLOSS_VERSION) -lphase1

# Change this if you want change which flags are passed to the C compiler.
CFLAGS += -Wall -g -std=gnu99
CFLAGS += -DDEBUG

# You shouldn't need to change anything below here. 

TARGET = libphase1.a

INCLUDES = -I$(PREFIX)/include -I.

ifeq ($(shell uname),Darwin)
	DEFINES += -D_XOPEN_SOURCE
	OS = macosx
	CFLAGS += -Wno-int-to-void-pointer-cast -Wno-extra-tokens -Wno-unused-label -Wno-unused-function
else
	OS = linux
	CFLAGS += -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -Wno-unused-but-set-variable -Wno-unused-function
endif

CC=gcc
LD=gcc
AR=ar    
CFLAGS += $(INCLUDES) $(DEFINES) -Werror
LDFLAGS = -L$(PREFIX)/lib -L.
COBJS = ${SRCS:.c=.o}
DEPS = ${COBJS:.o=.d}
TSRCS = {$TESTS:=.c}
TOBJS = ${TESTS:=.o}
TOUTS = ${TESTS:=.out}
TVS = ${TESTS:=.v}
STUBS = p3/p3stubs.o

# The following is to deal with circular dependencies between the USLOSS and phase1
# libraries. Unfortunately the linkers handle this differently on the two OSes.

ifeq ($(OS), macosx)
	LIBFLAGS = -Wl,-all_load $(LIBS)
else
	LIBFLAGS = -Wl,--start-group $(LIBS) -Wl,--end-group
endif

%.d: %.c
	$(CC) -c $(CFLAGS) -MM -MF $@ $<

all: phase1

phase1: $(TARGET) $(TESTS)


$(TARGET):  $(COBJS)
	$(AR) -r $@ $^

tests: $(TOUTS)

# Remove implicit rules so that "make phase1" doesn't try to build it from phase1.c or phase1.o
% : %.c

% : %.o

%.out: %
	./$< 1> $@ 2>&1

$(TESTS):   %: $(TARGET) %.o $(STUBS)
	$(LD) $(LDFLAGS) -o $@ $@.o $(STUBS) $(LIBFLAGS)

clean:
	rm -f $(COBJS) $(TARGET) $(TOBJS) $(TESTS) $(DEPS) $(TVS) p3/*.o p3/p3stubsTest *.out tests/*.out

%.d: %.c
	$(CC) -c $(CFLAGS) -MM -MF $@ $<

valgrind: $(TVS)

%.v: %
	valgrind $(VGFLAGS) ./$< 1> $@ 2>&1

p3/p3stubsTest: $(STUBS) p3/p3stubsTest.o
	$(LD) $(LDFLAGS) -o $@ $^


-include $(DEPS)
