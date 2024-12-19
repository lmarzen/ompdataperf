CC=clang++

ifeq ($(TARGET), GPU)
CFLAGS=-g3 -O0 -mllvm --nvptx-f32ftz -fopenmp -fopenmp-targets=nvptx64-nvidia-cuda -Xopenmp-target -march=$(ARCH) 
LIBS=-fopenmp -fopenmp-targets=nvptx64-nvidia-cuda -Xopenmp-target -march=$(ARCH)
else 
CFLAGS=-g3 -O0 -fopenmp -fopenmp-targets=x86_64 -march=$(ARCH) 
LIBS=-fopenmp -fopenmp-targets=x86_64 -march=$(ARCH) 
endif
