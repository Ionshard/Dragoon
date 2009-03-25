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
        RTexture_free(data->tiled);
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
static void parseSpriteSection(FILE *file, RSpriteData *data)
{
        const char *token;
        bool haveBox = FALSE, haveCenter = FALSE;

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
                                           atof(token) / 255.f : 1.f;
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
                        data->tile = RST_TILE;
                else if (!strcasecmp(token, "tileGlobal"))
                        data->tile = RST_TILE_GLOBAL;

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
                else if (!strcasecmp(token, "scale") && C_openBrace(file)) {
                        data->scale = C_token_vec(file);
                        C_closeBrace(file);
                }

                else
                        C_warning("Unknown sprite param '%s'", token);

        /* Defaults */
        if (!haveBox)
                data->boxSize = RTexture_size(data->texture);
        if (!haveCenter)
                data->center = CVec_divf(data->boxSize, 2.f);

        /* Create tiled subtexture */
        if (data->tile)
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

        if (!(file = C_fopen_read(filename)))
                return;
        C_debug("Parsing sprite config '%s'", filename);
        while (!feof(file)) {
                RSpriteData *data;
                const char *token;

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
                data = CNamed_alloc(&dataRoot, token, sizeof (RSpriteData),
                                    (CCallback)RSpriteData_cleanup, FALSE);
                if (data)
                        parseSpriteSection(file, data);
                else
                        C_warning("Sprite '%s' already defined", token);
                C_closeBrace(file);
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
                return CVec_zero();
        return data->boxSize;
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
        RSprite_center(sprite, center, CVec_zero());
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
                return CVec_zero();
        center = sprite->data->center;
        if (sprite->mirror)
                center.x = sprite->data->boxSize.x - center.x;
        if (sprite->flip)
                center.x = sprite->data->boxSize.y - center.y;
        return CVec_div(CVec_scale(center, sprite->size),
                        sprite->data->boxSize);
}

/******************************************************************************\
 Draw a sprite on screen.
\******************************************************************************/
void RSprite_draw(RSprite *sprite)
{
        const RSpriteData *data;
        RTexture *texture;
        RVertex verts[4];
        CColor modulate;
        CVec surfaceSize, center;
        const unsigned short indices[] = { 0, 1, 2, 3, 0 };
        bool smooth, flip, mirror;

        if (!sprite || !(data = sprite->data) || sprite->z > 0.f ||
            sprite->modulate.a <= 0.f)
                return;

        /* Check if sprite is on-screen */
        if (!CVec_intersect(r_cameraOn ? r_camera : CVec_zero(),
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

        /* Bind texture */
        texture = data->tile ? data->tiled : data->texture;
        RTexture_select(texture, smooth, data->additive);

        /* Additive blending */
        if (data->additive) {
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                glDisable(GL_ALPHA_TEST);
        }

        /* Alpha blending */
        else {
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glEnable(GL_ALPHA_TEST);
        }

        /* Modulate color */
        modulate = CColor_scale(sprite->modulate, data->modulate);
        glColor4f(modulate.r, modulate.g, modulate.b, modulate.a);

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
        } else if (data->tile) {
                verts[0].uv = CVec_zero();
                verts[2].uv = CVec_div(sprite->size, surfaceSize);
        } else {
                verts[0].uv = CVec_div(data->boxOrigin, surfaceSize);
                verts[2].uv = CVec_div(CVec_add(data->boxOrigin, data->boxSize),
                                       surfaceSize);
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

