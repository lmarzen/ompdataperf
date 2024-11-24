#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "chunk.h"

// static void dump_data(FILE *out, const char *name, double *data, int size) {
//   auto *host = static_cast<double *>(std::malloc(size * sizeof(double)));
//   std::memcpy(host, data, size * sizeof(double));
//
//   bool all_zero = true;
//   for (int i = 0; i < size; i++) {
//     if (host[i] != 0.0) {
//       all_zero = false;
//       break;
//     }
//   }
//
//   std::fprintf(out, "[%s,+0]", name);
//   if (all_zero) {
//     std::fprintf(out, "(0.0 * %d)", size);
//   } else {
//     for (int i = 0; i < size; i++) {
//       std::fprintf(out, "%.5f,", host[i]);
//       if (i % 20 == 0) {
//         std::fprintf(out, "\n[%s,+%d]", name, i);
//       }
//     }
//   }
//   std::fprintf(out, "\n");
//   free(host);
// }
//
// void dump_chunk(const char *prefix, const char *suffix, Chunk *chunk, Settings &settings) {
//   char name[256] = {};
//   sprintf(name, "%s_rank=%d+%s.txt", prefix, settings.rank, suffix);
//   FILE *out = fopen(name, "w");
//
//   std::fprintf(out, "x=%d\n", chunk->x);
//   std::fprintf(out, "y=%d\n", chunk->y);
//   std::fprintf(out, "dt_init=%f\n", chunk->dt_init);
//
//   std::fprintf(out, "left=%d\n", chunk->left);
//   std::fprintf(out, "right=%d\n", chunk->right);
//   std::fprintf(out, "bottom=%d\n", chunk->bottom);
//   std::fprintf(out, "top=%d\n", chunk->top);
//
////  dump_data(out, "density", chunk->density, chunk->x * chunk->y);
////  dump_data(out, "energy", chunk->energy, chunk->x * chunk->y);
////  dump_data(out, "u", chunk->u, chunk->x * chunk->y);
////  dump_data(out, "p", chunk->p, chunk->x * chunk->y);
////  dump_data(out, "r", chunk->r, chunk->x * chunk->y);
////  dump_data(out, "w", chunk->w, chunk->x * chunk->y);
////  dump_data(out, "kx", chunk->kx, chunk->x * chunk->y);
////  dump_data(out, "ky", chunk->ky, chunk->x * chunk->y);
//
//  std::fclose(out);
//}

// Initialise the chunk
void initialise_chunk(Chunk *chunk, Settings &settings, int x, int y) {
  // Initialise the key variables
  chunk->x = x + settings.halo_depth * 2;
  chunk->y = y + settings.halo_depth * 2;
  chunk->dt_init = settings.dt_init;

  // Allocate the neighbour list
  chunk->neighbours = static_cast<int *>(std::malloc(sizeof(int) * NUM_FACES));

  // Allocate the MPI comm buffers
  //  int lr_len = chunk->y * settings.halo_depth * NUM_FIELDS;
  //  chunk->left_send = static_cast<double *>(std::malloc(sizeof(double) * lr_len));
  //  chunk->left_recv = static_cast<double *>(std::malloc(sizeof(double) * lr_len));
  //  chunk->right_send = static_cast<double *>(std::malloc(sizeof(double) * lr_len));
  //  chunk->right_recv = static_cast<double *>(std::malloc(sizeof(double) * lr_len));

  //  int tb_len = chunk->x * settings.halo_depth * NUM_FIELDS;
  //  chunk->top_send = static_cast<double *>(std::malloc(sizeof(double) * tb_len));
  //  chunk->top_recv = static_cast<double *>(std::malloc(sizeof(double) * tb_len));
  //  chunk->bottom_send = static_cast<double *>(std::malloc(sizeof(double) * tb_len));
  //  chunk->bottom_recv = static_cast<double *>(std::malloc(sizeof(double) * tb_len));

  //    int lr_len = chunk->y * settings.halo_depth * NUM_FIELDS;
  //    chunk->staging_left_send = static_cast<double *>(std::malloc(sizeof(double) * lr_len));
  //    chunk->staging_left_recv = static_cast<double *>(std::malloc(sizeof(double) * lr_len));
  //    chunk->staging_right_send = static_cast<double *>(std::malloc(sizeof(double) * lr_len));
  //    chunk->staging_right_recv = static_cast<double *>(std::malloc(sizeof(double) * lr_len));

  //    int tb_len = chunk->x * settings.halo_depth * NUM_FIELDS;
  //    chunk->staging_top_send = static_cast<double *>(std::malloc(sizeof(double) * tb_len));
  //    chunk->staging_top_recv = static_cast<double *>(std::malloc(sizeof(double) * tb_len));
  //    chunk->staging_bottom_send = static_cast<double *>(std::malloc(sizeof(double) * tb_len));
  //    chunk->staging_bottom_recv = static_cast<double *>(std::malloc(sizeof(double) * tb_len));

  // Initialise the ChunkExtension, which allows composition of extended
  // fields specific to individual implementations
  chunk->ext = static_cast<ChunkExtension *>(std::malloc(sizeof(ChunkExtension)));
}

// Finalise the chunk
void finalise_chunk(Chunk *chunk) {
  free(chunk->neighbours);
  free(chunk->ext);
  //  free(chunk->left_send);
  //  free(chunk->left_recv);
  //  free(chunk->right_send);
  //  free(chunk->right_recv);
  //  free(chunk->top_send);
  //  free(chunk->top_recv);
  //  free(chunk->bottom_send);
  //  free(chunk->bottom_recv);
}
