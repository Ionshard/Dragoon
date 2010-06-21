/******************************************************************************\
 Dragoon - Copyright (C) 2009 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#pragma once
#include "log.h"

namespace dragoon {
namespace os {

  /** Returns the path to the program installation. */
  const char* AppPath();

  /** Returns the path to the user's writeable program directory without
      trailing slash. */
  const char* UserDir();

  /** Creates directory if it does not already exist. Returns true if the
      directory exists after the call. */
  bool Mkdir(const char* path);

  /** Set the callback function that handles Unix signals */
  void HandleSignals(void (*func)(int signal));

  /** Open file for reading */
  static inline FILE* OpenRead(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file)
      WARN("Failed to open '%s' for reading", filename);
    return file;
  }

  /** Open file for writing */
  static inline FILE* OpenWrite(const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (!file)
      WARN("Failed to open '%s' for writing", filename);
    return file;
  }
}

} // namespace dragoon
