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
 Return a static string buffer.
\******************************************************************************/
char *C_buf(void)
{
        static char buffer[C_BUFS][C_BUF_SIZE];
        static int which;

        if (++which >= C_BUFS)
                which = 0;
        return buffer[which];
}

/******************************************************************************\
 These functions parse variable argument lists into a static buffer and return
 a pointer to it. You can also pass a pointer to an integer to get the length
 of the output. Not thread safe, but very convenient. Note that after repeated
 calls to these functions, C_vanv will eventually run out of buffers and start
 overwriting old ones. The idea for these functions comes from the Quake 3
 source code.
\******************************************************************************/
char *C_vanv(int *plen, const char *fmt, va_list va)
{
        int len;
        char *buffer;

        buffer = C_buf();
        len = vsnprintf(buffer, C_BUF_SIZE, fmt, va);
        if (plen)
                *plen = len;
        return buffer;
}

char *C_van(int *plen, const char *fmt, ...)
{
        va_list va;
        char *string;

        va_start(va, fmt);
        string = C_vanv(plen, fmt, va);
        va_end(va);
        return string;
}

char *C_va(const char *fmt, ...)
{
        va_list va;
        char *string;

        va_start(va, fmt);
        string = C_vanv(NULL, fmt, va);
        va_end(va);
        return string;
}

/******************************************************************************\
 Equivalent to the standard library strncpy, but ensures that there is always
 a NUL terminator. Sometimes known as "strncpyz". Can copy overlapped strings.
 Returns the length of the source string.
\******************************************************************************/
int C_strncpy_full(const char *func, char *dest, const char *src, int len)
{
        int src_len;

        if (!dest)
                return 0;
        if (!src) {
                if (len > 0)
                        dest[0] = NUL;
                return 0;
        }
        src_len = (int)strlen(src);
        if (src_len > --len) {
                C_log(CLL_DEBUG, func, "dest (%d bytes) too short to hold src "
                                       "(%d bytes)", len, src_len, src);
                src_len = len;
        }
        memmove(dest, src, src_len);
        dest[src_len] = NUL;
        return src_len;
}

/******************************************************************************\
 Wraps a string in double-quotes and escapes any special characters. Returns
 a statically allocated string.
\******************************************************************************/
char *C_escape(const char *str)
{
        char *pbuf, *buf;

        buf = C_buf();
        buf[0] = '"';
        for (pbuf = buf + 1; *str && pbuf <= buf + sizeof (buf) - 3; str++) {
                if (*str == '"' || *str == '\\')
                        *pbuf++ = '\\';
                else if (*str == '\n') {
                        *pbuf++ = '\\';
                        *pbuf = 'n';
                        continue;
                } else if(*str == '\t') {
                        *pbuf++ = '\\';
                        *pbuf = 't';
                        continue;
                } else if (*str == '\r')
                        continue;
                *pbuf++ = *str;
        }
        *pbuf++ = '"';
        *pbuf = NUL;
        return buf;
}

/******************************************************************************\
 Returns the statically-allocated name of a SDL key.
\******************************************************************************/
const char *C_keyName(SDLKey key)
{
        static char buf[C_BUF_SIZE];

        /* Return ASCII directly */
        if (key >= ' ' && key < 0x7f) {
                buf[0] = key;
                buf[1] = 0;
                return buf;
        }

        /* Keypad numerals */
        if (key >= SDLK_KP0 && key <= SDLK_KP9) {
                snprintf(buf, sizeof (buf), "KP %d", key - SDLK_KP0);
                return buf;
        }

        /* Function keys */
        if (key >= SDLK_F1 && key <= SDLK_F15) {
                snprintf(buf, sizeof (buf), "F%d", key - SDLK_F1 + 1);
                return buf;
        }

        /* Special keys */
        switch (key) {
        case SDLK_SPACE:
                return "Space";
        case SDLK_TAB:
                return "Tab";
        case SDLK_CLEAR:
                return "Clear";
        case SDLK_PAUSE:
                return "Pause";
        case SDLK_ESCAPE:
                return "Escape";
        case SDLK_BACKSPACE:
                return "Bksp";
        case SDLK_DELETE:
                return "Delete";
        case SDLK_RETURN:
                return "Enter";
        case SDLK_KP_PERIOD:
                return "KP .";
        case SDLK_KP_DIVIDE:
                return "KP /";
        case SDLK_KP_MULTIPLY:
                return "KP *";
        case SDLK_KP_MINUS:
                return "KP -";
        case SDLK_KP_PLUS:
                return "KP +";
        case SDLK_KP_ENTER:
                return "KP Ent";
        case SDLK_KP_EQUALS:
                return "KP =";
        case SDLK_UP:
                return "Up";
        case SDLK_DOWN:
                return "Down";
        case SDLK_RIGHT:
                return "Right";
        case SDLK_LEFT:
                return "Left";
        case SDLK_INSERT:
                return "Insert";
        case SDLK_HOME:
                return "Home";
        case SDLK_END:
                return "End";
        case SDLK_PAGEUP:
                return "PgUp";
        case SDLK_PAGEDOWN:
                return "PgDn";
        case SDLK_NUMLOCK:
                return "NumLk";
        case SDLK_CAPSLOCK:
                return "CapsLk";
        case SDLK_SCROLLOCK:
                return "ScrlLk";
        case SDLK_RSHIFT:
                return "RShift";
        case SDLK_LSHIFT:
                return "LShift";
        case SDLK_RCTRL:
                return "RCtrl";
        case SDLK_LCTRL:
                return "LCtrl";
        case SDLK_RALT:
                return "RAlt";
        case SDLK_LALT:
                return "LAlt";
        case SDLK_RMETA:
                return "RMeta";
        case SDLK_LMETA:
                return "LMeta";
        case SDLK_RSUPER:
                return "RSuper";
        case SDLK_LSUPER:
                return "LSuper";
        case SDLK_MODE:
                return "Mode";
        case SDLK_COMPOSE:
                return "Compose";
        default:
                snprintf(buf, sizeof (buf), "0x%d", key);
                return buf;
        }
}

