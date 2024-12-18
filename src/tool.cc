#include <cassert>
#include <chrono>
#include <iostream>
#include <map>
#include <mutex>

#include <omp-tools.h>

#include "analyze.hh"
#include "symbolizer.hh"

using namespace std::chrono;

namespace {
/* Stores the start and end time of the target application being analyzed.
 */
steady_clock::time_point s_start_time;
steady_clock::time_point s_end_time;

/* As the user's program runs we log information about target operations to this
 * array which is analyzed once the execution of the user's program has
 * completed.
 */
std::vector<target_info_t> *s_target_log_ptr;
std::mutex s_target_log_mutex;
std::vector<data_op_info_t> *s_data_op_log_ptr;
std::mutex s_data_op_log_mutex;

#ifdef ENABLE_COLLISION_CHECKING
std::map<HASH_T, std::set<data_info_t>> *s_collision_map_ptr;
std::mutex s_collision_map_mutex;
#endif // ENABLE_COLLISION_CHECKING

#ifdef MEASURE_HASHING_OVERHEAD
duration<uint64_t, std::nano> s_hash_overhead;
std::mutex s_hash_overhead_mutex;

HASH_T hash_fn_measure(void *key, size_t len) {
  steady_clock::time_point start_time = steady_clock::now();
  HASH_T hash = HASH_FN(key, len);
  steady_clock::time_point end_time = steady_clock::now();
  duration<uint64_t, std::nano> overhead = end_time - start_time;
  s_hash_overhead_mutex.lock();
  s_hash_overhead += overhead;
  s_hash_overhead_mutex.unlock();
  return hash;
}
#undef HASH_FN
#define HASH_FN(key, len) hash_fn_measure(key, len)
#endif // MEASURE_HASHING_OVERHEAD

/* Binding Entry Points in the OMPT Callback Interface
 */
ompt_enumerate_states_t ompt_enumerate_states;
ompt_enumerate_mutex_impls_t ompt_enumerate_mutex_impls;
ompt_set_callback_t ompt_set_callback;
ompt_get_callback_t ompt_get_callback;
ompt_get_thread_data_t ompt_get_thread_data;
ompt_get_num_procs_t ompt_get_num_procs;
ompt_get_num_places_t ompt_get_num_places;
ompt_get_place_proc_ids_t ompt_get_place_proc_ids;
ompt_get_place_num_t ompt_get_place_num;
ompt_get_partition_place_nums_t ompt_get_partition_place_nums;
ompt_get_proc_id_t ompt_get_proc_id;
ompt_get_parallel_info_t ompt_get_parallel_info;
ompt_get_task_info_t ompt_get_task_info;
ompt_get_task_memory_t ompt_get_task_memory;
ompt_get_target_info_t ompt_get_target_info;
ompt_get_num_devices_t ompt_get_num_devices;
ompt_get_unique_id_t ompt_get_unique_id;
ompt_finalize_tool_t ompt_finalize_tool;
ompt_function_lookup_t ompt_function_lookup;
} // namespace

#ifdef ENABLE_COLLISION_CHECKING
/* This function will try to insert a copy of data in the corresponding set in
 * the collision map.
 * It is the caller's responsibility to call 'free' on data.
 */
void try_collision_map_insert(
    std::map<HASH_T, std::set<data_info_t>> *collision_map_ptr, HASH_T hash,
    void *data, size_t bytes) {
  assert(data != nullptr);

  const data_info_t key(data, bytes);
  std::set<data_info_t> &set = (*collision_map_ptr)[hash];
  const auto &hint = set.upper_bound(key);
  if (!set.empty()) {
    if (hint != set.begin() && *std::prev(hint) == key) {
      // data is already present
      return;
    }
  }

  void *data_copy = new (std::nothrow) char[bytes];
  if (data_copy == nullptr) {
    std::cerr << "warning: memory allocation failed. Hash collision checking "
                 "is degraded.\n";
    return;
  }
  memcpy(data_copy, data, bytes);
  const data_info_t new_key(data_copy, bytes);
  set.emplace_hint(hint, new_key);
  return;
}
#endif // ENABLE_COLLISION_CHECKING

