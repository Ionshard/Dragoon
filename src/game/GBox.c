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

/* Impact offset effect parameters */
#define OFFSET_MAX 2
#define OFFSET_VEL 5000
#define OFFSET_DRAG 5

/* Box class */
typedef struct GBoxClass {
        GEntityClass entity;
        PImpactType impact, impactOther;
        float mass, elasticity, friction, impactDrag, gibForce, gibDensity;
        char impactEntity[C_NAME_MAX], gibEntity[C_NAME_MAX];
        bool invisible, playerSpawn;
} GBoxClass;

/* Box entity */
typedef struct GBox {
        PEntity entity;
        RSprite sprite;
        CVec offset, offsetVel;
} GBox;

/******************************************************************************\
 Gib a box entity.
\******************************************************************************/
void GBox_gib(GBox *box)
{
        GBoxClass *boxClass = box->entity.entityClass;
        CVec origin;
        int i, gibs;

        /* Single centered gib */
        gibs = box->entity.size.x * box->entity.size.y * boxClass->gibDensity;
        if (gibs < 2)
                G_spawn_at(boxClass->gibEntity, PEntity_center(&box->entity));

        /* Spread out lots of gibs */
        else
                for (i = 0; i < gibs; i++) {
                        origin = CVec_add(box->entity.origin,
                                          CVec_scale(box->entity.size,
                                                     CVec(C_rand(), C_rand())));
                        G_spawn_at(boxClass->gibEntity, origin);
                }

        PEntity_kill(&box->entity);
}

/******************************************************************************\
 Box event function.
\******************************************************************************/
int GBox_eventFunc(GBox *box, int event, void *args)
{
        GBoxClass *boxClass = box->entity.entityClass;
        PImpactEvent *impactEvent;

        switch (event) {

        /* Update player spawn location */
        case GE_INITED:
                if (boxClass->playerSpawn)
                        g_playerSpawn = box->entity.origin;
                break;

        case PE_UPDATE:

                /* Don't render invisible boxes */
                if (boxClass->invisible && !g_edit[0])
                        break;
                box->sprite.origin = box->entity.origin;
                box->sprite.size = box->entity.size;

                /* Impact reeling effect */
                if (box->offset.x || box->offset.y) {
                        R_updateShake(&box->offset, &box->offsetVel,
                                      OFFSET_VEL, OFFSET_DRAG, p_frameSec,
                                      OFFSET_MAX);
                        box->sprite.origin = CVec_add(box->sprite.origin,
                                                      box->offset);
                }

                RSprite_draw(&box->sprite);
                break;

        case PE_IMPACT:

                /* Only bother impacting with entities */
                impactEvent = args;
                if (!impactEvent->other->linkEntity.root &&
                    impactEvent->other->impactOther != PIT_ENTITY)
                        break;

                /* Did this impact kill the box? */
                if (boxClass->gibForce > 0) {
                        CVec offset;

                        if (impactEvent->impulse >= boxClass->gibForce) {
                                CVec vel;
                                float prop;

                                vel = impactEvent->other->velocity;
                                prop = 1 - boxClass->gibForce /
                                           impactEvent->impulse;
                                vel = CVec_scalef(vel, prop);
                                impactEvent->other->velocity = vel;
                                GBox_gib(box);
                                return TRUE;
                        }

                        /* Not dead -- reel from the impact */
                        offset = CVec_scalef(impactEvent->dir, -OFFSET_MAX *
                                             impactEvent->impulse /
                                             boxClass->gibForce);
                        box->offset = CVec_add(box->offset, offset);
                }

                /* Impact effect entity */
                G_spawn_at(boxClass->impactEntity, impactEvent->origin);

                /* Drag effect for glue boxes */
                if (boxClass->impactDrag > 0)
                        impactEvent->other->velocity =
                                CVec_scalef(impactEvent->other->velocity,
                                            1 - boxClass->impactDrag);

                break;

        /* Can't jump away from glue boxes */
        case GE_JUMPED_AWAY:
                return boxClass->impactDrag > 0;

        default:
                break;
        }
        return 0;
}

/******************************************************************************\
 Spawn a fixture box.
\******************************************************************************/
GBox *GBox_spawn(GBoxClass *boxClass)
{
        GBox *box;

        C_new(&box);
        RSprite_init_time(&box->sprite, boxClass->entity.spriteName,
                          &p_timeMsec);
        box->entity.eventFunc = (PEventFunc)GBox_eventFunc;
        box->sprite.z = boxClass->entity.z;
        box->entity.mass = boxClass->mass;
        box->entity.elasticity = boxClass->elasticity;
        box->entity.friction = boxClass->friction;
        box->entity.impactOther = boxClass->impactOther;
        PEntity_spawn(&box->entity, "Box");
        PEntity_impactType(&box->entity, boxClass->impact);
        return box;
}

/******************************************************************************\
 Parse a box class definition.
\******************************************************************************/
void GBox_parseClass(FILE *file, const char *className)
{
        GBoxClass *boxClass;
        const char *token;

        if (!(boxClass = CNamed_alloc(&g_classRoot, className,
                                      sizeof (GBoxClass), NULL, CNP_NULL))) {
                C_warning("Class '%s' already defined", className);
                return;
        }
        GEntityClass_init(&boxClass->entity);
        boxClass->entity.spawnFunc = (GSpawnFunc)GBox_spawn;

        /* Sprite defaults to class name */
        C_strncpy_buf(boxClass->entity.spriteName, className);
        boxClass->entity.size = R_spriteSize(className);

        /* Initial physics params */
        boxClass->friction = 1;

        /* Parse box class parameters */
        for (token = C_token(file); token[0]; token = C_token(file))
                if (GEntityClass_parseToken(&boxClass->entity, file, token));

                /* Impact class */
                else if (!strcasecmp(token, "impact"))
                        boxClass->impact = G_token_impact(file);

                /* Impact-other class */
                else if (!strcasecmp(token, "impactOther"))
                        boxClass->impactOther = G_token_impact(file);

                /* Entity to spawn on impact */
                else if (!strcasecmp(token, "impactEntity"))
                        C_strncpy_buf(boxClass->impactEntity, C_token(file));

                /* Impact drag */
                else if (!strcasecmp(token, "impactDrag")) {
                        boxClass->impactDrag = C_token_float(file);
                        C_limit_float(&boxClass->impactDrag, 0, 1);
                }

                /* Mass */
                else if (!strcasecmp(token, "mass"))
                        boxClass->mass = C_token_float(file);

                /* Elasticity */
                else if (!strcasecmp(token, "elasticity"))
                        boxClass->elasticity = C_token_float(file);

                /* Friction */
                else if (!strcasecmp(token, "friction"))
                        boxClass->friction = C_token_float(file);

                /* Maximum impact force before dying */
                else if (!strcasecmp(token, "gibForce"))
                        boxClass->gibForce = C_token_float(file);

                /* Entity to spawn when killed */
                else if (!strcasecmp(token, "gibEntity"))
                        C_strncpy_buf(boxClass->gibEntity, C_token(file));

                /* How densely to spread out gibs */
                else if (!strcasecmp(token, "gibDensity"))
                        boxClass->gibDensity = C_token_float(file);

                /* This is a player spawn entity */
                else if (!strcasecmp(token, "playerSpawn"))
                        boxClass->playerSpawn = TRUE;

                /* Only visible in the editor */
                else if (!strcasecmp(token, "invisible"))
                        boxClass->invisible = TRUE;

                else
                        C_warning("Unknown box param '%s'", token);
}

