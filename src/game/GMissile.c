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

/* Missile class definition */
typedef struct GMissileClass {
        GEntityClass entity;
        float pushForce, pushRadius, size, velocity, lead;
        char impactEntity[C_NAME_MAX];
} GMissileClass;

/* Missile entity */
typedef struct GMissile {
        PEntity entity, *parent;
        RSprite sprite;
} GMissile;

/******************************************************************************\
 Missile event function.
\******************************************************************************/
static int GMissile_eventFunc(GMissile *mis, int event, void *args)
{
        GMissileClass *class;
        CVec origin;

        class = mis->entity.entityClass;
        C_assert(class);

        /* Ensure parent pointer is valid */
        if (mis->parent && mis->parent->dead)
                mis->parent = NULL;

        switch (event) {
        case PE_IMPACT:
                C_assert(!mis->parent ||
                         ((PImpactEvent *)args)->other != mis->parent);
                PEntity_kill(&mis->entity);
                origin = CVec_add(mis->entity.origin,
                                  CVec_divf(mis->entity.size, 2));
                P_pushRadius(origin, PIT_ENTITY,
                             class->pushForce, class->pushRadius);
                G_spawn_at(class->impactEntity, origin);
                return TRUE;
        case PE_UPDATE:
                RSprite_center(&mis->sprite, mis->entity.origin,
                               mis->entity.size);
                mis->sprite.angle = CVec_angle(mis->entity.velocity) - C_PI / 2;
                RSprite_draw(&mis->sprite);
                break;

        /* Don't impact parent */
        case PE_PHYSICS:
                if (mis->parent)
                        mis->parent->ignore = TRUE;
                break;
        case PE_PHYSICS_DONE:
                if (mis->parent)
                        mis->parent->ignore = FALSE;
                break;
        }
        return 0;
}

/******************************************************************************\
 Spawn a missile instance.
\******************************************************************************/
GMissile *GMissile_spawn(GMissileClass *misClass)
{
        GMissile *mis;

        C_new(&mis);
        RSprite_init(&mis->sprite, misClass->entity.spriteName);
        mis->sprite.z = G_Z_MID;
        mis->entity.eventFunc = (PEventFunc)GMissile_eventFunc;
        mis->entity.size = CVec(misClass->size, misClass->size);
        mis->entity.mass = 1;
        mis->entity.fly = TRUE;
        mis->entity.impactOther = PIT_ENTITY;
        PEntity_spawn(&mis->entity, "Missile");
        return mis;
}

/******************************************************************************\
 Parse a missile class definition.
\******************************************************************************/
void GMissile_parseClass(FILE *file, const char *className)
{
        GMissileClass *misClass;
        const char *token;

        if (!(misClass = CNamed_alloc(&g_classRoot, className,
                                      sizeof (GMissileClass),
                                      NULL, CNP_NULL))) {
                C_warning("Class '%s' already defined", className);
                return;
        }
        GEntityClass_init(&misClass->entity);
        misClass->entity.spawnFunc = (GSpawnFunc)GMissile_spawn;

        /* Sprite defaults to class name */
        C_strncpy_buf(misClass->entity.spriteName, className);
        misClass->entity.size = R_spriteSize(className);

        /* Parse missile class parameters */
        for (token = C_token(file); token[0]; token = C_token(file))
                if (GEntityClass_parseToken(&misClass->entity, file, token));

                /* Impact push params */
                else if (!strcasecmp(token, "impactPush") &&
                         C_openBrace(file)) {
                        misClass->pushForce = C_token_float(file);
                        misClass->pushRadius = C_token_float(file);
                        C_closeBrace(file);
                }

                /* Impact spawn entity */
                else if (!strcasecmp(token, "impactEntity"))
                        C_strncpy_buf(misClass->impactEntity, C_token(file));

                /* Size */
                else if (!strcasecmp(token, "size"))
                        misClass->size = C_token_float(file);

                /* Launch velocity */
                else if (!strcasecmp(token, "velocity"))
                        misClass->velocity = C_token_float(file);

                /* Launch lead distance */
                else if (!strcasecmp(token, "lead"))
                        misClass->lead = C_token_float(file);

                else
                        C_warning("Unknown missile param '%s'", token);
}

/******************************************************************************\
 Spawn a missile entity.
\******************************************************************************/
GMissile *G_fireMissile(PEntity *parent, const char *className,
                        CVec from, CVec at)
{
        GMissile *mis;
        GMissileClass *misClass;
        CVec dir;

        if (!(mis = (GMissile *)G_spawn(className)))
                return NULL;
        misClass = mis->entity.entityClass;
        if ((void *)misClass->entity.spawnFunc != (void *)GMissile_spawn) {
                C_warning("Class '%s' is not a missile", className);
                return NULL;
        }
        from = CVec_sub(from, CVec_divf(mis->entity.size, 2));
        dir = CVec_norm(CVec_sub(at, from));
        mis->entity.origin = from;
        mis->entity.velocity = CVec_scalef(dir, misClass->velocity);
        mis->parent = parent;

        /* Trace lead distance */
        if (misClass->lead) {
                PTrace trace;

                if (parent)
                        parent->ignore = TRUE;
                at = CVec_add(from, CVec_scalef(dir, misClass->lead));
                trace = PEntity_trace(&mis->entity, at);
                mis->entity.origin = trace.end;
                if (parent)
                        parent->ignore = FALSE;
        }

        return mis;
}

