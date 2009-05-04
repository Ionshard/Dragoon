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
 Trace a box through the world. Returns the proportion of the path
 that is traceable. Any of the output argument pointers may be NULL.
 FIXME: May have a bug with corners.
\******************************************************************************/
PTrace PTrace_box(CVec from, CVec to, CVec size, PImpactType impactType)
{
        PTrace trace;
        CLink *link;
        CVec boxOrigin, boxSize;
        float slope, iA, iB, iC;
        bool diagonal, diagonalNE;

        C_zero(&trace);

        /* Can't impact anything or didn't go anywhere */
        if (impactType == PIT_NONE || CVec_eq(from, to)) {
                trace.end = to;
                trace.prop = 1.f;
                return trace;
        }

        /* Impact class */
        link = p_linkEntity;
        if (impactType == PIT_WORLD)
                link = p_linkWorld;
        if (impactType == PIT_ALL)
                link = p_linkAll;

        /* Calculate trace slope and intercepts for diagonal paths */
        if ((diagonal = to.x != from.x && to.y != from.y)) {
                slope = CVec_slope(CVec_sub(to, from));
                if ((diagonalNE = (to.x >= from.x) != (to.y >= from.y))) {
                        iA = from.y - from.x * slope;
                        iB = from.y + size.y - (from.x + size.x) * slope;
                        if (to.x > from.x)
                                iC = from.y - (from.x + size.x) * slope;
                        else
                                iC = from.y + size.y - from.x * slope;
                } else {
                        iA = from.y - (from.x + size.x) * slope;
                        iB = from.y + size.y - from.x * slope;
                        if (to.x > from.x)
                                iC = from.y + size.y -
                                     (from.x + size.x) * slope;
                        else
                                iC = from.y - from.x * slope;
                }
        }

        /* Collision area bounding box */
        boxOrigin = CVec_min(from, to);
        boxSize = CVec_add(size, CVec_abs(CVec_sub(to, from)));

        /* Iterate over all entities to see if we hit anything */
        for (trace.prop = 1.f; link; link = CLink_next(link)) {
                PEntity *other;
                float dist;
                int side;

                other = CLink_get(link);
                C_assert(other != NULL);

                /* Skip this entity */
                if (other->ignore || other->dead)
                        continue;

                /* Must intersect bounding box */
                if (!CVec_intersect(boxOrigin, boxSize,
                                    other->origin, other->size))
                        continue;

                /* Started within this entity */
                if (CVec_intersect(from, size, other->origin, other->size)) {
                        trace.end = from;
                        trace.other = other;
                        trace.startSolid = TRUE;
                        trace.prop = 0.f;
                        trace.dir = CVec(0.f, 0.f);
                        return trace;
                }

                /* Must intersect diagonal path */
                if (diagonal) {
                        CVec a, b;

                        if (diagonalNE) {
                                a = CVec_add(other->origin, other->size);
                                b = other->origin;
                        } else {
                                a.x = other->origin.x;
                                a.y = other->origin.y + other->size.y;
                                b.x = other->origin.x + other->size.x;
                                b.y = other->origin.y;
                        }
                        if (a.x * slope + iA > a.y || b.x * slope + iB <= b.y)
                                continue;
                }

                /* Non-diagonal case */
                if (!diagonal)
                        side = to.y == from.y ? to.x < from.x :
                                                2 + (to.y > from.y);

                /* Diagonal case, find out which side of the box we hit */
                else {
                        CVec c;
                        bool fcx;

                        /* Corner of other entity closest to [from] */
                        c = other->origin;
                        if (to.x < from.x)
                                c.x += other->size.x;
                        if (to.y < from.y)
                                c.y += other->size.y;

                        /*         |        fcx        |       !fcx
                                   | ty < fy | ty > fy | ty < fy | ty > fy
                           --------+---------+---------+---------+--------
                           tx < fx |    2    |    1    |    1    |    3
                           tx > fx |    2    |    0    |    0    |    3    */
                        fcx = slope * c.x + iC > c.y;
                        side = (to.y < from.y) == fcx ? 3 - fcx : to.x < from.x;
                }

                /* Calculate how far we got */
                switch (side) {
                default:
                        C_assert(0);

                case 0: /* Numerical error sanity check */
                        if (other->origin.x < from.x + size.x)
                                continue;

                        /* Left */
                        dist = (other->origin.x - size.x - from.x) /
                               (to.x - from.x);
                        trace.dir = CVec(1.f, 0.f);
                        break;

                case 1: /* Numerical error sanity check */
                        if (other->origin.x + other->size.x > from.x)
                                continue;

                        /* Right */
                        dist = (from.x - other->origin.x - other->size.x) /
                               (from.x - to.x);
                        trace.dir = CVec(-1.f, 0.f);
                        break;

                case 2: /* Numerical error sanity check */
                        if (other->origin.y + other->size.y > from.y)
                                continue;

                        /* Bottom */
                        dist = (from.y - other->origin.y - other->size.y) /
                               (from.y - to.y);
                        trace.dir = CVec(0.f, -1.f);
                        break;

                case 3: /* Numerical error sanity check */
                        if (other->origin.y < from.y + size.y)
                                continue;

                        /* Top */
                        dist = (other->origin.y - size.y - from.y) /
                               (to.y - from.y);
                        trace.dir = CVec(0.f, 1.f);
                        break;
                }

                /* Numerical precision error */
                if (dist > 1.f)
                        dist = 1.f;
                if (dist < 0.f)
                        dist = 0.f;

                trace.prop *= dist;
                to = CVec_lerp(from, dist, to);

                /* We have a hit */
                trace.other = other;

                /* Done tracing */
                if (trace.prop <= 0)
                        break;
        }

        /* Return values */
        trace.end = to;
        return trace;
}

