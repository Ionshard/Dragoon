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

/* Distance below a entity to trace to look for ground */
#define GROUND_DIST 0.003125f

/* Limit the length of time a physics frame will simulate to account for
   lagged frames and gdb debugging */
#define FRAME_MSEC_MAX (1000 / 30)

/* Velocity at which an entity is judged to be broken */
#define V_LIMIT 10000

/* Positions outside of this limit are assumed to be broken entities */
#define CO_LIMIT 100000

/* Linked lists of entities for each collision category */
CLink *p_linkAll, *p_linkWorld, *p_linkEntity;

/* Gravity, pixel velocity per second */
float p_gravity = 500.f;

/* Physics time elapsed this frame (may be less than real time) */
float p_frameSec, p_speed = 1.f;
int p_timeMsec, p_frameMsec;

/* Entity counters */
CCount p_countEntities;
static int numEntities;

/******************************************************************************\
 Free memory used by dead entities.
\******************************************************************************/
static void freeDead(bool force)
{
        PEntity *entity;
        CLink *link;

        for (link = p_linkAll; link; ) {
                entity = CLink_get(link);
                link = CLink_next(link);
                if (!force && (!entity->dead || --entity->dead))
                        continue;

                /* Can only remove from linked lists safely from here */
                CLink_remove(&entity->linkAll);
                CLink_remove(&entity->linkWorld);
                CLink_remove(&entity->linkEntity);

                /* Free memory */
                if (!PEntity_event(entity, PE_FREE, NULL))
                        C_free(entity->data ? entity->data : entity);
        }
}

/******************************************************************************\
 Kill all entities and free memory used.
\******************************************************************************/
void P_cleanupEntities(void)
{
        CLink *link;

        for (link = p_linkAll; link; link = CLink_next(link))
                PEntity_kill(CLink_get(link));
        freeDead(TRUE);
}

/******************************************************************************\
 Change what entities will impact this entity.
\******************************************************************************/
void PEntity_impact(PEntity *entity, PImpactType type)
{
        if (type >= PIT_ENTITY)
                CLink_add(&entity->linkEntity, &p_linkEntity, entity);
        else
                CLink_remove(&entity->linkEntity);
        if (type >= PIT_WORLD)
                CLink_add(&entity->linkWorld, &p_linkWorld, entity);
        else
                CLink_remove(&entity->linkWorld);

        /* Cannot impact anything so we cannot have a ground entity */
        if (type == PIT_NONE)
                entity->ground = NULL;
}

/******************************************************************************\
 Spawn a new entity into the world.
\******************************************************************************/
void PEntity_spawn(PEntity *entity, const char *className)
{
        if (!entity)
                return;
        entity->dead = 0;
        C_snprintf_buf(entity->name, "#%d (%s)", ++numEntities, className);
        CLink_add_rear(&entity->linkAll, &p_linkAll, entity);
}

/******************************************************************************\
 Send an entity an event.
\******************************************************************************/
int PEntity_event(PEntity *entity, int event, void *args)
{
        if (!entity)
                return 0;
        C_assert(entity->eventFunc != NULL);
        return entity->eventFunc(entity->data ? entity->data : entity,
                                 event, args);
}

/******************************************************************************\
 Cleanup resources used by an entity without delivering a kill event.
\******************************************************************************/
void PEntity_cleanup(PEntity *entity)
{
        PEntity *other;
        CLink *link;

        /* Flag for removal */
        entity->dead = 2;

        /* This entity can't be any others' ground entity anymore */
        for (link = p_linkEntity; link; link = CLink_next(link)) {
                other = CLink_get(link);
                if (other->ground == entity)
                        other->ground = NULL;
        }
}

/******************************************************************************\
 Delete an entity from the world.
\******************************************************************************/
void PEntity_kill(PEntity *entity)
{
        if (!entity)
                return;
        if (!PEntity_event(entity, PE_KILL, NULL))
                PEntity_cleanup(entity);
}

