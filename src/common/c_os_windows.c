/******************************************************************************\
 Dragoon - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* This file should only be included on Microsoft Windows systems */

#include "c_private.h"
#include <errno.h>
#include <direct.h>
#include <shlobj.h>

/******************************************************************************\
 Creates directory if it does not already exist. Returns TRUE if the directory
 exists after the call. Windows version.
\******************************************************************************/
int C_mkdir(const char *path)
{
        if (!_mkdir(path))
                C_debug("Created directory '%s'", path);
        else if (errno != EEXIST) {
                C_warning("Failed to create: %s", strerror(errno));
                return FALSE;
        }
        return TRUE;
}

/******************************************************************************\
 Returns the path to the user's writeable Plutocracy directory without
 trailing slash. Windows version.
\******************************************************************************/
const char *C_userDir(void)
{
        static char userDir[256];
        TCHAR app_data[MAX_PATH];

        if (userDir[0])
                return userDir;
        if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, app_data) != S_OK) {
                C_warning("Failed to get Application Data directory");
                return "";
        }
        snprintf(userDir, sizeof (userDir), "%s\\" PACKAGE, 
                 C_utf8Encode_str(app_data, NULL));
        C_debug("Home directory is '%s'", userDir);
        C_mkdir(userDir);
        return userDir;
}

/******************************************************************************\
 Returns the path to the program installation. Windows version.
\******************************************************************************/
const char *C_appDir(void)
{
        static char appDir[256];

        if (!appDir[0]) {
                wchar_t wideDir[256];
                
                GetModuleFileName(NULL, wideDir, sizeof (wideDir));
                C_strncpy_buf(appDir, C_utf8Encode_str(wideDir, NULL));
        }
        return appDir;
}

/******************************************************************************\
 Attach a cleanup signal handler and ignore certain signals. Does nothing on
 Windows.
\******************************************************************************/
void C_signalHandler(CSignalHandler func)
{
        C_debug("Signal handling not implemented on Windows");
}

/******************************************************************************\
 Returns TRUE for an absolute path. A bit more complicated on Windows because
 either slash is valid and network paths are different from drive paths.
\******************************************************************************/
bool C_absolute_path(const char *path)
{
        return (path[0] == '\\' && path[1] == '\\') ||
               (path[0] == '/' && path[1] == '/') ||
               (isalpha(path[0]) && path[1] == ':' &&
                (path[2] == '\\' || path[2] == '/'));
}