static void on_ompt_callback_target_emi(ompt_target_t kind,
                                        ompt_scope_endpoint_t endpoint,
                                        int device_num, ompt_data_t *task_data,
                                        ompt_data_t *target_task_data,
                                        ompt_data_t *target_data,
                                        const void *codeptr_ra) {
  static thread_local uint64_t id = 0;
  // used to time synchronous data op
  static thread_local steady_clock::time_point s_sync_target_start_time =
      steady_clock::time_point();
  // used to time asynchronous data op
  static thread_local std::map<uint64_t /*id*/,
                               steady_clock::time_point /*start_time*/>
      s_async_target_start_times;

  if (!is_target_exec(kind)) {
    return;
  }

  const steady_clock::time_point time_now = steady_clock::now();

  bool is_async = is_async_target_exec(kind);
  if (endpoint == ompt_scope_begin) {
    // commit start timestamp
    s_sync_target_start_time = time_now;
    if (is_async) {
      assert((target_task_data != nullptr) &&
             !s_async_target_start_times.contains(target_task_data->value));
      if (target_task_data != nullptr) {
        target_task_data->value = id;
        s_async_target_start_times[id] = time_now;
        ++id;
      }
    } else {
      s_sync_target_start_time = time_now;
    }

  } else if (endpoint == ompt_scope_end) {
    // commit end timestamp
    steady_clock::time_point start_time;
    if (is_async) {
      assert((target_task_data != nullptr) &&
             !s_async_target_start_times.contains(target_task_data));
      if (target_task_data != nullptr) {
        start_time = s_async_target_start_times[target_task_data->value];
        s_async_target_start_times.erase(target_task_data->value);
      }
    } else {
      start_time = s_sync_target_start_time;
    }
    s_target_log_mutex.lock();
    s_target_log_ptr->emplace_back(kind, device_num, start_time, time_now);
    s_target_log_mutex.unlock();
  }

  return;
}

static void on_ompt_callback_target_data_op_emi(
    ompt_scope_endpoint_t endpoint, ompt_data_t *target_task_data,
    ompt_data_t *target_data, ompt_id_t *host_op_id,
    ompt_target_data_op_t optype, void *src_addr, int src_device_num,
    void *dest_addr, int dest_device_num, size_t bytes,
    const void *codeptr_ra) {
  // used to time synchronous data op
  static thread_local steady_clock::time_point s_sync_data_op_start_time =
      steady_clock::time_point();
  // used to time asynchronous data op
  static thread_local std::map<
      std::pair<int /*dest_device_num*/, const void * /*dest_addr*/>,
      steady_clock::time_point /*start_time*/>
      s_async_data_op_start_times;

  if (!(is_transfer_op(optype) || is_alloc_op(optype) ||
        is_delete_op(optype))) {
    return;
  }

  const steady_clock::time_point time_now = steady_clock::now();

  bool is_async = is_async_op(optype);

  if (endpoint == ompt_scope_begin) {
    // commit start timestamp
    s_sync_data_op_start_time = time_now;
    if (is_async) {
      const std::pair<int, const void *> key(dest_device_num, dest_addr);
      assert(!s_async_data_op_start_times.contains(key));
      s_async_data_op_start_times[key] = time_now;
    } else {
      s_sync_data_op_start_time = time_now;
    }

  } else if (endpoint == ompt_scope_end) {
    // commit end timestamp
    HASH_T hash = {};
    if (is_transfer_to_op(optype)) {
      assert(src_addr != nullptr);
      assert(dest_addr != nullptr);
      hash = HASH_FN(src_addr, bytes);
    } else if (is_transfer_from_op(optype)) {
      assert(src_addr != nullptr);
      assert(dest_addr != nullptr);
      hash = HASH_FN(dest_addr, bytes);
    }

    steady_clock::time_point start_time;
    if (is_async) {
      const std::pair<int, const void *> key(dest_device_num, dest_addr);
      assert(s_async_data_op_start_times.contains(key));
      start_time = s_async_data_op_start_times[key];
      s_async_data_op_start_times.erase(key);
    } else {
      start_time = s_sync_data_op_start_time;
    }
    s_data_op_log_mutex.lock();
    s_data_op_log_ptr->emplace_back(optype, src_addr, dest_addr, src_device_num,
                                    dest_device_num, bytes, codeptr_ra,
                                    start_time, time_now, hash);
    s_data_op_log_mutex.unlock();

#ifdef ENABLE_COLLISION_CHECKING
    s_collision_map_mutex.lock();
    if (is_transfer_to_op(optype)) {
      try_collision_map_insert(s_collision_map_ptr, hash, src_addr, bytes);
    } else if (is_transfer_from_op(optype)) {
      try_collision_map_insert(s_collision_map_ptr, hash, dest_addr, bytes);
    }
    s_collision_map_mutex.unlock();
#endif // ENABLE_COLLISION_CHECKING
  }

  return;
}

/* OpenMP API Specification 5.2 Section 19.2.3
 * "If a tool initializer returns a non-zero value, the OMPT interface state
 * remains active for the execution; otherwise, the OMPT interface state
 * changes to inactive."
 */
