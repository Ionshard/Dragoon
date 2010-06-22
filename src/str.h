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
namespace str {

  /** Returns true if \c s ends with \c end. */
  static inline bool EndsWith(const char* s, const char* end,
                              bool ignore_case = false) {
    size_t s_len = strlen(s);
    size_t end_len = strlen(end);
    if (s_len >= end_len) {
      if (ignore_case)
        return !strcmp(s + s_len - end_len, end);
      else
        return !strcasecmp(s + s_len - end_len, end);
    }
    return false;
  }

} // namespace dragoon
} // namespace str
