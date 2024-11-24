#include "chunk.h"
#include "kernel_interface.h"

// Invokes the set chunk state kernel
void set_chunk_state_driver(Chunk *chunks, Settings &settings, State *states) {
  // Issue kernel to all local chunks
  for (int cc = 0; cc < settings.num_chunks_per_rank; ++cc) {
    if (settings.kernel_language == Kernel_Language::C) {
      run_set_chunk_state(&(chunks[cc]), settings, states);
    } else if (settings.kernel_language == Kernel_Language::FORTRAN) {
      // Fortran store energy kernel
    }
  }
}
