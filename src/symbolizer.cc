#include "symbolizer.hh"

#include <cstring>
#include <cxxabi.h>
#include <iostream>
#include <sstream>
#include <unistd.h>

#include <elfutils/libdwfl.h>

namespace {
const Dwfl_Callbacks s_callbacks = {
    /* find_elf        = */ &dwfl_linux_proc_find_elf,
    /* find_debuginfo  = */ &dwfl_standard_find_debuginfo,
    /* section_address = */ &dwfl_offline_section_address,
    /* debuginfo_path  = */ nullptr};
}

Symbolizer::Symbolizer(bool verbose)
    : m_dwfl(dwfl_begin(&s_callbacks)), m_verbose(verbose) {

  if (m_dwfl == nullptr) {
    m_errmsg = "error: failed to initialize dwfl";
    if (m_verbose) {
      std::cerr << m_errmsg << std::endl;
    }
    return;
  }

  dwfl_report_begin(m_dwfl);
  int success = dwfl_linux_proc_report(m_dwfl, getpid());
  dwfl_report_end(m_dwfl, nullptr, nullptr);

  if (success != 0) {
    std::ostringstream oss;
    oss << "error: failed to report process to dwfl. "
        << dwfl_errmsg(dwfl_errno());
    m_errmsg = oss.str();
    if (m_verbose) {
      std::cerr << m_errmsg << std::endl;
    }
    dwfl_end(m_dwfl);
    m_dwfl = nullptr;
    return;
  }
  return;
}

Symbolizer::~Symbolizer() {
  if (m_dwfl != nullptr) {
    dwfl_end(m_dwfl);
    m_dwfl = nullptr;
  }
  return;
}

void Symbolizer::info(const void *ip, const char **symbol,
                      const char **filename, int *lineno, int *colno) {
  if (m_dwfl == nullptr) {
    // We do not have an active dwfl session. Give up.
    return;
  }

  Dwfl_Module *module = dwfl_addrmodule(m_dwfl, (Dwarf_Addr)ip);
  if (module == nullptr) {
    std::ostringstream oss;
    oss << "warning: failed to find module containing address" << std::hex << ip
        << ". " << dwfl_errmsg(dwfl_errno());
    m_errmsg = oss.str();
    if (m_verbose) {
      std::cerr << m_errmsg << std::endl;
    }
    return;
  }

  if (symbol != nullptr) {
    *symbol = dwfl_module_addrname(module, (GElf_Addr)ip);
  }

  Dwfl_Line *line = dwfl_module_getsrc(module, (Dwarf_Addr)ip);
  if (line == nullptr) {
    std::ostringstream oss;
    oss << "warning: failed to resolve line information for address "
        << std::hex << ip << ". " << dwfl_errmsg(dwfl_errno()) << "\n";
    oss << "info: recompiling target with debug information enabled may fix "
           "this (add flag '-g')";
    m_errmsg = oss.str();
    if (m_verbose) {
      std::cerr << m_errmsg << std::endl;
    }
    return;
  }

  if (filename != nullptr || lineno != nullptr || colno != nullptr) {
    const char *_filename = nullptr;
    if (lineno != nullptr) {
      *lineno = 0;
    }
    if (colno != nullptr) {
      *colno = 0;
    }
    _filename = dwfl_lineinfo(line, nullptr, lineno, colno, nullptr, nullptr);
    if (filename != nullptr) {
      *filename = _filename;
    }
  }

  return;
}

std::string Symbolizer::demangle(const char *symbol) {
  if (symbol == nullptr) {
    return "";
  }
  if (strlen(symbol) < 3 || symbol[0] != '_' || symbol[1] != 'Z') {
    return symbol;
  }
  char *demangled = abi::__cxa_demangle(symbol, nullptr, nullptr, nullptr);
  if (demangled == nullptr) {
    return symbol;
  }
  std::string ret = demangled;
  free(demangled);
  return ret;
}
