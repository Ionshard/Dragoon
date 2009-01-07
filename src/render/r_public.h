/******************************************************************************\
 Dragoon - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Internal screen size */
#define R_WIDTH 256
#define R_HEIGHT 192

/* Longest printable string */
#define R_TEXT_MAX 64

/* Menu fade-in/out rate */
#define R_MENU_FADE 2

/* Animated sprite object */
typedef struct {
        struct RSpriteData *data;
        CColor modulate;
        CVec origin, size;
        float angle, z;
        bool flip, mirror;
} RSprite;

/* Text object */
typedef struct {
        char string[R_TEXT_MAX];
        RSprite sprites[R_TEXT_MAX];
        struct RTexture *texture;
        CVec boxSize, origin, explode;
        CColor modulate;
        float jiggleRadius, jiggleSpeed, z;
} RText;

/* Menu option object */
typedef struct RMenuOption {
        RText text;
        struct RMenuOption *next;
        float fade, value;
} RMenuOption;

/* Menu entry object */
typedef struct RMenuEntry {
        RText label;
        struct RMenuEntry *next;
        RMenuOption *options, *selected;
        CCallback onActivate;
        float fade;
        bool enabled;
} RMenuEntry;

/* Menu container object */
typedef struct {
        RMenuEntry *entries, *selected;
        CVec origin, size;
        float fade, z;
        bool shown;
} RMenu;

/* r_draw.c */
void R_drawRect(CVec origin, float z, CVec size, CColor add, CColor mod);

/* RMenu.c */
void RMenu_activate(RMenu *, bool next);
void RMenu_add(RMenu *, RMenuEntry *, float margin);
void RMenu_cleanup(RMenu *);
#define RMenu_init(m) C_zero(m)
void RMenu_select(RMenu *, bool up);
void RMenu_update(RMenu *);
RMenuOption *RMenuEntry_add(RMenuEntry *, const char *text, float value);
RMenuEntry *RMenuEntry_new(const char *label, CCallback onActivate);

/* r_mode.c */
void R_begin(void);
void R_beginCam(void);
void R_end(void);
void R_endCam(void);
void R_initGl(void);
bool R_setVideoMode(void);

extern CCount r_countFaces;
extern CVec r_camera;
extern int r_height, r_width, r_widthScaled, r_heightScaled, r_scale;
extern bool r_clear, r_fullscreen;

/* RSprite.c */
void R_cleanupSprites(void);
void R_cleanupTextures(void);
void R_parseSpriteCfg(const char *filename);
bool RSprite_init(RSprite *, const char *name);
void RSprite_center(RSprite *, CVec origin, CVec size);
void RSprite_draw(const RSprite *);
CVec R_spriteSize(const char *name);

/* RText.c */
void RText_init(RText *, const char *font, const char *string);
void RText_draw(RText *);
CVec RText_size(const RText *);

