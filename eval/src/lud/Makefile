all: lud_omp lud_omp_offload

ifeq ($(DEBUG), 1)
else
  DEBUG := 0
endif

lud_omp:
	cd omp; make DEBUG=$(DEBUG)

lud_omp_offload:
	cd omp; make -f Makefile.offload DEBUG=$(DEBUG)

clean:
	cd omp; make clean ; make -f Makefile.offload clean
