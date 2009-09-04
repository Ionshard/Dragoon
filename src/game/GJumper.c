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

/* Jumper entity */
typedef struct {
        PEntity entity;
        RSprite sprite;
} GJumper;

/* Jumper class */
typedef struct GJumperClass {
        GEntityClass entity;
        float mass, elasticity, friction;
        char hiddenName[C_NAME_MAX], idleName[C_NAME_MAX],
             riseName[C_NAME_MAX], runName[C_NAME_MAX];
} GJumperClass;

/******************************************************************************\
 Box event function.
\******************************************************************************/
int GJumper_eventFunc(GJumper *jumper, int event, void *args)
{
        GJumperClass *jumperClass = jumper->entity.entityClass;
        PImpactEvent *impactEvent;

        switch (event) {
        case PE_UPDATE:
                RSprite_draw(&jumper->sprite);
                break;
        case PE_IMPACT:
                break;
        default:
                break;
        }
        return 0;
}

/******************************************************************************\
 Spawn a jumper.
\******************************************************************************/
GJumper *GJumper_spawn(GJumperClass *jumperClass)
{
        GJumper *jumper;

        C_new(&jumper);
        RSprite_init_time(&jumper->sprite, jumperClass->entity.spriteName,
                          &p_timeMsec);
        jumper->entity.eventFunc = (PEventFunc)GJumper_eventFunc;
        jumper->sprite.z = jumperClass->entity.z;
        jumper->entity.mass = jumperClass->mass;
        jumper->entity.elasticity = jumperClass->elasticity;
        jumper->entity.friction = jumperClass->friction;
        jumper->entity.impactOther = PIT_ENTITY;
        PEntity_spawn(&jumper->entity, "Jumper");
        PEntity_impactType(&jumper->entity, PIT_ENTITY);
        return jumper;
}

/******************************************************************************\
 Parse definition for a jumper-type NPC.
\******************************************************************************/
void GJumper_parseClass(FILE *file, const char *className)
{
        GJumperClass *jumperClass;
        const char *token;

        if (!(jumperClass = CNamed_alloc(&g_classRoot, className,
                                         sizeof (GJumperClass), NULL,
                                         CNP_NULL))) {
                C_warning("Class '%s' already defined", className);
                return;
        }
        GEntityClass_init(&jumperClass->entity);
        jumperClass->entity.spawnFunc = (GSpawnFunc)GFountain_spawn;

        /* Entity defaults */
        jumperClass->entity.z = G_Z_CHAR;
        jumperClass->mass = 1;
        jumperClass->elasticity = 0.2;
        jumperClass->friction = 0.2;

        /* Sprite defaults to class name */
        C_strncpy_buf(jumperClass->entity.spriteName, className);
        jumperClass->entity.size = R_spriteSize(className);

        /* Parse box class parameters */
        for (token = C_token(file); token[0]; token = C_token(file))
                if (GEntityClass_parseToken(&jumperClass->entity, file,
                                            token));

                /* Jumper sprites */
                else if (!strcasecmp(token, "sprite_hidden"))
                        R_parseInlineSprite(file, C_va("%s.hidden", className),
                                            jumperClass->hiddenName);
                else if (!strcasecmp(token, "sprite_idle"))
                        R_parseInlineSprite(file, C_va("%s.idle", className),
                                            jumperClass->idleName);
                else if (!strcasecmp(token, "sprite_rise"))
                        R_parseInlineSprite(file, C_va("%s.rise", className),
                                            jumperClass->riseName);
                else if (!strcasecmp(token, "sprite_run"))
                        R_parseInlineSprite(file, C_va("%s.run", className),
                                            jumperClass->runName);

                /* Mass */
                else if (!strcasecmp(token, "mass"))
                        jumperClass->mass = C_token_float(file);

                /* Elasticity */
                else if (!strcasecmp(token, "elasticity"))
                        jumperClass->elasticity = C_token_float(file);

                /* Friction */
                else if (!strcasecmp(token, "friction"))
                        jumperClass->friction = C_token_float(file);

                else
                        C_warning("Unknown jumper param '%s'", token);
}

