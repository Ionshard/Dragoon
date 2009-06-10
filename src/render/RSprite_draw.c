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
 Renders a window sprite. A window sprite is composed of a grid of nine quads
 where the corner quads have a fixed size and the connecting quads stretch to
 fill the rest of the sprite size.

 The array is indexed this way:

   0---2------4---6
   |   |      |   |
   1---3------5---7
   |   |      |   |
   |   |      |   |
   11--10-----9---8
   |   |      |   |
   12--13-----14--15

\******************************************************************************/
static void RSprite_draw_window(RSprite *sprite, bool smooth)
{
        RSpriteData *data = sprite->data;
        RVertex verts[16];
        CVec corner, corner_uv, surfaceSize, uvSize;

        /* If the window dimensions are too small to fit the corners in,
           we need to trim the corner size a little */
        corner = sprite->data->corner;
        if (sprite->size.x <= corner.x * 2)
                corner.x = sprite->size.x / 2;
        if (sprite->size.y <= corner.y * 2)
                corner.y = sprite->size.y / 2;
        surfaceSize = RTexture_size(data->texture);
        uvSize = CVec_div(data->boxSize, surfaceSize);
        corner_uv = CVec_div(corner, surfaceSize);
        corner = CVec_div(corner, sprite->size);

        /* Vertex coordinates */
        verts[0].co = CVec(-0.5, -0.5);
        verts[1].co = CVec(-0.5, -0.5 + corner.y);
        verts[2].co = CVec(-0.5 + corner.x, -0.5);
        verts[3].co = CVec(-0.5 + corner.x, -0.5 + corner.y);
        verts[4].co = CVec(0.5 - corner.x, -0.5);
        verts[5].co = CVec(0.5 - corner.x, -0.5 + corner.y);
        verts[6].co = CVec(0.5, -0.5);
        verts[7].co = CVec(0.5, -0.5 + corner.y);
        verts[8].co = CVec(0.5, 0.5 - corner.y);
        verts[9].co = CVec(0.5 - corner.x, 0.5 - corner.y);
        verts[10].co = CVec(-0.5 + corner.x, 0.5 - corner.y);
        verts[11].co = CVec(-0.5, 0.5 - corner.y);
        verts[12].co = CVec(-0.5, 0.5);
        verts[13].co = CVec(-0.5 + corner.x, 0.5);
        verts[14].co = CVec(0.5 - corner.x, 0.5);
        verts[15].co = CVec(0.5, 0.5);

        /* Vertex UVs */
        verts[0].uv = CVec_div(data->boxOrigin, surfaceSize);
        verts[1].uv = CVec(verts[0].uv.x, verts[0].uv.y + corner_uv.y);
        verts[2].uv = CVec(verts[0].uv.x + corner_uv.x, verts[0].uv.y);
        verts[3].uv = CVec(verts[2].uv.x, verts[1].uv.y);
        verts[4].uv = CVec(verts[0].uv.x + uvSize.x - corner_uv.x,
                           verts[0].uv.y);
        verts[5].uv = CVec(verts[4].uv.x, verts[1].uv.y);
        verts[6].uv = CVec(verts[0].uv.x + uvSize.x, verts[0].uv.y);
        verts[7].uv = CVec(verts[6].uv.x, verts[1].uv.y);
        verts[8].uv = CVec(verts[6].uv.x,
                           verts[0].uv.y + uvSize.y - corner_uv.y);
        verts[9].uv = CVec(verts[4].uv.x, verts[8].uv.y);
        verts[10].uv = CVec(verts[2].uv.x, verts[8].uv.y);
        verts[11].uv = CVec(verts[0].uv.x, verts[8].uv.y);
        verts[12].uv = CVec(verts[0].uv.x, verts[0].uv.y + uvSize.y);
        verts[13].uv = CVec(verts[2].uv.x, verts[12].uv.y);
        verts[14].uv = CVec(verts[4].uv.x, verts[12].uv.y);
        verts[15].uv = CVec(verts[6].uv.x, verts[12].uv.y);

        /* Untiled quads can be rendered in one call */
        if (!sprite->data->tile) {
                unsigned short indices[] = {
                        0, 1, 3, 2,
                        2, 3, 5, 4,
                        4, 5, 7, 6,
                        1, 11, 10, 3,
                        3, 10, 9, 5,
                        5, 9, 8, 7,
                        11, 12, 13, 10,
                        10, 13, 14, 9,
                        9, 14, 15, 8,
                };

                RTexture_select(data->texture, smooth, data->additive);
                glInterleavedArrays(RV_FORMAT, 0, verts);
                glDrawElements(GL_QUADS, 36, GL_UNSIGNED_SHORT, indices);
        }

        /* The tiled portions of the window must be rendered separately */
        else {
                RVertex tVerts[4];
                CVec corners, corner_prop;
                unsigned short indices[] = {
                        0, 1, 3, 2,
                        4, 5, 7, 6,
                        11, 12, 13, 10,
                        9, 14, 15, 8,
                };

                /* Corner proportion of tiled texture */
                corner_prop = CVec_div(CVec_divf(sprite->size, 2),
                                       sprite->data->corner);
                if (corner_prop.x > 1)
                        corner_prop.x = 1;
                if (corner_prop.y > 1)
                        corner_prop.y = 1;

                /* First render the non-tiling portions */
                RTexture_select(data->texture, smooth, data->additive);
                glInterleavedArrays(RV_FORMAT, 0, verts);
                glDrawElements(GL_QUADS, 16, GL_UNSIGNED_SHORT, indices);

                /* Top quad */
                RTexture_select(data->edges[0], smooth, data->additive);
                corners = CVec_scalef(data->corner, 2);
                uvSize = CVec_div(CVec_sub(sprite->size, corners),
                                  CVec_sub(data->boxSize, corners));
                tVerts[0].co = verts[2].co;
                tVerts[1].co = verts[3].co;
                tVerts[2].co = verts[5].co;
                tVerts[3].co = verts[4].co;
                tVerts[0].uv = CVec(0, 0);
                tVerts[1].uv = CVec(0, corner_prop.y);
                tVerts[2].uv = CVec(uvSize.x, corner_prop.y);
                tVerts[3].uv = CVec(uvSize.x, 0);
                glInterleavedArrays(RV_FORMAT, 0, tVerts);
                glDrawArrays(GL_QUADS, 0, 4);

                /* Bottom quad */
                RTexture_select(data->edges[3], smooth, data->additive);
                tVerts[0].co = verts[10].co;
                tVerts[1].co = verts[13].co;
                tVerts[2].co = verts[14].co;
                tVerts[3].co = verts[9].co;
                glInterleavedArrays(RV_FORMAT, 0, tVerts);
                glDrawArrays(GL_QUADS, 0, 4);

                /* Left quad */
                RTexture_select(data->edges[1], smooth, data->additive);
                tVerts[0].co = verts[1].co;
                tVerts[1].co = verts[11].co;
                tVerts[2].co = verts[10].co;
                tVerts[3].co = verts[3].co;
                tVerts[1].uv = CVec(0, uvSize.y);
                tVerts[2].uv = CVec(corner_prop.x, uvSize.y);
                tVerts[3].uv = CVec(corner_prop.x, 0);
                glInterleavedArrays(RV_FORMAT, 0, tVerts);
                glDrawArrays(GL_QUADS, 0, 4);

                /* Right quad */
                RTexture_select(data->edges[2], smooth, data->additive);
                tVerts[0].co = verts[5].co;
                tVerts[1].co = verts[9].co;
                tVerts[2].co = verts[8].co;
                tVerts[3].co = verts[7].co;
                glInterleavedArrays(RV_FORMAT, 0, tVerts);
                glDrawArrays(GL_QUADS, 0, 4);

                /* Middle quad */
                RTexture_select(data->tiled, smooth, data->additive);
                tVerts[0].co = verts[3].co;
                tVerts[1].co = verts[10].co;
                tVerts[2].co = verts[9].co;
                tVerts[3].co = verts[5].co;
                tVerts[1].uv = CVec(0, uvSize.y);
                tVerts[2].uv = CVec(uvSize.x, uvSize.y);
                tVerts[3].uv = CVec(uvSize.x, 0);
                glInterleavedArrays(RV_FORMAT, 0, tVerts);
                glDrawArrays(GL_QUADS, 0, 4);
        }

        /* Count quads */
        if (CHECKED)
                CCount_add(&r_countFaces, 18);
}

