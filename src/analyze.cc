#include "analyze.hh"

#include <cassert>
#include <cmath>
#include <deque>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>

using namespace std::chrono;

// OUTPUT FORMATTING CONSTANTS
// max maximum number of profiling results to display
constexpr size_t f_list_len = 24;
constexpr size_t f_sublist_len = 8; // max length sub lists
// column widths
constexpr int f_w = 10;           // column width
constexpr int f_w_bytes = 13;     // column width for bytes
constexpr int f_w_device_id = 13; // column width for device ids
constexpr int f_w_optype = 21;    // column width for optype

float round_to(float value, float precision = 1.0) {
  return std::roundf(value / precision) * precision;
}

std::string format_uint(uint64_t value, int width) {
  assert(width > 0);
  std::ostringstream oss;
  oss << std::setw(width) << value;
  return oss.str();
}

std::string format_percent(float percent, int width) {
  assert(width > 1);
  std::ostringstream oss;
  constexpr int decimals = 2;
  constexpr float precision = 0.01; // pow is not constexpr, so hard coded :(
  percent *= 100;
  percent = round_to(percent, precision);
  oss << std::setw(width - 1) << std::fixed << std::showpoint
      << std::setprecision(decimals) << percent << "%";
  return oss.str();
}

std::string format_duration(uint64_t ns, int width) {
  assert(width > 2);
  std::ostringstream oss;

  // Decide the best unit for the duration and print with a fixed width
  oss << std::setprecision(5);
  if (ns >= 1'000'000'000) {
    const float sec = ns / 1'000'000'000.f;
    oss << std::setw(width - 1) << sec << "s";
  } else if (ns >= 1'000'000) {
    const float ms = ns / 1'000'000.f;
    oss << std::setw(width - 2) << ms << "ms";
  } else if (ns >= 1'000) {
    const float us = ns / 1'000.f;
    oss << std::setw(width - 2) << us << "µs";
  } else {
    oss << std::setw(width - 2) << ns << "ns";
  }

  return oss.str();
}

std::string optype_to_string(ompt_target_data_op_t optype) {
  switch (optype) {
  case ompt_target_data_alloc:
    return "alloc";
  case ompt_target_data_transfer_to_device:
    return "to device";
  case ompt_target_data_transfer_from_device:
    return "from device";
  case ompt_target_data_delete:
    return "delete";
  case ompt_target_data_associate:
    return "associate";
  case ompt_target_data_disassociate:
    return "disassociate";
  case ompt_target_data_alloc_async:
    return "alloc (async)";
  case ompt_target_data_transfer_to_device_async:
    return "to device (async)";
  case ompt_target_data_transfer_from_device_async:
    return "from device (async)";
  case ompt_target_data_delete_async:
    return "delete (async)";
  default:
    assert(false);
    return "unknown";
  }
}

std::string format_optype(ompt_target_data_op_t optype, int width) {
  assert(width > 19);
  std::ostringstream oss;
  oss << "  " << std::left << std::setw(width - 2) << optype_to_string(optype);
  return oss.str();
}

std::string format_symbol(Symbolizer &symbolizer, const void *codeptr_ra) {
  if (codeptr_ra == nullptr) {
    assert(false);
    return "  ";
  }
  if (!symbolizer.is_valid()) {
    return "  <symbolizer error>";
  }
  std::ostringstream oss;
  const char *symbol = nullptr;
  // const char *filename = nullptr;
  int lineno = 0;
  // int colno = 0;
  symbolizer.info(codeptr_ra, &symbol, nullptr, &lineno, nullptr);
  if (symbol == nullptr) {
    return "  <optimized out>";
  }
  oss << "  " << symbolizer.demangle(symbol) << ":";
  if (lineno > 0) {
    oss << lineno;
  } else {
    oss << "<optimized out>";
  }
  return oss.str();
}

std::string format_device_num(int num_devices, int device_num, int width) {
  assert(width > 9);
  std::ostringstream oss;
  oss << std::left << std::setw(width);
  // OpenMP API Specification 5.2 Section 18.7.7
  // "The effect of this routine is to return the device number of the host
  // device. The value of the device number is the value returned by the
  // omp_get_num_devices routine."
  if (device_num == num_devices) {
    oss << "  host";
  } else {
    oss << ("  device " + std::to_string(device_num));
  }
  return oss.str();
}

