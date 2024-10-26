#pragma once

#include <chrono>
#include <set>
#include <vector>

#ifdef ENABLE_COLLISION_CHECKING
#include <cassert>
#include <map>
#include <string.h>
#endif // ENABLE_COLLISION_CHECKING

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
std::string format_device_num(int num_devices, int device_num, int width);

std::string optype_to_string(ompt_target_data_op_t optype);
std::string omp_version_to_string(unsigned int omp_version);

void print_duplicate_transfers(
    Symbolizer &symbolizer,
    const std::set<
        std::pair<std::chrono::duration<uint64_t, std::nano> /*total_time*/,
                  const std::vector<const data_op_info_t *> *>>
        &duplicate_transfers_durations,
    std::chrono::duration<uint64_t, std::nano> exec_time, int num_devices);
void print_round_trip_transfers(
    Symbolizer &symbolizer,
    const std::set<
        std::pair<std::chrono::duration<uint64_t, std::nano> /*total_time*/,
                  const std::vector<std::pair<const data_op_info_t *,
                                              const data_op_info_t *>> *>>
        &round_trip_durations,
    std::chrono::duration<uint64_t, std::nano> exec_time, int num_devices);
void print_repeated_allocs(
    Symbolizer &symbolizer,
    const std::set<
        std::pair<std::chrono::duration<uint64_t, std::nano> /*total_time*/,
                  const std::vector<std::pair<const data_op_info_t *,
                                              const data_op_info_t *>> *>>
        &repeated_alloc_durations,
    std::chrono::duration<uint64_t, std::nano> exec_time, int num_devices);
void print_potential_resource_savings(
    const std::set<
        std::pair<std::chrono::duration<uint64_t, std::nano> /*total_time*/,
                  const std::vector<const data_op_info_t *> *>>
        &duplicate_transfers_durations,
    const std::set<
        std::pair<std::chrono::duration<uint64_t, std::nano> /*total_time*/,
                  const std::vector<std::pair<const data_op_info_t *,
                                              const data_op_info_t *>> *>>
        &round_trip_durations,
    const std::set<
        std::pair<std::chrono::duration<uint64_t, std::nano> /*total_time*/,
                  const std::vector<std::pair<const data_op_info_t *,
                                              const data_op_info_t *>> *>>
        &repeated_alloc_durations,
    std::chrono::duration<uint64_t, std::nano> exec_time);
void analyze_redundant_transfers(
    Symbolizer &symbolizer, const std::vector<data_op_info_t> *data_op_log_ptr,
    std::chrono::duration<uint64_t, std::nano> exec_time, int num_devices);
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

#ifdef ENABLE_COLLISION_CHECKING

typedef struct data_info {
  void *data;
  size_t bytes;

  bool operator<=>(const struct data_info &other) const {
    assert(data != nullptr && other.data != nullptr);
    if (bytes != other.bytes) {
      return bytes - other.bytes;
    }
    return memcmp(data, other.data, bytes);
  }
  bool operator==(const struct data_info &other) const {
    assert(data != nullptr && other.data != nullptr);
    return bytes == other.bytes && memcmp(data, other.data, bytes) == 0;
  }
} data_info_t;

void try_collision_map_insert(
    std::map<uint64_t /*hash*/, std::set<data_info_t>> *collision_map_ptr,
    uint64_t hash, void *data, size_t bytes);
void print_collision_summary(
    const std::map<uint64_t /*hash*/, std::set<data_info_t>>
        *collision_map_ptr);
void free_data(const std::map<uint64_t /*hash*/, std::set<data_info_t>>
                   *collision_map_ptr);

#endif // ENABLE_COLLISION_CHECKING
