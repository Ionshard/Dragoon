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

/* If an entity bounces off with less velocity than this, just stop it */
#define BOUNCE_V_MIN 40

/******************************************************************************\
 Compute impact event origin.
\******************************************************************************/
static void PImpactEvent_computeOrigin(PImpactEvent *event, PEntity *entity)
{
        float min, max;

        if (fabsf(event->dir.x) >= fabsf(event->dir.y)) {
                event->origin.x = event->dir.x < 0 ? entity->origin.x :
                                  entity->origin.x + entity->size.x;
                min = entity->origin.y;
                if (event->other->origin.y > min)
                        min = event->other->origin.y;
                max = entity->origin.y + entity->size.y;
                if (event->other->origin.y + event->other->size.y < max)
                        max = event->other->origin.y + event->other->size.y;
                event->origin.y = (max + min) / 2;
        } else {
                min = entity->origin.x;
                if (event->other->origin.x > min)
                        min = event->other->origin.x;
                max = entity->origin.x + entity->size.x;
                if (event->other->origin.x + event->other->size.x < max)
                        max = event->other->origin.x + event->other->size.x;
                event->origin.x = (max + min) / 2;
                event->origin.y = event->dir.y < 0 ? entity->origin.y :
                                  entity->origin.y + entity->size.y;
        }
}

/******************************************************************************\
 Check if either entity does not want the impact. Triggers impact effects.
 Returns FALSE if impact was avoided.
\******************************************************************************/
bool PEntity_impact(PEntity *entity, PEntity *other, CVec dir)
{
        PImpactEvent impactEvent;
        float velA, velB;

        velA = CVec_dot(entity->velocity, dir);
        velB = CVec_dot(other->velocity, dir);
        impactEvent.other = other;
        impactEvent.dir = dir;
        impactEvent.impulse = velA * entity->mass + velB * other->mass;
        PImpactEvent_computeOrigin(&impactEvent, entity);
        if (PEntity_event(entity, PE_IMPACT, &impactEvent))
                return FALSE;
        impactEvent.other = entity;
        impactEvent.dir = CVec_scalef(dir, -1.f);
        PImpactEvent_computeOrigin(&impactEvent, other);
        if (PEntity_event(other, PE_IMPACT, &impactEvent))
                return FALSE;
        return TRUE;
}

/******************************************************************************\
 Entity elastic collision with a mobile entity.
\******************************************************************************/
static void PEntity_bounceMobile(PEntity *entity, PEntity *other, CVec dir)
{
        float velA, velB, velANew, velBNew;

        C_assert(entity->mass > 0.f);
        C_assert(other->mass > 0.f);

        /* Check if either entity does not want the impact */
        if (!PEntity_impact(entity, other, dir))
                return;

        /* Recompute velocities in case one entity changed another's velocity */
        velA = CVec_dot(entity->velocity, dir);
        velB = CVec_dot(other->velocity, dir);

        /* Compute new velocities */
        velANew = (velA * (entity->mass - other->mass) +
                   2 * other->mass * velB) / (entity->mass + other->mass);
        velBNew = velANew + velA - velB;

        /* Apply elasticity */
        velANew *= entity->elasticity;
        velBNew *= other->elasticity;

        /* If the entity is bouncing off with very low speed, just stop */
        if (fabsf(velANew) < BOUNCE_V_MIN)
                velANew = 0;
        if (fabsf(velBNew) < BOUNCE_V_MIN)
                velBNew = 0;

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
        float vel, velNew;

        /* Check if either entity does not want the impact */
        if (!PEntity_impact(entity, other, dir))
                return;

        /* New velocity */
        velNew = -(vel = CVec_dot(entity->velocity, dir)) * entity->elasticity;

        /* If the entity is bouncing off with very low speed, just stop */
        if (fabsf(velNew) < BOUNCE_V_MIN)
                velNew = 0;

        /* Apply bounce impulse */
        entity->velocity = CVec_add(entity->velocity,
                                    CVec_scalef(dir, velNew - vel));
}

/******************************************************************************\
 Unstick an entity from inside of another.
\******************************************************************************/
static void PEntity_unstick(PEntity *entity, const PEntity *other)
{
        CVec entityCenter, otherCenter, centerDiff;
        float dist_h, dist_v;

        entityCenter = CVec_add(entity->origin, CVec_divf(entity->size, 2));
        otherCenter = CVec_add(other->origin, CVec_divf(other->size, 2));
        centerDiff = CVec_sub(otherCenter, entityCenter);
        dist_h = centerDiff.x >= 0 ?
                 entity->origin.x + entity->size.x - other->origin.x :
                 other->origin.x + other->size.x - entity->origin.x;
        dist_v = centerDiff.y >= 0 ?
                 entity->origin.y + entity->size.y - other->origin.y :
                 other->origin.y + other->size.y - entity->origin.y;
        if (dist_v < dist_h)
                entity->origin.y += centerDiff.y >= 0 ? -dist_v : dist_v;
        else
                entity->origin.x += centerDiff.x >= 0 ? -dist_h : dist_h;
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
                                 other->origin.y) {
                                entity->ground = other;
                                entity->groundOrigin = other->origin;
                        }
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

        /* Move with ground entity */
        if (entity->ground && !entity->ground->dead &&
            !CVec_eq(entity->groundOrigin, entity->ground->origin)) {
                delS = CVec_sub(entity->ground->origin, entity->groundOrigin);
                trace = PEntity_trace(entity, CVec_add(entity->origin, delS));
                entity->origin = trace.end;
        }

        /* Clip acceleration to make sure we move this frame */
        PEntity_clipAccel(entity, delT);

        /* Add acceleration for the frame */
        lastV = entity->velocity;
        entity->velocity = CVec_add(entity->velocity,
                                    CVec_scalef(entity->accel, delT));

        /* On ground, apply friction */
        if (entity->ground) {
                if ((prop = 1 - entity->ground->friction *
                                entity->friction * delT) < 0)
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
        entity->origin = trace.end;

        /* Entity started out stuck in something */
        if (trace.startSolid) {
                PImpactEvent impactEvent;

                /* Generate an impact event */
                impactEvent.other = trace.other;
                impactEvent.dir = CVec_sub(entity->origin, trace.other->origin);
                impactEvent.dir = CVec_norm(impactEvent.dir);
                impactEvent.impulse = 0;
                PImpactEvent_computeOrigin(&impactEvent, entity);
                PEntity_event(entity, PE_IMPACT, &impactEvent);
                impactEvent.other = entity;
                impactEvent.dir = CVec_scalef(impactEvent.dir, -1.f);
                PImpactEvent_computeOrigin(&impactEvent, trace.other);
                PEntity_event(trace.other, PE_IMPACT, &impactEvent);

                /* Unstick the entity and skip it */
                PEntity_unstick(entity, trace.other);
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
        if (entity->origin.x < -P_POSITION_LIMIT ||
            entity->origin.x > P_POSITION_LIMIT ||
            entity->origin.y < -P_POSITION_LIMIT ||
            entity->origin.y > P_POSITION_LIMIT) {
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
        if (PEntity_stepOver(entity, trace.other))
                return;

        /* Handle impact physics */
        if (trace.other->mass)
                PEntity_bounceMobile(entity, trace.other, trace.dir);
        else
                PEntity_bounceFixture(entity, trace.other, trace.dir);
}