std::string omp_version_to_string(unsigned int omp_version) {
  switch (omp_version) {
  case 199710:
    return "FORTRAN version 1.0";
  case 199810:
    return "C/C++ version 1.0";
  case 199911:
    return "FORTRAN version 1.1";
  case 200011:
    return "FORTRAN version 2.0";
  case 200203:
    return "C/C++ version 2.0";
  case 200505:
    return "2.5";
  case 200805:
    return "3.0";
  case 201107:
    return "3.1";
  case 201211:
    return "TR1 directives for attached accelerators";
  case 201305:
    return "TR ompt and ompd";
  case 201307:
    return "4.0";
  case 201403:
    return "TR2 ompt";
  case 201411:
    return "TR3 4.1 draft";
  case 201511:
    return "4.5";
  case 201611:
    return "TR4 5.0 preview 1";
  case 201701:
    return "TR5 memory management support for 5.0";
  case 201711:
    return "TR6 5.0 preview 2";
  case 201807:
    return "TR7 5.0 draft";
  case 201811:
    return "5.0";
  case 201911:
    return "TR8 5.1 preview 1";
  case 202008:
    return "TR9 5.1 draft";
  case 202011:
    return "5.1";
  case 202107:
    return "TR10 5.2 draft";
  case 202111:
    return "5.2";
  case 202211:
    return "TR11 6.0 preview 1";
  case 202311:
    return "TR12 6.0 preview 2";
  case 202408:
    return "TR13 6.0 draft";
  default:
    return std::to_string(omp_version);
  }
}

void print_duplicate_transfers(
    Symbolizer &symbolizer,
    const std::set<std::pair<duration<uint64_t, std::nano> /*total_time*/,
                             const std::vector<const data_op_info_t *> *>>
        &duplicate_transfers_durations,
    duration<uint64_t, std::nano> exec_time, int num_devices) {

  std::cerr << "\n=== OpenMP Duplicate Target Data Transfer Analysis ===\n";
  if (duplicate_transfers_durations.empty()) {
    std::cerr << "  SUCCESS - no duplicate data transfers detected\n";
    return;
  }

  size_t idx = 0;
  // clang-format off
  std::cerr << std::setw(f_w) << "time(%)"
            << std::setw(f_w) << "time"
            << std::setw(f_w) << "calls"
            << std::setw(f_w) << "avg"
            << std::setw(f_w_bytes) << "bytes"
            << std::setw(f_w) << "size"
            << std::left << std::setw(f_w_device_id) << "  dest device"
            << std::right << "   "
            << std::setw(f_w) << "calls"
            << std::left << std::setw(f_w_device_id) << "  src device"
            << std::right << "  location\n";
  // clang-format on

  // // reverse iterate since we want to display greatest times first
  for (auto it = duplicate_transfers_durations.rbegin();
       it != duplicate_transfers_durations.rend(); ++it) {
    if (idx >= f_list_len) {
      break;
    }

    const duration<uint64_t, std::nano> time = it->first;
    const std::vector<const data_op_info_t *> *info_list_ptr = it->second;
    const float time_percent = time.count() / (float)exec_time.count();
    const uint64_t calls = info_list_ptr->size();
    const duration<uint64_t, std::nano> time_avg(
        (uint64_t)std::roundf(time.count() / (float)calls));
    assert(!info_list_ptr->empty());
    const int dest_device_num = (*info_list_ptr)[0]->dest_device_num;
    const uint64_t transfer_size = (*info_list_ptr)[0]->bytes;
    const uint64_t bytes = transfer_size * info_list_ptr->size();

    // count the number of calls for each src_device and codeptr
    std::map<std::pair<int /*src_device_num*/, const void * /*codeptr_ra*/>,
             uint64_t /*calls*/>
        device_codeptr_to_calls;
    for (const data_op_info_t *entry_ptr : *info_list_ptr) {
      const int src_device_num = entry_ptr->src_device_num;
      const void *codeptr_ra = entry_ptr->codeptr_ra;
      const std::pair<int, const void *> key(src_device_num, codeptr_ra);
      device_codeptr_to_calls[key] += 1;
    }

    // sort by number of calls
    std::set<std::tuple<uint64_t /*calls*/, int /*src_device_num*/,
                        const void * /*codeptr_ra*/>>
        calls_device_codeptr;
    for (const auto &dc_c : device_codeptr_to_calls) {
      const uint64_t calls = dc_c.second;
      const int src_device_num = dc_c.first.first;
      const void *codeptr_ra = dc_c.first.second;
      calls_device_codeptr.emplace(calls, src_device_num, codeptr_ra);
    }

    // finally we can print the sub list
    size_t subidx = 0;
    assert(!calls_device_codeptr.empty());

    // reverse iterator since we want to display highest call count first
    for (auto it = calls_device_codeptr.rbegin();
         it != calls_device_codeptr.rend(); ++it) {
      if (subidx >= f_sublist_len) {
        break;
      }
      const uint64_t sub_calls = std::get<0>(*it);
      const int src_device_num = std::get<1>(*it);
      const void *codeptr_ra = std::get<2>(*it);

      if (subidx == 0) {
        // clang-format off
        std::cerr << format_percent(time_percent, f_w)
                  << format_duration(time.count(), f_w)
                  << format_uint(calls, f_w)
                  << format_duration(time_avg.count(), f_w)
                  << format_uint(bytes, f_w_bytes)
                  << format_uint(transfer_size, f_w)
                  << format_device_num(num_devices, dest_device_num,
                                       f_w_device_id);
        // clang-format on
        if (calls_device_codeptr.size() > 1) {
          std::cerr << " ┬─";
        } else {
          std::cerr << " ──";
        }
      } else {
        std::cerr << std::string(5 * f_w + f_w_bytes + f_w_device_id, ' ');
        if (calls_device_codeptr.size() > subidx + 1) {
          std::cerr << " ├─";
        } else {
          std::cerr << " └─";
        }
      }
      // clang-format off
      std::cerr << format_uint(sub_calls, f_w)
                << format_device_num(num_devices, src_device_num,
                                     f_w_device_id)
                << format_symbol(symbolizer, codeptr_ra)
                << "\n";
      // clang-format on
      ++subidx;
    }
    ++idx;
  }
  return;
}

