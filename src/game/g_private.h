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

/* Z values for game layers */
#define G_Z_HUD  0.00f
#define G_Z_FORE -0.25f
#define G_Z_CHAR -0.33f
#define G_Z_MID  -0.50f
#define G_Z_REAR -0.75f
#define G_Z_BACK -1.00f

/* Player muzzle vertical offset from entity center */
#define G_MUZZLE_OFFSET 2

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
struct GEntityClass;
typedef PEntity *(*GSpawnFunc)(struct GEntityClass *);

/* Entity class struct */
typedef struct GEntityClass {
        CNamed named;
        GSpawnFunc spawnFunc;
        CVec size;
        float z;
        char editorKey, spriteName[C_NAME_MAX];
} GEntityClass;

/* g_editor.c */
bool G_dispatch_editor(GEvent);

/* g_entity.c */
void GEntityClass_init(GEntityClass *);
bool GEntityClass_parseToken(GEntityClass *, FILE *, const char *token);
void G_depthSortEntity(PEntity *);
void G_pushBackEntity(PEntity *);
void G_pushForwardEntity(PEntity *);
PEntity *G_spawn(const char *className);

extern CNamed *g_classRoot;

/* GBox.c */
struct GBox;
struct GBoxClass;
void GBox_parseClass(FILE *file, const char *className);
struct GBox *GBox_spawn(struct GBoxClass *);

/* GFountain.c */
struct GFountain;
struct GFountainClass;
void GFountain_parseClass(FILE *, const char *className);
struct GFountain *GFountain_spawn(struct GFountainClass *);

/* GGroup.c */
void GGroup_parseClass(FILE *, const char *className);

/* g_input.c */
bool G_controlEvent(GEvent);

extern CVec g_control, g_mouse;
extern int g_key, g_button;
extern bool g_shift, g_alt, g_ctrl;

/* g_menu.c */
bool G_dispatch_menu(GEvent);
void G_hideMenu(void);

extern bool g_limbo;

/* GMissile.c */
struct GMissile *G_fireMissile(PEntity *parent, const char *className,
                               CVec from, CVec at);
void GMissile_parseClass(FILE *, const char *className);

/* g_player.c */
bool G_dispatch_player(GEvent);
void G_drawHud(void);
void G_spawnPlayer(CVec origin);
void G_updatePlayer(void);

