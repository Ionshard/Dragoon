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

/* Missile entity */
typedef struct {
        PEntity entity, *parent;
        RSprite sprite;
} GMissile;

/******************************************************************************\
 Missile event function.
\******************************************************************************/
static int GMissile_eventFunc(GMissile *mis, int event, void *args)
{
        CVec origin;

        /* Ensure parent pointer is valid */
        if (mis->parent && mis->parent->dead)
                mis->parent = NULL;

        switch (event) {
        case PE_IMPACT:
                C_assert(((PEventImpact *)args)->other != mis->parent);
                PEntity_kill(&mis->entity);
                origin = CVec_add(mis->entity.origin,
                                  CVec_divf(mis->entity.size, 2));
                P_pushRadius(origin, PIT_ENTITY, 200, 200);
                G_spawn_at("repelImpact", origin);
                G_spawn_at("repelFlash", origin);
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
 Spawn a missile entity.
\******************************************************************************/
void G_fireMissile(PEntity *parent, CVec from, CVec to, int size)
{
        GMissile *mis;
        CVec dir;

        dir = CVec_norm(CVec_sub(to, from));
        C_new(&mis);
        RSprite_init(&mis->sprite, "missile");
        mis->parent = parent;
        mis->sprite.z = G_Z_MID;
        mis->entity.eventFunc = (PEventFunc)GMissile_eventFunc;
        mis->entity.size = CVec(size, size);
        mis->entity.origin = CVec_add(CVec_sub(from,
                                               CVec_divf(mis->entity.size, 2)),
                                      CVec_scalef(dir, 4));
        mis->entity.mass = 1;
        mis->entity.fly = TRUE;
        mis->entity.impactOther = PIT_ENTITY;
        mis->entity.velocity = CVec_scalef(dir, 400);
        PEntity_spawn(&mis->entity, "Missile");
}

