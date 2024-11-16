.PHONY: all clean 
.PHONY: bfs cfd hotspot lud miniFE nw RSBench XSBench
.PHONY: bfs_clean cfd_clean hotspot_clean lud_clean miniFE_clean nw_clean RSBench_clean XSBench_clean

all: bfs cfd hotspot lud miniFE nw RSBench XSBench
clean: bfs_clean cfd_clean hotspot_clean lud_clean miniFE_clean nw_clean RSBench_clean XSBench_clean

ifeq ($(DEBUG), 1)
else
  DEBUG := 0
endif

bfs:
	$(MAKE) -j -C src/bfs -f Makefile all DEBUG=$(DEBUG)

cfd:
	$(MAKE) -j -C src/cfd -f makefile all DEBUG=$(DEBUG)

hotspot:
	$(MAKE) -j -C src/hotspot -f Makefile all DEBUG=$(DEBUG)

lud:
	$(MAKE) -j -C src/lud -f Makefile all DEBUG=$(DEBUG)

miniFE:
	$(MAKE) -j -C src/miniFE/src-openmp45-opt -f Makefile all DEBUG=$(DEBUG)
	$(MAKE) -j -C src/miniFE/src-openmp45-opt-fix -f Makefile all DEBUG=$(DEBUG)

nw:
	$(MAKE) -j -C src/nw -f Makefile all DEBUG=$(DEBUG)

RSBench:
	$(MAKE) -j -C src/RSBench -f Makefile DEBUG=$(DEBUG)
	$(MAKE) -j -C src/RSBench -f Makefile.fix DEBUG=$(DEBUG)

XSBench:
	$(MAKE) -j -C src/XSBench -f Makefile DEBUG=$(DEBUG)
	$(MAKE) -j -C src/XSBench -f Makefile.fix DEBUG=$(DEBUG)

bfs_clean:
	$(MAKE) -j -C src/bfs -f Makefile clean DEBUG=$(DEBUG)

cfd_clean:
	$(MAKE) -j -C src/cfd -f makefile clean DEBUG=$(DEBUG)

hotspot_clean:
	$(MAKE) -j -C src/hotspot -f Makefile clean DEBUG=$(DEBUG)

lud_clean:
	$(MAKE) -j -C src/lud -f Makefile clean DEBUG=$(DEBUG)

miniFE_clean:
	$(MAKE) -j -C src/miniFE/src-openmp45-opt -f Makefile clean DEBUG=$(DEBUG)
	$(MAKE) -j -C src/miniFE/src-openmp45-opt-fix -f Makefile clean DEBUG=$(DEBUG)

nw_clean:
	$(MAKE) -j -C src/nw -f Makefile clean DEBUG=$(DEBUG)

RSBench_clean:
	$(MAKE) -j -C src/RSBench -f Makefile clean DEBUG=$(DEBUG)
	$(MAKE) -j -C src/RSBench -f Makefile.fix clean DEBUG=$(DEBUG)

XSBench_clean:
	$(MAKE) -j -C src/XSBench -f Makefile clean DEBUG=$(DEBUG)
	$(MAKE) -j -C src/XSBench -f Makefile.fix clean DEBUG=$(DEBUG)

