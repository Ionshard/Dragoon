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

static CNamed *dataRoot;

/******************************************************************************\
 Cleanup sprite database.
\******************************************************************************/
void R_cleanupSprites(void)
{
        CNamed_free(&dataRoot, NULL);
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

        for (token = C_token(file); token[0]; token = C_token(file))

                /* Texture file */
                if (!strcasecmp(token, "file"))
                        data->texture = RTexture_load(C_token(file));

                /* Modulation color */
                else if (!strcasecmp(token, "color") && C_openBrace(file)) {
                        data->modulate.r = C_token_float(file);
                        data->modulate.g = C_token_float(file);
                        data->modulate.b = C_token_float(file);
                        data->modulate.a = C_token_float(file);
                        C_closeBrace(file);
                }

                /* Additive blending */
                else if (!strcasecmp(token, "additive"))
                        data->additive = TRUE;

                /* Bounding box */
                else if (!strcasecmp(token, "box") && C_openBrace(file)) {
                        data->boxOrigin.x = C_token_int(file);
                        data->boxOrigin.y = C_token_int(file);
                        data->boxSize.x = C_token_int(file);
                        data->boxSize.y = C_token_int(file);
                        C_closeBrace(file);
                        haveBox = TRUE;
                }

                /* Box center */
                else if (!strcasecmp(token, "center") && C_openBrace(file)) {
                        data->center.x = C_token_int(file);
                        data->center.y = C_token_int(file);
                        C_closeBrace(file);
                        haveCenter = TRUE;
                }

                else
                        C_warning("Unknown sprite param '%s'", token);

        /* Defaults */
        if (!haveBox)
                data->boxSize = RTexture_size(data->texture);
        if (!haveCenter)
                data->center = CVec_divf(data->boxSize, 2.f);
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

                /* Read sprite name */
                token = C_token(file);
                if (!token[0])
                        continue;
                data = CNamed_get(&dataRoot, token, sizeof (RSpriteData));

                /* Parse properties */
                if (!C_openBrace(file))
                        continue;
                parseSpriteSection(file, data);
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

        if (!(data = CNamed_get(&dataRoot, name, 0)))
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
        if (name && name[0] && !(data = CNamed_get(&dataRoot, name, 0))) {
                C_warning("Sprite '%s' not loaded", name);
                return FALSE;
        }
        sprite->data = data;
        sprite->size = data->boxSize;
        sprite->modulate = CColor_white();
        return TRUE;
}

/******************************************************************************\
 Draw a sprite on screen.
\******************************************************************************/
void RSprite_draw(const RSprite *sprite)
{
        const RSpriteData *data;
        RVertex verts[4];
        CColor modulate;
        CVec half, surfaceSize;
        const unsigned short indices[] = { 0, 1, 2, 3, 0 };

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
        glTranslatef(sprite->origin.x + sprite->size.x / 2,
                     sprite->origin.y + sprite->size.y / 2, sprite->z);
        if (sprite->angle) {
                glRotatef(C_radToDeg(sprite->angle), 0.0, 0.0, 1.0);

                /* Make sure that any sprite we rotate has been upscaled */
                if (data->texture && !data->texture->upScale) {
                        data->texture->upScale = TRUE;
                        RTexture_upload(data->texture);
                }
        }

        /* Bind texture */
        RTexture_select(data->texture);

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

        /* Render textured quad */
        half = CVec_divf(sprite->size, 2.f);
        surfaceSize = RTexture_size(data->texture);
        verts[0].co = CVec_xy(-half.x, -half.y);
        verts[0].uv = CVec_div(data->boxOrigin, surfaceSize);
        verts[0].z = 0.f;
        verts[2].co = CVec_xy(half.x, half.y);
        verts[2].uv = CVec_div(CVec_add(data->boxOrigin, data->boxSize),
                               surfaceSize);
        verts[2].z = 0.f;
        verts[1].co = CVec_xy(-half.x, half.y);
        verts[1].uv.x = verts[0].uv.x;
        verts[1].uv.y = verts[2].uv.y;
        verts[1].z = 0.f;
        verts[3].co = CVec_xy(half.x, -half.y);
        verts[3].uv.x = verts[2].uv.x;
        verts[3].uv.y = verts[0].uv.y;
        verts[3].z = 0.f;
        if (CHECKED)
                CCount_add(&r_countFaces, 2);
        glInterleavedArrays(RV_FORMAT, 0, verts);
        glDrawElements(GL_QUADS, 4, GL_UNSIGNED_SHORT, indices);

        /* Cleanup */
        glPopMatrix();
        R_checkErrors();
}

