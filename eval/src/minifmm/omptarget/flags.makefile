CC=clang++

ifeq ($(TARGET), GPU)
CFLAGS=-O3 -ffast-math -mllvm --nvptx-f32ftz -fopenmp -fopenmp-targets=nvptx64-nvidia-cuda -Xopenmp-target -march=$(ARCH) 
LIBS=-fopenmp -fopenmp-targets=nvptx64-nvidia-cuda -Xopenmp-target -march=$(ARCH)
else 
CFLAGS=-O3 -ffast-math -fopenmp -fopenmp-targets=x86_64 -march=$(ARCH) 
LIBS=-fopenmp -fopenmp-targets=x86_64 -march=$(ARCH) 
endif
