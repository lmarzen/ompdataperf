# C compiler
CC = clang++
ICC = clang++
CC_FLAGS = -fopenmp -fopenmp-targets=nvptx64 -O2
#CC_FLAGS = -g3 -fopenmp -fopenmp-targets=nvptx64 -O0

all: bfs bfs_offload bfs_offload_fix

bfs: bfs.cpp 
	$(CC) $(CC_FLAGS) bfs.cpp -o bfs 

bfs_offload: bfs.cpp
	$(ICC) $(CC_FLAGS) -DOMP_OFFLOAD bfs.cpp -o bfs_offload

bfs_offload_fix: bfs_fix.cpp
	$(ICC) $(CC_FLAGS) -DOMP_OFFLOAD bfs_fix.cpp -o bfs_offload_fix

clean:
	rm -f bfs bfs_offload bfs_offload_fix result.txt