void print_round_trip_transfers(
    Symbolizer &symbolizer,
    const std::set<
        std::pair<duration<uint64_t, std::nano> /*total_time*/,
                  const std::vector<std::pair<const data_op_info_t *,
                                              const data_op_info_t *>> *>>
        &round_trip_durations,
    duration<uint64_t, std::nano> exec_time, int num_devices) {

  size_t idx = 0;
  std::cerr << "\n=== OpenMP Round-Trip Target Data Transfer Analysis ===\n";
  if (round_trip_durations.empty()) {
    std::cerr << "  SUCCESS - no round-trip data transfers detected\n";
    return;
  }
  // clang-format off
  std::cerr << std::setw(f_w) << "time(%)"
            << std::setw(f_w) << "time"
            << std::setw(f_w) << "trips"
            << std::setw(f_w) << "avg"
            << std::setw(f_w_bytes) << "bytes"
            << std::setw(f_w) << "size"
            << "   "
            << std::left << std::setw(f_w_device_id) << "  src device"
            << std::setw(f_w_device_id) << "  dest device"
            << std::setw(f_w_optype) << "  optype"
            << std::right << "  location\n";
  // clang-format on
  // // reverse iterate since we want to display greatest times first
  for (auto it = round_trip_durations.rbegin();
       it != round_trip_durations.rend(); ++it) {
    if (idx >= f_list_len) {
      break;
    }
    const duration<uint64_t, std::nano> time = it->first;
    const data_op_info_t *tx_ptr = it->second->front().first;
    const data_op_info_t *rx_ptr = it->second->front().second;
    const float time_percent = time.count() / (float)exec_time.count();
    const uint64_t cnt = it->second->size();
    const duration<uint64_t, std::nano> time_avg(
        (uint64_t)std::roundf(time.count() / (float)cnt));
    const uint64_t transfer_size = tx_ptr->bytes;
    const uint64_t bytes = cnt * (tx_ptr->bytes + rx_ptr->bytes);
    const ompt_target_data_op_t tx_optype = tx_ptr->optype;
    const ompt_target_data_op_t rx_optype = rx_ptr->optype;
    const void *tx_codeptr_ra = tx_ptr->codeptr_ra;
    const void *rx_codeptr_ra = rx_ptr->codeptr_ra;
    const int src_device_num = tx_ptr->src_device_num;
    const int dest_device_num = tx_ptr->dest_device_num;
    // clang-format off
    std::cerr << format_percent(time_percent, f_w)
              << format_duration(time.count(), f_w)
              << format_uint(cnt, f_w)
              << format_duration(time_avg.count(), f_w)
              << format_uint(bytes, f_w_bytes)
              << format_uint(transfer_size, f_w)
              << " ┬─"
              << format_device_num(num_devices, src_device_num,
                                   f_w_device_id)
              << format_device_num(num_devices, dest_device_num,
                                   f_w_device_id)
              << format_optype(tx_optype, f_w_optype)
              << format_symbol(symbolizer, tx_codeptr_ra)
              << "\n";
    std::cerr << std::string(5 * f_w + f_w_bytes, ' ')
              << " └─"
              << format_device_num(num_devices, dest_device_num,
                                   f_w_device_id)
              << format_device_num(num_devices, src_device_num,
                                   f_w_device_id)
              << format_optype(rx_optype, f_w_optype)
              << format_symbol(symbolizer, rx_codeptr_ra)
              << "\n";
    // clang-format on
    ++idx;
  }

  return;
}

