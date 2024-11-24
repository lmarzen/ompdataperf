#include "chunk.h"
#include "shared.h"

// Update left halo.
void update_left(const int x, const int y, const int halo_depth, const int depth, double *buffer, bool is_offload) {

#ifdef OMP_TARGET
  #pragma omp target teams distribute parallel for simd if (is_offload) collapse(2)
#else
  #pragma omp parallel for
#endif
  for (int jj = halo_depth; jj < y - halo_depth; ++jj) {
    for (int kk = 0; kk < depth; ++kk) {
      int base = jj * x;
      buffer[base + (halo_depth - kk - 1)] = buffer[base + (halo_depth + kk)];
    }
  }
}

// Update right halo.
void update_right(const int x, const int y, const int halo_depth, const int depth, double *buffer, bool is_offload) {
#ifdef OMP_TARGET
  #pragma omp target teams distribute parallel for simd if (is_offload) collapse(2)
#else
  #pragma omp parallel for
#endif
  for (int jj = halo_depth; jj < y - halo_depth; ++jj) {
    for (int kk = 0; kk < depth; ++kk) {
      int base = jj * x;
      buffer[base + (x - halo_depth + kk)] = buffer[base + (x - halo_depth - 1 - kk)];
    }
  }
}

// Update top halo.
void update_top(const int x, const int y, const int halo_depth, const int depth, double *buffer, bool is_offload) {
#ifdef OMP_TARGET
  #pragma omp target teams distribute parallel for simd if (is_offload) collapse(2)
#endif
  for (int jj = 0; jj < depth; ++jj) {
#ifndef OMP_TARGET
  #pragma omp parallel for
#endif
    for (int kk = halo_depth; kk < x - halo_depth; ++kk) {
      int base = kk;
      buffer[base + (y - halo_depth + jj) * x] = buffer[base + (y - halo_depth - 1 - jj) * x];
    }
  }
}

// Updates bottom halo.
void update_bottom(const int x, const int y, const int halo_depth, const int depth, double *buffer, bool is_offload) {
#ifdef OMP_TARGET
  #pragma omp target teams distribute parallel for simd if (is_offload) collapse(2)
#endif
  for (int jj = 0; jj < depth; ++jj) {
#ifndef OMP_TARGET
  #pragma omp parallel for
#endif
    for (int kk = halo_depth; kk < x - halo_depth; ++kk) {
      int base = kk;
      buffer[base + (halo_depth - jj - 1) * x] = buffer[base + (halo_depth + jj) * x];
    }
  }
}

// Updates faces in turn.
void update_face(const int x, const int y, const int halo_depth, const int *chunk_neighbours, const int depth, double *buffer,
                 bool is_offload) {
  if (chunk_neighbours[CHUNK_LEFT] == EXTERNAL_FACE) {
    update_left(x, y, halo_depth, depth, buffer, is_offload);
  }
  if (chunk_neighbours[CHUNK_RIGHT] == EXTERNAL_FACE) {
    update_right(x, y, halo_depth, depth, buffer, is_offload);
  }
  if (chunk_neighbours[CHUNK_TOP] == EXTERNAL_FACE) {
    update_top(x, y, halo_depth, depth, buffer, is_offload);
  }
  if (chunk_neighbours[CHUNK_BOTTOM] == EXTERNAL_FACE) {
    update_bottom(x, y, halo_depth, depth, buffer, is_offload);
  }
}

// The kernel for updating halos locally
void local_halos(const int x, const int y, const int depth, const int halo_depth, const int *chunk_neighbours,
                 const bool *fields_to_exchange, double *density, double *energy0, double *energy, double *u, double *p, double *sd,
                 bool is_offload) {
  if (fields_to_exchange[FIELD_DENSITY]) {
    update_face(x, y, halo_depth, chunk_neighbours, depth, density, is_offload);
  }
  if (fields_to_exchange[FIELD_P]) {
    update_face(x, y, halo_depth, chunk_neighbours, depth, p, is_offload);
  }
  if (fields_to_exchange[FIELD_ENERGY0]) {
    update_face(x, y, halo_depth, chunk_neighbours, depth, energy0, is_offload);
  }
  if (fields_to_exchange[FIELD_ENERGY1]) {
    update_face(x, y, halo_depth, chunk_neighbours, depth, energy, is_offload);
  }
  if (fields_to_exchange[FIELD_U]) {
    update_face(x, y, halo_depth, chunk_neighbours, depth, u, is_offload);
  }
  if (fields_to_exchange[FIELD_SD]) {
    update_face(x, y, halo_depth, chunk_neighbours, depth, sd, is_offload);
  }
}

// Solver-wide kernels
void run_local_halos(Chunk *chunk, Settings &settings, int depth) {
  START_PROFILING(settings.kernel_profile);
  local_halos(chunk->x, chunk->y, depth, settings.halo_depth, chunk->neighbours, settings.fields_to_exchange, chunk->density,
              chunk->energy0, chunk->energy, chunk->u, chunk->p, chunk->sd, settings.is_offload);
  STOP_PROFILING(settings.kernel_profile, __func__);
}
