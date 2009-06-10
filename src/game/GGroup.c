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

/* Group child definition */
typedef struct {
        CVec origin, size;
        char name[C_NAME_MAX];
} GGroupChild;

/* Group class */
typedef struct GGroupClass {
        GEntityClass entity;
        int children_len;
        GGroupChild children[];
} GGroupClass;

/* Group entity */
typedef struct GGroup {
        PEntity entity;
        CVec oldOrigin;
        PEntity *children[];
} GGroup;

/******************************************************************************\
 Group event function.
\******************************************************************************/
int GGroup_eventFunc(GGroup *group, int event, void *args)
{
        GGroupClass *class;
        CVec originDiff;
        int i, dead;

        class = group->entity.entityClass;
        switch (event) {

        /* Propagate kills to children */
        case PE_KILL:
                for (i = 0; i < class->children_len; i++)
                        PEntity_kill(group->children[i]);
                break;

        case PE_UPDATE:

                /* Propagate movement */
                originDiff = CVec_sub(group->entity.origin, group->oldOrigin);
                if (originDiff.x || originDiff.y) {
                        group->oldOrigin = group->entity.origin;
                        for (i = 0; i < class->children_len; i++)
                                group->children[i]->origin =
                                        CVec_add(group->children[i]->origin,
                                                 originDiff);
                }

                /* Check if all children are dead */
                for (dead = i = 0; i < class->children_len; i++)
                        if (!group->children[i])
                                dead++;
                        else if (group->children[i]->dead) {
                                dead++;
                                group->children[i] = NULL;
                        } else
                                group->children[i]->manualUpdate = FALSE;
                if (dead >= class->children_len)
                        PEntity_kill(&group->entity);

                break;
        default:
                break;
        }

        return 0;
}

/******************************************************************************\
 Spawn the children of a group entity.
\******************************************************************************/
PEntity *GGroup_spawn(GGroupClass *class)
{
        GGroup *group;
        int i;

        group = C_calloc(sizeof (*group) + sizeof (*group->children) *
                                           class->children_len);
        group->entity.eventFunc = (PEventFunc)GGroup_eventFunc;
        for (i = 0; i < class->children_len; i++) {
                group->children[i] = G_spawn(class->children[i].name);
                if (!group->children[i])
                        continue;
                group->children[i]->origin = class->children[i].origin;
                group->children[i]->size = class->children[i].size;

                /* We want to make sure the group entity gets at least one
                   frame in before the children are updated */
                group->children[i]->manualUpdate = TRUE;
        }
        PEntity_spawn(&group->entity, "Group");
        return &group->entity;
}

/******************************************************************************\
 Parse a box class definition.
\******************************************************************************/
void GGroup_parseClass(FILE *file, const char *className)
{
        CArray children;
        GGroupClass *groupClass;
        size_t childrenSize;
        const char *token;

        CArray_init(&children, sizeof (GGroupChild), 2);

        /* Parse group class parameters */
        for (token = C_token(file); token[0]; token = C_token(file)) {
                GGroupChild child;

                C_zero(&child);
                C_strncpy_buf(child.name, token);
                if (C_openBrace(file)) {
                        child.origin = C_token_vec(file);
                        child.size = C_token_vec(file);
                        C_closeBrace(file);
                }
                CArray_append(&children, &child);
        }

        /* Initialize class structure */
        childrenSize = children.len * children.size;
        groupClass = CNamed_alloc(&g_classRoot, className, childrenSize +
                                  sizeof (GGroupClass), NULL, CNP_NULL);
        if (!groupClass)
                C_warning("Class '%s' already defined", className);
        else {
                GEntityClass_init(&groupClass->entity);
                memcpy(groupClass->children, children.data, childrenSize);
                groupClass->children_len = children.len;
                groupClass->entity.spawnFunc = (GSpawnFunc)GGroup_spawn;

                /* Push the group entity to the rear so it is always updated
                   before its children are */
                groupClass->entity.z = G_Z_BACK;
        }
        CArray_cleanup(&children);
}