/******************************************************************************\
 Run a trace using the entity's box.
\******************************************************************************/
PTrace PEntity_trace(PEntity *entity, CVec to)
{
        PTrace trace;

        entity->ignore = TRUE;
        trace = PTrace(entity->origin, to, entity->size, entity->impactOther);
        entity->ignore = FALSE;
        return trace;
}

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
        to.x += entity->velocity.x > 0 ? GROUND_DIST : -GROUND_DIST;
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
static void PEntity_clipAccel(PEntity *entity)
{
        CLink *link;

        /* Impact class */
        link = p_linkEntity;
        if (entity->impactOther == PIT_WORLD)
                link = p_linkWorld;
        if (entity->impactOther == PIT_ALL)
                link = p_linkAll;

        /* Iterate over collision class linked list */
        entity->ground = NULL;
        for (; link; link = CLink_next(link)) {
                PEntity *other;

                if ((other = CLink_get(link)) == entity)
                        continue;
                C_assert(other);

                /* Horizontal */
                if (entity->accel.x &&
                    other->origin.y < entity->origin.y + entity->size.y &&
                    other->origin.y + other->size.y > entity->origin.y) {

                        /* Clip left */
                        if (entity->accel.x < 0) {
                                if (fabsf(entity->origin.x - other->origin.x -
                                          other->size.x) <= GROUND_DIST)
                                        entity->accel.x = 0;
                        }

                        /* Clip right */
                        else if (fabsf(entity->origin.x + entity->size.x -
                                       other->origin.x) <= GROUND_DIST)
                                entity->accel.x = 0;
                }

                /* Vertical */
                if (!entity->accel.y ||
                    other->origin.x >= entity->origin.x + entity->size.x ||
                    other->origin.x + other->size.x <= entity->origin.x)
                        continue;

                /* Clip up */
                if (entity->accel.y < 0) {
                        if (fabsf(entity->origin.y - other->origin.y -
                                  other->size.y) <= GROUND_DIST)
                                entity->accel.y = 0;
                }

                /* Clip down (ground) */
                else if (fabsf(entity->origin.y + entity->size.y -
                               other->origin.y) <= GROUND_DIST) {
                        entity->accel.y = 0;
                        entity->ground = other;
                }
        }
}

/******************************************************************************\
 Update physics for an entity.
\******************************************************************************/
static void PEntity_physics(PEntity *entity, float delT)
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
        if (entity->frameSkip * FRAME_MSEC_MAX * 0.001f > delT) {
                entity->lagSec = delT;
                return;
        }
        entity->lagSec = 0.f;

        /* Add gravity */
        if (!entity->fly)
                entity->accel.y += p_gravity;

        /* Clip acceleration to make sure we move this frame */
        PEntity_clipAccel(entity);

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

/******************************************************************************\
 Update an entity.
\******************************************************************************/
void PEntity_update(PEntity *entity)
{
        if (entity->dead)
                return;

        /* Update entity physics */
        PEntity_event(entity, PE_PHYSICS, NULL);
        PEntity_physics(entity, p_frameSec);
        PEntity_event(entity, PE_PHYSICS_DONE, NULL);

        /* Acceleration vector is reset every frame */
        entity->accel = CVec_zero();

        /* Entity might have died */
        if (entity->dead)
                return;

        /* During updates entities draw themselves and can change their
           parameters including setting a new accel vector */
        PEntity_event(entity, PE_UPDATE, NULL);
}

/******************************************************************************\
 Update entities.
\******************************************************************************/
void P_updateEntities(void)
{
        PEntity *entity;
        CLink *link;

        /* Update physics time */
        p_frameMsec = c_frameMsec * p_speed;
        if (p_frameMsec > FRAME_MSEC_MAX)
                p_frameMsec = FRAME_MSEC_MAX;
        p_frameSec = p_frameMsec * 0.001f;
        p_timeMsec += p_frameMsec;

        /* Update every entity */
        for (link = p_linkAll; link; link = CLink_next(link)) {
                entity = CLink_get(link);
                if (CHECKED)
                        CCount_add(&p_countEntities, 1);
                if (entity->manualUpdate)
                        continue;
                PEntity_update(entity);
        }

        /* Remove any entities that died this frame */
        freeDead(FALSE);
}

