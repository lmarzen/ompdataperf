#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <omp.h>

void usage(int argc, char **argv) {
  fprintf(stderr, "Usage: %s <num_iterations> <allocation_size>\n", argv[0]);
}

int main(int argc, char **argv) {
  if (argc != 3) {
    usage(argc, argv);
    exit(1);
  }

  const long long num_iterations = atoll(argv[1]);
  const long long allocation_size = atoll(argv[2]);

  int8_t *a = (int8_t *)calloc(allocation_size, sizeof(*a));

  double start_time = omp_get_wtime();

  for (long long iter = 0; iter < num_iterations; ++iter) {

    #pragma omp target data map(tofrom:a[0:allocation_size]) // duplicate (a), repeated alloc, unused data transfer
    {
      #pragma omp target update to(a[0:allocation_size]) // duplicate (b), round-trip (a)
      #pragma omp target teams distribute parallel for
      for (long long i = 0; i < allocation_size; ++i) {
        a[i] += 1;
      }
    } // end map(tofrom:a[0:allocation_size]) // round-trip (b)
    size_t i = iter % allocation_size; // choosing an index to avoid more repeated allocs
    #pragma omp target data map(alloc:a[i:i+1]) // unused alloc
    {
    }

  }

  double end_time = omp_get_wtime();
  printf("Elapsed time: %f seconds\n", end_time - start_time);

  free(a);
  return 0;
}
