# C compiler
CC = clang++
CC_FLAGS = -fopenmp -Wno-unused-comparison
OFFLOAD_CC_FLAGS = -fopenmp-targets=nvptx64
ifeq ($(DEBUG), 1)
  CC_FLAGS += -DDEBUG -g3 -O0
else
  CC_FLAGS += -DNDEBUG -O3
  DEBUG := 0
endif

all: needle needle_offload needle_offload_syn

needle: 
	$(CC) $(CC_FLAGS) needle.cpp -o needle 

needle_offload:
	$(CC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -DOMP_OFFLOAD needle.cpp -o needle_offload

needle_offload_syn:
	$(CC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -DOMP_OFFLOAD needle_syn.cpp -o needle_offload_syn

clean:
	rm -f needle needle_offload needle_offload_syn result.txt
