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

/* A tiny distance used to guard against floating point errors */
#define EPSILON_DIST 0.0001f

/* Velocity at which an entity is judged to be broken */
#define V_LIMIT 10000

/* Positions outside of this limit are assumed to be broken entities */
#define CO_LIMIT 100000

/******************************************************************************\
 Entity elastic collision with a mobile entity.
\******************************************************************************/
static void PEntity_bounceMobile(PEntity *entity, PEntity *other, CVec dir)
{
        PEventImpact impactEvent;
        float velA, velB, velANew, velBNew;

        C_assert(entity->mass > 0.f);
        C_assert(other->mass > 0.f);

        /* Velocity component in the impact direction */
        velA = CVec_dot(entity->velocity, dir);
        velB = CVec_dot(other->velocity, dir);

        /* Check if either entity does not want the impact */
        impactEvent.other = other;
        impactEvent.dir = dir;
        impactEvent.impulse = velA * entity->mass + velB * other->mass;
        if (PEntity_event(entity, PE_IMPACT, &impactEvent))
                return;
        impactEvent.other = entity;
        impactEvent.dir = CVec_scalef(dir, -1.f);
        if (PEntity_event(other, PE_IMPACT, &impactEvent))
                return;

        /* Compute new velocities */
        velANew = (velA * (entity->mass - other->mass) +
                   2 * other->mass * velB) / (entity->mass + other->mass);
        velBNew = velANew + velA - velB;

        /* Apply elasticity */
        velANew *= entity->elasticity;
        velBNew *= other->elasticity;

        /* Apply bounce impulses */
        entity->velocity = CVec_add(entity->velocity,
                                    CVec_scalef(dir, velANew - velA));
        other->velocity = CVec_add(other->velocity,
                                   CVec_scalef(dir, velBNew - velB));
}

/******************************************************************************\
 Entity elastic collision with a fixture entity.
\******************************************************************************/
static void PEntity_bounceFixture(PEntity *entity, PEntity *other, CVec dir)
{
        PEventImpact impactEvent;
        float vel, velNew;

        /* Check if either entity does not want the impact */
        vel = CVec_dot(entity->velocity, dir);
        impactEvent.other = other;
        impactEvent.dir = dir;
        impactEvent.impulse = entity->mass * vel;
        if (PEntity_event(entity, PE_IMPACT, &impactEvent))
                return;
        impactEvent.other = entity;
        impactEvent.dir = CVec_scalef(dir, -1.f);
        if (PEntity_event(other, PE_IMPACT, &impactEvent))
                return;

        /* New velocity */
        velNew = -vel * entity->elasticity;

        /* Apply bounce impulse */
        entity->velocity = CVec_add(entity->velocity,
                                    CVec_scalef(dir, velNew - vel));
}

/******************************************************************************\
 Unstick an entity from inside of another.
\******************************************************************************/
static void PEntity_unstick(PEntity *entity, const PEntity *other)
{
        CVec entityCenter, otherCenter, diff;

        entityCenter = CVec_add(entity->origin, CVec_divf(entity->size, 2));
        otherCenter = CVec_add(other->origin, CVec_divf(other->size, 2));
        diff = CVec_sub(otherCenter, entityCenter);
        if (fabsf(diff.x) < fabsf(diff.y))
                entity->origin.x = entityCenter.x > otherCenter.x ?
                                   other->origin.x + other->size.x :
                                   other->origin.x - entity->size.x;
        else
                entity->origin.y = entityCenter.y > otherCenter.y ?
                                   other->origin.y + other->size.y :
                                   other->origin.y - entity->size.y;
}

