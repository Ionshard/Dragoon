/******************************************************************************\
 Dragoon - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* The MSVC compiler defines these but we want to make sure they are there
   even if we're using GCC and MinGW */
#ifndef WIN32
#define WIN32
#endif
#ifndef _CONSOLE
#define _CONSOLE
#endif

/* Prevents deprecation warnings */
#define _CRT_SECURE_NO_DEPRECATE 1

/* OpenGL uses some Windows stuff */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

/* Constants normally defined via SCons */
#define PACKAGE "dragoon"
#define PACKAGE_STRING "Dragoon 0.1.0"
#define PKGDATADIR "."

/* Compatibility */
#define inline __inline
#define __func__ __FUNCTION__
#define snprintf _snprintf
#define snwprintf _snwprintf
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define isfinite _finite

/* Debug code */
#ifndef CHECKED
#define CHECKED 1
#endif

/* Windows code */
#define WINDOWS 1