void print_repeated_allocs(
    Symbolizer &symbolizer,
    const std::set<
        std::pair<std::chrono::duration<uint64_t, std::nano> /*total_time*/,
                  const std::vector<std::pair<const data_op_info_t *,
                                              const data_op_info_t *>> *>>
        &repeated_alloc_durations,
    std::chrono::duration<uint64_t, std::nano> exec_time, int num_devices) {

  size_t idx = 0;
  std::cerr << "\n=== OpenMP Repeated Target Device Allocations Analysis ===\n";
  if (repeated_alloc_durations.empty()) {
    std::cerr << "  SUCCESS - no repeated target device allocations detected\n";
    return;
  }
  // clang-format off
  std::cerr << std::setw(f_w) << "time(%)"
            << std::setw(f_w) << "time"
            << std::setw(f_w) << "allocs"
            << std::setw(f_w) << "avg"
            << std::setw(f_w_bytes) << "bytes"
            << std::setw(f_w) << "size"
            << std::left << std::setw(f_w_device_id) << "  tgt device"
            << std::right << "   "
            << "  location\n";
  // clang-format on
  // // reverse iterate since we want to display greatest times first
  for (auto it = repeated_alloc_durations.rbegin();
       it != repeated_alloc_durations.rend(); ++it) {
    if (idx >= f_list_len) {
      break;
    }
    const duration<uint64_t, std::nano> time = it->first;
    const data_op_info_t *alloc_ptr = it->second->front().first;
    const data_op_info_t *delete_ptr = it->second->front().second;
    const float time_percent = time.count() / (float)exec_time.count();
    const uint64_t allocs = it->second->size();
    const duration<uint64_t, std::nano> time_avg(
        (uint64_t)std::roundf(time.count() / (float)allocs));
    const uint64_t transfer_size = alloc_ptr->bytes;
    const uint64_t bytes = allocs * alloc_ptr->bytes;
    const ompt_target_data_op_t alloc_optype = alloc_ptr->optype;
    const ompt_target_data_op_t delete_optype = delete_ptr->optype;
    const void *alloc_codeptr_ra = alloc_ptr->codeptr_ra;
    const void *delete_codeptr_ra = delete_ptr->codeptr_ra;
    // const int host_device_num = alloc_ptr->src_device_num;
    const int tgt_device_num = alloc_ptr->dest_device_num;
    // clang-format off
    std::cerr << format_percent(time_percent, f_w)
              << format_duration(time.count(), f_w)
              << format_uint(allocs, f_w)
              << format_duration(time_avg.count(), f_w)
              << format_uint(bytes, f_w_bytes)
              << format_uint(transfer_size, f_w)
              << format_device_num(num_devices, tgt_device_num,
                                   f_w_device_id)
              << " ┬─"
              << format_optype(alloc_optype, f_w_optype)
              << format_symbol(symbolizer, alloc_codeptr_ra)
              << "\n";
    std::cerr << std::string(5 * f_w + f_w_bytes + f_w_device_id, ' ')
              << " └─"
              << format_optype(delete_optype, f_w_optype)
              << format_symbol(symbolizer, delete_codeptr_ra)
              << "\n";
    // clang-format on
    ++idx;
  }

  return;
}

