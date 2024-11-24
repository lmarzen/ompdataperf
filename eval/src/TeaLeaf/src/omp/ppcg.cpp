#include "chunk.h"
#include "shared.h"

/*
 *		PPCG SOLVER KERNEL
 */

// Initialises the PPCG solver
void ppcg_init(const int x, const int y, const int halo_depth, double theta, const double *r, double *sd) {
#ifdef OMP_TARGET
  #pragma omp target teams distribute parallel for simd collapse(2)
#else
  #pragma omp parallel for
#endif
  for (int jj = halo_depth; jj < y - halo_depth; ++jj) {
    for (int kk = halo_depth; kk < x - halo_depth; ++kk) {
      const int index = kk + jj * x;
      sd[index] = r[index] / theta;
    }
  }
}

// The PPCG inner iteration
void ppcg_inner_iteration(const int x, const int y, const int halo_depth, double alpha, double beta, double *u, double *r, const double *kx,
                          const double *ky, double *sd) {
#ifdef OMP_TARGET
  #pragma omp target teams distribute parallel for simd collapse(2)
#else
  #pragma omp parallel for
#endif
  for (int jj = halo_depth; jj < y - halo_depth; ++jj) {
    for (int kk = halo_depth; kk < x - halo_depth; ++kk) {
      const int index = kk + jj * x;
      const double smvp = tealeaf_SMVP(sd);
      r[index] -= smvp;
      u[index] += sd[index];
    }
  }

#ifdef OMP_TARGET
  #pragma omp target teams distribute parallel for simd collapse(2)
#else
  #pragma omp parallel for
#endif
  for (int jj = halo_depth; jj < y - halo_depth; ++jj) {
    for (int kk = halo_depth; kk < x - halo_depth; ++kk) {
      const int index = kk + jj * x;
      sd[index] = alpha * sd[index] + beta * r[index];
    }
  }
}

// PPCG solver kernels
void run_ppcg_init(Chunk *chunk, Settings &settings) {
  START_PROFILING(settings.kernel_profile);
  ppcg_init(chunk->x, chunk->y, settings.halo_depth, chunk->theta, chunk->r, chunk->sd);
  STOP_PROFILING(settings.kernel_profile, __func__);
}

void run_ppcg_inner_iteration(Chunk *chunk, Settings &settings, double alpha, double beta) {
  START_PROFILING(settings.kernel_profile);
  ppcg_inner_iteration(chunk->x, chunk->y, settings.halo_depth, alpha, beta, chunk->u, chunk->r, chunk->kx, chunk->ky, chunk->sd);
  STOP_PROFILING(settings.kernel_profile, __func__);
}
