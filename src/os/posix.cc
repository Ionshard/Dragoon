/******************************************************************************\
 Dragoon - Copyright (C) 2010 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "../log.h"
#if !WINDOWS

namespace dragoon {
namespace os {

bool Mkdir(const char* path) {
  if (!::mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
    DEBUG("Created directory '%s'", path);
  else if (errno != EEXIST) {
    WARN("Failed to create: %s", strerror(errno));
    return false;
  }
  return true;
}

const char* UserDir(void) {
  static char userDir[256];
  if (userDir[0])
    return userDir;
  snprintf(userDir, sizeof(userDir), "%s/." PACKAGE, getenv("HOME"));
  DEBUG("Home directory is '%s'", userDir);
  Mkdir(userDir);
  return userDir;
}

const char *AppDir(void) {
  return PKGDATADIR;
}

void HandleSignals(void (*func)(int signal)) {

  // Ignore certain signals
  signal(SIGPIPE, SIG_IGN);
  signal(SIGFPE, SIG_IGN);

  // Signals we catch and die on
  int catchSignals[] = { SIGSEGV, SIGHUP, SIGINT,
                         SIGTERM, SIGQUIT, SIGALRM, -1 };

  // Hook handler
  int* ps = catchSignals;
  sigset_t sigset;
  sigemptyset(&sigset);
  while (*ps != -1) {
    signal(*ps, func);
    sigaddset(&sigset, *(ps++));
  }
  if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) == -1)
    WARN("Failed to set signal blocking mask");
}

} // namespace os
} // namespace dragoon

#endif // !WINDOWS
