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

/******************************************************************************\
 Wrap file opening to print a warning when the file fails to open.
\******************************************************************************/
FILE *C_fopen_full(const char *func, const char *filename, bool write)
{
        FILE *file;

        file = fopen(filename, write ? "wb" : "rb");
        if (!file)
                C_log(CLL_WARNING, func, "Failed to open '%s' for %s",
                      filename, write ? "writing" : "reading");
        return file;
}

/******************************************************************************\
 Gets the next character skipping comment lines.
\******************************************************************************/
static int readConfig(FILE *file)
{
        int ch;

        ch = fgetc(file);
        if (!C_isComment(ch))
                return ch;
        for (ch = fgetc(file); ch >= 0 && ch != '\n'; ch = fgetc(file));
        return ch;
}

/******************************************************************************\
 Skips non-printable characters. Returns the character that stopped the
 sequence of spaces.
\******************************************************************************/
int C_skipSpace(FILE *file)
{
        int ch;

        for (ch = readConfig(file); C_isSpace(ch); ch = readConfig(file));
        if (ch >= 0)
                ungetc(ch, file);
        return ch;
}

/******************************************************************************\
 Returns TRUE and consumes a '{' character if it is the next non-space
 character to be read.
\******************************************************************************/
bool C_openBrace(FILE *file)
{
        int ch;

        ch = C_skipSpace(file);
        if (ch == '{') {
                readConfig(file);
                return TRUE;
        }
        return FALSE;
}

/******************************************************************************\
 Consumes characters until the closing '}' or end of file.
\******************************************************************************/
void C_closeBrace(FILE *file)
{
        int ch;

        for (ch = readConfig(file); ch >= 0 && ch != '}';
             ch = readConfig(file))
                if (ch == '\\' && fgetc(file) < 0)
                        break;
}

/******************************************************************************\
 Read a token from a configuration file. The returned string is statically
 allocated.
\******************************************************************************/
const char *C_token(FILE *file)
{
        int ch, pos;
        char *buffer;
        bool section;

        /* Don't read past the end of a section */
        if ((ch = C_skipSpace(file)) == '}')
                return "";

        /* An entire section can be read in as a token */
        if ((section = ch == '{'))
                fgetc(file);

        /* Read in the token */
        buffer = C_buf();
        for (pos = 0, ch = readConfig(file); ch >= 0; ch = readConfig(file)) {
                if (pos >= C_BUF_SIZE - 2) {
                        C_warning("Oversize token");
                        break;
                }
                if ((section && ch == '}') || (!section && C_isSpace(ch)))
                        break;
                if (ch == '}') {
                        ungetc(ch, file);
                        break;
                }
                buffer[pos++] = (char)ch;
                if (ch == '\\' && fgetc(file) < 0) {
                        buffer[pos++] = (char)ch;
                        break;
                }
        }
        buffer[pos] = NUL;
        return buffer;
}

/******************************************************************************\
 Returns a vector
\******************************************************************************/
CVec C_token_vec(FILE *file)
{
        CVec v;

        v.x = C_token_float(file);
        v.y = C_token_float(file);
        return v;
}

/******************************************************************************\
 Read a NUL-terminated string from a binary file.
\******************************************************************************/
char *C_readString(FILE *file, int *plen)
{
        int i, ch;
        char *buf;

        buf = C_buf();
        ch = fgetc(file);
        for (i = 0; ch > 0 && i < C_BUF_SIZE - 1; ch = fgetc(file))
                buf[i++] = (char)ch;
        buf[i] = NUL;
        if (plen)
                *plen = i;
        return buf;
}

