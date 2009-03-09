/******************************************************************************\
 Dragoon - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "p_private.h"

/******************************************************************************\
 Push entities away from a point.
\******************************************************************************/
void P_pushRadius(CVec origin, PImpactType impactType,
                  float speed, float dist_max)
{
        PEntity *entity;
        CLink *link;
        CVec dir;
        float dist;

        /* Impact class */
        link = p_linkEntity;
        if (impactType == PIT_WORLD)
                link = p_linkWorld;
        if (impactType == PIT_ALL)
                link = p_linkAll;

        /* Affect every entity in class */
        for (; link; link = CLink_next(link)) {
                entity = CLink_get(link);

                /* Eligible to be pushed */
                if (entity->dead || !entity->mass)
                        continue;

                /* Check the distance */
                dir = CVec_sub(entity->origin, origin);
                dist = CVec_len(dir);
                if (dist >= dist_max || dist <= 0)
                        continue;

                /* Perform the push */
                dir = CVec_scalef(dir, speed * (1 / dist - 1 / dist_max));
                entity->velocity = CVec_add(entity->velocity, dir);
        }
}

