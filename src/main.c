/******************************************************************************\
 Dragoon - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "common/c_public.h"
#include "render/r_public.h"
#include "physics/p_public.h"
#include "game/g_public.h"

/* Debug variables */
static int memCheck;

/******************************************************************************\
 Called when the program quits (normally or not). Performs an orderly cleanup.
\******************************************************************************/
static void cleanup(void)
{
        static int ran_once;

        /* It is possible that this function will get called multiple times
           for certain kinds of exits, do not clean-up twice! */
        if (ran_once) {
                C_warning("Cleanup already called");
                return;
        }
        ran_once = TRUE;
        C_debug("Cleaning up");
        C_archiveVars(C_va("%s/autogen.cfg", C_userDir()));
        G_saveMap(g_edit);
        G_cleanupMenu();
        G_cleanupEntities();
        R_cleanupSprites();
        R_cleanupTextures();
        C_cleanupVars();
        C_checkLeaks();
}

/******************************************************************************\
 Called when the program quits via signal. Performs an orderly cleanup.
\******************************************************************************/
static void cleanupSignal(int signal)
{
        cleanup();
        C_error("Caught signal %d", signal);
}

/******************************************************************************\
 Register variables.
\******************************************************************************/
static void registerVars(void)
{
        /* Screen parameters */
        C_register_int(&r_width, "r_width", "Screen width", TRUE);
        C_register_int(&r_height, "r_height", "Screen height", TRUE);
        C_register_int(&r_fullscreen, "r_fullscreen", "Fullscreen mode", TRUE);

        /* Editor */
        C_register_buf(g_edit, "edit", "Filename of map to edit", FALSE);

        /* Debug variables */
        if (CHECKED)
                C_register_int(&memCheck, "memCheck",
                               "Test memory checking: 0-9", FALSE);
}

/******************************************************************************\
 Initialize SDL library.
\******************************************************************************/
static void initSdl(void)
{
        SDL_version compiled;
        const SDL_version *linked;

        SDL_VERSION(&compiled);
        C_debug("Compiled with SDL %d.%d.%d",
                compiled.major, compiled.minor, compiled.patch);
        linked = SDL_Linked_Version();
        if (!linked)
                C_error("Failed to get SDL linked version");
        C_debug("Linked with SDL %d.%d.%d",
                linked->major, linked->minor, linked->patch);
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
                C_error("Failed to initialize SDL: %s", SDL_GetError());
        SDL_WM_SetCaption(PACKAGE_STRING, PACKAGE);
}

/******************************************************************************\
 Program entry point.
\******************************************************************************/
int main(int argc, char *argv[])
{
        RText status;

        /* Use the cleanup function instead of lots of atexit() calls to
           control the order of cleanup */
        atexit(cleanup);
        C_signalHandler((CSignalHandler)cleanupSignal);

        C_debug(PACKAGE_STRING);

        /* Parse variables */
        registerVars();
        C_parseVarConfig(C_va("%s/autogen.cfg", C_userDir()));
        C_parseCommandLine(argc, argv);

        /* Debug */
        if (CHECKED) {
                if (memCheck)
                        C_testMemCheck(memCheck);
        }

        /* Initialize */
        initSdl();
        if (!R_setVideoMode())
                return 1;
        R_initGl();
        R_parseSpriteCfg("gfx/sprites.cfg");
        G_initMenu();

        /* Seed random number generator */
        srand(time(NULL));

        /* Start the game */
        G_parseEntityCfg("game/entities.cfg");
        if (g_edit[0])
                G_initEditor();
        else {
                G_loadMap("map/limbo", CVec_zero());
                G_showMenu();
        }

        /* Main loop */
        C_initTime();
        if (CHECKED)
                RText_init(&status, NULL, NULL);
        for (;;) {
                SDL_Event ev;

                /* Dispatch events */
                while (SDL_PollEvent(&ev)) {

                        /* In checked mode, escape quits */
                        if (ev.type == SDL_QUIT ||
                            (CHECKED && ev.type == SDL_KEYDOWN &&
                             ev.key.keysym.sym == SDLK_ESCAPE))
                                return 0;

                        G_dispatch(&ev);
                }

                /* Update FPS counter */
                if (CHECKED && CCount_poll(&c_throttled, 2000)) {
                        const char *str;

                        str = C_va("%.1f fps (%.0f%% throt), %.0f faces, "
                                   "%.0f ents",
                                   CCount_fps(&c_throttled),
                                   CCount_perFrame(&c_throttled) * 100,
                                   CCount_perFrame(&r_countFaces),
                                   CCount_perFrame(&p_countEntities));
                        CCount_reset(&c_throttled);
                        CCount_reset(&r_countFaces);
                        CCount_reset(&p_countEntities);
                        RText_init(&status, NULL, str);
                }

                /* Frame */
                R_begin();
                G_update();
                if (CHECKED)
                        RText_draw(&status);
                R_end();
                C_updateTime();
                C_throttleFps();
        }
}