/******************************************************************************\
 Returns the number of bytes in a single UTF-8 character:
 http://www.cl.cam.ac.uk/%7Emgk25/unicode.html#utf-8
\******************************************************************************/
int C_utf8Size(unsigned char first_byte)
{
        /* U-00000000 - U-0000007F (ASCII) */
        if (first_byte < 192)
                return 1;

        /* U-00000080 - U-000007FF */
        else if (first_byte < 224)
                return 2;

        /* U-00000800 - U-0000FFFF */
        else if (first_byte < 240)
                return 3;

        /* U-00010000 - U-001FFFFF */
        else if (first_byte < 248)
                return 4;

        /* U-00200000 - U-03FFFFFF */
        else if (first_byte < 252)
                return 5;

        /* U-04000000 - U-7FFFFFFF */
        return 6;
}

/******************************************************************************\
 One UTF-8 character from [src] will be copied to [dest]. The index of the
 current position in [dest], [dest_i] and the index in [src], [src_i], will
 be advanced accordingly. [dest] will not be allowed to overrun [dest_sz]
 bytes. Returns the size of the UTF-8 character or 0 if there is not enough
 room or if [src] starts with NUL.
\******************************************************************************/
int C_utf8Append(char *dest, int *dest_i, int dest_sz, const char *src)
{
        int len, char_len;

        if (!*src)
                return 0;
        char_len = C_utf8Size(*src);
        if (*dest_i + char_len > dest_sz)
                return 0;
        for (len = char_len; *src && len > 0; len--)
                dest[(*dest_i)++] = *src++;
        return char_len;
}

/******************************************************************************\
 Convert a Unicode token to a UTF-8 character sequence. The length of the
 token in bytes is output to [plen], which can be NULL.
\******************************************************************************/
char *C_utf8Encode(wchar_t unicode, int *plen)
{
        static char buf[7];
        int len;

        /* ASCII is an exception */
        if (unicode <= 0x7f) {
                buf[0] = (char)unicode;
                buf[1] = NUL;
                if (plen)
                        *plen = 1;
                return buf;
        }

        /* Size of the sequence depends on the range */
        if (unicode <= 0xff)
                len = 2;
        else if (unicode <= 0xffff)
                len = 3;
        else if (unicode <= 0x1fffff)
                len = 4;
        else if (unicode <= 0x3FFFFFF)
                len = 5;
        else if (unicode <= 0x7FFFFFF)
                len = 6;
        else {
                C_warning("Invalid Unicode character 0x%x", unicode);
                buf[0] = NUL;
                if (plen)
                        *plen = 0;
                return buf;
        }
        if (plen)
                *plen = len;

        /* The first byte is 0b*110x* and the rest are 0b10xxxxxx */
        buf[0] = 0xfc << (6 - len);
        while (--len > 0) {
                buf[len] = 0x80 + (unicode & 0x3f);
                unicode >>= 6;
        }
        buf[0] += unicode;
        return buf;
}

char *C_utf8Encode_str(wchar_t *wide, int *plen)
{
        static char buf[C_BUF_SIZE];
        int len = 0, size;

        while (*wide) {
                strncpy(buf + len, C_utf8Encode(*wide, &size), sizeof (buf) - len);
                if ((len += size) >= sizeof (buf))
                        len = sizeof (buf) - 1;
                wide++;
        }
        buf[len] = 0;
        if (plen)
                *plen = len;
        return buf;
}
