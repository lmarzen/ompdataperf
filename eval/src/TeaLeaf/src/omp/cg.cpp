#include "chunk.h"
#include "shared.h"

/*
 *		CONJUGATE GRADIENT SOLVER KERNEL
 */

// Initialises the CG solver
void cg_init(const int x, const int y, const int halo_depth, const int coefficient, double rx, double ry, double *rro,
             const double *density, const double *energy, double *u, double *p, double *r, double *w, double *kx, double *ky) {
  if (coefficient != CONDUCTIVITY && coefficient != RECIP_CONDUCTIVITY) {
    die(__LINE__, __FILE__, "Coefficient %d is not valid.\n", coefficient);
  }

#ifdef OMP_TARGET
  #pragma omp target teams distribute parallel for simd collapse(2)
#else
  #pragma omp parallel for
#endif
  for (int jj = 0; jj < y; ++jj) {
    for (int kk = 0; kk < x; ++kk) {
      const int index = kk + jj * x;
      p[index] = 0.0;
      r[index] = 0.0;
      u[index] = energy[index] * density[index];
    }
  }

#ifdef OMP_TARGET
  #pragma omp target teams distribute parallel for simd collapse(2)
#else
  #pragma omp parallel for
#endif
  for (int jj = 1; jj < y - 1; ++jj) {
    for (int kk = 1; kk < x - 1; ++kk) {
      const int index = kk + jj * x;
      w[index] = (coefficient == CONDUCTIVITY) ? density[index] : 1.0 / density[index];
    }
  }

#ifdef OMP_TARGET
  #pragma omp target teams distribute parallel for simd collapse(2)
#else
  #pragma omp parallel for
#endif
  for (int jj = halo_depth; jj < y - 1; ++jj) {
    for (int kk = halo_depth; kk < x - 1; ++kk) {
      const int index = kk + jj * x;
      kx[index] = rx * (w[index - 1] + w[index]) / (2.0 * w[index - 1] * w[index]);
      ky[index] = ry * (w[index - x] + w[index]) / (2.0 * w[index - x] * w[index]);
    }
  }

  double rro_temp = 0.0;

#ifdef OMP_TARGET
  #pragma omp target teams distribute parallel for simd reduction(+ : rro_temp) collapse(2)
#else
  #pragma omp parallel for reduction(+ : rro_temp)
#endif
  for (int jj = halo_depth; jj < y - halo_depth; ++jj) {
    for (int kk = halo_depth; kk < x - halo_depth; ++kk) {
      const int index = kk + jj * x;
      const double smvp = tealeaf_SMVP(u);
      w[index] = smvp;
      r[index] = u[index] - w[index];
      p[index] = r[index];
      rro_temp += r[index] * p[index];
    }
  }

  // Sum locally
  *rro += rro_temp;
}

// Calculates w
void cg_calc_w(const int x, const int y, const int halo_depth, double *pw, const double *p, double *w, const double *kx, const double *ky) {
  double pw_temp = 0.0;

#ifdef OMP_TARGET
  #pragma omp target teams distribute parallel for simd reduction(+ : pw_temp) collapse(2)
#else
  #pragma omp parallel for reduction(+ : pw_temp)
#endif
  for (int jj = halo_depth; jj < y - halo_depth; ++jj) {
    for (int kk = halo_depth; kk < x - halo_depth; ++kk) {
      const int index = kk + jj * x;
      const double smvp = tealeaf_SMVP(p);
      w[index] = smvp;
      pw_temp += w[index] * p[index];
    }
  }

  *pw += pw_temp;
}

// Calculates u and r
void cg_calc_ur(const int x, const int y, const int halo_depth, const double alpha, double *rrn, double *u, const double *p, double *r,
                const double *w) {
  double rrn_temp = 0.0;

#ifdef OMP_TARGET
  #pragma omp target teams distribute parallel for simd reduction(+ : rrn_temp) collapse(2)
#else
  #pragma omp parallel for reduction(+ : rrn_temp)
#endif
  for (int jj = halo_depth; jj < y - halo_depth; ++jj) {
    for (int kk = halo_depth; kk < x - halo_depth; ++kk) {
      const int index = kk + jj * x;

      u[index] += alpha * p[index];
      r[index] -= alpha * w[index];
      rrn_temp += r[index] * r[index];
    }
  }

  *rrn += rrn_temp;
}

// Calculates p
void cg_calc_p(const int x, const int y, const int halo_depth, const double beta, double *p, const double *r) {
#ifdef OMP_TARGET
  #pragma omp target teams distribute parallel for simd collapse(2)
#else
  #pragma omp parallel for
#endif
  for (int jj = halo_depth; jj < y - halo_depth; ++jj) {
    for (int kk = halo_depth; kk < x - halo_depth; ++kk) {
      const int index = kk + jj * x;

      p[index] = beta * p[index] + r[index];
    }
  }
}

// CG solver kernels
void run_cg_init(Chunk *chunk, Settings &settings, double rx, double ry, double *rro) {
  START_PROFILING(settings.kernel_profile);
  cg_init(chunk->x, chunk->y, settings.halo_depth, settings.coefficient, rx, ry, rro, chunk->density, chunk->energy, chunk->u, chunk->p,
          chunk->r, chunk->w, chunk->kx, chunk->ky);
  STOP_PROFILING(settings.kernel_profile, __func__);
}

void run_cg_calc_w(Chunk *chunk, Settings &settings, double *pw) {
  START_PROFILING(settings.kernel_profile);
  cg_calc_w(chunk->x, chunk->y, settings.halo_depth, pw, chunk->p, chunk->w, chunk->kx, chunk->ky);
  STOP_PROFILING(settings.kernel_profile, __func__);
}

void run_cg_calc_ur(Chunk *chunk, Settings &settings, double alpha, double *rrn) {
  START_PROFILING(settings.kernel_profile);
  cg_calc_ur(chunk->x, chunk->y, settings.halo_depth, alpha, rrn, chunk->u, chunk->p, chunk->r, chunk->w);
  STOP_PROFILING(settings.kernel_profile, __func__);
}

void run_cg_calc_p(Chunk *chunk, Settings &settings, double beta) {
  START_PROFILING(settings.kernel_profile);
  cg_calc_p(chunk->x, chunk->y, settings.halo_depth, beta, chunk->p, chunk->r);
  STOP_PROFILING(settings.kernel_profile, __func__);
}
