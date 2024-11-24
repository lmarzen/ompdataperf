CC = clang
CFLAGS = -fopenmp -fopenmp-targets=nvptx64
ifeq ($(DEBUG), 1)
  CFLAGS += -DDEBUG -g3 -O0
else
  CFLAGS += -DNDEBUG -O2
  DEBUG := 0
endif

all: torture torture_fix

torture: torture.c
	$(CC) $(CFLAGS) torture.c -o torture

torture_fix: torture_fix.c
	$(CC) $(CFLAGS) torture_fix.c -o torture_fix

clean:
	rm -f torture torture_fix