int ompt_initialize(ompt_function_lookup_t lookup, int initial_device_num,
                    ompt_data_t *data) {
  assert(lookup != nullptr);
  // clang-format off
  ompt_function_lookup = lookup;
  ompt_finalize_tool = reinterpret_cast<ompt_finalize_tool_t>(
      lookup("ompt_finalize_tool"));
  ompt_set_callback = reinterpret_cast<ompt_set_callback_t>(
      lookup("ompt_set_callback"));
  ompt_get_callback = reinterpret_cast<ompt_get_callback_t>(
      lookup("ompt_get_callback"));
  ompt_get_task_info = reinterpret_cast<ompt_get_task_info_t>(
      lookup("ompt_get_task_info"));
  ompt_get_task_memory = reinterpret_cast<ompt_get_task_memory_t>(
      lookup("ompt_get_task_memory"));
  ompt_get_thread_data = reinterpret_cast<ompt_get_thread_data_t>(
      lookup("ompt_get_thread_data"));
  ompt_get_parallel_info = reinterpret_cast<ompt_get_parallel_info_t>(
      lookup("ompt_get_parallel_info"));
  ompt_get_unique_id = reinterpret_cast<ompt_get_unique_id_t>(
      lookup("ompt_get_unique_id"));
  ompt_get_num_places = reinterpret_cast<ompt_get_num_places_t>(
      lookup("ompt_get_num_places"));
  ompt_get_num_devices = reinterpret_cast<ompt_get_num_devices_t>(
      lookup("ompt_get_num_devices"));
  ompt_get_num_procs = reinterpret_cast<ompt_get_num_procs_t>(
      lookup("ompt_get_num_procs"));
  ompt_get_place_proc_ids = reinterpret_cast<ompt_get_place_proc_ids_t>(
      lookup("ompt_get_place_proc_ids"));
  ompt_get_place_num = reinterpret_cast<ompt_get_place_num_t>(
      lookup("ompt_get_place_num"));
  ompt_get_partition_place_nums = reinterpret_cast<ompt_get_partition_place_nums_t>(
      lookup("ompt_get_partition_place_nums"));
  ompt_get_proc_id = reinterpret_cast<ompt_get_proc_id_t>(
      lookup("ompt_get_proc_id"));
  ompt_get_target_info = reinterpret_cast<ompt_get_target_info_t>(
      lookup("ompt_get_target_info"));
  ompt_enumerate_states = reinterpret_cast<ompt_enumerate_states_t>(
      lookup("ompt_enumerate_states"));
  ompt_enumerate_mutex_impls = reinterpret_cast<ompt_enumerate_mutex_impls_t>(
      lookup("ompt_enumerate_mutex_impls"));
  // clang-format on

  // clang-format off
  if ((ompt_enumerate_states         == nullptr) ||
      (ompt_enumerate_mutex_impls    == nullptr) ||
      (ompt_set_callback             == nullptr) ||
      (ompt_get_callback             == nullptr) ||
      (ompt_get_thread_data          == nullptr) ||
      (ompt_get_num_procs            == nullptr) ||
      (ompt_get_num_places           == nullptr) ||
      (ompt_get_place_proc_ids       == nullptr) ||
      (ompt_get_place_num            == nullptr) ||
      (ompt_get_partition_place_nums == nullptr) ||
      (ompt_get_proc_id              == nullptr) ||
      (ompt_get_parallel_info        == nullptr) ||
      (ompt_get_task_info            == nullptr) ||
      (ompt_get_task_memory          == nullptr) ||
      (ompt_get_target_info          == nullptr) ||
      (ompt_get_num_devices          == nullptr) ||
      (ompt_get_unique_id            == nullptr) ||
      (ompt_finalize_tool            == nullptr) ||
      (ompt_function_lookup          == nullptr)) {
    // clang-format on
    // OpenMP API Specification 5.2 Section 19.6.3
    // "If the provided function name is unknown to the OpenMP implementation,
    // the function returns NULL. In a compliant implementation, the lookup
    // function provided by the tool initializer for the OMPT callback interface
    // returns a valid function pointer for any OMPT runtime entry point name
    // listed in Table 19.1."
    std::cerr << "error: non-compliant OpenMP implementation\n";
    return 0;
  }

  ompt_set_result_t result = ompt_set_error;
  result = ompt_set_callback(
      ompt_callback_target_data_op_emi,
      reinterpret_cast<ompt_callback_t>(on_ompt_callback_target_data_op_emi));
  if (result != ompt_set_always) {
    return 0;
  }
  result = ompt_set_callback(
      ompt_callback_target_emi,
      reinterpret_cast<ompt_callback_t>(on_ompt_callback_target_emi));
  if (result != ompt_set_always) {
    return 0;
  }

  s_start_time = steady_clock::now();
  return 1;
}
#include <algorithm>
void ompt_finalize(ompt_data_t *data) {
  s_end_time = steady_clock::now();

  const steady_clock::time_point analysis_start = steady_clock::now();

  // total program execution time
  const duration<uint64_t, std::nano> exec_time = s_end_time - s_start_time;
  const int num_devices = ompt_get_num_devices();

  // ensure that event logs are in chronological order
  std::sort(s_target_log_ptr->begin(), s_target_log_ptr->end(),
            [](const target_info_t &a, const target_info_t &b) {
              if (a.start_time != b.start_time) {
                return a.start_time < b.start_time;
              }
              return a.end_time < b.end_time;
            });
  std::sort(s_data_op_log_ptr->begin(), s_data_op_log_ptr->end(),
            [](const data_op_info_t &a, const data_op_info_t &b) {
              if (a.start_time != b.start_time) {
                return a.start_time < b.start_time;
              }
              return a.end_time < b.end_time;
            });

  Symbolizer symbolizer;
  analyze_inefficient_transfers(symbolizer, s_target_log_ptr, s_data_op_log_ptr,
                                exec_time, num_devices);
  analyze_codeptr_durations(symbolizer, s_data_op_log_ptr, exec_time);
  print_summary(s_data_op_log_ptr, exec_time);
#ifdef ENABLE_COLLISION_CHECKING
  print_collision_summary(s_collision_map_ptr);
  free_data(s_collision_map_ptr);
#endif
#ifdef MEASURE_HASHING_OVERHEAD
  print_hash_overhead_summary(s_data_op_log_ptr, s_hash_overhead);
#endif
#ifdef PRINT_TRANSFER_RATE
  print_transfer_rate_summary(s_data_op_log_ptr);
#endif
  const steady_clock::time_point analysis_end = steady_clock::now();
  const duration<uint64_t, std::nano> analysis_time =
      analysis_end - analysis_start;

  // clang-format off
  std::cerr << "\n  execution time "
            << format_duration(exec_time.count(), 10) << "\n";
  std::cerr <<   "  analysis time  "
            << format_duration(analysis_time.count(), 10) << "\n";
  // clang-format on

  if (symbolizer.has_errmsg()) {
    std::cerr << "\n" << symbolizer.get_errmsg() << "\n";
  }

  delete s_target_log_ptr;
  delete s_data_op_log_ptr;
#ifdef ENABLE_COLLISION_CHECKING
  delete s_collision_map_ptr;
#endif // ENABLE_COLLISION_CHECKING

  if (data != nullptr && data->ptr != nullptr) {
    ompt_start_tool_result_t *result =
        static_cast<ompt_start_tool_result_t *>(data->ptr);
    delete result;
  }

  return;
}

