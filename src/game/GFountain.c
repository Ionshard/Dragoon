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

/* Particle entity */
typedef struct {
        PEntity entity;
        RSprite sprite;
        int deathMsec;
} GParticle;

/* Particle fountain class */
typedef struct GFountainClass {
        GEntityClass entity;
        CVec velocity, velocityRand;
        float interval, intervalRand, mass, drag;
        int lifetime;
} GFountainClass;

/* Particle fountain entity */
typedef struct GFountain {
        PEntity entity;
        RSprite sprite;
        float intervalNext;
} GFountain;

/******************************************************************************\
 Particle event function.
\******************************************************************************/
static int GParticle_eventFunc(GParticle *particle, int event, void *args)
{
        if (event == PE_IMPACT ||
            (event == PE_UPDATE && particle->deathMsec <= c_timeMsec))
                PEntity_kill(&particle->entity);
        else if (event == PE_UPDATE) {
                particle->sprite.origin = particle->entity.origin;
                particle->sprite.size = particle->entity.size;
                particle->sprite.angle = CVec_angle(particle->entity.velocity) -
                                         C_PI / 2;
                RSprite_draw(&particle->sprite);
        }
        return 0;
}

/******************************************************************************\
 Fountain event function.
\******************************************************************************/
static int GFountain_eventFunc(GFountain *fountain, int event, void *args)
{
        GParticle *particle;
        GFountainClass *fountainClass;

        /* Just draw a placeholder in the editor */
        if (event == PE_UPDATE && g_edit[0]) {
                fountain->sprite.origin = fountain->entity.origin;
                fountain->sprite.size = fountain->entity.size;
                RSprite_draw(&fountain->sprite);
                return 0;
        }

        /* Not time to spawn yet */
        else if (event != PE_UPDATE || c_timeMsec < fountain->intervalNext)
                return 0;

        /* Spawn a new particle */
        C_new(&particle);
        fountainClass = fountain->entity.entityClass;
        C_assert(fountainClass);
        RSprite_init(&particle->sprite, fountainClass->entity.spriteName);
        particle->sprite.z = fountainClass->entity.z;
        particle->entity.eventFunc = (PEventFunc)GParticle_eventFunc;
        particle->entity.origin = fountain->entity.origin;
        particle->entity.size = fountain->entity.size;
        particle->entity.drag = fountainClass->drag;
        particle->entity.mass = 1;
        particle->entity.impactOther = PIT_WORLD;
        particle->entity.velocity =
                CVec_add(fountainClass->velocity,
                         CVec_rand(fountainClass->velocityRand));
        particle->deathMsec = c_timeMsec + fountainClass->lifetime;
        PEntity_spawn(&particle->entity, "Particle");

        fountain->intervalNext = c_timeMsec + fountainClass->interval +
                                 C_rand() * fountainClass->intervalRand;
        return 0;
}

/******************************************************************************\
 Spawn a particle fountain.
\******************************************************************************/
GFountain *GFountain_spawn(GFountainClass *fountainClass,
                           const GSpawnParams *params)
{
        GFountain *fountain;

        C_new(&fountain);
        RSprite_init(&fountain->sprite, fountainClass->entity.spriteName);
        fountain->sprite.z = fountainClass->entity.z;
        fountain->entity.eventFunc = (PEventFunc)GFountain_eventFunc;
        fountain->entity.origin = params->origin;
        fountain->entity.size = params->size;
        fountain->intervalNext = c_timeMsec + fountainClass->interval +
                                 C_rand() * fountainClass->intervalRand;
        PEntity_spawn(&fountain->entity, "Fountain");
        return fountain;
}

/******************************************************************************\
 Parse a particle fountain class definition.
\******************************************************************************/
void GFountain_parseClass(FILE *file, const char *className)
{
        GFountainClass *fountainClass;
        const char *token;

        if (!(fountainClass = CNamed_alloc(&g_classRoot, className,
                                           sizeof (GFountainClass), NULL,
                                           CNP_NULL))) {
                C_warning("Class '%s' already defined", className);
                return;
        }
        GEntityClass_init(&fountainClass->entity);
        fountainClass->entity.spawnFunc = (GSpawnFunc)GFountain_spawn;

        /* Sprite defaults to class name */
        C_strncpy_buf(fountainClass->entity.spriteName, className);
        fountainClass->entity.size = R_spriteSize(className);

        /* Parse box class parameters */
        for (token = C_token(file); token[0]; token = C_token(file))
                if (GEntityClass_parseToken(&fountainClass->entity, file,
                                            token));

                /* Particle launch interval */
                else if (!strcasecmp(token, "interval") && C_openBrace(file)) {
                        fountainClass->interval = C_token_float(file);
                        fountainClass->intervalRand = C_token_float(file);
                        C_closeBrace(file);
                }

                /* Particle launch angle */
                else if (!strcasecmp(token, "velocity") && C_openBrace(file)) {
                        fountainClass->velocity.x = C_token_float(file);
                        fountainClass->velocity.y = C_token_float(file);
                        fountainClass->velocityRand.x = C_token_float(file);
                        fountainClass->velocityRand.y = C_token_float(file);
                        C_closeBrace(file);
                }

                /* Particle drag */
                else if (!strcasecmp(token, "drag"))
                        fountainClass->drag = C_token_float(file);

                /* Particle lifetime */
                else if (!strcasecmp(token, "lifetime"))
                        fountainClass->lifetime = C_token_float(file);

                else
                        C_warning("Unknown fountain param '%s'", token);
}

