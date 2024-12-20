COMPILER ?= CLANG
ARCH ?= native
MODEL = omptarget

EXES=$(addprefix fmm., $(MODEL))
SINGLE=$(addsuffix .single, $(EXES))
DOUBLE=$(addsuffix .double, $(EXES))

default: fmm.$(MODEL)

ifeq ($(DEBUG), 1)
  include $(MODEL)/debug_flags.makefile
else
  include $(MODEL)/flags.makefile
endif

.PHONY: default clean

COMMON_HEADERS=$(wildcard common/*.hh)
COMMON_INC=-I./common

main_${MODEL}.o: main.cc $(MODEL)/*.hh $(COMMON_HEADERS)
	$(CC) $(CFLAGS) $(EXTRA_FLAGS) -I./$(MODEL) $(COMMON_INC) -o main_${MODEL}.o main.cc -c

fmm.$(MODEL): main_${MODEL}.o
	$(CC) main_${MODEL}.o -o $@ $(LIBS) 

#fmm.%.single: main.cc %/*.hh $(COMMON_HEADERS)
#	$(CC) $(CFLAGS) -I $* $(COMMON_INC) main.cc -o $@ $(LIBS)
#
#fmm.%.double: main.cc %/*.hh $(COMMON_HEADERS)
#	$(CC) $(CFLAGS) -DFMM_DOUBLE -I $* $(COMMON_INC) main.cc -o $@ $(LIBS)

clean:
	-rm -f fmm.* main_${MODEL}.o

