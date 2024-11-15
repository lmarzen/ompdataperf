# C compiler
CC = clang++
CC_FLAGS = -fopenmp
ifeq ($(DEBUG), 1)
  CC_FLAGS += -DDEBUG -g3 -O0
else
  CC_FLAGS += -DNDEBUG -O2
  DEBUG := 0
endif
OFFLOAD_CC_FLAGS = -fopenmp-targets=nvptx64

all: hotspot hotspot_offload 

hotspot: hotspot_openmp.cpp Makefile 
	$(CC) $(CC_FLAGS) hotspot_openmp.cpp -o hotspot 

hotspot_offload: hotspot_openmp.cpp Makefile
	$(CC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -DOMP_OFFLOAD hotspot_openmp.cpp -o hotspot_offload

clean:
	rm -f hotspot hotspot_offload
