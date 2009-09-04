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

/* Clipping limit */
#define R_CLIP_LIMIT 100000

/* Menu visibility types */
typedef enum {
        R_MENU_HIDE_RIGHT,
        R_MENU_SHOWN,
        R_MENU_HIDE_LEFT,
} RMenuVisibility;

/* Animated sprite object */
typedef struct {
        struct RSpriteData *data;
        CColor modulate;
        CVec origin, size;
        float angle, z;
        int *timeMsec, initMsec;
        bool flip, mirror;
        char name[C_NAME_MAX];
} RSprite;

/* Text object */
typedef struct {
        char string[R_TEXT_MAX];
        RSprite sprites[R_TEXT_MAX];
        struct RTexture *texture;
        CVec scale, boxSize, origin, explode;
        CColor modulate;
        float jiggleRadius, jiggleSpeed, z;
        int rows, cols, start, *timeMsec;
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
        void *data;
} RMenuEntry;

/* Menu container object */
typedef struct {
        RMenuEntry *entries, *selected;
        CVec origin, size;
        float fade, z;
        bool shown, hideLeft;
} RMenu;

/* r_distort.c */
void R_cleanupDistort(void);
void R_drawDistort(void);
void R_initDistort(void);

/* r_draw.c */
void R_clip(CVec origin, CVec size);
void R_clip_disable(void);
void R_clip_left(float);
void R_clip_top(float);
void R_clip_right(float);
void R_clip_bottom(float);
void R_drawRect(CVec origin, float z, CVec size, CColor add, CColor mod);
void R_updateShake(CVec *offset, CVec *vel, float accel, float drag, float sec,
                   float offset_max);
void R_pushClip(void);
void R_popClip(void);

/* RMenu.c */
void RMenu_activate(RMenu *, bool next);
void RMenu_activate_last(RMenu *);
void RMenu_add(RMenu *, RMenuEntry *, float margin);
void RMenu_cleanup(RMenu *);
#define RMenu_init(m) C_zero(m)
void RMenu_scroll(RMenu *, bool up);
void RMenu_update(RMenu *);
RMenuOption *RMenuEntry_append(RMenuEntry *, const char *text, float value);
RMenuEntry *RMenuEntry_new(const char *label, CCallback onActivate);
RMenuOption *RMenuEntry_prepend(RMenuEntry *, const char *text, float value);
bool RMenuEntry_selectable(const RMenuEntry *);

/* r_mode.c */
void R_begin(void);
void R_beginCam(void);
void R_end(void);
void R_endCam(void);
void R_initGl(void);
const char *R_screenshot(void);
bool R_setVideoMode(void);

extern CColor r_screenFlash;
extern CCount r_countFaces;
extern CVec r_camera, r_cameraTo;
extern float r_cameraSec, r_cameraShake;
extern int r_height, r_width, r_widthScaled, r_heightScaled, r_scale;
extern bool r_clear, r_fullscreen;

/* RSprite.c, RSprite_draw.c */
void R_cleanupSprites(void);
void R_cleanupTextures(void);
void R_parseSpriteCfg(const char *filename);
void RSprite_animate(RSprite *);
#define RSprite_init(s, n) RSprite_init_time(s, n, &c_timeMsec)
bool RSprite_init_time(RSprite *, const char *name, int *timeMsec);
void RSprite_center(RSprite *, CVec origin, CVec size);
void RSprite_draw(RSprite *);
void RSprite_draw_still(RSprite *);
CVec RSprite_getCenter(const RSprite *);
void RSprite_lookAt(RSprite *, CVec origin);
bool RSprite_play(RSprite *, const char *name);
void R_parseSpriteSection(FILE *, const char *name);
#define R_parseInlineSprite(f, n, b) \
        R_parseInlineSprite_full(f, n, b, sizeof (b))
void R_parseInlineSprite_full(FILE *, const char *defaultName,
                              char *buf, int buf_size);
CVec R_spriteSize(const char *name);

/* RText.c */
#define RText_init(t, f, s) RText_init_range(t, f, 32, 16, 6, s)
void RText_init_range(RText *, const char *font, int first,
                      int cols, int rows, const char *);
void RText_center(RText *, CVec origin);
void RText_draw(RText *);
CVec RText_naturalSize(const RText *);
CVec RText_size(const RText *);

