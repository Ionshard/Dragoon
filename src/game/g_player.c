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
#define JUMP_V 150
#define JUMP_DELAY 50
#define WALLJUMP_VX (JUMP_V / C_SQRT2)
#define WALLJUMP_VY (JUMP_V / C_SQRT2)
#define GROUND_A 800
#define AIR_MOVE 0.2

/* Weapon delays */
#define CANNON_DELAY 300

/* Minimal distance the pointer keeps from the player */
#define CURSOR_DIST 18

static GEntityClass playerClass;
static PEntity playerEntity;
static RSprite playerHead, playerBody, playerWeapon, cursor;
static CVec aim, muzzle;
static int fireDelay, jumpDelay;
static bool jumpHeld;

/******************************************************************************\
 Draw the HUD.
\******************************************************************************/
void G_drawHud(void)
{
        RSprite_lookAt(&cursor, muzzle);
        RSprite_center(&cursor, aim, CVec(0, 0));
        RSprite_draw(&cursor);
}

/******************************************************************************\
 Ensures the mouse position follows positioning rules.
\******************************************************************************/
static CVec constrainedMouse(void)
{
        CVec center, diff;
        float dist;

        center = CVec(r_widthScaled / 2, r_heightScaled / 2);
        center.y += G_MUZZLE_OFFSET;
        diff = CVec_sub(g_mouse, center);
        dist = CVec_len(diff);
        if (!dist)
                return CVec(CURSOR_DIST, 0);
        else if (dist < CURSOR_DIST)
                return CVec_add(center, CVec_scalef(diff, CURSOR_DIST / dist));
        return g_mouse;
}

/******************************************************************************\
 Player entity event function.
\******************************************************************************/
static int playerEvent(PEntity *entity, int event, void *args)
{
        bool mirror;

        switch (event) {
        case PE_FREE:
                return 1;
        case PE_UPDATE:

                /* Weapon muzzle */
                muzzle = CVec_add(playerEntity.origin,
                                  CVec_divf(playerEntity.size, 2));
                muzzle.y += G_MUZZLE_OFFSET;
                aim = CVec_add(r_cameraTo, constrainedMouse());

                /* Position body parts */
                mirror = aim.x > playerEntity.origin.x +
                                 playerEntity.size.x / 2.f;
                RSprite_center(&playerBody, playerEntity.origin,
                               playerEntity.size);
                RSprite_center(&playerWeapon, muzzle, CVec(0, 0));
                RSprite_center(&playerHead, playerEntity.origin,
                               playerEntity.size);

                /* Position the head and weapon */
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
                RSprite_center(&playerWeapon, muzzle, CVec(0, 0));
                RSprite_center(&playerHead, playerEntity.origin,
                               playerEntity.size);

                RSprite_draw(&playerBody);
                RSprite_draw(&playerHead);
                RSprite_draw(&playerWeapon);
                break;
        default:
                break;
        }
        return 0;
}

/******************************************************************************\
 Checks if the player can jump and handles the acceleration.
\******************************************************************************/
static void checkJump(void)
{
        /* Release jump */
        if (g_control.y >= 0)
                jumpHeld = FALSE;

        /* Can't jump yet */
        if ((jumpDelay -= p_frameMsec) < 0)
                jumpDelay = 0;
        else
                return;

        /* Don't want to jump */
        if (jumpHeld || g_control.y >= 0)
                return;

        /* Jump off of a surface */
        if (playerEntity.ground) {
                if (playerEntity.velocity.y > -JUMP_V)
                        playerEntity.velocity.y = -JUMP_V;
        } else if (playerEntity.rightWall) {
                if (playerEntity.velocity.x > -WALLJUMP_VX)
                        playerEntity.velocity.x = -WALLJUMP_VX;
                playerEntity.velocity.y -= WALLJUMP_VY;
        } else if (playerEntity.leftWall) {
                if (playerEntity.velocity.x < WALLJUMP_VX)
                        playerEntity.velocity.x = WALLJUMP_VX;
                playerEntity.velocity.y -= WALLJUMP_VY;
        } else
                return;

        /* Jumped somehow */
        jumpHeld = TRUE;
        jumpDelay = JUMP_DELAY;
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
        checkJump();

        /* Fire wait time */
        if ((fireDelay -= p_frameMsec) < 0)
                fireDelay = 0;

        /* Horizontal movement */
        accelX = 0;
        if (g_control.x > 0)
                accelX += GROUND_A;
        else if (g_control.x < 0)
                accelX -= GROUND_A;

        /* On the ground */
        if (playerEntity.ground) {
                playerEntity.accel.x += accelX;
                RSprite_play(&playerBody,
                             g_control.x ? "playerBodyRun" : "playerBody");
        }

        /* In the air */
        else {
                playerEntity.accel.x += accelX * AIR_MOVE;
                RSprite_play(&playerBody, playerEntity.velocity.y < 0 ?
                                          "playerBodyUp" : "playerBodyDown");
        }

        /* Make sure player sprite depth is correct */
        playerHead.z = playerBody.z = playerWeapon.z = playerClass.z;

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
        if (!CLink_linked(&playerEntity.linkAll))
                return FALSE;

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
                RSprite_play(&playerWeapon, "repelCannon");
                G_fireMissile(&playerEntity, "repelMissile", muzzle, aim);
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
        /* Init player class */
        C_zero(&playerClass);
        C_strncpy_buf(playerClass.named.name, "Player");
        playerClass.z = G_Z_CHAR;

        /* Init sprites */
        RSprite_init(&cursor, "cursor");
        RSprite_init(&playerHead, "playerHead");
        RSprite_init(&playerBody, "playerBody");
        RSprite_init(&playerWeapon, "repelCannon");
        cursor.z = G_Z_HUD;

        /* Spawn player */
        playerEntity.entityClass = &playerClass;
        playerEntity.origin = origin;
        playerEntity.size = CVec(10, 16);
        playerEntity.mass = 1;
        playerEntity.friction = 5;
        playerEntity.drag = 0;
        playerEntity.eventFunc = (PEventFunc)playerEvent;
        playerEntity.impactOther = PIT_ENTITY;
        playerEntity.manualUpdate = FALSE;
        playerEntity.stepSize = 8;
        PEntity_spawn(&playerEntity, "Player");
        PEntity_impact(&playerEntity, PIT_ENTITY);
        G_depthSortEntity(&playerEntity);
}

