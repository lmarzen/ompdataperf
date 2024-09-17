#pragma once

#include <string>

struct Dwfl;

class Symbolizer {
private:
  Dwfl *m_dwfl;
  bool m_verbose;
  std::string m_errmsg;

public:
  /* Setting 'verbose' to true will immediately print error messages to stderr.
   */
  Symbolizer(bool verbose=false);
  ~Symbolizer();

  /* Given an instruction pointer for the running program, will return the
   * (demangled) symbol name, source filename, line number, and column number.
   * Arguments may be nullptr for unneeded fields. The 'symbol' and 'filename'
   * fields are set to null if cannot successfully be determined; 'lineno' and
   * 'colno' are set to 0.
   */
  void info(const void *ip, const char **symbol, const char **filename,
            int *lineno, int *colno);

  /* Returns true if an error occurred at any point during the Symbolizer's execution, false otherwise.
   */
  bool has_errmsg() const {return !m_errmsg.empty(); }

  /* Return a string containing the most recent error message. If no error has occurred, returns the empty string.
   */
  std::string get_errmsg() const { return m_errmsg; }

  /* Clears the current error message.
   */
  void clear_errmsg() { m_errmsg.clear(); }

  /* Returns true if the Symbolizer has an active session with dwfl, false otherwise.
   */
  bool is_valid() const { return m_dwfl != nullptr; };
};