/******************************************************************************\
 Draw a regular quad sprite on screen.
\******************************************************************************/
static void RSprite_draw_quad(RSprite *sprite, bool smooth)
{
        const RSpriteData *data = sprite->data;
        RVertex verts[4];
        RTexture *texture;
        CVec surfaceSize, origin;
        const unsigned short indices[] = { 0, 1, 2, 3, 0 };

        /* Select a texture */
        if (data->tile) {
                texture = data->tiled;

                /* Non-power-of-two tiles need to be upscaled */
                if (data->tile && (texture->pow2Width != texture->surface->w ||
                                   texture->pow2Height != texture->surface->h))
                        smooth = TRUE;
        } else
                texture = data->texture;
        RTexture_select(texture, smooth, data->additive);

        /* Setup textured quad vertex positions */
        surfaceSize = RTexture_size(texture);
        verts[0].co = CVec_xy(-0.5f, -0.5f);
        verts[0].z = 0.f;
        verts[2].co = CVec_xy(0.5f, 0.5f);
        verts[2].z = 0.f;
        verts[1].co = CVec_xy(-0.5f, 0.5f);
        verts[1].z = 0.f;
        verts[3].co = CVec_xy(0.5f, -0.5f);
        verts[3].z = 0.f;

        /* Setup vertex UV coordinates */
        if (data->tile == RST_TILE_GLOBAL) {
                origin = CVec_add(sprite->origin, data->tileOrigin);
                verts[0].uv = CVec_div(origin, surfaceSize);
                verts[2].uv = CVec_div(CVec_add(origin, sprite->size),
                                       surfaceSize);
        } else if (data->tile == RST_TILE_PARALLAX) {
                origin = CVec_sub(CVec_add(sprite->origin, data->tileOrigin),
                                  CVec_scalef(r_camera, data->parallax));
                verts[0].uv = CVec_div(origin, surfaceSize);
                verts[2].uv = CVec_div(CVec_add(origin, sprite->size),
                                       surfaceSize);
        } else if (data->tile) {
                verts[0].uv = CVec(0, 0);
                verts[2].uv = CVec_div(sprite->size, surfaceSize);
        } else {
                verts[0].uv = CVec_div(data->boxOrigin, surfaceSize);
                verts[2].uv = CVec_div(CVec_add(data->boxOrigin, data->boxSize),
                                       surfaceSize);
        }

        /* Scale UV for tiled sprites */
        if (data->tile) {
                verts[0].uv = CVec_div(verts[0].uv, data->scale);
                verts[2].uv = CVec_div(verts[2].uv, data->scale);
        }

        verts[1].uv.x = verts[0].uv.x;
        verts[1].uv.y = verts[2].uv.y;
        verts[3].uv.x = verts[2].uv.x;
        verts[3].uv.y = verts[0].uv.y;

        /* Render textured quad */
        if (CHECKED)
                CCount_add(&r_countFaces, 2);
        glInterleavedArrays(RV_FORMAT, 0, verts);
        glDrawElements(GL_QUADS, 4, GL_UNSIGNED_SHORT, indices);
}

