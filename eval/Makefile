.PHONY: all clean 
.PHONY: BabelStream bfs hotspot lud miniFE minifmm nw RSBench TeaLeaf XSBench
.PHONY: BabelStream_clean bfs_clean hotspot_clean lud_clean miniFE_clean minifmm_clean nw_clean RSBench_clean TeaLeaf_clean XSBench_clean
.PHONY: torture torture_clean null null_clean

all: BabelStream bfs hotspot lud miniFE minifmm nw RSBench TeaLeaf XSBench torture null
clean: BabelStream_clean bfs_clean hotspot_clean lud_clean miniFE_clean minifmm_clean nw_clean RSBench_clean TeaLeaf_clean XSBench_clean torture_clean null_clean

ifeq ($(DEBUG), 1)
  CMAKE_BUILD_TYPE=Debug
else
  CMAKE_BUILD_TYPE=Release
  DEBUG := 0
endif

BabelStream:
	cmake -Bsrc/BabelStream/build -Hsrc/BabelStream/. -DCMAKE_CXX_COMPILER=clang++ -DMODEL=omp -DOFFLOAD=ON -DOFFLOAD_FLAGS='-fopenmp -fopenmp-targets=nvptx64' -DCMAKE_BUILD_TYPE='${CMAKE_BUILD_TYPE}'
	cmake -Bsrc/BabelStream/build_syn -Hsrc/BabelStream/. -DCMAKE_CXX_COMPILER=clang++ -DMODEL=omp_syn -DOFFLOAD=ON -DOFFLOAD_FLAGS='-fopenmp -fopenmp-targets=nvptx64' -DCMAKE_BUILD_TYPE='${CMAKE_BUILD_TYPE}'
	cmake --build src/BabelStream/build -- -j
	cmake --build src/BabelStream/build_syn -- -j

bfs:
	$(MAKE) -j -C src/bfs -f Makefile all DEBUG=$(DEBUG)

hotspot:
	$(MAKE) -j -C src/hotspot -f Makefile all DEBUG=$(DEBUG)

lud:
	$(MAKE) -j -C src/lud -f Makefile all DEBUG=$(DEBUG)

miniFE:
	$(MAKE) -j -C src/miniFE/src-openmp45-opt -f Makefile all DEBUG=$(DEBUG)
	$(MAKE) -j -C src/miniFE/src-openmp45-opt-fix -f Makefile all DEBUG=$(DEBUG)

minifmm:
	$(MAKE) -j -C src/minifmm -f Makefile DEBUG=$(DEBUG)
	$(MAKE) -j -C src/minifmm -f Makefile.syn DEBUG=$(DEBUG)

nw:
	$(MAKE) -j -C src/nw -f Makefile all DEBUG=$(DEBUG)

RSBench:
	$(MAKE) -j -C src/RSBench -f Makefile DEBUG=$(DEBUG)
	$(MAKE) -j -C src/RSBench -f Makefile.fix DEBUG=$(DEBUG)

TeaLeaf:
	cmake -Bsrc/TeaLeaf/build -Hsrc/TeaLeaf/. -DMODEL=omp-target -DENABLE_MPI=OFF -DOFFLOAD=ON -DOFFLOAD_FLAGS="-fopenmp -fopenmp-targets=nvptx64" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE='${CMAKE_BUILD_TYPE}'
	cmake -Bsrc/TeaLeaf/build_syn -Hsrc/TeaLeaf/. -DMODEL=omp-target_syn -DENABLE_MPI=OFF -DOFFLOAD=ON -DOFFLOAD_FLAGS="-fopenmp -fopenmp-targets=nvptx64" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE='${CMAKE_BUILD_TYPE}'
	cmake --build src/TeaLeaf/build -- -j
	cmake --build src/TeaLeaf/build_syn -- -j

XSBench:
	$(MAKE) -j -C src/XSBench -f Makefile DEBUG=$(DEBUG)
	$(MAKE) -j -C src/XSBench -f Makefile.fix DEBUG=$(DEBUG)

BabelStream_clean:
	rm -rf src/BabelStream/build
	rm -rf src/BabelStream/build_syn

bfs_clean:
	$(MAKE) -j -C src/bfs -f Makefile clean DEBUG=$(DEBUG)

hotspot_clean:
	$(MAKE) -j -C src/hotspot -f Makefile clean DEBUG=$(DEBUG)

lud_clean:
	$(MAKE) -j -C src/lud -f Makefile clean DEBUG=$(DEBUG)

miniFE_clean:
	$(MAKE) -j -C src/miniFE/src-openmp45-opt -f Makefile realclean DEBUG=$(DEBUG)
	$(MAKE) -j -C src/miniFE/src-openmp45-opt-fix -f Makefile realclean DEBUG=$(DEBUG)

minifmm_clean:
	$(MAKE) -j -C src/minifmm -f Makefile clean DEBUG=$(DEBUG)

nw_clean:
	$(MAKE) -j -C src/nw -f Makefile clean DEBUG=$(DEBUG)

RSBench_clean:
	$(MAKE) -j -C src/RSBench -f Makefile clean DEBUG=$(DEBUG)
	$(MAKE) -j -C src/RSBench -f Makefile.fix clean DEBUG=$(DEBUG)

TeaLeaf_clean:
	rm -rf src/TeaLeaf/build
	rm -rf src/TeaLeaf/build_syn

XSBench_clean:
	$(MAKE) -j -C src/XSBench -f Makefile clean DEBUG=$(DEBUG)
	$(MAKE) -j -C src/XSBench -f Makefile.fix clean DEBUG=$(DEBUG)

torture:
	$(MAKE) -j -C src/torture -f Makefile torture all DEBUG=$(DEBUG)

torture_clean:
	$(MAKE) -j -C src/torture -f Makefile clean DEBUG=$(DEBUG)

null:
	$(MAKE) -j -C src/null -f Makefile all DEBUG=$(DEBUG)

null_clean:
	$(MAKE) -j -C src/null -f Makefile clean DEBUG=$(DEBUG)
