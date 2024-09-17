#include <iostream>
#include <map>
#include <mutex>

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <omp-tools.h>
#include <rapidhash.h>

#include "analyze.hh"
#include "symbolizer.hh"

using namespace std::chrono;

namespace {
/* Stores the start and end time of the target application being analyzed.
 */
steady_clock::time_point s_start_time;
steady_clock::time_point s_end_time;

/* As the user's program runs we log information about target data operations to
 * this array which is analyzed once the execution of the user's program has
 * completed.
 */
std::vector<data_op_info_t> *s_data_op_log_ptr;
std::mutex s_data_op_log_mutex;
} // namespace

static void on_ompt_callback_target_data_op_emi(
    ompt_scope_endpoint_t endpoint, ompt_data_t *target_task_data,
    ompt_data_t *target_data, ompt_id_t *host_op_id,
    ompt_target_data_op_t optype, void *src_addr, int src_device_num,
    void *dest_addr, int dest_device_num, size_t bytes,
    const void *codeptr_ra) {
  // get time
  const steady_clock::time_point time_now = steady_clock::now();

  // the index of the last synchronous entry into s_data_op_log for each thread
  static thread_local steady_clock::time_point s_sync_data_op_start_time =
      steady_clock::time_point();
  // used to time asynchronous data op
  static thread_local std::map<
      std::pair<int /*dest_device_num*/, const void * /*dest_addr*/>,
      steady_clock::time_point /*start_time*/>
      s_async_data_op_start_times;

  bool is_async = is_async_op(optype);

  // async operations have not been tested, as they are not emitted by llvm's
  // openmp runtime implementation as of llvm 19.1.0-rc3. However, I have
  // written the code to handle these operations. This assertion just serves as
  // a reminder to test this once an openmp runtime actually implements this.
  assert(!is_async);

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
    uint64_t hash = 0;
    switch (optype) {
    case ompt_target_data_transfer_to_device:
    case ompt_target_data_transfer_to_device_async:
      hash = rapidhash(src_addr, bytes);
      break;
    case ompt_target_data_transfer_from_device:
    case ompt_target_data_transfer_from_device_async:
      hash = rapidhash(dest_addr, bytes);
      break;
    default:
      break;
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

  } else {
    std::cerr << "warning: unknown ompt target data op. Not profiling optype="
              << std::dec << optype << "\n";
    assert(false);
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

  ompt_set_callback_t ompt_set_callback =
      reinterpret_cast<ompt_set_callback_t>(lookup("ompt_set_callback"));
  if (ompt_set_callback == nullptr) {
    std::cerr << "error: unable to create OpenMP region collector\n";
    return 0;
  }

  ompt_set_result_t result = ompt_set_error;
  result = ompt_set_callback(
      ompt_callback_target_data_op_emi,
      reinterpret_cast<ompt_callback_t>(on_ompt_callback_target_data_op_emi));

  if (result != ompt_set_always) {
    return 0;
  }

  s_start_time = steady_clock::now();
  return 1;
}

void ompt_finalize(ompt_data_t *data) {
  s_end_time = steady_clock::now();
  // total program execution time
  const duration<uint64_t, std::nano> exec_time = s_end_time - s_start_time;

  const steady_clock::time_point analysis_start = steady_clock::now();
  Symbolizer symbolizer;
  analyze_redundant_transfers(symbolizer, s_data_op_log_ptr, exec_time);
  analyze_codeptr_durations(symbolizer, s_data_op_log_ptr, exec_time);
  print_summary(s_data_op_log_ptr, exec_time);
  const steady_clock::time_point analysis_end = steady_clock::now();
  const duration<uint64_t, std::nano> analysis_time =
      analysis_end - analysis_start;

  // clang-format off
  std::cerr << "\n  execution time "
            << format_duration(exec_time.count(), 10) << "\n";
  std::cerr << "  analysis time  "
            << format_duration(analysis_time.count(), 10) << "\n";
  // clang-format on

  if (symbolizer.has_errmsg()) {
    std::cerr << "\n" << symbolizer.get_errmsg() << "\n";
  }

  delete s_data_op_log_ptr;

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
    std::cerr << "warning: OMPDataProf requires OMPT interface version 5.1 (or "
                 "later), but found version "
              << omp_version_to_string(omp_version)
              << ". Some features may be degraded.\n";
  }

  s_data_op_log_ptr = new std::vector<data_op_info_t>();
  ompt_start_tool_result_t *result = new ompt_start_tool_result_t;
  result->initialize = ompt_initialize;
  result->finalize = ompt_finalize;
  result->tool_data.value = 0;
  result->tool_data.ptr = result;

  return result;
}
