#include "kernel_interface.h"
#include <omp.h>

// Allocates, and zeroes and individual buffer
void allocate_buffer(double **a, int x, int y) {
  *a = static_cast<double *>(std::malloc(sizeof(double) * x * y));
  if (*a == nullptr) {
    die(__LINE__, __FILE__, "Error allocating buffer %s\n");
  }
#pragma omp parallel for
  for (int jj = 0; jj < y; ++jj) {
    for (int kk = 0; kk < x; ++kk) {
      const int index = kk + jj * x;
      (*a)[index] = 0.0;
    }
  }
}

// Initialisation kernels
void run_set_chunk_data(Chunk *chunk, Settings &settings) {
  double xMin = settings.grid_x_min + settings.dx * (double)chunk->left;
  double yMin = settings.grid_y_min + settings.dy * (double)chunk->bottom;
  for (int ii = 0; ii < chunk->x + 1; ++ii) {
    chunk->vertex_x[ii] = xMin + settings.dx * (ii - settings.halo_depth);
  }
  for (int ii = 0; ii < chunk->y + 1; ++ii) {
    chunk->vertex_y[ii] = yMin + settings.dy * (ii - settings.halo_depth);
  }
  for (int ii = 0; ii < chunk->x; ++ii) {
    chunk->cell_x[ii] = 0.5 * (chunk->vertex_x[ii] + chunk->vertex_x[ii + 1]);
  }
  for (int ii = 0; ii < chunk->y; ++ii) {
    chunk->cell_y[ii] = 0.5 * (chunk->vertex_y[ii] + chunk->vertex_y[ii + 1]);
  }
  for (int ii = 0; ii < chunk->x * chunk->y; ++ii) {
    chunk->volume[ii] = settings.dx * settings.dy;
    chunk->x_area[ii] = settings.dy;
    chunk->y_area[ii] = settings.dx;
  }
}

void run_set_chunk_state(Chunk *chunk, Settings &settings, State *states) {
  // Set the initial state
  for (int ii = 0; ii != chunk->x * chunk->y; ++ii) {
    chunk->energy0[ii] = states[0].energy;
    chunk->density[ii] = states[0].density;
  }
  // Apply all of the states in turn
  for (int ss = 1; ss < settings.num_states; ++ss) {
    for (int jj = 0; jj < chunk->y; ++jj) {
      for (int kk = 0; kk < chunk->x; ++kk) {
        int applyState = 0;

        if (states[ss].geometry == Geometry::RECTANGULAR) {
          applyState = (chunk->vertex_x[kk + 1] >= states[ss].x_min && chunk->vertex_x[kk] < states[ss].x_max &&
                        chunk->vertex_y[jj + 1] >= states[ss].y_min && chunk->vertex_y[jj] < states[ss].y_max);
        } else if (states[ss].geometry == Geometry::CIRCULAR) {
          double radius = std::sqrt((chunk->cell_x[kk] - states[ss].x_min) * (chunk->cell_x[kk] - states[ss].x_min) +
                                    (chunk->cell_y[jj] - states[ss].y_min) * (chunk->cell_y[jj] - states[ss].y_min));

          applyState = (radius <= states[ss].radius);
        } else if (states[ss].geometry == Geometry::POINT) {
          applyState = (chunk->vertex_x[kk] == states[ss].x_min && chunk->vertex_y[jj] == states[ss].y_min);
        }

        // Check if state applies at this vertex, and apply
        if (applyState) {
          const int index1 = kk + jj * chunk->x;
          chunk->energy0[index1] = states[ss].energy;
          chunk->density[index1] = states[ss].density;
        }
      }
    }
  }

  // Set an initial state for u
  for (int jj = 1; jj != chunk->y - 1; ++jj) {
    for (int kk = 1; kk != chunk->x - 1; ++kk) {
      const int index1 = kk + jj * chunk->x;
      chunk->u[index1] = chunk->energy0[index1] * chunk->density[index1];
    }
  }
}

void run_model_info(Settings &settings) {
#ifdef OMP_TARGET
  settings.model_name = "OpenMP Target";
  settings.model_kind = ModelKind::Offload;
#else
  settings.model_name = "OpenMP (CPU)";
  settings.model_kind = ModelKind::Host;
#endif
}

