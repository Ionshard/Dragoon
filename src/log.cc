/******************************************************************************\
 Dragoon - Copyright (C) 2010 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "log.h"

namespace dragoon {
namespace log {

namespace {
  std::string program_name$;
  int detail$;
  bool debug$;
  bool color$;

  // Return a bash color code
  const char* ColorCode(int a, int b) {
    static char buf[128];
    if (!color$)
      return "";
    if (a < 0 || b < 0)
      return "\033[;m";
    snprintf(buf, sizeof(buf), "\033[%d;%dm", a, b);
    return buf;
  }

  // Print log message header. Returns 0 if message should not be printed.
  int PrintStart(const char* file, int line, const char* func, Level level) {

    // No debug prints unless debug mode is on
    if (level < LEVEL_WARN && !debug$)
      return 0;

    // Print color-coded program, file, function, and line identifier
    bool first = true;
    fprintf(stderr, "%s", ColorCode(1, 30));
    if (!program_name$.empty()) {
      fprintf(stderr, "%s", program_name$.c_str());
      first = false;
    }
    if (file && (detail$ & DETAIL_FILE)) {
      first = !first ? fprintf(stderr, ":"), 0: 0;
      fprintf(stderr, "%s", file);
    }
    if (line > 0 && (detail$ & DETAIL_LINE)) {
      first = !first ? fprintf(stderr, ":"), 0: 0;
      fprintf(stderr, "%d", line);
    }
    if (func && (detail$ & DETAIL_FUNC)) {
      first = !first ? fprintf(stderr, ":"), 0: 0;
      fprintf(stderr, "%s", func);
    }
    if (color$) {
      if (!first)
        fprintf(stderr, detail$ && color$ ? " " : ": ");
      switch (level) {
      case LEVEL_ERROR:
        fprintf(stderr, "%s", ColorCode(1, 31));
        break;
      case LEVEL_WARN:
        fprintf(stderr, "%s", ColorCode(1, 33));
        break;
      default:
        fprintf(stderr, "%s", ColorCode(-1, -1));
        break;
      }
    } else if (!first)
      fprintf(stderr, ": ");

    return 1;
  }

}

void set_program_name(const char* value) { program_name$ = value; }
void set_debug(bool value) { debug$ = value; }
void set_color(bool value) { color$ = value; }
void set_detail(int flags) { detail$ = flags; }

void Print(const char* file, int line, const char* func,
           Level level, const char* string) {
  if (PrintStart(file, line, func, level))
    fprintf(stderr, "%s\n%s", string, ColorCode(-1,-1));

  // Errors are fatal
  if (level == LEVEL_ERROR)
    abort();
}

void Printv(const char* file, int line, const char* func,
            Level level, const char* fmt, va_list va) {
  if (PrintStart(file, line, func, level)) {
    vfprintf(stderr, fmt, va);
    fprintf(stderr, "\n%s", ColorCode(-1,-1));
  }

  /// Errors are fatal
  if (level == LEVEL_ERROR)
    abort();
}

void Printf(const char* file, int line, const char* func,
            Level level, const char* fmt, ...) {
  va_list va;
  va_start(va, fmt);
  Printv(file, line, func, level, fmt, va);
  va_end(va);
}

void Assert(const char* file, int line, const char* func,
            int statement, const char* string) {
  if (!statement)
    Printf(file, line, func, LEVEL_ERROR, "Assertion failed: %s", string);
}

} // namespace log
} // namespace dragoon
