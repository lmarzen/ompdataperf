#pragma once

#include "chunk_extension.h"
#include "settings.h"
#include <cmath>

// The core Tealeaf interface class.
struct Chunk {
  // Solve-wide variables
  double dt_init;

  // Neighbouring ranks
  int *neighbours;

  // MPI comm buffers
  FieldBufferType left_send;
  FieldBufferType left_recv;
  FieldBufferType right_send;
  FieldBufferType right_recv;
  FieldBufferType top_send;
  FieldBufferType top_recv;
  FieldBufferType bottom_send;
  FieldBufferType bottom_recv;

  StagingBufferType staging_left_send;
  StagingBufferType staging_left_recv;
  StagingBufferType staging_right_send;
  StagingBufferType staging_right_recv;
  StagingBufferType staging_top_send;
  StagingBufferType staging_top_recv;
  StagingBufferType staging_bottom_send;
  StagingBufferType staging_bottom_recv;

  // Mesh chunks
  int left;
  int right;
  int bottom;
  int top;

  // Field dimensions
  int x;
  int y;

  // Field buffers
  FieldBufferType density0;
  FieldBufferType density;
  FieldBufferType energy0;
  FieldBufferType energy;

  FieldBufferType u;
  FieldBufferType u0;
  FieldBufferType p;
  FieldBufferType r;
  FieldBufferType mi;
  FieldBufferType w;
  FieldBufferType kx;
  FieldBufferType ky;
  FieldBufferType sd;

  FieldBufferType cell_x;
  FieldBufferType cell_y;
  FieldBufferType cell_dx;
  FieldBufferType cell_dy;

  FieldBufferType vertex_dx;
  FieldBufferType vertex_dy;
  FieldBufferType vertex_x;
  FieldBufferType vertex_y;

  FieldBufferType volume;
  FieldBufferType x_area;
  FieldBufferType y_area;

  // Cheby and PPCG
  double theta;
  double eigmin;
  double eigmax;

  double *cg_alphas;
  double *cg_betas;
  double *cheby_alphas;
  double *cheby_betas;

  ChunkExtension *ext;
};

struct Settings;

void dump_chunk(const char *prefix, const char *suffix, Chunk *chunk, Settings &settings);
void initialise_chunk(Chunk *chunk, Settings &settings, int x, int y);
void finalise_chunk(Chunk *chunk);
