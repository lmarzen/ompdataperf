#include "shared.h"
#include "comms.h"

// Initialises the log file pointer
void initialise_log(Settings &settings) {
  // Only write to log in master rank
  if (settings.rank != MASTER) {
    return;
  }

  std::printf("# Opening %s as log file.\n", settings.tea_out_filename);
  std::fflush(stdout);
  settings.tea_out_fp = std::fopen(settings.tea_out_filename, "w");

  if (!settings.tea_out_fp) {
    die(__LINE__, __FILE__, "Could not open log %s\n", settings.tea_out_filename);
  }
}

// Prints to stdout and then logs message in log file
void print_and_log(Settings &settings, const char *format, ...) {
  // Only master rank should print
  if (settings.rank != MASTER) {
    return;
  }

  va_list arglist;
  va_start(arglist, format);
  std::vprintf(format, arglist);
  va_end(arglist);
  std::fflush(stdout);

  if (!settings.tea_out_fp) {
    die(__LINE__, __FILE__, "Attempted to write to log before it was initialised\n");
  }

  // Obtuse, but necessary
  va_list arglist2;
  va_start(arglist2, format);
  std::vfprintf(settings.tea_out_fp, format, arglist2);
  va_end(arglist2);
  std::fflush(settings.tea_out_fp);
}

// Logs message in log file
void print_to_log(Settings &settings, const char *format, ...) {
  // Only master rank should log
  if (settings.rank != MASTER) {
    return;
  }

  if (!settings.tea_out_fp) {
    die(__LINE__, __FILE__, "Attempted to write to log before it was initialised\n");
  }

  va_list arglist;
  va_start(arglist, format);
  std::vfprintf(settings.tea_out_fp, format, arglist);
  va_end(arglist);
  std::fflush(settings.tea_out_fp);
}

// Plots a two-dimensional dat file.
void plot_2d(int x, int y, const double *buffer, const char *name) {
  // Open the plot file
  FILE *fp = std::fopen("plot2d.dat", "wb");
  if (!fp) {
    std::printf("Could not open plot file.\n");
  }

  double b_sum = 0.0;

  for (int jj = 0; jj < y; ++jj) {
    for (int kk = 0; kk < x; ++kk) {
      double val = buffer[kk + jj * x];
      std::fprintf(fp, "%d %d %.12E\n", kk, jj, val);
      b_sum += val;
    }
  }

  std::printf("%s: %.12E\n", name, b_sum);
  std::fclose(fp);
}

// Aborts the application.
void die(int lineNum, const char *file, const char *format, ...) {
  // Print location of error
  std::printf("\x1b[31m");
  std::printf("\nError at line %d in %s:", lineNum, file);
  std::printf("\x1b[0m \n");

  va_list arglist;
  va_start(arglist, format);
  std::vprintf(format, arglist);
  va_end(arglist);
  std::fflush(stdout);

  abort_comms();
}

// Write out data for visualisation in visit
void write_to_visit(const int nx, const int ny, const int x_off, const int y_off, const double *data, const char *name, const int step,
                    const double time) {
  char bovname[256]{};
  char datname[256]{};
  std::sprintf(bovname, "%s%d.bov", name, step);
  std::sprintf(datname, "%s%d.dat", name, step);

  FILE *bovfp = std::fopen(bovname, "w");

  if (!bovfp) {
    std::printf("Could not open file %s\n", bovname);
    std::exit(1);
  }

  std::fprintf(bovfp, "TIME: %.4f\n", time);
  std::fprintf(bovfp, "DATA_FILE: %s\n", datname);
  std::fprintf(bovfp, "DATA_SIZE: %d %d 1\n", nx, ny);
  std::fprintf(bovfp, "DATA_FORMAT: DOUBLE\n");
  std::fprintf(bovfp, "VARIABLE: density\n");
  std::fprintf(bovfp, "DATA_ENDIAN: LITTLE\n");
  std::fprintf(bovfp, "CENTERING: zone\n");
  std::fprintf(bovfp, "BRICK_ORIGIN: 0. 0. 0.\n");

  std::fprintf(bovfp, "BRICK_SIZE: %d %d 1\n", nx, ny);
  std::fclose(bovfp);

  FILE *datfp = std::fopen(datname, "wb");
  if (!datfp) {
    std::printf("Could not open file %s\n", datname);
    std::exit(1);
  }

  std::fwrite(data, sizeof(double), nx * ny, datfp);
  std::fclose(datfp);
}
