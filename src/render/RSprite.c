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

/* Sprite data named linked list */
static CNamed *dataRoot;

/******************************************************************************\
 Cleanup sprite data.
\******************************************************************************/
static void RSpriteData_cleanup(RSpriteData *data)
{
        int i;

        RTexture_free(data->tiled);
        for (i = 0; i < 4; i++)
                RTexture_free(data->edges[i]);
}

/******************************************************************************\
 Cleanup sprite database.
\******************************************************************************/
void R_cleanupSprites(void)
{
        CNamed_freeAll(&dataRoot);
}

/******************************************************************************\
 Parses a sprite section of a configuration file.
\******************************************************************************/
static void parseTileSection(FILE *file, RSpriteData *data)
{
        const char *token;

        data->tile = RST_TILE;
        if (!C_openBrace(file))
                return;
        for (token = C_token(file); token[0]; token = C_token(file))
                if (!strcasecmp(token, "global"))
                        data->tile = RST_TILE_GLOBAL;
                else if (!strcasecmp(token, "parallax")) {
                        data->tile = RST_TILE_PARALLAX;
                        data->parallax = C_token_float(file);
                }
        C_closeBrace(file);
}

void R_parseSpriteSection(FILE *file, const char *name)
{
        RSpriteData *data;
        const char *token;
        bool haveBox = FALSE, haveCenter = FALSE;

        /* Allocate a new data structure */
        data = CNamed_alloc(&dataRoot, name, sizeof (RSpriteData),
                            (CCallback)RSpriteData_cleanup, FALSE);
        if (!data) {
                C_warning("Sprite '%s' already defined", name);
                C_closeBrace(file);
                return;
        }

        /* Defaults */
        data->modulate = CColor_white();
        data->scale = CVec(1, 1);

        for (token = C_token(file); token[0]; token = C_token(file))

                /* Texture file */
                if (!strcasecmp(token, "file"))
                        data->texture = RTexture_load(C_token(file));

                /* Modulation color */
                else if (!strcasecmp(token, "color") && C_openBrace(file)) {
                        data->modulate.r = C_token_float(file) / 255.f;
                        data->modulate.g = C_token_float(file) / 255.f;
                        data->modulate.b = C_token_float(file) / 255.f;
                        data->modulate.a = (token = C_token(file))[0] ?
                                           atof(token) / 255.f : 1;
                        C_closeBrace(file);
                }

                /* Additive blending */
                else if (!strcasecmp(token, "additive"))
                        data->additive = TRUE;

                /* Flip */
                else if (!strcasecmp(token, "flip"))
                        data->flip = TRUE;

                /* Mirror */
                else if (!strcasecmp(token, "mirror"))
                        data->mirror = TRUE;

                /* Tiling */
                else if (!strcasecmp(token, "tile"))
                        parseTileSection(file, data);

                /* Bounding box */
                else if (!strcasecmp(token, "box") && C_openBrace(file)) {
                        data->boxOrigin = C_token_vec(file);
                        data->boxSize = C_token_vec(file);
                        C_closeBrace(file);
                        haveBox = TRUE;
                }

                /* Box center */
                else if (!strcasecmp(token, "center") && C_openBrace(file)) {
                        data->center = C_token_vec(file);
                        C_closeBrace(file);
                        haveCenter = TRUE;
                }

                /* Next animation frame */
                else if (!strcasecmp(token, "next") && C_openBrace(file)) {
                        for (;;) {
                                token = C_token(file);
                                if (C_isDigit(token[0]) || !token[0])
                                        break;
                                if (data->nextNames_len == R_NEXT_NAMES)
                                        C_warning("Random frame limit reached");
                                if (data->nextNames_len >= R_NEXT_NAMES)
                                        continue;
                                C_strncpy_buf(data->nextNames
                                              [(int)data->nextNames_len++],
                                              token);
                        }
                        data->nextMsec = atoi(token);
                        C_closeBrace(file);
                }

                /* Scale factor */
                else if (!strcasecmp(token, "scale")) {

                        /* Vector form */
                        if (C_openBrace(file)) {
                                data->scale = C_token_vec(file);
                                C_closeBrace(file);
                        }

                        /* Scalar form */
                        else
                                data->scale.y = data->scale.x =
                                                C_token_float(file);
                }

                /* Force upscale */
                else if (!strcasecmp(token, "upscale"))
                        data->upscale = TRUE;

                /* Window sprite */
                else if (!strcasecmp(token, "window") && C_openBrace(file)) {
                        data->corner = C_token_vec(file);
                        C_closeBrace(file);
                }

                else
                        C_warning("Unknown sprite param '%s'", token);

        C_closeBrace(file);

        /* Defaults */
        if (!haveBox)
                data->boxSize = RTexture_size(data->texture);
        if (!haveCenter)
                data->center = CVec_divf(data->boxSize, 2.f);

        /* Create tiled window subtextures */
        if ((data->corner.x || data->corner.y) && data->tile) {
                CVec origin, size;

                origin = CVec_add(data->boxOrigin, data->corner);
                size = CVec_sub(data->boxSize, CVec_scalef(data->corner, 2));
                data->tiled = RTexture_extract(data->texture, origin.x,
                                               origin.y, size.x, size.y);

                /* Extract edges: top, left, right, bottom */
                data->edges[0] = RTexture_extract(data->texture,
                                                  origin.x, data->boxOrigin.y,
                                                  size.x, data->corner.y);
                data->edges[1] = RTexture_extract(data->texture,
                                                  data->boxOrigin.x, origin.y,
                                                  data->corner.x, size.y);
                data->edges[2] = RTexture_extract(data->texture,
                                                  origin.x + size.x, origin.y,
                                                  data->corner.x, size.y);
                data->edges[3] = RTexture_extract(data->texture,
                                                  origin.x, origin.y + size.y,
                                                  size.x, data->corner.y);
        }

        /* Create tiled subtexture */
        else if (data->tile)
                data->tiled = RTexture_extract(data->texture, data->boxOrigin.x,
                                               data->boxOrigin.y,
                                               data->boxSize.x,
                                               data->boxSize.y);
}

