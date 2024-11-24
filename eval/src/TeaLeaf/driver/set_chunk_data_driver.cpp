#include "chunk.h"
#include "kernel_interface.h"
#include "settings.h"

// Invokes the set chunk data kernel
void set_chunk_data_driver(Chunk *chunks, Settings &settings) {
  for (int cc = 0; cc < settings.num_chunks_per_rank; ++cc) {
    if (settings.kernel_language == Kernel_Language::C) {
      run_set_chunk_data(&(chunks[cc]), settings);
    } else if (settings.kernel_language == Kernel_Language::FORTRAN) {
      // Fortran store energy kernel
    }
  }
}
