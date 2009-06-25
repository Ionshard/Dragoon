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

/* Menu parameters */
#define MENU_WIDTH 80
#define MENU_HEIGHT 80
#define MENU_MARGIN 16

/* Menu names */
enum {
        MENU_MAIN,
        MENU_OPTIONS,
        MENU_KEYS,
        MENUS
};

/* TRUE if the game is in limbo menu */
bool g_limbo;

/* Filename of map to start on */
char g_play[C_NAME_MAX];

static RMenu menus[MENUS], *menuShown;
static float menuBgFade;

/******************************************************************************\
 Quit callback.
\******************************************************************************/
static void onQuit(void)
{
        exit(0);
}

/******************************************************************************\
 New game callback.
\******************************************************************************/
void G_newGame(void)
{
        g_limbo = FALSE;
        G_hideMenu();
        P_cleanupEntities();

        /* Load starting map */
        G_loadMap(g_play[0] ? g_play : "map/test", CVec(0, 0));
        G_spawnPlayer(g_playerSpawn);
        G_initHud();
}

/******************************************************************************\
 Show the menu referenced in the menu entry's data.
\******************************************************************************/
static void showMenu(RMenuEntry *entry, bool hideLeft)
{
        if (menuShown) {
                menuShown->hideLeft = hideLeft;
                menuShown->shown = FALSE;
        }
        if (!entry || !entry->data)
                return;
        menuShown = entry->data;
        menuShown->hideLeft = !hideLeft;
        menuShown->shown = TRUE;
}

static void showMenu_right(RMenuEntry *entry)
{
        showMenu(entry, TRUE);
}

static void showMenu_left(RMenuEntry *entry)
{
        showMenu(entry, FALSE);
}

/******************************************************************************\
 Initialize menus.
\******************************************************************************/
void G_initMenu(void)
{
        RMenuEntry *entry;

        g_limbo = TRUE;

        /* Main menu */
        RMenu_init(menus + MENU_MAIN);
        menus[MENU_MAIN].size.x = MENU_WIDTH;
        RMenu_add(menus + MENU_MAIN,
                  RMenuEntry_new("New Game", (CCallback)G_newGame), 0);
        entry = RMenuEntry_new("Continue", NULL);
        entry->enabled = FALSE;
        RMenu_add(menus + MENU_MAIN, entry, 0);
        entry = RMenuEntry_new("Options", (CCallback)showMenu_right);
        entry->data = menus + MENU_OPTIONS;
        RMenu_add(menus + MENU_MAIN, entry, 4);
        RMenu_add(menus + MENU_MAIN,
                  RMenuEntry_new("Quit", (CCallback)onQuit), 4);

        /* Options menu */
        RMenu_init(menus + MENU_OPTIONS);
        menus[MENU_OPTIONS].size.x = MENU_WIDTH;
        entry = RMenuEntry_new("Key Bindings", (CCallback)showMenu_right);
        entry->data = menus + MENU_KEYS;
        RMenu_add(menus + MENU_OPTIONS, entry, 0);
        entry = RMenuEntry_new("Resolution", NULL);
        entry->enabled = FALSE;
        RMenu_add(menus + MENU_OPTIONS, entry, 0);
        entry = RMenuEntry_new("Back", (CCallback)showMenu_left);
        entry->data = menus + MENU_MAIN;
        RMenu_add(menus + MENU_OPTIONS, entry, 4);

        /* Key bindings menu */
        RMenu_init(menus + MENU_KEYS);
        menus[MENU_KEYS].size.x = MENU_WIDTH;
        entry = RMenuEntry_new("Back", (CCallback)showMenu_left);
        entry->data = menus + MENU_OPTIONS;
        RMenu_add(menus + MENU_KEYS, entry, 4);
}

/******************************************************************************\
 Clean up the main menu.
\******************************************************************************/
void G_cleanupMenu(void)
{
        int i;

        for (i = 0; i < MENUS; i++)
                RMenu_cleanup(menus + i);
}

/******************************************************************************\
 Hide the main menu.
\******************************************************************************/
void G_hideMenu(void)
{
        if (g_limbo)
                return;
        if (menuShown)
                menuShown->shown = FALSE;
        menuShown = NULL;

        /* Don't allow mouse outside of the game window */
        if (!CHECKED)
                SDL_WM_GrabInput(SDL_GRAB_ON);

        /* Unpause the game */
        p_speed = 1;
}

/******************************************************************************\
 Show the main menu.
\******************************************************************************/
void G_showMenu(void)
{
        if (menuShown)
                menuShown->shown = FALSE;
        menuShown = menus + MENU_MAIN;
        menus[MENU_MAIN].shown = TRUE;

        /* Allow mouse outside of the game window */
        if (!CHECKED)
                SDL_WM_GrabInput(SDL_GRAB_OFF);

        /* Pause the game */
        p_speed = 0;
}

/******************************************************************************\
 Returns TRUE if the main menu is shown and the key event was absorbed.
\******************************************************************************/
bool G_dispatch_menu(GEvent event)
{
        if (event == GE_KEY_DOWN) {
                if (menuShown) {

                        /* Navigate menu */
                        if (g_key == SDLK_DOWN)
                                RMenu_select(menuShown, FALSE);
                        else if (g_key == SDLK_UP)
                                RMenu_select(menuShown, TRUE);
                        else if (g_key == SDLK_RIGHT || g_key == SDLK_RETURN)
                                RMenu_activate(menuShown, TRUE);
                        else if (g_key == SDLK_LEFT)
                                RMenu_activate(menuShown, FALSE);

                        /* Hide menu via key */
                        else if (g_key == SDLK_ESCAPE ||
                                 g_key == SDLK_BACKSPACE)
                                G_hideMenu();

                        return TRUE;
                }

                /* Show menu via key */
                else if (g_key == SDLK_ESCAPE || g_key == SDLK_BACKSPACE) {
                        G_showMenu();
                        return TRUE;
                }
        }

        /* Swallow key-ups and mouse events when open */
        else if ((event == GE_KEY_UP || event == GE_MOUSE_DOWN ||
                  event == GE_MOUSE_UP || event == GE_MOUSE_MOVE) && menuShown)
                return TRUE;

        /* Update menu */
        else if (event == GE_UPDATE) {
                CVec origin;
                int i;

                /* Update menu positions */
                origin = CVec(r_widthScaled / 2 - MENU_WIDTH / 2,
                              r_heightScaled - MENU_HEIGHT / 2 - MENU_MARGIN);
                for (i = 0; i < MENUS; i++)
                        menus[i].origin = origin;

                /* Let some time go by before showing the menu for the
                   first time */
                if (c_timeMsec < 1000)
                        return FALSE;

                if (!C_fade(&menuBgFade, menuShown != NULL, R_MENU_FADE))
                        return FALSE;
                R_drawRect(CVec(0, r_heightScaled - MENU_HEIGHT - MENU_MARGIN),
                           G_Z_MENU, CVec(r_widthScaled, MENU_HEIGHT),
                           CColor(0, 0, 0, 0), CColor(0.1f, 0.1f, 0.1f,
                                                      menuBgFade * 0.6f));

                /* Update menus */
                for (i = 0; i < MENUS; i++)
                        RMenu_update(menus + i);
        }

        return FALSE;
}