/******************************************************************************\
 Parses the sprite configuration file.
\******************************************************************************/
void R_parseSpriteCfg(const char *filename)
{
        FILE *file;
        const char *token;

        if (!(file = C_fopen_read(filename)))
                return;
        C_debug("Parsing sprite config '%s'", filename);
        while (!feof(file)) {

                /* Read sprite name or command */
                token = C_token(file);
                if (!token[0])
                        continue;

                /* Parse commands */
                if (!C_openBrace(file)) {

                        /* Include another config */
                        if (!strcasecmp(token, "include")) {
                                token = C_token(file);
                                R_parseSpriteCfg(token);
                        }

                        else
                                C_warning("Unrecognized command '%s'", token);
                        continue;
                }

                /* Parse sprite section */
                R_parseSpriteSection(file, token);
        }
        fclose(file);
}

/******************************************************************************\
 Return the dimensions of a sprite by name.
\******************************************************************************/
CVec R_spriteSize(const char *name)
{
        RSpriteData *data;

        if (!(data = CNamed_get(dataRoot, name)))
                return CVec(0, 0);
        return CVec_scale(data->boxSize, data->scale);
}

/******************************************************************************\
 Initialize an RSprite to display a named sprite. Returns FALSE if the named
 sprite has not been loaded. Resets sprite size and modulation.
\******************************************************************************/
bool RSprite_init(RSprite *sprite, const char *name)
{
        RSpriteData *data;

        C_zero(sprite);
        if (name && name[0] && !(data = CNamed_get(dataRoot, name))) {
                C_warning("Sprite '%s' not loaded", name);
                return FALSE;
        }
        sprite->data = data;
        sprite->size = CVec_scale(data->boxSize, data->scale);
        sprite->modulate = CColor_white();
        sprite->initMsec = c_timeMsec;
        C_strncpy_buf(sprite->name, name);
        return TRUE;
}

