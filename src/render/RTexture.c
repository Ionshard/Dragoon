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

static CNamed *textureRoot;

/******************************************************************************\
 Texture cleanup function.
\******************************************************************************/
void RTexture_cleanup(RTexture *texture)
{
        R_freeSurface(texture->surface);
}

/******************************************************************************\
 Cleanup texture database.
\******************************************************************************/
void R_cleanupTextures(void)
{
        CNamed_free(&textureRoot, (CCallback)RTexture_cleanup);
}

/******************************************************************************\
 If the texture's SDL surface has changed, the image must be reloaded into
 OpenGL. This function will do this. It is assumed that the texture surface
 format has not changed since the texture was created.
\******************************************************************************/
void RTexture_upload(RTexture *pt)
{
        SDL_Rect rect;
        SDL_Surface *pow2Surface;
        CVec realSize;

        /* Scale this texture onto a larger power-of-two surface */
        if (pt->upScale) {
                realSize = CVec_xy(pt->surface->w * r_scale,
                                   pt->surface->h * r_scale);
                pt->pow2Size = CVec_xy(C_nextPow2(realSize.x + 2),
                                       C_nextPow2(realSize.y + 2));
                pow2Surface = R_allocSurface(pt->pow2Size.x, pt->pow2Size.y);
                R_scaleSurface(pt->surface, pow2Surface, r_scale, 1, 1);
                pt->scaleUV = CVec_div(realSize, pt->pow2Size);
        }

        /* No scaling requested */
        else {
                realSize = CVec(pt->surface->w, pt->surface->h);
                rect.x = 1;
                rect.y = 1;
                rect.w = realSize.x;
                rect.h = realSize.y;
                pt->pow2Size = CVec_xy(C_nextPow2(realSize.x + 2),
                                       C_nextPow2(realSize.y + 2));
                pow2Surface = R_allocSurface(pt->pow2Size.x, pt->pow2Size.y);
                SDL_BlitSurface(pt->surface, NULL, pow2Surface, &rect);
                pt->scaleUV = CVec_div(realSize, pt->pow2Size);
        }

        /* Upload the texture to OpenGL and build mipmaps */
        glBindTexture(GL_TEXTURE_2D, pt->glName);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pow2Surface->w, pow2Surface->h,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, pow2Surface->pixels);

        /* Free the temporary surface */
        R_freeSurface(pow2Surface);

        R_checkErrors();
}

/******************************************************************************\
 Load a texture image from disk.
\******************************************************************************/
RTexture *RTexture_load(const char *filename)
{
        RTexture *texture;

        texture = CNamed_get(&textureRoot, filename, sizeof (RTexture));
        if (texture->surface)
                return texture;
        texture->surface = R_loadSurface(filename);

        /* Load the texture into OpenGL */
        if (texture->surface) {
                glGenTextures(1, &texture->glName);
                RTexture_upload(texture);
                R_checkErrors();
        }
        return texture;
}

/******************************************************************************\
 Selects (binds) a texture for rendering in OpenGL. Also sets whatever options
 are necessary to get the texture to show up properly.
\******************************************************************************/
void RTexture_select(const RTexture *texture)
{
        if (!texture) {
                glDisable(GL_TEXTURE_2D);
                return;
        }
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture->glName);

        /* Repeat wrapping (not supported for NPOT textures) */
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

        /* Scale filters */
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                        texture->upScale ? GL_LINEAR : GL_NEAREST);

        /* Non-power-of-two textures are pasted onto larger textures that
           require a texture coordinate transformation */
        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        glTranslatef(1.f / texture->pow2Size.x, 1.f / texture->pow2Size.y, 0.f);
        glScalef(texture->scaleUV.x, texture->scaleUV.y, 1.f);
        glMatrixMode(GL_MODELVIEW);

        R_checkErrors();
}

