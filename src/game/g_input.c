/******************************************************************************\
 Dragoon - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "g_private.h"

/* Keyboard */
CVec g_control;
int g_key, g_keyCode;
bool g_shift, g_alt, g_ctrl;
static bool leftHeld, rightHeld, upHeld, downHeld;

/* Mouse */
CVec g_mouse, g_mouseRel;
int g_button;

/******************************************************************************\
 Dispatch an event from SDL.
\******************************************************************************/
void G_dispatch(const SDL_Event *ev)
{
        SDLMod mod;
        GEvent event;

        /* Update modifiers */
        mod = SDL_GetModState();
        g_shift = mod & KMOD_SHIFT;
        g_alt = mod & KMOD_ALT;
        g_ctrl = mod & KMOD_CTRL;

        /* Before dispatch */
        switch (ev->type) {
        case SDL_ACTIVEEVENT:

                /* If the mouse focus was lost, reset the cursor position */
                if (!ev->active.gain && (ev->active.state & SDL_APPMOUSEFOCUS))
                        g_mouse = CVec(-1, -1);

                return;
        case SDL_KEYDOWN:
                event = GE_KEY_DOWN;
                g_key = (int)ev->key.keysym.sym;
                g_keyCode = (int)ev->key.keysym.scancode;
                break;
        case SDL_KEYUP:
                event = GE_KEY_UP;
                g_key = (int)ev->key.keysym.sym;
                g_keyCode = (int)ev->key.keysym.scancode;
                break;
        case SDL_MOUSEMOTION:
                event = GE_MOUSE_MOVE;
                g_mouseRel = CVec_divf(CVec(ev->motion.xrel, ev->motion.yrel),
                                       r_scale);
                g_mouse = CVec_divf(CVec(ev->motion.x, ev->motion.y), r_scale);
                break;
        case SDL_MOUSEBUTTONDOWN:
                event = GE_MOUSE_DOWN;
                g_button = ev->button.button;
                break;
        case SDL_MOUSEBUTTONUP:
                event = GE_MOUSE_UP;
                g_button = ev->button.button;
                break;
        default:
                return;
        }

        /* Propagate event */
        if (G_dispatch_editor(event) || G_dispatch_menu(event) ||
            G_dispatch_player(event))
                return;
}

/******************************************************************************\
 Update game entities, the editor, and menus.
\******************************************************************************/
void G_update(void)
{
        G_updatePlayer();

        /* Should only render entities now so offset the camera */
        R_beginCam();
        P_updateEntities();
        R_endCam();
        G_drawHud();

        /* Propagate event */
        if (G_dispatch_editor(GE_UPDATE) || G_dispatch_menu(GE_UPDATE))
                return;
}

/******************************************************************************\
 Convert a keycode to a direction.
\******************************************************************************/
static CVec keyToDir(int key)
{
        if (key == 'a' || key == 'j' || key == SDLK_LEFT)
                return CVec(-1, 0);
        if (key == 'd' || key == 'l' || key == SDLK_RIGHT)
                return CVec(1, 0);
        if (key == 'w' || key == 'i' || key == SDLK_UP || key == ' ')
                return CVec(0, -1);
        if (key == 's' || key == 'k' || key == SDLK_DOWN)
                return CVec(0, 1);
        return CVec(0, 0);
}

/******************************************************************************\
 Process control events.
\******************************************************************************/
bool G_controlEvent(GEvent event)
{
        CVec dir;

        if (event == GE_KEY_DOWN) {
                dir = keyToDir(g_key);
                leftHeld |= dir.x < 0;
                rightHeld |= dir.x > 0;
                upHeld |= dir.y < 0;
                downHeld |= dir.y > 0;
        } else if (event == GE_KEY_UP) {
                dir = keyToDir(g_key);
                leftHeld &= dir.x >= 0;
                rightHeld &= dir.x <= 0;
                upHeld &= dir.y >= 0;
                downHeld &= dir.y <= 0;
        } else
                return FALSE;
        g_control = CVec(rightHeld - leftHeld, downHeld - upHeld);
        return TRUE;
}

