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

/* Player movement parameters */
#define JUMP_V -200
#define GROUND_A 800
#define AIR_MOVE 0.2

static PEntity playerEntity;
static RSprite playerHead, playerBody, cursor;
static bool jumpHeld;

/******************************************************************************\
 Player entity event function.
\******************************************************************************/
static int playerEvent(PEntity *entity, int event, void *args)
{
        if (event == PE_FREE)
                return 1;
        return 0;
}

/******************************************************************************\
 Draw the player entity.
\******************************************************************************/
void G_drawPlayer(void)
{
        CVec center;

        RSprite_center(&playerBody, playerEntity.origin, playerEntity.size);
        playerHead.origin = playerBody.origin;
        playerHead.origin.y -= 7;
        playerBody.origin.y += 8;
        cursor.origin = CVec_add(r_camera, g_mouse);
        center = CVec_divf(CVec(r_widthScaled, r_heightScaled), 2);
        cursor.angle = CVec_angle(CVec_sub(g_mouse, center));
        RSprite_draw(&playerHead);
        RSprite_draw(&playerBody);
        RSprite_draw(&cursor);
}

/******************************************************************************\
 Player must be updated outside of normal entities once per frame because the
 player entity's motion affects the camera.
\******************************************************************************/
void G_updatePlayer(void)
{
        float accelX;

        if (!CLink_linked(&playerEntity.linkAll))
                return;

        /* Horizontal movement */
        accelX = 0;
        if (g_control.x > 0)
                accelX += GROUND_A;
        else if (g_control.x < 0)
                accelX -= GROUND_A;

        /* Release jump */
        if (g_control.y >= 0)
                jumpHeld = FALSE;

        /* On the ground */
        if (playerEntity.ground) {
                playerEntity.accel.x += accelX;
                if (!jumpHeld && g_control.y < 0 &&
                    playerEntity.velocity.y > JUMP_V) {
                        jumpHeld = TRUE;
                        playerEntity.velocity.y = JUMP_V;
                }
        }

        /* In the air */
        else
                playerEntity.accel.x += accelX * AIR_MOVE;

        /* The player entity is updated before all others so we know where to
           place the camera this frame */
        PEntity_update(&playerEntity);

        /* Update camera */
        r_cameraTo = CVec_add(playerEntity.origin,
                              CVec_divf(playerEntity.size, 2));
        r_cameraTo.x -= r_widthScaled / 2;
        r_cameraTo.y -= r_heightScaled / 2;
}

/******************************************************************************\
 Handles control events for the player entity.
\******************************************************************************/
bool G_dispatch_player(GEvent event)
{
        /* Update controls */
        if (!G_controlEvent(event))
                return FALSE;

        /* Mirror the sprite */
        if (g_control.x > 0)
                playerHead.mirror = playerBody.mirror = TRUE;
        else if (g_control.x < 0)
                playerHead.mirror = playerBody.mirror = FALSE;

        return TRUE;
}

/******************************************************************************\
 Spawn the player within the world.
\******************************************************************************/
void G_spawnPlayer(CVec origin)
{
        RSprite_init(&cursor, "cursor");
        RSprite_init(&playerHead, "playerHead");
        RSprite_init(&playerBody, "playerBody");
        playerEntity.origin = origin;
        playerEntity.size = CVec(12, 32);
        playerEntity.mass = 1;
        playerEntity.friction = 5;
        playerEntity.drag = 0;
        playerEntity.eventFunc = (PEventFunc)playerEvent;
        playerEntity.impactOther = PIT_ENTITY;
        playerEntity.manualUpdate = TRUE;
        playerEntity.stepSize = 16;
        PEntity_spawn(&playerEntity, "Player");
}

