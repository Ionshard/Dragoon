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
 Find all entities that fall within a box. Returns the number of entries of
 [array] that were used.
\******************************************************************************/
int P_entsInBox(CVec origin, CVec size, PImpactType impactType,
                PEntity **array, int length)
{
        CLink *link;
        int i;

        /* Impact class */
        if (impactType == PIT_NONE || length < 1)
                return 0;
        link = p_linkAll;
        if (impactType == PIT_ENTITY)
                link = p_linkEntity;
        if (impactType == PIT_WORLD)
                link = p_linkWorld;

        /* Iterate over all entities to see if we hit anything */
        for (i = 0; link; link = CLink_next(link)) {
                PEntity *other;

                other = CLink_get(link);
                C_assert(other != NULL);

                /* Skip this entity */
                if (other->dead)
                        continue;

                /* Must intersect bounding box */
                if (!CVec_intersect(origin, size, other->origin, other->size))
                        continue;

                /* Add to array */
                array[i++] = other;
                if (i >= length)
                        break;
        }
        return i;
}

