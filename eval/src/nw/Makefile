# C compiler
CC = clang++
#CC_FLAGS = -O3 -fopenmp
CC_FLAGS = -g3 -O0 -fopenmp
OFFLOAD_CC_FLAGS = -fopenmp-targets=nvptx64

all: needle needle_offload

needle: 
	$(CC) $(CC_FLAGS) needle.cpp -o needle 

needle_offload:
	$(CC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -DOMP_OFFLOAD needle.cpp -o needle_offload

clean:
	rm -f needle needle_offload
