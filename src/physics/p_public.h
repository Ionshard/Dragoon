/******************************************************************************\
 Dragoon - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Physics events */
typedef enum {

        /* Free events means the entity should free any allocated memory.
           Returning non-zero means the entity itself will not be freed. */
        PE_FREE,

        /* The entity has been killed somehow and should remove itself from
           the game world */
        PE_KILL,

        /* The entity is about to participate in a collision. Returning non-zero
           will prevent the collision */
        PE_IMPACT,

        /* Event sent out before and after physics are updated */
        PE_PHYSICS,
        PE_PHYSICS_DONE,

        /* The entity should render itself and update game mechanics */
        PE_UPDATE,

        P_EVENTS
} PEvent;

/* Impact class */
typedef enum {
        PIT_NONE,
        PIT_ENTITY,
        PIT_WORLD,
        PIT_ALL,
} PImpactType;

/* Physics event callback */
typedef int (*PEventFunc)(void *data, int type, void *eventArgs);

/* Base structure for physics entities */
typedef struct PEntity {
        char name[64];
        struct PEntity *ground, *ceiling, *rightWall, *leftWall;
        PEventFunc eventFunc;
        PImpactType impactOther;
        CLink linkAll, linkWorld, linkEntity;
        CVec origin, size, velocity, accel, groundOrigin;
        float mass, friction, drag, elasticity, lagSec, stepSize;
        int frameSkip;
        bool dead, manualUpdate, ignore, fly;
        void *data, *entityClass;
} PEntity;

/* Impact event */
typedef struct {
        PEntity *other;
        CVec dir, origin;
        float impulse;
} PImpactEvent;

/* Trace return structure */
typedef struct {
        PEntity *other;
        CVec end, dir;
        float prop;
        bool startSolid;
} PTrace;

/* p_box.c */
int P_entsInBox(CVec origin, CVec size, PImpactType,
                PEntity **array, int length);
#define P_entsInBox_buf(o, s, t, b) \
        P_entsInBox(o, s, t, b, sizeof (b) / sizeof (*(b)))

/* PEntity.c */
void PEntity_cleanup(PEntity *);
int PEntity_event(PEntity *, int event, void *args);
void PEntity_impact(PEntity *, PImpactType);
void PEntity_kill(PEntity *);
void PEntity_spawn(PEntity *, const char *className);
PTrace PEntity_trace(PEntity *, CVec to);
void PEntity_update(PEntity *);
void P_cleanupEntities(void);
void P_updateEntities(void);

extern CLink *p_linkAll, *p_linkWorld, *p_linkEntity;
extern CCount p_countEntities;
extern CVec p_topLeft, p_bottomRight;
extern float p_frameSec, p_gravity, p_speed;
extern int p_timeMsec, p_frameMsec;

/* PTrace.c */
#define PTrace(a, b, c, d) PTrace_box(a, b, c, d)
PTrace PTrace_box(CVec from, CVec to, CVec size, PImpactType);
#define PTrace_line(from, to, type) PTrace_box(from, to, CVec(1, 1), type)

/* p_push.c */
void P_pushRadius(CVec origin, PImpactType, float speed, float dist_max);

