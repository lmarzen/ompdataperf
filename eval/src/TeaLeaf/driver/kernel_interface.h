#pragma once

#include "chunk.h"
#include "comms.h"
#include "settings.h"

/*
 *      This is the main interface file for C based implementations.
 */

// Initialisation kernels
void run_model_info(Settings &settings);
void run_set_chunk_data(Chunk *chunk, Settings &settings);
void run_set_chunk_state(Chunk *chunk, Settings &settings, State *states);
void run_kernel_initialise(Chunk *chunk, Settings &settings, int comms_lr_len, int comms_tb_len);
void run_kernel_finalise(Chunk *chunk, Settings &settings);

// Solver-wide kernels
void run_local_halos(Chunk *chunk, Settings &settings, int depth);

// void run_pack_or_unpack(Chunk *chunk, Settings &settings, int depth, int face, bool pack, FieldBufferType field,
//                         FieldBufferType destination);

void run_pack_or_unpack(Chunk *chunk, Settings &settings, int depth, int face, bool pack, FieldBufferType field,
                        FieldBufferType destination, int offset);
//
void run_send_recv_halo(Chunk *chunk, Settings &settings,                                                       //
                        FieldBufferType src_send_buffer, FieldBufferType src_recv_buffer,                       //
                        StagingBufferType dest_staging_send_buffer, StagingBufferType dest_staging_recv_buffer, //
                        int buffer_len, int neighbour,                                                          //
                        int send_tag, int recv_tag,                                                             //
                        MPI_Request *send_request, MPI_Request *recv_request);
void run_before_waitall_halo(Chunk *chunk, Settings &settings);
void run_restore_recv_halo(Chunk *chunk, Settings &settings, //
                           FieldBufferType dest_recv_buffer, StagingBufferType src_staging_recv_buffer, int buffer_len);

void run_store_energy(Chunk *chunk, Settings &settings);
void run_field_summary(Chunk *chunk, Settings &settings, double *vol, double *mass, double *ie, double *temp);

// CG solver kernels
void run_cg_init(Chunk *chunk, Settings &settings, double rx, double ry, double *rro);
void run_cg_calc_w(Chunk *chunk, Settings &settings, double *pw);
void run_cg_calc_ur(Chunk *chunk, Settings &settings, double alpha, double *rrn);
void run_cg_calc_p(Chunk *chunk, Settings &settings, double beta);

// Chebyshev solver kernels
void run_cheby_init(Chunk *chunk, Settings &settings);
void run_cheby_iterate(Chunk *chunk, Settings &settings, double alpha, double beta);

// Jacobi solver kernels
void run_jacobi_init(Chunk *chunk, Settings &settings, double rx, double ry);
void run_jacobi_iterate(Chunk *chunk, Settings &settings, double *error);

// PPCG solver kernels
void run_ppcg_init(Chunk *chunk, Settings &settings);
void run_ppcg_inner_iteration(Chunk *chunk, Settings &settings, double alpha, double beta);

// Shared solver kernels
void run_copy_u(Chunk *chunk, Settings &settings);
void run_calculate_residual(Chunk *chunk, Settings &settings);
void run_calculate_2norm(Chunk *chunk, Settings &settings, FieldBufferType buffer, double *norm);
void run_finalise(Chunk *chunk, Settings &settings);
