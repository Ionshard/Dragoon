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
        CVec scaleUV, pow2Size;
        SDL_Surface *surface;
        GLuint glName;
        bool upScale;
} RTexture;

/* Sprite data object */
typedef struct RSpriteData {
        CNamed named;
        RTexture *texture;
        CColor modulate;
        CVec boxOrigin, boxSize, center;
        int nextMsec;
        bool additive, flip, mirror;
        char nextName[C_NAME_MAX];
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
void R_freeSurface(SDL_Surface *);
CColor R_getPixel(const SDL_Surface *, int x, int y);
void R_putPixel(SDL_Surface *, int x, int y, CColor);
SDL_Surface *R_loadSurface(const char *filename);
int R_saveSurface(SDL_Surface *, const char *filename);
void R_scaleSurface(SDL_Surface *src, SDL_Surface *dest,
                    int scale, int dx, int dy);
#define R_surfaceSize(s) CVec_xy((s)->w, (s)->h)

extern int r_videoMem, r_videoMemMax;

/* RTexture.c */
void RTexture_cleanup(RTexture *);
#define RTexture_deselect(t) RTexture_select(NULL, FALSE, FALSE)
RTexture *RTexture_load(const char *filename);
void RTexture_select(RTexture *, bool smooth, bool additive);
#define RTexture_size(t) \
        ((t) && (t)->surface ? R_surfaceSize((t)->surface) : CVec_zero())
void RTexture_upload(RTexture *);

