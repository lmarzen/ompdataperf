CC = clang
CFLAGS = -fopenmp -fopenmp-targets=nvptx64
ifeq ($(DEBUG), 1)
  CFLAGS += -DDEBUG -g3 -O0
else
  CFLAGS += -DNDEBUG -O2
  DEBUG := 0
endif

all: null

null: null.c
	$(CC) $(CFLAGS) null.c -o null

clean:
	rm -f null
