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
        if (!texture)
                return;
        R_freeSurface(texture->surface);
        if (texture->glName)
                glDeleteTextures(1, &texture->glName);
}

/******************************************************************************\
 Free a texture object.
\******************************************************************************/
void RTexture_free(RTexture *texture)
{
        if (texture)
                CNamed_free(&texture->named);
}

/******************************************************************************\
 Cleanup texture database.
\******************************************************************************/
void R_cleanupTextures(void)
{
        CNamed_freeAll(&textureRoot);
}

/******************************************************************************\
 Reset texture cache after a resolution change.
\******************************************************************************/
void R_resetTextures(void)
{
        RTexture *pt;

        if (!textureRoot)
                return;
        for (pt = (RTexture *)textureRoot; pt; 
             pt = (RTexture *)pt->named.next) {
                pt->upScale = FALSE;
                if (WINDOWS) {
                        glDeleteTextures(1, &pt->glName);
                        pt->glName = 0;
                }
        }
        C_debug("Reset texture cache");
        R_checkErrors();
}

/******************************************************************************\
 If the texture's SDL surface has changed, the image must be reloaded into
 OpenGL. This function will do this. It is assumed that the texture surface
 format has not changed since the texture was created.
\******************************************************************************/
void RTexture_upload(RTexture *pt)
{
        SDL_Surface *pow2Surface;
        int realWidth, realHeight, scale;

        /* Texture has no surface data */
        if (!pt->surface)
                return;

        /* Surface size can be upscaled or not */
        scale = 1;
        realWidth = pt->surface->w;
        realHeight = pt->surface->h;
        if (pt->upScale) {
                realWidth *= scale = r_scale;
                realHeight *= scale;
        }

        /* Tiled texture span the entire surface */
        if (pt->tile) {
                pt->scaleUV = CVec(1, 1);

                /* Allocate power-of-two surface */
                pt->pow2Width = C_nextPow2(realWidth);
                pt->pow2Height = C_nextPow2(realHeight);

                /* We can just upload this surface if its already
                   power-of-two */
                if (pt->pow2Width == pt->surface->w &&
                    pt->pow2Height == pt->surface->h)
                        pow2Surface = pt->surface;

                /* Otherwise we need to blit onto a new surface */
                else {
                        pow2Surface = R_allocSurface(pt->pow2Width,
                                                     pt->pow2Height);
                        R_blitSurface(pt->surface, pow2Surface,
                                      0, 0, pt->surface->w, pt->surface->h,
                                      0, 0, pt->pow2Width, pt->pow2Height);
                }
        }

        /* Otherwise we use texture coords to isolate a piece and add a
           one pixel border around the texture */
        else {
                pt->pow2Width = C_nextPow2(realWidth + 1);
                pt->pow2Height = C_nextPow2(realHeight + 1);

                /* We can just upload this surface if its already
                   power-of-two */
                if (pt->pow2Width == pt->surface->w &&
                    pt->pow2Height == pt->surface->h)
                        pow2Surface = pt->surface;

                /* Otherwise we need to blit onto a new surface */
                else {
                        pow2Surface = R_allocSurface(pt->pow2Width,
                                                     pt->pow2Height);
                        R_scaleSurface(pt->surface, pow2Surface,
                                       scale, scale, 1, 1);
                }

                /* Save UV scale */
                pt->scaleUV = CVec((float)realWidth / pt->pow2Width,
                                   (float)realHeight / pt->pow2Height);
        }

        /* Upload the texture to OpenGL and build mipmaps */
        glBindTexture(GL_TEXTURE_2D, pt->glName);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pow2Surface->w, pow2Surface->h,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, pow2Surface->pixels);
        pt->frame = c_frame;

        /* Repeat wrapping (not supported for NPOT textures) */
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        /* Free the temporary surface */
        if (pow2Surface != pt->surface)
                R_freeSurface(pow2Surface);

        R_checkErrors();
}

/******************************************************************************\
 Load a texture image from disk.
\******************************************************************************/
RTexture *RTexture_load(const char *filename)
{
        RTexture *texture;

        texture = CNamed_alloc(&textureRoot, filename, sizeof (RTexture),
                               (CCallback)RTexture_cleanup, CNP_RETURN);
        if (texture->surface)
                return texture;
        texture->surface = R_loadSurface(filename);
        R_deseamSurface(texture->surface);
        return texture;
}

/******************************************************************************\
 Allocates a blank texture.
\******************************************************************************/
RTexture *RTexture_alloc(int w, int h)
{
        RTexture *texture;

        C_new(&texture);
        texture = CNamed_alloc(&textureRoot, NULL, sizeof (RTexture),
                               (CCallback)RTexture_cleanup, CNP_RETURN);
        texture->surface = R_allocSurface(w, h);
        return texture;
}

/******************************************************************************\
 Cuts a tilable chunk out of an already loaded texture.
\******************************************************************************/
RTexture *RTexture_extract(const RTexture *src, int x, int y, int w, int h)
{
        RTexture *dest;

        if (!src || !src->surface)
                return NULL;

        /* Allocate a new surface and copy the portion of the old surface
           onto it */
        dest = RTexture_alloc(w, h);
        dest->tile = TRUE;
        R_blitSurface(src->surface, dest->surface,
                      x, y, w, h, 0, 0, w, h);
        R_deseamSurface(dest->surface);
        return dest;
}

/******************************************************************************\
 Selects (binds) a texture for rendering in OpenGL. Also sets whatever options
 are necessary to get the texture to show up properly.
\******************************************************************************/
void RTexture_select(RTexture *texture, bool smooth, bool additive)
{
        bool upload;

        if (!texture) {
                glDisable(GL_TEXTURE_2D);
                return;
        }

        /* Make sure the texture has been uploaded to OpenGL */
        upload = FALSE;
        if (!texture->glName) {
                glGenTextures(1, &texture->glName);
                upload = TRUE;
        }

        /* Make sure that any texture we want to smooth has been upscaled */
        if (smooth && !texture->upScale) {
                texture->upScale = TRUE;
                upload = TRUE;
        }
        
        /* Stale textures must be uploaded again */
        if (texture->frame < r_initFrame)
                upload = TRUE;

        /* Select a blending function */
        if (additive)
                glBlendFunc(GL_ONE, GL_ONE);
        else
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        /* Upload texture to OpenGL */
        if (upload)
                RTexture_upload(texture);

        /* Select texture */
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture->glName);

        /* Scale filters */
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                        smooth ? GL_LINEAR : GL_NEAREST);

        /* Non-power-of-two textures are pasted onto larger textures that
           require a texture coordinate transformation */
        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        if (!texture->tile)
                glTranslatef(1.f / texture->pow2Width,
                             1.f / texture->pow2Height, 0.f);
        glScalef(texture->scaleUV.x, texture->scaleUV.y, 1.f);
        glMatrixMode(GL_MODELVIEW);

        R_checkErrors();
}

