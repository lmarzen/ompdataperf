#include <cassert>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <getopt.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

const char *OMPDATAPERF_VERSION = "0.0.1-alpha";

void print_help() {
  std::cout << "Usage: ompdataperf [options] [program] [program arguments]\n";
  std::cout << "Options:\n";
  std::cout << "  -h, --help              Show this help message\n";
  std::cout << "  -q, --quiet             Suppress warnings\n";
  std::cout << "  -v, --verbose           Enable verbose output\n";
  std::cout << "  --version               Print the version of ompdataperf\n";
}

void print_version() {
  std::cout << "ompdataperf version " << OMPDATAPERF_VERSION << "\n";
}

void print_env(const char *name) {
  assert(name != nullptr);
  const char *value = getenv(name);
  if (value == nullptr) {
    std::cerr << "info: " << name << " not set" << "\n";
    return;
  }
  std::cerr << "info: " << name << "=" << value << "\n";
  return;
}

/* Attempts to set an environment variable using setenv. Returns on success,
 * otherwise prints an error message and exits.
 */
void safe_setenv(const char *name, const char *value, int overwrite) {
  assert(name != nullptr);
  assert(value != nullptr);
  if (setenv(name, value, overwrite)) {
    std::cerr << "error: failed to set environment variable " << name << ". "
              << strerror(errno) << "\n";
    exit(EXIT_FAILURE);
  }
  return;
}

/* Attempts to set up the OMP_TOOL environment variable for ompdataperf. Returns
 * on success, otherwise prints an error message and exits.
 */
void setenv_omp_tool() {
  const char *env_omp_tool = getenv("OMP_TOOL");
  if (env_omp_tool != nullptr && strcmp(env_omp_tool, "enabled") != 0) {
    std::cerr << "warning: OMP_TOOL is defined but is not set to \'enabled\'. "
                 "Ignoring set value.\n";
  }
  // Ensure OpenMP runtime will try to register tools
  safe_setenv("OMP_TOOL", "enabled", 1 /*overwrite*/);
  return;
}

/* Attempts to set up the OMP_TOOL_LIBRARIES environment variable for
 * ompdataperf. Assumes that libompdataperf.so is in the same directory as the
 * current running executable. Returns on success, otherwise prints an error
 * message and exits.
 */
void setenv_omp_tool_libraries(const char *exec_path) {
  namespace fs = std::filesystem;
  const char *lib_name = "libompdataperf.so";
  fs::path lib_path;
  try {
    fs::path exec_full_path = fs::canonical(exec_path);
    lib_path = exec_full_path.parent_path().append(lib_name);
  } catch (const std::exception &ex) {
    std::cerr << "error: failed to resolve canonical path for " << lib_name
              << ". " << ex.what() << "\n";
    exit(EXIT_FAILURE);
  }

  const char *env_omp_tool_libraries = getenv("OMP_TOOL_LIBRARIES");
  std::string new_env_omp_tool_libraries;
  if (env_omp_tool_libraries == nullptr) {
    new_env_omp_tool_libraries = lib_path.string();
  } else {
    new_env_omp_tool_libraries =
        std::string(env_omp_tool_libraries) + ":" + lib_path.string();
  }
  safe_setenv("OMP_TOOL_LIBRARIES", new_env_omp_tool_libraries.c_str(),
              1 /*overwrite*/);
  return;
}

/* Attempts to set up the OMP_TOOL_VERBOSE_INIT environment variable for
 * ompdataperf. Returns on success, otherwise prints an error message and exits.
 */
void setenv_omp_tool_verbose_init(int verbose) {
  // If OMP_TOOL_VERBOSE_INIT is already set, don't overwrite it.
  if (verbose) {
    safe_setenv("OMP_TOOL_VERBOSE_INIT", "stderr", 0 /*overwrite*/);
  } else {
    safe_setenv("OMP_TOOL_VERBOSE_INIT", "disabled", 0 /*overwrite*/);
  }
  return;
}

int main(int argc, char *argv[]) {
  // default values for options
  int verbose = false;
  // std::string outfile;

  // clang-format off
  static struct option long_options[] = {
    {"help",    no_argument,       nullptr, 'h'},
    {"verbose", no_argument,       nullptr, 'v'},
    {"version", no_argument,       nullptr,  0 },
 // {"outfile", required_argument, nullptr, 'o'},
    {nullptr,   0,                 nullptr,  0 }
  };
  // clang-format on

  int option_index = 0;
  int c;
  // parse options
  while ((c = getopt_long(argc, argv, "++hv", long_options, &option_index)) !=
         -1) {
    switch (c) {
    case 'h':
      print_help();
      return 0;
    case 'v':
      verbose = true;
      break;
    // example of output file which takes a path parameter
    // case 'o': // add o: to optstring
    //   output_file = optarg;
    //   break;
    case 0: // handle long options with no short equivalent
      if (strcmp(long_options[option_index].name, "version") == 0) {
        print_version();
        return 0;
      }
      break;
    case '?':
      // getopt_long already printed an error message.
      return 1;
    default:
      return 1;
    }
  }

  // remaining arguments after options should be the user's program and its args
  if (optind == argc) {
    std::cerr << "error: no program specified to profile\n";
    return 1;
  }

  std::vector<char *> program_args;
  // the first argument is the program to be executed
  char *program = argv[optind];
  program_args.push_back(program);
  // collect the remaining arguments (if any) as the program's arguments
  for (int i = optind + 1; i < argc; ++i) {
    program_args.push_back(argv[i]);
  }
  // end the argument list with a nullptr for execvp
  program_args.push_back(nullptr);

  setenv_omp_tool();
  setenv_omp_tool_libraries(argv[0]);
  setenv_omp_tool_verbose_init(verbose);

  if (verbose) {
    print_env("OMP_TOOL");
    print_env("OMP_TOOL_LIBRARIES");
    print_env("OMP_TOOL_VERBOSE_INIT");

    // print command being profiled
    std::cout << "info: profiling \'" << argv[optind];
    for (int i = optind + 1; i < argc; ++i) {
      std::cout << " " << argv[i];
    }
    std::cout << "\'\n";
  }

  // on success, execvp() does not return
  execvp(program, program_args.data());

  // if execvp returns, an error occurred and errno is set
  std::cerr << "error: failed to execute program. " << strerror(errno) << "\n";
  return 1;
}
