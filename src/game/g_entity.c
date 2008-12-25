/******************************************************************************\
 Dragoon - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "g_private.h"

/* Named linked list of class definitions */
CNamed *g_classRoot;

/* Box class structure */
typedef struct {
        GEntityClass entity;
} GBoxClass;

/* Box entity structure */
typedef struct {
        PEntity entity;
        RSprite sprite;
} GBox;

/* Game camera offset */
CVec g_camera;

/******************************************************************************\
 Parse a token/value pair from a generic class definition. Returns TRUE if the
 token was valid or FALSE if it is not a generic class token.
\******************************************************************************/
bool GEntityClass_parseToken(GEntityClass *entity, FILE *file,
                             const char *token)
{
        /* The entity display sprite (usually for the editor) */
        if (!strcasecmp(token, "sprite")) {
                C_strncpy_buf(entity->spriteName, C_token(file));
                if (entity->size.x <= 0 && entity->size.y <= 0)
                        entity->size = R_spriteSize(entity->spriteName);
        }

        /* Entity size */
        else if (!strcasecmp(token, "size") && C_openBrace(file)) {
                entity->size.x = C_token_float(file);
                entity->size.y = C_token_float(file);
                C_closeBrace(file);
        }

        /* Key the entity is bound to in the editor */
        else if (!strcasecmp(token, "editor"))
                entity->editorKey = C_token(file)[0];

        /* Unrecognized */
        else
                return FALSE;

        return TRUE;
}

/******************************************************************************\
 Test box event function.
\******************************************************************************/
static int GBox_eventFunc(GBox *box, int event, void *args)
{
        if (event == PE_UPDATE) {
                box->sprite.origin = box->entity.origin;
                box->sprite.size = box->entity.size;
                RSprite_draw(&box->sprite);
        }
        return 0;
}

/******************************************************************************\
 Spawn a fixture box.
\******************************************************************************/
GBox *GBox_spawn(GBoxClass *boxClass, GSpawnParams *params)
{
        GBox *box;

        C_new(&box);
        RSprite_init(&box->sprite, boxClass->entity.spriteName);
        box->sprite.z = -1;
        box->entity.eventFunc = (PEventFunc)GBox_eventFunc;
        box->entity.origin = params->origin;
        box->entity.size = params->size;
        PEntity_spawn(&box->entity, "Box");
        return box;
}

/******************************************************************************\
 Parse a test box class definition.
\******************************************************************************/
static void GBox_parseClass(FILE *file, const char *className)
{
        GBoxClass *boxClass;
        const char *token;

        boxClass = CNamed_get(&g_classRoot, className, sizeof (GBoxClass));
        boxClass->entity.spawnFunc = (GSpawnFunc)GBox_spawn;
        for (token = C_token(file); token[0]; token = C_token(file))
                if (GEntityClass_parseToken(&boxClass->entity, file, token));
                else
                        C_warning("Unknown box param '%s'", token);
}

/******************************************************************************\
 Load entity definitions.
\******************************************************************************/
void G_parseEntityCfg(const char *filename)
{
        FILE *file;

        if (!(file = C_fopen_read(filename)))
                return;
        C_debug("Parsing entity config '%s'", filename);
        while (!feof(file)) {
                const char *token;
                char baseName[64], className[64];

                /* Read entity base and class name */
                if (!(token = C_token(file))[0])
                        continue;
                C_strncpy_buf(baseName, token);
                if (!(token = C_token(file))[0])
                        continue;
                C_strncpy_buf(className, token);

                /* Parse class properties */
                if (!C_openBrace(file))
                        continue;
                if (!strcasecmp(baseName, "box"))
                        GBox_parseClass(file, className);
                else
                        C_warning("Unknown entity base '%s'", baseName);
                C_closeBrace(file);
        }
        fclose(file);
}

/******************************************************************************\
 Spawn a named entity.
\******************************************************************************/
PEntity *G_spawn(const char *className, const GSpawnParams *params)
{
        GEntityClass *entityClass;
        PEntity *entity;

        if (!(entityClass = CNamed_get(&g_classRoot, className, 0))) {
                C_warning("Class '%s' not found", className);
                return NULL;
        }
        if ((entity = entityClass->spawnFunc(entityClass, params)))
                entity->entityClass = entityClass;
        return entity;
}

/******************************************************************************\
 Cleanup game entities.
\******************************************************************************/
void G_cleanupEntities(void)
{
        P_cleanupEntities();
        CNamed_free(&g_classRoot, NULL);
}

