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

                /* Tile with respect to a global origin */
                if (!strcasecmp(token, "global")) {
                        data->tile = RST_TILE_GLOBAL;
                        if (C_openBrace(file)) {
                                data->tileOrigin = C_token_vec(file);
                                C_closeBrace(file);
                        }
                }

                /* Parallax motion */
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

                /* Flicker alpha */
                else if (!strcasecmp(token, "flicker"))
                        data->flicker = C_token_float(file);

                else
                        C_warning("Unknown sprite param '%s'", token);

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
                return CVec(0, 0);
        return CVec_scale(data->boxSize, data->scale);
}

/******************************************************************************\
 Initialize an RSprite to display a named sprite. Returns FALSE if the named
 sprite has not been loaded. Resets sprite size and modulation.
\******************************************************************************/
bool RSprite_init_time(RSprite *sprite, const char *name, int *timeMsec)
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
        sprite->initMsec = *(sprite->timeMsec = timeMsec);
        C_strncpy_buf(sprite->name, name);
        return TRUE;
}

/******************************************************************************\
 Pump the current animation frame.
\******************************************************************************/
void RSprite_animate(RSprite *sprite)
{
        RSpriteData *data;
        CVec center;
        int next;

        if (!sprite || !sprite->data || !sprite->data->nextNames_len ||
            sprite->initMsec + sprite->data->nextMsec > *sprite->timeMsec)
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
        sprite->initMsec = *sprite->timeMsec;
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
        return RSprite_init_time(sprite, name, sprite->timeMsec);
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