/******************************************************************************\
 Pump the current animation frame.
\******************************************************************************/
static void RSprite_animate(RSprite *sprite)
{
        RSpriteData *data;
        CVec center;
        int next;

        if (!sprite || !sprite->data || !sprite->data->nextNames_len ||
            sprite->initMsec + sprite->data->nextMsec > c_timeMsec)
                return;
        next = (int)(C_rand() * sprite->data->nextNames_len);
        if (!(data = CNamed_get(dataRoot, sprite->data->nextNames[next]))) {
                C_warning("Sprite '%s' not loaded",
                          sprite->data->nextNames[next]);
                return;
        }
        center = CVec_add(sprite->origin, sprite->data->center);
        sprite->data = data;
        sprite->size = CVec_scale(data->boxSize, data->scale);
        sprite->initMsec = c_timeMsec;
        RSprite_center(sprite, center, CVec(0, 0));
}

/******************************************************************************\
 Change the animation of an initialized sprite.
\******************************************************************************/
bool RSprite_play(RSprite *sprite, const char *name)
{
        if (!sprite || !sprite->data)
                return FALSE;
        if (!strcmp(name, sprite->name))
                return TRUE;
        return RSprite_init(sprite, name);
}

/******************************************************************************\
 Get the coordinates of a sprite's center.
\******************************************************************************/
CVec RSprite_getCenter(const RSprite *sprite)
{
        CVec center;

        if (!sprite || !sprite->data || !sprite->size.x || !sprite->size.y ||
            !sprite->data->boxSize.x || !sprite->data->boxSize.y)
                return CVec(0, 0);
        center = sprite->data->center;
        if (sprite->mirror)
                center.x = sprite->data->boxSize.x - center.x;
        if (sprite->flip)
                center.x = sprite->data->boxSize.y - center.y;
        return CVec_div(CVec_scale(center, sprite->size),
                        sprite->data->boxSize);
}

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
        verts[1].uv = CVec(verts[0].uv.x, verts[0].uv.x + corner_uv.y);
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
                CVec corners;
                unsigned short indices[] = {
                        0, 1, 3, 2,
                        4, 5, 7, 6,
                        11, 12, 13, 10,
                        9, 14, 15, 8,
                };

                /* First render the non-tiling portions */
                RTexture_select(data->texture, smooth || sprite->data->upscale,
                                data->additive);
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
                tVerts[1].uv = CVec(0, 1);
                tVerts[2].uv = CVec(uvSize.x, 0);
                tVerts[3].uv = CVec(uvSize.x, 1);
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
                tVerts[2].uv = CVec(1, uvSize.y);
                tVerts[3].uv = CVec(1, 0);
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
        CVec surfaceSize;
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
                verts[0].uv = CVec_div(sprite->origin, surfaceSize);
                verts[2].uv = CVec_div(CVec_add(sprite->origin, sprite->size),
                                       surfaceSize);
        } else if (data->tile == RST_TILE_PARALLAX) {
                CVec origin;

                origin = CVec_sub(sprite->origin,
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

        if (!sprite || !sprite->data || sprite->z > 0.f ||
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

/******************************************************************************\
 Center a sprite on a box.
\******************************************************************************/
void RSprite_center(RSprite *sprite, CVec origin, CVec size)
{
        if (!sprite || !sprite->data)
                return;
        sprite->origin = CVec_sub(CVec_add(origin, CVec_divf(size, 2)),
                                  RSprite_getCenter(sprite));
}

/******************************************************************************\
 Angle a sprite to look at a point.
\******************************************************************************/
void RSprite_lookAt(RSprite *sprite, CVec origin)
{
        CVec center, diff;

        if (!sprite || !sprite->data)
                return;
        center = CVec_add(sprite->origin, RSprite_getCenter(sprite));
        diff = CVec_sub(origin, center);
        sprite->angle = CVec_angle(diff);
}

