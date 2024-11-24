#include "chunk.h"
#include "shared.h"

/*
 *		CHEBYSHEV SOLVER KERNEL
 */

// Calculates the new value for u.
void cheby_calc_u(const int x, const int y, const int halo_depth, double *u, const double *p) {
#ifdef OMP_TARGET
  #pragma omp target teams distribute parallel for simd collapse(2)
#else
  #pragma omp parallel for
#endif
  for (int jj = halo_depth; jj < y - halo_depth; ++jj) {
    for (int kk = halo_depth; kk < x - halo_depth; ++kk) {
      const int index = kk + jj * x;
      u[index] += p[index];
    }
  }
}

// Initialises the Chebyshev solver
void cheby_init(const int x, const int y, const int halo_depth, const double theta, double *u, const double *u0, double *p, double *r,
                double *w, const double *kx, const double *ky) {
#ifdef OMP_TARGET
  #pragma omp target teams distribute parallel for simd collapse(2)
#else
  #pragma omp parallel for
#endif
  for (int jj = halo_depth; jj < y - halo_depth; ++jj) {
    for (int kk = halo_depth; kk < x - halo_depth; ++kk) {
      const int index = kk + jj * x;
      const double smvp = tealeaf_SMVP(u);
      w[index] = smvp;
      r[index] = u0[index] - w[index];
      p[index] = r[index] / theta;
    }
  }

  cheby_calc_u(x, y, halo_depth, u, p);
}

// The main chebyshev iteration
void cheby_iterate(const int x, const int y, const int halo_depth, double alpha, double beta, double *u, const double *u0, double *p,
                   double *r, double *w, const double *kx, const double *ky) {
#ifdef OMP_TARGET
  #pragma omp target teams distribute parallel for simd collapse(2)
#else
  #pragma omp parallel for
#endif
  for (int jj = halo_depth; jj < y - halo_depth; ++jj) {
    for (int kk = halo_depth; kk < x - halo_depth; ++kk) {
      const int index = kk + jj * x;
      const double smvp = tealeaf_SMVP(u);
      w[index] = smvp;
      r[index] = u0[index] - w[index];
      p[index] = alpha * p[index] + beta * r[index];
    }
  }

  cheby_calc_u(x, y, halo_depth, u, p);
}

// Chebyshev solver kernels
void run_cheby_init(Chunk *chunk, Settings &settings) {
  START_PROFILING(settings.kernel_profile);
  cheby_init(chunk->x, chunk->y, settings.halo_depth, chunk->theta, chunk->u, chunk->u0, chunk->p, chunk->r, chunk->w, chunk->kx,
             chunk->ky);
  STOP_PROFILING(settings.kernel_profile, __func__);
}

void run_cheby_iterate(Chunk *chunk, Settings &settings, double alpha, double beta) {
  START_PROFILING(settings.kernel_profile);
  cheby_iterate(chunk->x, chunk->y, settings.halo_depth, alpha, beta, chunk->u, chunk->u0, chunk->p, chunk->r, chunk->w, chunk->kx,
                chunk->ky);
  STOP_PROFILING(settings.kernel_profile, __func__);
}
