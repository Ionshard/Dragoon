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

