# C compiler
CC = clang++
ICC = clang++
CC_FLAGS = -fopenmp -fopenmp-targets=nvptx64
#CC_FLAGS = -g3 -fopenmp -fopenmp-targets=nvptx64 -O0
ifeq ($(DEBUG), 1)
  CC_FLAGS += -DDEBUG -g3 -O0
else
  CC_FLAGS += -DNDEBUG -O2
  DEBUG := 0
endif

all: bfs bfs_offload bfs_offload_fix

bfs: bfs.cpp 
	$(CC) $(CC_FLAGS) bfs.cpp -o bfs 

bfs_offload: bfs.cpp
	$(ICC) $(CC_FLAGS) -DOMP_OFFLOAD bfs.cpp -o bfs_offload

bfs_offload_fix: bfs_fix.cpp
	$(ICC) $(CC_FLAGS) -DOMP_OFFLOAD bfs_fix.cpp -o bfs_offload_fix

clean:
	rm -f bfs bfs_offload bfs_offload_fix result.txt