void print_potential_resource_savings(
    const std::set<std::pair<duration<uint64_t, std::nano> /*total_time*/,
                             const std::vector<const data_op_info_t *> *>>
        &duplicate_transfers_durations,
    const std::set<
        std::pair<duration<uint64_t, std::nano> /*total_time*/,
                  const std::vector<std::pair<const data_op_info_t *,
                                              const data_op_info_t *>> *>>
        &round_trip_durations,
    const std::set<
        std::pair<std::chrono::duration<uint64_t, std::nano> /*total_time*/,
                  const std::vector<std::pair<const data_op_info_t *,
                                              const data_op_info_t *>> *>>
        &repeated_alloc_durations, // TODO
    duration<uint64_t, std::nano> exec_time) {

  duration<uint64_t, std::nano> pot_dd_time(0);
  float pot_dd_time_percent = 0.f;
  uint64_t pot_dd_calls = 0;
  uint64_t pot_dd_bytes = 0;
  for (auto it = duplicate_transfers_durations.rbegin();
       it != duplicate_transfers_durations.rend(); ++it) {
    const duration<uint64_t, std::nano> time = it->first;
    const std::vector<const data_op_info_t *> *info_list_ptr = it->second;
    const uint64_t calls = info_list_ptr->size();
    const duration<uint64_t, std::nano> time_avg(
        (uint64_t)std::roundf(time.count() / (float)calls));
    assert(!info_list_ptr->empty());
    const uint64_t transfer_size = (*info_list_ptr)[0]->bytes;
    const uint64_t bytes = transfer_size * (info_list_ptr->size() - 1);

    const duration<uint64_t, std::nano> pot_time_diff = time - time_avg;
    pot_dd_time += pot_time_diff;
    pot_dd_time_percent += pot_time_diff.count() / (float)exec_time.count();
    pot_dd_calls += calls - 1;
    pot_dd_bytes += bytes - transfer_size;
  }

  duration<uint64_t, std::nano> pot_rt_time(0);
  float pot_rt_time_percent = 0.f;
  uint64_t pot_rt_calls = 0;
  // uint64_t pot_rt_bytes = 0;
  uint64_t pot_rt_unique_calls = 0;
  uint64_t pot_rt_unique_bytes = 0;

  for (auto it = round_trip_durations.rbegin();
       it != round_trip_durations.rend(); ++it) {
    // const duration<uint64_t, std::nano> time = it->first;
    // const data_op_info_t *tx_ptr = it->second->front().first;
    const data_op_info_t *rx_ptr = it->second->front().second;
    const duration<uint64_t, std::nano> pot_time_diff =
        rx_ptr->end_time - rx_ptr->start_time;
    pot_rt_time += pot_time_diff;
    pot_rt_time_percent += pot_time_diff.count() / (float)exec_time.count();
    pot_rt_calls += it->second->size();
    // pot_rt_bytes += (2 * pot_rt_calls * rx_ptr->bytes) - 1;
    // If there are multiple round trips between the same 2 devices with the
    // same hash, we only count it once, since duplicate data transfers will
    // catch that and since it helps us more easily provide a more accurate
    // potential speedup since we don't double count any types of redundant
    // data transfers.
    pot_rt_unique_calls += 1;
    pot_rt_unique_bytes += rx_ptr->bytes;
  }

  duration<uint64_t, std::nano> pot_ad_time(0);
  float pot_ad_time_percent = 0.f;
  uint64_t pot_ad_calls = 0;
  uint64_t pot_ad_bytes = 0;
  for (auto it = repeated_alloc_durations.rbegin();
       it != repeated_alloc_durations.rend(); ++it) {
    const duration<uint64_t, std::nano> time = it->first;
    const std::vector<std::pair<const data_op_info_t *, const data_op_info_t *>>
        *info_list_ptr = it->second;
    const uint64_t calls = info_list_ptr->size();
    const duration<uint64_t, std::nano> time_avg(
        (uint64_t)std::roundf(time.count() / (float)calls));
    assert(!info_list_ptr->empty());
    const uint64_t alloc_size = (*info_list_ptr)[0].first->bytes;
    const uint64_t bytes = alloc_size * (info_list_ptr->size() - 1);

    const duration<uint64_t, std::nano> pot_time_diff = time - time_avg;
    pot_ad_time += pot_time_diff;
    pot_ad_time_percent += pot_time_diff.count() / (float)exec_time.count();
    pot_ad_calls += calls - 1;
    pot_ad_bytes += bytes - alloc_size;
  }

  const duration<uint64_t, std::nano> pot_time =
      pot_dd_time + pot_rt_time + pot_ad_time;
  const float pot_time_percent =
      pot_dd_time_percent + pot_rt_time_percent + pot_ad_time_percent;
  const uint64_t pot_calls = pot_dd_calls + pot_rt_unique_calls;
  const uint64_t pot_bytes = pot_dd_bytes + pot_rt_unique_bytes;

  std::cerr << "\n  Found " << std::dec << pot_dd_calls
            << " potential duplicate data transfer(s) with "
            << duplicate_transfers_durations.size() << " unique hash(es).\n";
  std::cerr << "  Found " << std::dec << pot_rt_calls
            << " potential round trip data transfer(s).\n";
  std::cerr << "  Found " << std::dec << pot_ad_calls
            << " potential repeated device memory allocation(s).\n";

  std::cerr << "  Potential Resource Savings\n";
  constexpr int w = std::max(f_w, f_w_bytes);
  // clang-format off
  std::cerr <<   "    time(%)           "
            << format_percent(pot_time_percent, w)
            << "\n    time              "
            << format_duration(pot_time.count(), w)
            << "\n    data transfers    "
            << format_uint(pot_calls, w)
            << "\n    bytes transferred "
            << format_uint(pot_bytes, w)
            << "\n    allocations       "
            << format_uint(pot_ad_calls, w)
            << "\n    bytes allocated   "
            << format_uint(pot_ad_bytes, w)
            << "\n";
  // clang-format on
  return;
}

