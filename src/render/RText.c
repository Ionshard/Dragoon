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
void RText_init_range(RText *text, const char *font, int start,
                      int cols, int rows, const char *string)
{
        int i;

        if (!text)
                return;
        C_zero(text);
        C_strncpy_buf(text->string, string);
        text->modulate = CColor_white();
        text->rows = rows;
        text->cols = cols;
        text->start = start;
        text->scale = CVec(1, 1);
        text->timeMsec = &c_timeMsec;

        /* Load font texture */
        if (!font || !font[0])
                font = "gfx/font.png";
        if (!(text->texture = RTexture_load(font)) || !text->texture->surface)
                return;

        /* Setup sprites array */
        text->boxSize = RText_naturalSize(text);
        for (i = 0; i < R_TEXT_MAX; i++) {
                text->sprites[i].size = text->boxSize;
                text->sprites[i].modulate = CColor_white();
        }
}

/******************************************************************************\
 Returns natural size for a text character.
\******************************************************************************/
CVec RText_naturalSize(const RText *text)
{
        if (!text || !text->texture || !text->texture->surface)
                return CVec(0, 0);
        return CVec(text->texture->surface->w / text->cols - 1,
                    text->texture->surface->h / text->rows - 1);
}

/******************************************************************************\
 Draw a text object on the screen.
\******************************************************************************/
void RText_draw(RText *text)
{
        RSpriteData spriteData;
        CVec halfSize, offsetSize;
        int i, seed, ch_max;
        float explodeNorm;

        if (!text->texture || !text->texture->surface)
                return;

        /* Setup fake sprite data */
        C_zero(&spriteData);
        spriteData.texture = text->texture;
        spriteData.modulate = text->modulate;
        spriteData.boxSize = text->boxSize;
        spriteData.scale = text->scale;
        halfSize = CVec_divf(spriteData.boxSize, 2);
        offsetSize = CVec_addf(spriteData.boxSize, 1);

        /* Draw letters */
        glMatrixMode(GL_MODELVIEW);
        seed = C_randDet((int)(size_t)text);
        explodeNorm = text->explode.x || text->explode.y ?
                      sqrtf(CVec_len(text->explode)) : 0;
        ch_max = text->rows * text->cols;
        for (i = 0; text->string[i]; i++) {
                CVec origin, coords, diff, size_old;
                int ch;

                glPushMatrix();
                origin = text->origin;
                origin.x += text->boxSize.x * i * text->scale.x;

                /* Letter jiggle effect */
                if (text->jiggleRadius > 0) {
                        float time;

                        time = *text->timeMsec * text->jiggleSpeed;
                        diff = CVec(sin(time + 787 * i), cos(time + 386 * i));
                        diff = CVec_scalef(diff, text->jiggleRadius);
                        origin = CVec_add(origin, diff);
                        text->sprites[i].angle = 0.1 * text->jiggleRadius *
                                                 sin(time + 911 * i);
                } else
                        text->sprites[i].angle = 0;

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
                if ((ch = text->string[i] - text->start) < 0 || ch >= ch_max) {
                        glPopMatrix();
                        continue;
                }
                coords = CVec(ch % text->cols, ch / text->cols);
                spriteData.boxOrigin = CVec_scale(offsetSize, coords);
                spriteData.center = halfSize;
                text->sprites[i].data = &spriteData;
                size_old = text->sprites[i].size;
                text->sprites[i].size = CVec_scale(text->sprites[i].size,
                                                   text->scale);
                text->sprites[i].timeMsec = text->timeMsec;
                RSprite_draw(text->sprites + i);
                text->sprites[i].size = size_old;
                glPopMatrix();
        }
}

/******************************************************************************\
 Return the dimensions of a text object.
\******************************************************************************/
CVec RText_size(const RText *text)
{
        return CVec_scale(CVec(strlen(text->string) * text->boxSize.x,
                               text->boxSize.y), text->scale);
}

/******************************************************************************\
 Center a text object on a point.
\******************************************************************************/
void RText_center(RText *text, CVec origin)
{
        CVec size;

        size = RText_size(text);
        text->origin = CVec(origin.x - size.x / 2, origin.y - size.y / 2);
}

