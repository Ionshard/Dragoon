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

/* Player head angle limit */
#define HEAD_ANGLE_LIMIT (M_PI / 4)

/* Weapon delays */
#define CANNON_DELAY 1000

static PEntity playerEntity;
static RSprite playerHead, playerBody, playerWeapon, cursor;
static CVec aim, muzzle;
static int fireDelay;
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
        bool mirror;

        mirror = aim.x > playerEntity.origin.x + playerEntity.size.x / 2.f;
        RSprite_center(&playerBody, playerEntity.origin, playerEntity.size);
        RSprite_center(&playerWeapon, muzzle, CVec_zero());
        RSprite_center(&playerHead, playerEntity.origin, playerEntity.size);

        /* Position the cursor */
        RSprite_center(&cursor, aim, CVec_zero());
        RSprite_lookAt(&cursor, muzzle);
        RSprite_lookAt(&playerHead, aim);
        RSprite_lookAt(&playerWeapon, aim);

        /* Mirror the sprite */
        if (!(playerBody.mirror = playerHead.mirror =
                                  playerWeapon.mirror = mirror)) {
                playerHead.angle += M_PI;
                playerWeapon.angle += M_PI;
        }
        RSprite_center(&playerBody, playerEntity.origin,
                       playerEntity.size);
        RSprite_center(&playerWeapon, muzzle, CVec_zero());
        RSprite_center(&playerHead, playerEntity.origin,
                       playerEntity.size);

        RSprite_draw(&playerBody);
        RSprite_draw(&playerHead);
        RSprite_draw(&playerWeapon);
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

        /* Fire wait time */
        if ((fireDelay -= p_frameMsec) < 0)
                fireDelay = 0;

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
                RSprite_play(&playerBody,
                             g_control.x ? "playerBodyRun" : "playerBody");
        }

        /* In the air */
        else {
                playerEntity.accel.x += accelX * AIR_MOVE;
                RSprite_play(&playerBody, playerEntity.velocity.y < 0 ?
                                          "playerBodyUp" : "playerBodyDown");
        }

        /* The player entity is updated before all others so we know where to
           place the camera this frame */
        PEntity_update(&playerEntity);

        /* Update camera */
        r_cameraTo = CVec_add(playerEntity.origin,
                              CVec_divf(playerEntity.size, 2));
        r_cameraTo.x -= r_widthScaled / 2;
        r_cameraTo.y -= r_heightScaled / 2;

        /* Weapon muzzle */
        muzzle = CVec_add(playerEntity.origin, CVec_divf(playerEntity.size, 2));
        muzzle.y += G_MUZZLE_OFFSET;
        aim = CVec_add(r_cameraTo, g_mouse);
}

/******************************************************************************\
 Handles control events for the player entity.
\******************************************************************************/
bool G_dispatch_player(GEvent event)
{
        /* Swing sword */
        if (g_button == SDL_BUTTON_LEFT) {
                if (event == GE_MOUSE_DOWN)
                        RSprite_play(&playerWeapon, "sword");
                if (event == GE_MOUSE_UP)
                        RSprite_play(&playerWeapon, "repelCannon");
        }

        /* Fire cannon */
        else if (event == GE_MOUSE_DOWN && g_button == SDL_BUTTON_RIGHT &&
                 fireDelay <= 0) {
                G_fireMissile(&playerEntity, muzzle, aim, 3);
                fireDelay += CANNON_DELAY;
        }


        /* Update controls */
        if (!G_controlEvent(event))
                return FALSE;

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
        RSprite_init(&playerWeapon, "repelCannon");
        playerEntity.origin = origin;
        playerEntity.size = CVec(10, 18);
        playerEntity.mass = 1;
        playerEntity.friction = 5;
        playerEntity.drag = 0;
        playerEntity.eventFunc = (PEventFunc)playerEvent;
        playerEntity.impactOther = PIT_ENTITY;
        playerEntity.manualUpdate = TRUE;
        playerEntity.stepSize = 8;
        PEntity_spawn(&playerEntity, "Player");
        PEntity_impact(&playerEntity, PIT_ENTITY);
}