void analyze_redundant_transfers(
    Symbolizer &symbolizer, const std::vector<data_op_info_t> *data_op_log_ptr,
    duration<uint64_t, std::nano> exec_time, int num_devices) {
  // Map hashes and device_num of received data to the data op infos.
  // Create 2 received one for duplicate data transfers and one for round-trips,
  // since the round-trip algorithm needs to modify its.
  std::map<std::pair<HASH_T, int /*dest_device_num*/>,
           std::vector<const data_op_info_t *>>
      received_dd;
  std::map<std::pair<HASH_T, int /*dest_device_num*/>,
           std::deque<const data_op_info_t *>>
      received_rt;
  std::set<std::pair<duration<uint64_t, std::nano> /*total_time*/,
                     const std::vector<const data_op_info_t *> *>>
      duplicate_transfers_durations;

  for (const data_op_info_t &entry : *data_op_log_ptr) {
    if (!is_transfer_op(entry.optype)) {
      continue;
    }
    const std::pair<HASH_T, int> key(entry.hash, entry.dest_device_num);
    received_dd[key].push_back(&entry);
    received_rt[key].push_back(&entry);
  }

  for (auto &entry : received_dd) {
    std::vector<const data_op_info_t *> *duplicate_transfers_ptr =
        &entry.second;
    if (duplicate_transfers_ptr->size() < 2) {
      // not a duplicate transfer if it was unique hash
      continue;
    }
    duration<uint64_t, std::nano> duration(0);
    for (const data_op_info_t *transfer_ptr : *duplicate_transfers_ptr) {
      duration += transfer_ptr->end_time - transfer_ptr->start_time;
    }
    duplicate_transfers_durations.emplace(duration, duplicate_transfers_ptr);
  }

  print_duplicate_transfers(symbolizer, duplicate_transfers_durations,
                            exec_time, num_devices);

  // _Round Trip Transfers_ are when data is transferred then the same data is
  // transferred back (unmodified).
  std::map<
      std::tuple<HASH_T, int /*src_device_num*/, int /*dest_device_num*/>,
      std::vector<std::pair<const data_op_info_t *, const data_op_info_t *>>>
      round_trip_transfers;
  for (const data_op_info_t &tx_entry : *data_op_log_ptr) {
    if (!is_transfer_op(tx_entry.optype)) {
      continue;
    }

    // Check if this data is later received by this device. If so, this is a
    // candidate for a round trip transfer.
    const std::pair<HASH_T, int> rx_key(tx_entry.hash, tx_entry.src_device_num);
    const auto &rx_it = received_rt.find(rx_key);
    if (rx_it == received_rt.end() || rx_it->second.empty()) {
      // the round-trip is never completed, the data is never sent back
      continue;
    }
    const std::tuple<HASH_T, int, int> trip_key(
        tx_entry.hash, tx_entry.src_device_num, tx_entry.dest_device_num);
    const data_op_info_t *rx_entry = rx_it->second.front();
    const std::pair<const data_op_info_t *, const data_op_info_t *> tx_rx(
        &tx_entry, rx_entry);
    round_trip_transfers[trip_key].emplace_back(tx_rx);
    const std::pair<HASH_T, int> tx_key(tx_entry.hash,
                                        tx_entry.dest_device_num);
    const auto &tx_it = received_rt.find(tx_key);
#ifdef DEBUG
    const data_op_info_t *_tx_entry = tx_it->second.front();
    assert(_tx_entry == &tx_entry);
#endif // DEBUG

    tx_it->second.pop_front(); // Remove so that this is not falsely counted as
                               // a completion for other round-trips.
  }

  std::set<std::pair<duration<uint64_t, std::nano> /*total_time*/,
                     const std::vector<std::pair<const data_op_info_t *,
                                                 const data_op_info_t *>> *>>
      round_trip_durations;
  for (const auto &entry : round_trip_transfers) {
    duration<uint64_t, std::nano> tx_duration(0);
    duration<uint64_t, std::nano> rx_duration(0);
    for (const auto [rx_ptr, tx_ptr] : entry.second) {
      tx_duration += tx_ptr->end_time - tx_ptr->start_time;
      rx_duration += rx_ptr->end_time - rx_ptr->start_time;
    }
    const duration<uint64_t, std::nano> trip_duration =
        tx_duration + rx_duration;
    round_trip_durations.emplace(trip_duration, &entry.second);
  }

  print_round_trip_transfers(symbolizer, round_trip_durations, exec_time,
                             num_devices);

  // _Repeated Device Memory Allocations_ are when data is repeatedly
  // reallocated.
  std::map<std::tuple<void * /*host_addr*/, int /*tgt_device_num*/,
                      size_t /*bytes*/>,
           std::vector<std::pair<const data_op_info_t * /*alloc*/,
                                 const data_op_info_t * /*delete*/>>>
      repeated_allocs;
  std::map<std::pair<void * /*tgt_addr*/, int /*tgt_device_num*/>,
           const data_op_info_t * /*alloc*/>
      current_allocs;

  for (const data_op_info_t &entry : *data_op_log_ptr) {
    if (is_alloc_op(entry.optype)) {
      std::pair<void *, int> akey(entry.dest_addr, entry.dest_device_num);
      current_allocs[akey] = &entry;
    } else if (is_delete_op(entry.optype)) {
      std::pair<void *, int> akey(entry.src_addr, entry.src_device_num);
      const data_op_info_t *alloc_entry = current_allocs.extract(akey).mapped();
      std::tuple<void *, int, size_t> rkey(alloc_entry->src_addr,
                                           alloc_entry->dest_device_num,
                                           alloc_entry->bytes);
      repeated_allocs[rkey].emplace_back(alloc_entry, &entry);
    } else {
      continue;
    }
  }

  for (auto it = repeated_allocs.begin(); it != repeated_allocs.end();) {
    if (it->second.size() < 2) {
      it = repeated_allocs.erase(it); // erase returns the next iterator
    } else {
      ++it; // only increment if not erasing
    }
  }

  std::set<std::pair<
      duration<uint64_t, std::nano> /*total_time*/,
      const std::vector<std::pair<const data_op_info_t * /*alloc*/,
                                  const data_op_info_t * /*delete*/>> *>>
      repeated_alloc_durations;
  for (const auto &entry : repeated_allocs) {
    duration<uint64_t, std::nano> alloc_duration(0);
    duration<uint64_t, std::nano> delete_duration(0);
    for (const auto [alloc_ptr, delete_ptr] : entry.second) {
      alloc_duration += alloc_ptr->end_time - alloc_ptr->start_time;
      delete_duration += delete_ptr->end_time - delete_ptr->start_time;
    }
    const duration<uint64_t, std::nano> total_duration =
        alloc_duration + delete_duration;
    repeated_alloc_durations.emplace(total_duration, &entry.second);
  }
  print_repeated_allocs(symbolizer, repeated_alloc_durations, exec_time,
                        num_devices);

  print_potential_resource_savings(duplicate_transfers_durations,
                                   round_trip_durations,
                                   repeated_alloc_durations, exec_time);
  return;
}