extern "C" ompt_start_tool_result_t *
ompt_start_tool(unsigned int omp_version, const char *runtime_version) {
  std::cerr << "\ninfo: OpenMP OMPT interface version "
            << omp_version_to_string(omp_version) << "\n";
  std::cerr << "info: OpenMP runtime " << runtime_version << "\n";

  if (omp_version < 202011 /*5.1*/) {
    // This tool depends on the ompt_callback_target_data_op_emi_t callback
    // which was introduced in OpenMP version 5.1.

    // Since compilers progressively implement various parts of OpenMP standards
    // and can't claim support for a standard that is only partially
    // implemented, it doesn't make sense to not load this tool or to exit the
    // program entirely. Instead we just print this warning and pray that the
    // features we require are implemented.
    // Note: llvm 19 (as of 19.1.0-rc3) implements all OpenMP 4.5, almost all
    // of 5.0 and most of 5.1/2. llvm 19 DOES implement the APIs this tool
    // depends on, but it claims omp_version 201611 (5.0 preview 1) so this
    // warning is printed, though no features have been degraded.
    std::cerr << "warning: OMPDataPerf requires OMPT interface version 5.1 (or "
                 "later), but found version "
              << omp_version_to_string(omp_version)
              << ". Some features may be degraded.\n";
  }

  s_target_log_ptr = new std::vector<target_info_t>();
  s_data_op_log_ptr = new std::vector<data_op_info_t>();
#ifdef ENABLE_COLLISION_CHECKING
  s_collision_map_ptr = new std::map<HASH_T, std::set<data_info_t>>();
#endif // ENABLE_COLLISION_CHECKING
  ompt_start_tool_result_t *result = new ompt_start_tool_result_t;
  result->initialize = ompt_initialize;
  result->finalize = ompt_finalize;
  result->tool_data.value = 0;
  result->tool_data.ptr = result;

  return result;
}
