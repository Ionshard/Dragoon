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
#include "r_public.h"

/* Number of random animation names for a sprite */
#define R_NEXT_NAMES 8

/* 2D vertex */
#pragma pack(push, 4)
typedef struct {
        CVec uv, co;
        float z;
} RVertex;
#pragma pack(pop)
#define RV_FORMAT GL_T2F_V3F

/* Texture object */
typedef struct RTexture {
        CNamed named;
        CVec scaleUV;
        SDL_Surface *surface;
        GLuint glName;
        int pow2Width, pow2Height;
        bool upScale, tile;
} RTexture;

/* Sprite tile modes */
typedef enum {
        RST_SCALED = 0,
        RST_TILE,
        RST_TILE_GLOBAL,
        RST_TILE_PARALLAX,
} RSpriteTile;

/* Sprite data object */
typedef struct RSpriteData {
        CNamed named;
        RTexture *texture, *tiled, *edges[4];
        RSpriteTile tile;
        CColor modulate;
        CVec boxOrigin, boxSize, center, scale, corner;
        float parallax;
        int nextMsec;
        bool additive, flip, mirror, upscale;
        char nextNames_len, nextNames[R_NEXT_NAMES][C_NAME_MAX];
} RSpriteData;

/* r_mode.c */
#if CHECKED
#define R_checkErrors() R_checkErrors_full(__func__)
void R_checkErrors_full(const char *func);
#else
#define R_checkErrors() ;
#endif

extern int r_scale;
extern bool r_cameraOn;

/* r_surface.c */
SDL_Surface *R_allocSurface(int width, int height);
void R_blitSurface(SDL_Surface *src, SDL_Surface *dest,
                   int sx, int sy, int sw, int sh,
                   int dx, int dy, int dw, int dh);
void R_deseamSurface(SDL_Surface *);
void R_flipSurface(SDL_Surface *);
void R_freeSurface(SDL_Surface *);
CColor R_getPixel(const SDL_Surface *, int x, int y);
void R_putPixel(SDL_Surface *, int x, int y, CColor);
SDL_Surface *R_loadSurface(const char *filename);
int R_saveSurface(SDL_Surface *, const char *filename);
void R_scaleSurface(SDL_Surface *src, SDL_Surface *dest,
                    int scale_x, int scale_y, int dx, int dy);
SDL_Surface *R_screenSurface(int x, int y, int w, int h);
#define R_surfaceSize(s) CVec_xy((s)->w, (s)->h)

extern int r_videoMem, r_videoMemMax;

/* RTexture.c */
void RTexture_cleanup(RTexture *);
void RTexture_free(RTexture *);
#define RTexture_deselect() RTexture_select(NULL, FALSE, FALSE)
RTexture *RTexture_extract(const RTexture *, int x, int y, int w, int h);
RTexture *RTexture_load(const char *filename);
void RTexture_select(RTexture *, bool smooth, bool additive);
#define RTexture_size(t) \
        ((t) && (t)->surface ? R_surfaceSize((t)->surface) : CVec(0, 0))
void RTexture_upload(RTexture *);