/******************************************************************************\
 Try to step over an entity, returns TRUE if succeeded.
\******************************************************************************/
static bool PEntity_stepOver(PEntity *entity, PEntity *other)
{
        PTrace trace;
        CVec to, oldOrigin;
        float stepHeight;

        if (!entity->velocity.x)
                return FALSE;

        /* Check the step height */
        stepHeight = entity->origin.y + entity->size.y - other->origin.y;
        if (stepHeight <= 0 || stepHeight > entity->stepSize)
                return FALSE;

        /* Trace to the new position */
        to = CVec(entity->origin.x, entity->origin.y - stepHeight);
        trace = PEntity_trace(entity, to);
        if (trace.prop < 1.f)
                return FALSE;
        oldOrigin = entity->origin;
        entity->origin = to;

        /* Trace forward a little bit */
        to.x += entity->velocity.x > 0 ? EPSILON_DIST : -EPSILON_DIST;
        trace = PEntity_trace(entity, to);
        if (trace.prop < 1.f) {
                entity->origin = oldOrigin;
                return FALSE;
        }

        /* Cut vertical velocity */
        entity->velocity.y = 0;

        entity->origin = to;
        return TRUE;
}

/******************************************************************************\
 Clip acceleration vector so that the entity does not accelerate against
 ground and walls. Will also set ground entity properties.
\******************************************************************************/
static void PEntity_clipAccel(PEntity *entity, float delT)
{
        CLink *link;
        CVec newV;

        /* Impact class */
        link = p_linkEntity;
        if (entity->impactOther == PIT_WORLD)
                link = p_linkWorld;
        if (entity->impactOther == PIT_ALL)
                link = p_linkAll;

        /* Iterate over collision class linked list */
        entity->ceiling = NULL;
        entity->ground = NULL;
        entity->leftWall = NULL;
        entity->rightWall = NULL;
        for (; link; link = CLink_next(link)) {
                PEntity *other;

                if ((other = CLink_get(link)) == entity)
                        continue;
                C_assert(other);

                /* Horizontal */
                if (other->origin.y < entity->origin.y + entity->size.y &&
                    other->origin.y + other->size.y > entity->origin.y) {

                        /* Clip left */
                        if (other->origin.x + other->size.x == entity->origin.x)
                                entity->leftWall = other;

                        /* Clip right */
                        else if (entity->origin.x + entity->size.x ==
                                 other->origin.x)
                                entity->rightWall = other;
                }

                /* Vertical */
                if (other->origin.x < entity->origin.x + entity->size.x &&
                    other->origin.x + other->size.x > entity->origin.x) {

                        /* Clip up */
                        if (other->origin.y + other->size.y == entity->origin.y)
                                entity->ceiling = other;

                        /* Clip down */
                        else if (entity->origin.y + entity->size.y ==
                                 other->origin.y)
                                entity->ground = other;
                }
        }

        /* Clip acceleration and velocity */
        newV = CVec_add(entity->velocity, CVec_scalef(entity->accel, delT));
        if ((entity->leftWall && newV.x < 0) ||
            (entity->rightWall && newV.x > 0))
                entity->accel.x = 0;
        if ((entity->ceiling && newV.y < 0) ||
            (entity->ground && newV.y > 0))
                entity->accel.y = 0;
}

