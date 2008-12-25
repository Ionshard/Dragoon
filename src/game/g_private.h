/******************************************************************************\
 Dragoon - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "../common/c_public.h"
#include "../render/r_public.h"
#include "../physics/p_public.h"
#include "g_public.h"

/* Input events, extends physics events */
typedef enum {
        GE_UPDATE = PE_UPDATE,
        GE_KEY_DOWN = P_EVENTS,
        GE_KEY_UP,
        GE_MOUSE_MOVE,
        GE_MOUSE_DOWN,
        GE_MOUSE_UP,
        G_EVENTS
} GEvent;

/* Entity class spawning function */
struct GSpawnParams;
typedef PEntity *(*GSpawnFunc)(void *entityClass, const struct GSpawnParams *);

/* Entity class struct */
typedef struct {
        CNamed named;
        GSpawnFunc spawnFunc;
        CVec size;
        char editorKey, spriteName[C_NAME_MAX];
} GEntityClass;

/* Entity spawn parameters */
typedef struct GSpawnParams {
        CVec origin, size;
} GSpawnParams;

/* g_editor.c */
bool G_dispatch_editor(GEvent);

/* GEntity.c */
bool GEntityClass_parseToken(GEntityClass *, FILE *, const char *token);
PEntity *G_spawn(const char *className, const GSpawnParams *);

extern CNamed *g_classRoot;

/* g_input.c */
extern CVec g_mouse;
extern int g_key, g_button;
extern bool g_shift, g_alt, g_ctrl;

/* g_map.c */
void G_loadMap(const char *filename, CVec offset);

/* g_menu.c */
bool G_dispatch_menu(GEvent);

