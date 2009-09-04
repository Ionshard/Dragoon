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

/******************************************************************************\
 Initialize an entity class structure.
\******************************************************************************/
void GEntityClass_init(GEntityClass *entity)
{
        entity->z = G_Z_MID;
}

/******************************************************************************\
 Parse a token/value pair from a generic class definition. Returns TRUE if the
 token was valid or FALSE if it is not a generic class token.
\******************************************************************************/
bool GEntityClass_parseToken(GEntityClass *entity, FILE *file,
                             const char *token)
{
        /* The entity display sprite (usually for the editor) */
        if (!strcasecmp(token, "sprite")) {
                R_parseInlineSprite(file, entity->named.name,
                                    entity->spriteName);
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

        /* Entity layer z */
        else if (!strcasecmp(token, "z")) {
                token = C_token(file);
                if (!strcasecmp(token, "fore"))
                        entity->z = G_Z_FORE;
                else if (!strcasecmp(token, "char"))
                        entity->z = G_Z_CHAR;
                else if (!strcasecmp(token, "mid"))
                        entity->z = G_Z_MID;
                else if (!strcasecmp(token, "rear"))
                        entity->z = G_Z_REAR;
                else if (!strcasecmp(token, "back"))
                        entity->z = G_Z_BACK;
                else
                        entity->z = atof(token);
        }

        /* Clone an entity definition */
        else if (!strcasecmp(token, "clone")) {
                GEntityClass *base;

                token = C_token(file);
                if (!(base = CNamed_get(g_classRoot, token)))
                        C_warning("Entity class '%s' is undefined", token);
                else if (base->spawnFunc != entity->spawnFunc ||
                         base->cleanupFunc != entity->cleanupFunc)
                        C_warning("Entity class '%s' of different type", token);
                else if (entity->named.size < base->named.size)
                        C_warning("Entity class '%s' of larger size", token);
                else {
                        CNamed named;

                        named = entity->named;
                        memcpy(entity, base, base->named.size);
                        entity->named = named;
                }
        }

        /* Unrecognized */
        else
                return FALSE;

        return TRUE;
}

/******************************************************************************\
 Load entity definitions.
\******************************************************************************/
void G_parseConfig(const char *filename)
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
                if (!C_openBrace(file)) {

                        /* Include another config */
                        if (!strcasecmp(baseName, "include"))
                                G_parseConfig(className);

                        else
                                C_warning("Class definition '%s' has no body",
                                          className);
                        continue;
                }
                if (!strcasecmp(baseName, "box"))
                        GBox_parseClass(file, className);
                else if (!strcasecmp(baseName, "fountain"))
                        GFountain_parseClass(file, className);
                else if (!strcasecmp(baseName, "group"))
                        GGroup_parseClass(file, className);
                else if (!strcasecmp(baseName, "missile"))
                        GMissile_parseClass(file, className);
                else if (!strcasecmp(baseName, "jumper"))
                        GJumper_parseClass(file, className);
                else if (!strcasecmp(baseName, "sprite"))
                        R_parseSpriteSection(file, className);
                else
                        C_warning("Unknown entity base '%s'", baseName);
                C_closeBrace(file);
        }
        fclose(file);
}

/******************************************************************************\
 Parse impact type.
\******************************************************************************/
PImpactType G_token_impact(FILE *file)
{
        const char *token;

        token = C_token(file);
        if (!strcasecmp(token, "world"))
                return PIT_WORLD;
        else if (!strcasecmp(token, "entity"))
                return PIT_ENTITY;
        else if (strcasecmp(token, "none"))
                C_warning("Unknown impact type '%s'", token);
        return PIT_NONE;
}

/******************************************************************************\
 Sort a newly spawned entity into the proper z order.
\******************************************************************************/
void G_depthSortEntity(PEntity *entity)
{
        GEntityClass *otherClass;
        PEntity *other;
        float z;

        if (!entity || !entity->entityClass)
                return;
        z = ((GEntityClass *)entity->entityClass)->z;
        while (CLink_prev(&entity->linkAll)) {
                other = CLink_get(CLink_prev(&entity->linkAll));
                otherClass = (GEntityClass *)other->entityClass;
                if (otherClass && otherClass->z >= z)
                        break;
                CLink_forward(&entity->linkAll);
        }
}

/******************************************************************************\
 Spawn a named entity.
\******************************************************************************/
PEntity *G_spawn(const char *className)
{
        GEntityClass *entityClass;
        PEntity *entity;

        if (!className || !*className)
                return NULL;
        if (!(entityClass = CNamed_get(g_classRoot, className))) {
                C_warning("Class '%s' not found", className);
                return NULL;
        }
        if ((entity = entityClass->spawnFunc(entityClass))) {
                entity->entityClass = entityClass;
                G_depthSortEntity(entity);
        }
        return entity;
}

PEntity *G_spawn_at(const char *className, CVec origin)
{
        PEntity *entity;

        if ((entity = G_spawn(className)))
                entity->origin = origin;
        return entity;
}

/******************************************************************************\
 Push an entity forward in the linked list. Does not push it behind entities
 z-ordered in behind it.
\******************************************************************************/
void G_pushForwardEntity(PEntity *entity)
{
        GEntityClass *entityClass, *otherClass;
        PEntity *other;

        if (!entity || !CLink_prev(&entity->linkAll))
                return;
        other = CLink_get(CLink_prev(&entity->linkAll));
        otherClass = (GEntityClass *)other->entityClass;
        entityClass = (GEntityClass *)entity->entityClass;
        if (entityClass && otherClass && otherClass->z < entityClass->z)
                return;
        CLink_forward(&entity->linkAll);
}

/******************************************************************************\
 Push an entity back in the linked list. Does not push it in front of
 entities z-ordered in front of it.
\******************************************************************************/
void G_pushBackEntity(PEntity *entity)
{
        GEntityClass *entityClass, *otherClass;
        PEntity *other;

        if (!entity || !CLink_next(&entity->linkAll))
                return;
        other = CLink_get(CLink_next(&entity->linkAll));
        otherClass = (GEntityClass *)other->entityClass;
        entityClass = (GEntityClass *)entity->entityClass;
        if (entityClass && otherClass && otherClass->z > entityClass->z)
                return;
        CLink_back(&entity->linkAll);
}

/******************************************************************************\
 Cleanup game entities.
\******************************************************************************/
void G_cleanupEntities(void)
{
        CNamed *named;
        GEntityClass *gc;

        P_cleanupEntities();
        for (named = g_classRoot; named; named = named->next) {
                gc = (GEntityClass *)named;
                if (!gc->cleanupFunc)
                        continue;
                gc->cleanupFunc(gc);
        }
        CNamed_freeAll(&g_classRoot);
}

