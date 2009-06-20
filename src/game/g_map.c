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

/* Saved entity parameters */
typedef struct {
        CVec origin, size;
} SpawnParams;

/* Player spawn location */
CVec g_playerSpawn;

/******************************************************************************\
 Load a map file into the current world.
\******************************************************************************/
void G_loadMap(const char *filename, CVec offset)
{
        SpawnParams params;
        PEntity *entity;
        FILE *file;
        const char *className;
        int entities;
        char header;

        if (!filename || !filename[0] || !(file = C_fopen_read(filename)))
                return;

        /* Read header */
        C_fread(file, &header, 1);
        if (header != sizeof (params)) {
                C_warning("Map '%s' is incompatible", filename);
                return;
        }

        /* Read in and spawn each entity */
        for (entities = 0; !feof(file); entities++) {
                if (!(className = C_readString(file, NULL))[0])
                        break;
                C_fread(file, &params, sizeof (params));
                if (!(entity = G_spawn(className)))
                        continue;
                entity->origin = CVec_add(params.origin, offset);
                entity->size = params.size;
                PEntity_event(entity, GE_INITED, NULL);
        }
        fclose(file);
        C_debug("Loaded map '%s', %d entities", filename, entities);
}

/******************************************************************************\
 Save a map after editing.
\******************************************************************************/
void G_saveMap(const char *filename)
{
        SpawnParams params;
        GEntityClass *entityClass;
        PEntity *entity;
        CLink *link;
        FILE *file;
        int entities;
        char header;

        if (!filename || !filename[0] || !(file = C_fopen_write(filename)))
                return;

        /* Don't save maps with no entities */
        if (!p_linkAll) {
                C_warning("No entities, refusing to save map");
                return;
        }

        /* Write the size of params struct as header */
        header = sizeof (params);
        C_fwrite(file, &header, 1);

        /* Save each entity currently spawned */
        entities = 0;
        for (link = p_linkAll; link; link = CLink_next(link)) {
                entity = CLink_get(link);
                C_assert(entity);
                if (!(entityClass = (GEntityClass *)entity->entityClass))
                        continue;
                params.origin = entity->origin;
                params.size = entity->size;
                C_fwrite(file, entityClass->named.name,
                         strlen(entityClass->named.name) + 1);
                C_fwrite(file, &params, sizeof (params));
                entities++;
        }
        fclose(file);

        C_debug("Saved map '%s', %d entities", filename, entities);
}

