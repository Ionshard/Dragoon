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
int g_key;
bool g_shift, g_alt, g_ctrl;

/* Mouse */
CVec g_mouse;
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
                g_key = ev->key.keysym.sym;
                break;
        case SDL_KEYUP:
                event = GE_KEY_UP;
                g_key = ev->key.keysym.sym;
                break;
        case SDL_MOUSEMOTION:
                event = GE_MOUSE_MOVE;
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
        G_drawPlayer();
        R_endCam();

        /* Propagate event */
        if (G_dispatch_editor(GE_UPDATE) || G_dispatch_menu(GE_UPDATE))
                return;
}

/******************************************************************************\
 Convert a key code to a direction vector.
\******************************************************************************/
static CVec keyToDirection(int key)
{
        if (key == 'a' || key == 'j' || key == SDLK_LEFT)
                return CVec(-1, 0);
        if (key == 'd' || key == 'l' || key == SDLK_RIGHT)
                return CVec(1, 0);
        if (key == 'w' || key == 'i' || key == SDLK_UP)
                return CVec(0, -1);
        if (key == 's' || key == 'k' || key == SDLK_DOWN)
                return CVec(0, 1);
        return CVec_zero();
}

/******************************************************************************\
 Keyboard direction control from keyboard events. Returns TRUE if [event]
 is a key down/up event that was interpreted as a direction command.
\******************************************************************************/
bool G_controlDirection(GEvent event, CVec *vec, float speed)
{
        CVec dir;
        int signSpeed;

        dir = keyToDirection(g_key);
        if (!dir.x && !dir.y)
                return FALSE;
        if (event == GE_KEY_UP)
                dir = CVec_scalef(dir, -1);
        else if (event != GE_KEY_DOWN)
                return FALSE;
        signSpeed = C_sign(speed);
        if (C_sign(vec->x) != dir.x * signSpeed)
                vec->x += dir.x * speed;
        if (C_sign(vec->y) != dir.y * signSpeed)
                vec->y += dir.y * speed;
        return TRUE;
}