void run_kernel_initialise(Chunk *chunk, Settings &settings, int comms_lr_len, int comms_tb_len) {

#ifdef OMP_TARGET
  int count = omp_get_num_devices();
  print_and_log(settings, "Devices:\n");
  if (count == 0) {
    print_and_log(settings, "# WARNING: omp_get_num_devices returned 0 devices, code will fallback to host execution.\n");
  }
  for (int i = 0; i < count; ++i) {
    print_and_log(settings, " %d: target device #%d\n", i, i);
  }
#endif

  if (settings.device_selector) {
    int device = static_cast<int>(std::strtol(settings.device_selector, nullptr, 10));
#ifdef OMP_TARGET
    print_and_log(settings, " - Device: #%d\n", device);
    omp_set_default_device(device);
#else
    print_and_log(settings, "# Ignoring device selector (#%d) plain OpenMP\n", device);
#endif
  }

  allocate_buffer(&(chunk->density0), chunk->x, chunk->y);
  allocate_buffer(&(chunk->density), chunk->x, chunk->y);
  allocate_buffer(&(chunk->energy0), chunk->x, chunk->y);
  allocate_buffer(&(chunk->energy), chunk->x, chunk->y);
  allocate_buffer(&(chunk->u), chunk->x, chunk->y);
  allocate_buffer(&(chunk->u0), chunk->x, chunk->y);
  allocate_buffer(&(chunk->p), chunk->x, chunk->y);
  allocate_buffer(&(chunk->r), chunk->x, chunk->y);
  allocate_buffer(&(chunk->mi), chunk->x, chunk->y);
  allocate_buffer(&(chunk->w), chunk->x, chunk->y);
  allocate_buffer(&(chunk->kx), chunk->x, chunk->y);
  allocate_buffer(&(chunk->ky), chunk->x, chunk->y);
  allocate_buffer(&(chunk->sd), chunk->x, chunk->y);
  allocate_buffer(&(chunk->volume), chunk->x, chunk->y);
  allocate_buffer(&(chunk->x_area), chunk->x + 1, chunk->y);
  allocate_buffer(&(chunk->y_area), chunk->x, chunk->y + 1);
  allocate_buffer(&(chunk->cell_x), chunk->x, 1);
  allocate_buffer(&(chunk->cell_y), 1, chunk->y);
  allocate_buffer(&(chunk->cell_dx), chunk->x, 1);
  allocate_buffer(&(chunk->cell_dy), 1, chunk->y);
  allocate_buffer(&(chunk->vertex_dx), chunk->x + 1, 1);
  allocate_buffer(&(chunk->vertex_dy), 1, chunk->y + 1);
  allocate_buffer(&(chunk->vertex_x), chunk->x + 1, 1);
  allocate_buffer(&(chunk->vertex_y), 1, chunk->y + 1);
  allocate_buffer(&(chunk->cg_alphas), settings.max_iters, 1);
  allocate_buffer(&(chunk->cg_betas), settings.max_iters, 1);
  allocate_buffer(&(chunk->cheby_alphas), settings.max_iters, 1);
  allocate_buffer(&(chunk->cheby_betas), settings.max_iters, 1);

  allocate_buffer(&(chunk->left_send), comms_lr_len, 1);
  allocate_buffer(&(chunk->left_recv), comms_lr_len, 1);
  allocate_buffer(&(chunk->right_send), comms_lr_len, 1);
  allocate_buffer(&(chunk->right_recv), comms_lr_len, 1);
  allocate_buffer(&(chunk->top_send), comms_tb_len, 1);
  allocate_buffer(&(chunk->top_recv), comms_tb_len, 1);
  allocate_buffer(&(chunk->bottom_send), comms_tb_len, 1);
  allocate_buffer(&(chunk->bottom_recv), comms_tb_len, 1); //
}

void run_kernel_finalise(Chunk *chunk, Settings &) {
  std::free(chunk->density0);
  std::free(chunk->density);
  std::free(chunk->energy0);
  std::free(chunk->energy);
  std::free(chunk->u);
  std::free(chunk->u0);
  std::free(chunk->p);
  std::free(chunk->r);
  std::free(chunk->mi);
  std::free(chunk->w);
  std::free(chunk->kx);
  std::free(chunk->ky);
  std::free(chunk->sd);
  std::free(chunk->volume);
  std::free(chunk->x_area);
  std::free(chunk->y_area);
  std::free(chunk->cell_x);
  std::free(chunk->cell_y);
  std::free(chunk->cell_dx);
  std::free(chunk->cell_dy);
  std::free(chunk->vertex_dx);
  std::free(chunk->vertex_dy);
  std::free(chunk->vertex_x);
  std::free(chunk->vertex_y);
  std::free(chunk->cg_alphas);
  std::free(chunk->cg_betas);
  std::free(chunk->cheby_alphas);
  std::free(chunk->cheby_betas);

  std::free(chunk->left_send);
  std::free(chunk->left_recv);
  std::free(chunk->right_send);
  std::free(chunk->right_recv);
  std::free(chunk->top_send);
  std::free(chunk->top_recv);
  std::free(chunk->bottom_send);
  std::free(chunk->bottom_recv);
}
