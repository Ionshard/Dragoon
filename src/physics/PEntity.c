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

/* Linked lists of entities for each collision category */
CLink *p_linkAll, *p_linkWorld, *p_linkEntity;

/* Gravity, pixel velocity per second */
float p_gravity = 500.f;

/* Physics time elapsed this frame (may be less than real time) */
float p_frameSec, p_timeSec, p_speed = 1.f;
int p_timeMsec, p_frameMsec, p_frame;

/* Entity counters */
CCount p_countEntities;
static int numEntities;

/* Entity extremities, updated every frame */
CVec p_topLeft, p_bottomRight;

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
        p_frame = 0;
}

/******************************************************************************\
 Change what entities will impact this entity.
\******************************************************************************/
void PEntity_impactType(PEntity *entity, PImpactType type)
{
        if (type >= PIT_ENTITY)
                CLink_add(&entity->linkEntity, &p_linkEntity, entity);
        else
                CLink_remove(&entity->linkEntity);
        if (type >= PIT_WORLD)
                CLink_add(&entity->linkWorld, &p_linkWorld, entity);
        else
                CLink_remove(&entity->linkWorld);
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
 Returns the entity's center point.
\******************************************************************************/
CVec PEntity_center(const PEntity *entity)
{
        return CVec_add(entity->origin, CVec_divf(entity->size, 2));
}

/******************************************************************************\
 Slow down an entity with a given impulse.
\******************************************************************************/
void PEntity_slowImpulse(PEntity *entity, float impulse)
{
        CVec diff_v;
        float vel, diff;

        if (entity->mass <= 0)
                return;
        diff = impulse / entity->mass;
        if (diff >= (vel = CVec_len(entity->velocity))) {
                entity->velocity = CVec(0, 0);
                return;
        }
        diff_v = CVec_scalef(CVec_divf(entity->velocity, vel), diff);
        entity->velocity = CVec_sub(entity->velocity, diff_v);
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

        /* Update extremities */
        p_topLeft = CVec_min(p_topLeft, entity->origin);
        p_bottomRight = CVec_max(p_bottomRight,
                                 CVec_add(entity->origin, entity->size));

        /* Acceleration vector is reset every frame */
        entity->accel = CVec(0, 0);

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

        /* Reset extremities */
        p_topLeft = CVec(P_POSITION_LIMIT, P_POSITION_LIMIT);
        p_bottomRight = CVec(-P_POSITION_LIMIT, -P_POSITION_LIMIT);

        /* Update physics time */
        p_frameMsec = c_frameMsec * p_speed;
        if (p_frameMsec > P_FRAME_MSEC_MAX)
                p_frameMsec = P_FRAME_MSEC_MAX;
        p_frameSec = p_frameMsec * 0.001f;
        p_timeMsec += p_frameMsec;
        p_timeSec = p_timeMsec * 0.001f;

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

        p_frame++;
}

