#include "chunk.h"
#include "kernel_interface.h"

// Invokes the store energy kernel
void store_energy_driver(Chunk *chunks, Settings &settings) {
  for (int cc = 0; cc < settings.num_chunks_per_rank; ++cc) {
    if (settings.kernel_language == Kernel_Language::C) {
      run_store_energy(&(chunks[cc]), settings);
    } else if (settings.kernel_language == Kernel_Language::FORTRAN) {
      // Fortran store energy kernel
    }
  }
}
