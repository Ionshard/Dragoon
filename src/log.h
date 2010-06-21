/******************************************************************************\
 Dragoon - Copyright (C) 2010 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#pragma once

namespace dragoon {
namespace log {

/** Log event severity */
typedef enum {
  LEVEL_DEBUG, ///< Only shown in debug mode
  LEVEL_WARN,  ///< Non-fatal warning message
  LEVEL_ERROR, ///< Fatal error message (aborts)
} Level;

/** Print detail flags */
enum {
  DETAIL_FILE = 1, ///< Print filename
  DETAIL_LINE = 2, ///< Print line number
  DETAIL_FUNC = 4, ///< Print function
};

/** Display program name prefix */
void set_program_name(const char* value);

/** Display debug prints */
void set_debug(bool value);

/** Display colored prints */
void set_color(bool value);

/** Set print-out detail */
void set_detail(int flags);

/** Function to print to log */
void Print(const char *file, int line, const char* func,
           Level level, const char *string);
void Printv(const char *file, int line, const char* func,
            Level level, const char *fmt, va_list va);
void Printf(const char *file, int line, const char* func,
            Level level, const char *fmt, ...);

/** Assertion function */
void Assert(const char *file, int line, const char* func,
            int statement, const char* string);

/** Exception with given log level */
class Exception: public std::exception {
public:

  /** Initialize log message with printf-style formattin */
  Exception(const char *file, int line, const char* func,
            Level level, const char *fmt, va_list va):
    level_(level), line_(line) {
    strncpy(file_, file, sizeof (file_));
    strncpy(func_, func, sizeof (func_));
    vsnprintf(buf_, sizeof (buf_), fmt, va);
  }

  /** Initialize log message with printf-style formatting */
  Exception(const char *file, int line, const char* func,
            Level level, const char *fmt, ...):
    level_(level), line_(line) {
    strncpy(file_, file, sizeof (file_));
    strncpy(func_, func, sizeof (func_));
    va_list va;
    va_start(va, fmt);
    vsnprintf(buf_, sizeof (buf_), fmt, va);
    va_end(va);
  }

  /** Print the exception at its level */
  void Print() const { log::Print(file_, line_, func_, level_, buf_); }

  /** Print the exception at the specified level */
  void Print(Level l) const { log::Print(file_, line_, func_, l, buf_); }

  /** Get exception's original severity level */
  Level level() const { return level_; }

  /** Return error message */
  virtual const char* what() const throw() { return buf_; }

protected:
  Level level_;
  int line_;
  char file_[256];
  char func_[256];
  char buf_[4096];
};

} // namespace log
} // namespace dragoon

// Convenience macros
#define WARN(fmt, ...) \
  dragoon::log::Printf(__FILE__, __LINE__, __func__, \
                       dragoon::log::LEVEL_WARN, fmt, ## __VA_ARGS__)
#define ERROR(fmt, ...) \
  throw dragoon::log::Exception(__FILE__, __LINE__, __func__, \
                                dragoon::log::LEVEL_ERROR, fmt, ## __VA_ARGS__)
#if CHECKED
#define ASSERT(s) \
  dragoon::log::Assert(__FILE__, __LINE__, __func__, (int)(s), #s)
#define DEBUG(fmt, ...) \
  dragoon::log::Printf(__FILE__, __LINE__, __func__, \
                       dragoon::log::LEVEL_DEBUG, fmt, ## __VA_ARGS__)
#else
#define ASSERT(s)
#define DEBUG(...)
#endif