/******************************************************************************\
 Draw a sprite on screen.
\******************************************************************************/
void RSprite_draw(RSprite *sprite)
{
        CColor modulate;
        CVec center;
        bool smooth, flip, mirror;

        if (!sprite || !sprite->data || sprite->z < 0.f ||
            sprite->modulate.a <= 0.f)
                return;

        /* Check if sprite is on-screen */
        if (!CVec_intersect(r_cameraOn ? r_camera : CVec(0, 0),
                            CVec(r_widthScaled, r_heightScaled),
                            sprite->origin, sprite->size))
                return;

        /* Setup transformation matrix */
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        center = CVec_divf(sprite->size, 2);
        glTranslatef(sprite->origin.x + center.x,
                     sprite->origin.y + center.y, sprite->z);
        if ((smooth = sprite->angle != 0.f)) {
                CVec trans;

                trans = CVec_sub(RSprite_getCenter(sprite), center);
                glTranslatef(trans.x, trans.y, 0);
                glRotatef(C_radToDeg(sprite->angle), 0.0, 0.0, 1.0);
                glTranslatef(-trans.x, -trans.y, 0);
        }
        flip = sprite->flip ^ sprite->data->flip;
        mirror = sprite->mirror ^ sprite->data->mirror;
        glScalef(mirror ? -sprite->size.x : sprite->size.x,
                 flip ? -sprite->size.y : sprite->size.y, 0);

        /* Additive blending */
        if (sprite->data->additive) {
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                glDisable(GL_ALPHA_TEST);
        }

        /* Alpha blending */
        else {
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glEnable(GL_ALPHA_TEST);
        }

        /* Modulate color */
        modulate = CColor_scale(sprite->modulate, sprite->data->modulate);
        glColor4f(modulate.r, modulate.g, modulate.b, modulate.a);

        /* Render the sprite quad(s) */
        smooth |= sprite->data->upscale;
        if (sprite->data->corner.x || sprite->data->corner.y)
                RSprite_draw_window(sprite, smooth);
        else
                RSprite_draw_quad(sprite, smooth);

        /* Cleanup */
        glPopMatrix();
        R_checkErrors();

        /* Pump animation */
        RSprite_animate(sprite);
}