void print_codeptr_durations(
    Symbolizer &symbolizer,
    const std::set<std::pair<duration<uint64_t, std::nano> /*total_time*/,
                             const std::vector<const data_op_info_t *> *>>
        &codeptr_durations,
    duration<uint64_t, std::nano> exec_time) {

  size_t idx = 0;
  std::cerr << "\n=== OpenMP Target Data Operations Profiling Results ===\n";
  if (codeptr_durations.empty()) {
    std::cerr << "  no data operations profiled\n";
    return;
  }
  // clang-format off
  std::cerr << std::setw(f_w) << "time(%)"
            << std::setw(f_w) << "time"
            << std::setw(f_w) << "calls"
            << std::setw(f_w) << "avg"
            << std::setw(f_w) << "min"
            << std::setw(f_w) << "max"
            << std::setw(f_w_bytes) << "bytes"
            << std::left << std::setw(f_w_optype) << "  optype"
            << std::right << "  location\n";
  // clang-format on
  // // reverse iterate since we want to display greatest times first
  for (auto it = codeptr_durations.rbegin(); it != codeptr_durations.rend();
       ++it) {
    if (idx >= f_list_len) {
      break;
    }
    const duration<uint64_t, std::nano> time = it->first;
    const std::vector<const data_op_info_t *> *info_list_ptr = it->second;
    const float time_percent = time.count() / (float)exec_time.count();
    const uint64_t calls = info_list_ptr->size();
    const duration<uint64_t, std::nano> time_avg(
        (uint64_t)std::roundf(time.count() / (float)calls));
    duration<uint64_t, std::nano> time_min(UINT64_MAX);
    duration<uint64_t, std::nano> time_max(0);
    uint64_t bytes = 0;
    assert(!info_list_ptr->empty());
    const ompt_target_data_op_t optype = (*info_list_ptr)[0]->optype;
    const void *codeptr_ra = (*info_list_ptr)[0]->codeptr_ra;
    for (const data_op_info_t *entry_ptr : *info_list_ptr) {
      const duration<uint64_t, std::nano> entry_duration =
          (entry_ptr->end_time - entry_ptr->start_time);
      if (entry_duration < time_min) {
        time_min = entry_duration;
      }
      if (entry_duration > time_max) {
        time_max = entry_duration;
      }
      bytes += entry_ptr->bytes;
    }
    // clang-format off
    std::cerr << format_percent(time_percent, f_w)
              << format_duration(time.count(), f_w)
              << format_uint(calls, f_w)
              << format_duration(time_avg.count(), f_w)
              << format_duration(time_min.count(), f_w)
              << format_duration(time_max.count(), f_w)
              << format_uint(bytes, f_w_bytes)
              << format_optype(optype, f_w_optype)
              << format_symbol(symbolizer, codeptr_ra)
              << "\n";
    // clang-format on
    ++idx;
  }

  return;
}

