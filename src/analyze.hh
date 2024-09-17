#pragma once

#include <chrono>
#include <set>
#include <vector>

#include <omp-tools.h>

#include "symbolizer.hh"

/* Data structure used to store details about each data transfer event.
 */
typedef struct data_op_info {
  ompt_target_data_op_t optype;
  void *src_addr;
  void *dest_addr;
  int src_device_num;
  int dest_device_num;
  size_t bytes;
  const void *codeptr_ra;
  std::chrono::steady_clock::time_point start_time;
  std::chrono::steady_clock::time_point end_time;
  uint64_t hash; // hash of transferred data, unused for alloc/delete
} data_op_info_t;

inline bool is_alloc_op(ompt_target_data_op_t optype) {
  return (optype == ompt_target_data_alloc) ||
         (optype == ompt_target_data_alloc_async);
}

inline bool is_transfer_to_op(ompt_target_data_op_t optype) {
  return (optype == ompt_target_data_transfer_to_device) ||
         (optype == ompt_target_data_transfer_to_device_async);
}

inline bool is_transfer_from_op(ompt_target_data_op_t optype) {
  return (optype == ompt_target_data_transfer_from_device) ||
         (optype == ompt_target_data_transfer_from_device_async);
}

inline bool is_delete_op(ompt_target_data_op_t optype) {
  return (optype == ompt_target_data_delete) ||
         (optype == ompt_target_data_delete_async);
}

inline bool is_transfer_op(ompt_target_data_op_t optype) {
  return (optype == ompt_target_data_transfer_to_device) ||
         (optype == ompt_target_data_transfer_from_device) ||
         (optype == ompt_target_data_transfer_to_device_async) ||
         (optype == ompt_target_data_transfer_from_device_async);
}

inline bool is_async_op(ompt_target_data_op_t optype) {
  return (optype == ompt_target_data_alloc_async) ||
         (optype == ompt_target_data_transfer_to_device_async) ||
         (optype == ompt_target_data_transfer_from_device_async) ||
         (optype == ompt_target_data_delete_async);
}

std::string format_uint(uint64_t value, int width);
std::string format_percent(float percent, int width);
std::string format_duration(uint64_t ns, int width);
std::string format_optype(ompt_target_data_op_t optype, int width);
std::string format_symbol(Symbolizer &symbolizer, const void *codeptr_ra);
std::string format_device_num(int device_num, int width);

std::string optype_to_string(ompt_target_data_op_t optype);
std::string omp_version_to_string(unsigned int omp_version);

void print_duplicate_transfers(
    Symbolizer &symbolizer, const std::vector<data_op_info_t> *data_op_log_ptr,
    const std::set<
        std::pair<std::chrono::duration<uint64_t, std::nano> /*total_time*/,
                  const std::vector<const data_op_info_t *> *>>
        &duplicate_transfers_durations,
    std::chrono::duration<uint64_t, std::nano> exec_time);
void print_round_trip_transfers(
    Symbolizer &symbolizer,
    const std::set<
        std::tuple<std::chrono::duration<uint64_t, std::nano> /*total_time*/,
                   const data_op_info_t *, const data_op_info_t *>>
        &round_trip_durations,
    std::chrono::duration<uint64_t, std::nano> exec_time);
void print_potential_resource_savings(
    const std::set<
        std::pair<std::chrono::duration<uint64_t, std::nano> /*total_time*/,
                  const std::vector<const data_op_info_t *> *>>
        &duplicate_transfers_durations,
    const std::set<
        std::tuple<std::chrono::duration<uint64_t, std::nano> /*total_time*/,
                   const data_op_info_t *, const data_op_info_t *>>
        &round_trip_durations,
    std::chrono::duration<uint64_t, std::nano> exec_time);
void analyze_redundant_transfers(
    Symbolizer &symbolizer, const std::vector<data_op_info_t> *data_op_log_ptr,
    std::chrono::duration<uint64_t, std::nano> exec_time);
void print_codeptr_durations(
    Symbolizer &symbolizer,
    const std::set<
        std::pair<std::chrono::duration<uint64_t, std::nano> /*total_time*/,
                  const std::vector<const data_op_info_t *> *>>
        &codeptr_durations,
    std::chrono::duration<uint64_t, std::nano> exec_time);
void analyze_codeptr_durations(
    Symbolizer &symbolizer, const std::vector<data_op_info_t> *data_op_log_ptr,
    std::chrono::duration<uint64_t, std::nano> exec_time);
void print_summary(const std::vector<data_op_info_t> *data_op_log_ptr,
                   std::chrono::duration<uint64_t, std::nano> exec_time);
