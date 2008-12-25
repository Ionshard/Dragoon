/******************************************************************************\
 Dragoon - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "c_private.h"

/* Bash color codes */
#define BASH_COLOR(a, b) "\033[" #a ";" #b "m"
#define BASH_WHITE "[1;37m"
#define BASH_GRAY "[0;37m"
#define BASH_YELLOW_BRIGHT "[1;33m"
#define BASH_YELLOW_DARK "[0;33m"
#define BASH_RED_BRIGHT "[1;31m"
#define BASH_RED_DARK "[0;31m"

/******************************************************************************\
 If [boolean] is FALSE then generates an error. The error message contains the
 actual assertion statement and source location information.
\******************************************************************************/
#if CHECKED
void C_assert_full(int boolean, const char *func, const char *statement)
{
        if (!boolean)
                C_log(CLL_ERROR, func, "Assertion failed: %s", statement);
}
#endif

/******************************************************************************\
 Print a colored log message to standard out.
\******************************************************************************/
void C_log(CLogLevel level, const char *func, const char *fmt, ...)
{
        va_list va;

        /* Print color-coded function name */
        if (CHECKED)
                switch (level) {
                case CLL_ERROR:
                        fprintf(stderr, BASH_COLOR(1, 30) "%s "
                                BASH_COLOR(1, 31), func);
                        break;
                case CLL_WARNING:
                        fprintf(stderr, BASH_COLOR(1, 30) "%s "
                                BASH_COLOR(1, 33), func);
                        break;
                default:
                        fprintf(stderr, BASH_COLOR(1, 30) "%s "
                                BASH_COLOR(,), func);
                        break;
                }
        else
                fprintf(stderr, "%s: ", PACKAGE);

        /* Output message */
        va_start(va, fmt);
        vfprintf(stderr, fmt, va);
        if (CHECKED)
                fprintf(stderr, "\n" BASH_COLOR(,));
        else
                fprintf(stderr, "\n");
        va_end(va);

        /* Errors are fatal */
        if (level == CLL_ERROR)
                abort();
}

