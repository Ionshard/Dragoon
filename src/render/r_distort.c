/******************************************************************************\
 Dragoon - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "r_private.h"

/* Distortion grid block size */
float r_distortGrid = 0;

static RVertex *gridVerts;
static RTexture *screenTex;
static int gridWidth, gridHeight;
static unsigned short *gridIndices;

/******************************************************************************\
 Initialize resources for distortion effects.
\******************************************************************************/
void R_initDistort(void)
{
        int r, c;

        R_cleanupDistort();
        if (r_distortGrid <= 0)
                return;
        if ((gridWidth = ceilf(r_width / r_distortGrid)) < 2)
                gridWidth = 2;
        if ((gridHeight = ceilf(r_height / r_distortGrid)) < 2)
                gridHeight = 2;

        /* Initialize a grid of vertices (quads) */
        gridVerts = C_malloc((gridWidth + 1) * (gridHeight + 1) *
                             sizeof (*gridVerts));
        for (r = 0; r <= gridHeight; r++)
                for (c = 0; c <= gridWidth; c++) {
                        RVertex * v;

                        v = gridVerts + r * gridWidth + c;
                        v->co.x = (float)c * r_widthScaled / gridWidth;
                        v->co.y = (float)r * r_heightScaled / gridHeight;
                        v->uv.x = (float)c / gridWidth;
                        v->uv.y = 1 - (float)r / gridHeight;
                        v->z = 1;
                }

        /* Initialize quad indices */
        gridIndices = C_malloc(4 * gridWidth * gridHeight *
                               sizeof (*gridIndices));
        for (r = 0; r < gridHeight; r++)
                for (c = 0; c < gridWidth; c++) {
                        unsigned short i;

                        i = 4 * (r * gridWidth + c);
                        gridIndices[i] = r * gridWidth + c;
                        gridIndices[i + 1] = (r + 1) * gridWidth + c;
                        gridIndices[i + 2] = (r + 1) * gridWidth + c + 1;
                        gridIndices[i + 3] = r * gridWidth + c + 1;
                }

        /* Allocate screen buffer texture */
        screenTex = RTexture_alloc(r_width, r_height);
        screenTex->tile = TRUE;

        C_debug("%dx%d distortion grid", gridWidth, gridHeight);
}

/******************************************************************************\
 Free resources for distortion effects.
\******************************************************************************/
void R_cleanupDistort(void)
{
        C_free(gridVerts);
        gridVerts = NULL;
        C_free(gridIndices);
        gridIndices = NULL;
        RTexture_free(screenTex);
        screenTex = NULL;
}

/******************************************************************************\
 Render distortions for this frame.
\******************************************************************************/
void R_drawDistort(void)
{
        if (r_distortGrid <= 0)
                return;

        /* Read in the front buffer */
        if (SDL_LockSurface(screenTex->surface) < 0)
                return;
        glReadPixels(0, 0, r_width, r_height, GL_RGBA, GL_UNSIGNED_BYTE,
                     screenTex->surface->pixels);
        SDL_UnlockSurface(screenTex->surface->pixels);
        RTexture_upload(screenTex);
        RTexture_select(screenTex, FALSE, FALSE);

        /* Disable alpha-blending and depth testing */
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);

        /* Draw distortion grid */
        glInterleavedArrays(RV_FORMAT, 0, gridVerts);
        glDrawElements(GL_QUADS, 4 * gridWidth * gridHeight,
                       GL_UNSIGNED_SHORT, gridIndices);

        /* Re-enable alpha-blending and depth testing */
        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);

        R_checkErrors();
}

