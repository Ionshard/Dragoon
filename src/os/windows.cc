/******************************************************************************\
 Dragoon - Copyright (C) 2010 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

// Only compile on Windows
#if WINDOWS

namespace dragoon {

bool Mkdir(const char* path) {
  return false;
}

const char* UserDir(void) {
  return NULL;
}

const char *AppDir(void) {
  return NULL;
}

void HandleSignals(void (*func)(int signal)) {}

} // namespace dragoon

#endif // WINDOWS