void analyze_codeptr_durations(
    Symbolizer &symbolizer, const std::vector<data_op_info_t> *data_op_log_ptr,
    duration<uint64_t, std::nano> exec_time) {
  // for identifying most expensive code
  std::map<
      std::pair<const void * /*codeptr_ra*/, ompt_target_data_op_t /*optype*/>,
      std::vector<const data_op_info_t *>>
      codeptr_to_data_op;
  std::set<std::pair<duration<uint64_t, std::nano> /*total_time*/,
                     const std::vector<const data_op_info_t *> *>>
      codeptr_durations;

  for (const data_op_info_t &entry : *data_op_log_ptr) {
    const std::pair<const void *, ompt_target_data_op_t> key(entry.codeptr_ra,
                                                             entry.optype);
    codeptr_to_data_op[key].push_back(&entry);
  }
  for (auto &entry : codeptr_to_data_op) {
    const std::vector<const data_op_info_t *> *info_list_ptr = &entry.second;
    duration<uint64_t, std::nano> duration(0);
    assert(!info_list_ptr->empty());
    for (const data_op_info_t *info_ptr : *info_list_ptr) {
      duration += info_ptr->end_time - info_ptr->start_time;
    }
    codeptr_durations.emplace(duration, info_list_ptr);
  }

  print_codeptr_durations(symbolizer, codeptr_durations, exec_time);
  return;
}

void print_summary(const std::vector<data_op_info_t> *data_op_log_ptr,
                   duration<uint64_t, std::nano> exec_time) {
  std::map<ompt_target_data_op_t, duration<uint64_t, std::nano>> op_time_map;
  std::map<ompt_target_data_op_t, uint64_t> op_bytes_map;
  std::map<ompt_target_data_op_t, uint64_t> op_calls_map;
  for (const data_op_info_t &entry : *data_op_log_ptr) {
    const ompt_target_data_op_t optype = entry.optype;
    op_time_map[optype] += entry.end_time - entry.start_time;
    op_bytes_map[optype] += entry.bytes;
    op_calls_map[optype] += 1;
  }

  // rank optypes by execution time
  std::set<std::pair<duration<uint64_t, std::nano>, ompt_target_data_op_t>>
      time_op_set;
  for (const auto &[optype, time] : op_time_map) {
    time_op_set.emplace(time, optype);
  }

  std::cerr << "\n=== OpenMP Target Data Operations Timing Summary ===\n";
  if (data_op_log_ptr->empty()) {
    std::cerr << "  no data operations profiled\n";
    return;
  }
  // clang-format off
  std::cerr << std::setw(f_w) << "time(%)"
            << std::setw(f_w) << "time"
            << std::setw(f_w) << "calls"
            << std::setw(f_w_bytes) << "bytes"
            << std::left << std::setw(f_w_optype) << "  optype"
            << std::right << "\n";
  // clang-format on

  // reverse iterate since we want to display greatest times first
  for (auto it = time_op_set.rbegin(); it != time_op_set.rend(); ++it) {
    const duration<uint64_t, std::nano> time = it->first;
    const ompt_target_data_op_t optype = it->second;
    const float time_percent = time.count() / (float)exec_time.count();
    const uint64_t calls = op_calls_map[optype];
    const uint64_t bytes = op_bytes_map[optype];
    // clang-format off
    std::cerr << format_percent(time_percent, f_w)
              << format_duration(time.count(), f_w)
              << format_uint(calls, f_w)
              << format_uint(bytes, f_w_bytes)
              << format_optype(optype, f_w_optype)
              << "\n";
    // clang-format on
  }
  return;
}

#ifdef ENABLE_COLLISION_CHECKING
void print_collision_summary(
    const std::map<HASH_T, std::set<data_info_t>> *collision_map_ptr) {

  // count collisions
  uint64_t num_collisions = 0;
  for (const auto &hash_set : *collision_map_ptr) {
    assert(!hash_set.second.empty());
    num_collisions += hash_set.second.size() - 1;
  }

  const uint64_t num_unique_hashes = collision_map_ptr->size();
  float percent_collisions = 0.f;
  if (num_unique_hashes > 0) {
    percent_collisions = num_collisions / (float)num_unique_hashes;
  }
  std::ostringstream percent_collisions_oss;
  percent_collisions_oss << std::setprecision(2)
                         << round_to(percent_collisions, 0.01) << "%";
  std::cerr << "\nFound " << std::dec << num_collisions << " collisions out of "
            << num_unique_hashes << " total hashes for a collisions rate of "
            << percent_collisions_oss.str() << ".\n";
  return;
}

void free_data(
    const std::map<HASH_T, std::set<data_info_t>> *collision_map_ptr) {
  for (const auto &hash_set : *collision_map_ptr) {
    for (const data_info_t &entry : hash_set.second) {
      if (entry.data == nullptr) {
        continue;
      }
      delete[] static_cast<char *>(entry.data);
    }
  }
  return;
}
#endif