/******************************************************************************\
 Update physics for an entity.
\******************************************************************************/
void PEntity_physics(PEntity *entity, float delT)
{
        PTrace trace;
        CVec from, to, lastV, delS, avgV;
        float prop;

        /* Fixed entity, doesn't move */
        if (!entity->mass || entity->dead)
                return;

        /* Time traveled since last frame */
        if ((delT += entity->lagSec) <= 0)
                return;

        /* Low priority entities must rack up a large deficit before moving */
        if (entity->frameSkip * P_FRAME_MSEC_MAX * 0.001f > delT) {
                entity->lagSec = delT;
                return;
        }
        entity->lagSec = 0.f;

        /* Add gravity */
        if (!entity->fly)
                entity->accel.y += p_gravity;

        /* Clip acceleration to make sure we move this frame */
        PEntity_clipAccel(entity, delT);

        /* Add acceleration for the frame */
        lastV = entity->velocity;
        entity->velocity = CVec_add(entity->velocity,
                                    CVec_scalef(entity->accel, delT));

        /* On ground, apply friction */
        if (entity->ground) {
                if ((prop = 1 - entity->friction * delT) < 0)
                        prop = 0;
                entity->velocity.x *= prop;
        }

        /* Apply air drag */
        if ((prop = 1 - entity->drag * delT) < 0)
                prop = 0;
        entity->velocity = CVec_scalef(entity->velocity, prop);

        /* Bad entity broke speed limit */
        if (entity->velocity.x < -V_LIMIT || entity->velocity.x > V_LIMIT ||
            entity->velocity.y < -V_LIMIT || entity->velocity.y > V_LIMIT) {
                C_warning("Entity %s broke speed limit", entity->name);
                PEntity_kill(entity);
                return;
        }

        /* Not moving */
        avgV = CVec_avg(entity->velocity, lastV);
        if (!avgV.x && !avgV.y)
                return;

        /* Trace to our new position */
        delS = CVec_scalef(avgV, delT);
        from = entity->origin;
        to = CVec_add(from, delS);
        trace = PEntity_trace(entity, to);
        entity->origin = trace.position;

        /* Entity started out stuck in something */
        if (trace.startSolid) {
                PEventImpact impactEvent;

                /* Generate an impact event */
                impactEvent.other = trace.impactEntity;
                impactEvent.dir = CVec_sub(entity->origin,
                                           trace.impactEntity->origin);
                impactEvent.dir = CVec_norm(impactEvent.dir);
                impactEvent.impulse = 0;
                PEntity_event(entity, PE_IMPACT, &impactEvent);
                impactEvent.other = entity;
                impactEvent.dir = CVec_scalef(impactEvent.dir, -1.f);
                PEntity_event(trace.impactEntity, PE_IMPACT, &impactEvent);

                /* Unstick the entity and skip it */
                PEntity_unstick(entity, trace.impactEntity);
                entity->lagSec = delT;
                entity->velocity = lastV;
                return;
        }

        /* Entity has bad coordinates */
        if (!isfinite(entity->origin.x) || !isfinite(entity->origin.y)) {
                C_warning("Entity %s has bad coordinates", entity->name);
                PEntity_kill(entity);
                return;
        }

        /* Bad entity flew out of bounds */
        if (entity->origin.x < -CO_LIMIT || entity->origin.x > CO_LIMIT ||
            entity->origin.y < -CO_LIMIT || entity->origin.y > CO_LIMIT) {
                C_warning("Entity %s out of bounds", entity->name);
                PEntity_kill(entity);
                return;
        }

        /* No impact */
        if (trace.prop >= 1.f)
                return;

        /* Acceleration vector including drag/friction */
        entity->accel = CVec_divf(CVec_sub(entity->velocity, lastV), delT);

        /* Solve for impact velocity and time */
        delS = CVec_sub(entity->origin, from);
        if (fabsf(entity->velocity.x) <= fabsf(entity->velocity.y)) {
                entity->velocity.y = sqrtf(lastV.y * lastV.y +
                                           2 * entity->accel.y * delS.y);
                prop = 2 * delS.y / (entity->velocity.y + lastV.y);
        } else {
                entity->velocity.x = sqrtf(lastV.x * lastV.x +
                                           2 * entity->accel.x * delS.x);
                prop = 2 * delS.y / (entity->velocity.x + lastV.x);
        }

        /* Numeric error */
        if (prop < 0.)
                prop = 0.;
        if (prop > delT || !isfinite(prop))
                prop = delT;

        /* This entity is going to freeze in time for a bit so that we only
           trace the motion once per frame but we want to make up for the time
           deficit next frame */
        entity->lagSec = delT - prop;
        delT = prop;

        /* Get the corrected velocity */
        entity->velocity = CVec_add(lastV, CVec_scalef(entity->accel, delT));

        /* Try to avoid impact by stepping over this entity */
        if (PEntity_stepOver(entity, trace.impactEntity))
                return;

        /* Handle impact physics */
        if (trace.impactEntity->mass)
                PEntity_bounceMobile(entity, trace.impactEntity, trace.dir);
        else
                PEntity_bounceFixture(entity, trace.impactEntity, trace.dir);
}

