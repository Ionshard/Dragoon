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

/******************************************************************************\
 Initialize a text object.
\******************************************************************************/
void RText_init(RText *text, const char *font, const char *string)
{
        int i;

        if (!text)
                return;
        C_zero(text);
        C_strncpy_buf(text->string, string);
        text->modulate = CColor_white();

        /* Load font texture */
        if (!font || !font[0])
                font = "gfx/font.png";
        if (!(text->texture = RTexture_load(font)) || !text->texture->surface)
                return;

        /* Setup sprites array */
        text->boxSize = CVec(text->texture->surface->w / 16 - 1,
                             text->texture->surface->h / 6 - 1);
        for (i = 0; text->string[i]; i++) {
                text->sprites[i].size = text->boxSize;
                text->sprites[i].modulate = CColor_white();
        }
}

/******************************************************************************\
 Draw a text object on the screen.
\******************************************************************************/
void RText_draw(RText *text)
{
        RSpriteData spriteData;
        CVec halfSize, offsetSize;
        int i, seed;
        float explodeNorm;

        if (!text->texture || !text->texture->surface)
                return;

        /* Setup fake sprite data */
        C_zero(&spriteData);
        spriteData.texture = text->texture;
        spriteData.modulate = text->modulate;
        spriteData.boxSize = text->boxSize;
        halfSize = CVec_divf(spriteData.boxSize, 2);
        offsetSize = CVec_addf(spriteData.boxSize, 1);

        /* Draw letters */
        glMatrixMode(GL_MODELVIEW);
        seed = C_randDet((int)text);
        explodeNorm = text->explode.x || text->explode.y ?
                      sqrtf(CVec_len(text->explode)) : 0;
        for (i = 0; text->string[i]; i++) {
                CVec origin, coords, diff;

                glPushMatrix();
                origin = text->origin;
                origin.x += text->boxSize.x * i;

                /* Letter jiggle effect */
                if (text->jiggleRadius > 0) {
                        float time;

                        time = c_timeMsec * text->jiggleSpeed;
                        diff = CVec(sin(time + 787 * i), cos(time + 386 * i));
                        diff = CVec_scalef(diff, text->jiggleRadius);
                        origin = CVec_add(origin, diff);
                        text->sprites[i].angle = 0.1 * text->jiggleRadius *
                                                 sin(time + 911 * i);
                }

                /* Letter explode effect */
                if (explodeNorm) {
                        seed = C_randDet(seed);
                        diff.x = (float)seed / RAND_MAX - 0.5f;
                        seed = C_randDet(seed);
                        diff.y = (float)seed / RAND_MAX - 0.5f;
                        diff = CVec_scalef(diff, explodeNorm);
                        diff = CVec_add(text->explode, diff);
                        diff = CVec_scale(diff, CVec_abs(diff));
                        origin = CVec_add(origin, diff);
                }

                glTranslatef(origin.x, origin.y, text->z);
                coords = CVec(text->string[i] % 16, text->string[i] / 16 - 2);
                spriteData.boxOrigin = CVec_scale(offsetSize, coords);
                spriteData.center = halfSize;
                text->sprites[i].data = &spriteData;
                RSprite_draw(text->sprites + i);
                glPopMatrix();
        }
}

/******************************************************************************\
 Return the dimensions of a text object.
\******************************************************************************/
CVec RText_size(const RText *text)
{
        return CVec(strlen(text->string) * text->boxSize.x, text->boxSize.y);
}

