#pragma once

#ifdef __APPLE__
  #include <mach/mach.h>
  #include <mach/mach_time.h>
#else
  #include <ctime>
#endif

/*
 *		PROFILING TOOL
 *		Not thread safe.
 */

#define PROFILER_MAX_NAME 128
#define PROFILER_MAX_ENTRIES 2048

#ifdef __cplusplus
extern "C" {
#endif

struct ProfileEntry {
  int calls;
  double time;
  char name[PROFILER_MAX_NAME];
};

struct Profile {
#ifdef __APPLE__
  uint64_t profiler_start;
  uint64_t profiler_end;
#else
  struct timespec profiler_start;
  struct timespec profiler_end;
#endif

  int profiler_entry_count;
  ProfileEntry profiler_entries[PROFILER_MAX_ENTRIES];
};

Profile *profiler_initialise();
void profiler_finalise(Profile **profile);

void profiler_start_timer(Profile *profile);
void profiler_end_timer(Profile *profile, const char *entry_name);
void profiler_print_simple_profile(Profile *profile);
void profiler_print_full_profile(Profile *profile);
int profiler_get_profile_entry(Profile *profile, const char *entry_name);

#ifdef __cplusplus
}
#endif

// Allows compile-time optimised conditional profiling
#ifdef ENABLE_PROFILING

  #define START_PROFILING(profile) profiler_start_timer(profile)

  #define STOP_PROFILING(profile, name) profiler_end_timer(profile, name)

  #define PRINT_PROFILING_RESULTS(profile) profiler_print_full_profile(profile)

#else

  #define START_PROFILING(profile) \
    do {                           \
    } while (false)
  #define STOP_PROFILING(profile, name) \
    do {                                \
    } while (false)
  #define PRINT_PROFILING_RESULTS(profile) \
    do {                                   \
    } while (false)

#endif
