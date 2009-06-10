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
#define JUMP_DELAY 100
#define JUMP_TRACE 4
#define WALLJUMP_VX (JUMP_V / C_SQRT2)
#define WALLJUMP_VY (JUMP_V / C_SQRT2)
#define GROUND_A 800
#define AIR_MOVE 0.2

/* Weapon delays */
#define CANNON_DELAY 300

/* Minimal distance the pointer keeps from the player */
#define CURSOR_DIST 18

/* Velocity display params */
#define VEL_SCALE 0.001f
#define VEL_JIGGLE_SCALE 0.000005f
#define VEL_JIGGLE_SPEED 0.1f
#define VEL_MIN 200.f
#define VEL_YELLOW 350.f
#define VEL_RED 500.f

static GEntityClass playerClass;
static PEntity player;
static RSprite playerHead, playerBody, playerWeapon, cursor;
static RText hudVelocity;
static CVec aim, muzzle;
static int fireDelay, jumpDelay;
static bool jumpHeld;

/******************************************************************************\
 Draw velocity meter.
\******************************************************************************/
static void drawVelocity(void)
{
        CColor color_a, color_b;
        float lerp;
        int vel, vel2;

        vel = (int)CVec_len(player.velocity);
        vel2 = vel * vel;
        C_snprintf_buf(hudVelocity.string, "%d", vel);

        /* Color */
        if (vel <= VEL_MIN) {
                color_a = CColor(1, 1, 1, 0);
                color_b = CColor(1, 1, 1, 1);
                lerp = vel / VEL_MIN;
        } else if (vel <= VEL_YELLOW) {
                color_a = CColor(1, 1, 1, 1);
                color_b = CColor(1, 1, 0, 1);
                lerp = (vel - VEL_MIN) / (VEL_YELLOW - VEL_MIN);
        } else if (vel <= VEL_RED) {
                color_a = CColor(1, 1, 0, 1);
                color_b = CColor(1, 0, 0, 1);
                lerp = (vel - VEL_YELLOW) / (VEL_RED - VEL_YELLOW);
        } else {
                color_a = CColor(1, 0, 0, 1);
                color_b = CColor(1, 1, 1, 1);
                lerp = C_rand();
        }
        hudVelocity.modulate = CColor_lerp(color_a, lerp, color_b);

        /* Jiggle with speed */
        hudVelocity.jiggleRadius = vel * vel * VEL_JIGGLE_SCALE;
        hudVelocity.jiggleSpeed = VEL_JIGGLE_SPEED;

        /* Scale with speed */
        hudVelocity.scale = CVec(1 + vel * VEL_SCALE, 1 + vel * VEL_SCALE);

        RText_center(&hudVelocity,
                     CVec(r_widthScaled / 2, r_heightScaled - 10));
        RText_draw(&hudVelocity);
}

/******************************************************************************\
 Draw the HUD.
\******************************************************************************/
void G_drawHud(void)
{
        drawVelocity();

        /* Cursor */
        RSprite_lookAt(&cursor, CVec_sub(muzzle, r_camera));
        RSprite_center(&cursor, CVec_sub(aim, r_camera), CVec(0, 0));
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
                muzzle = CVec_add(player.origin,
                                  CVec_divf(player.size, 2));
                muzzle.y += G_MUZZLE_OFFSET;

                /* Position body parts */
                mirror = aim.x > player.origin.x +
                                 player.size.x / 2.f;
                RSprite_center(&playerBody, player.origin,
                               player.size);
                RSprite_center(&playerWeapon, muzzle, CVec(0, 0));
                RSprite_center(&playerHead, player.origin,
                               player.size);

                /* Position the head and weapon */
                RSprite_lookAt(&playerHead, aim);
                RSprite_lookAt(&playerWeapon, aim);

                /* Mirror the sprite */
                if (!(playerBody.mirror = playerHead.mirror =
                                          playerWeapon.mirror = mirror)) {
                        playerHead.angle += M_PI;
                        playerWeapon.angle += M_PI;
                }
                RSprite_center(&playerBody, player.origin,
                               player.size);
                RSprite_center(&playerWeapon, muzzle, CVec(0, 0));
                RSprite_center(&playerHead, player.origin,
                               player.size);

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
        PTrace trace;
        PEntity *other;
        CVec to;

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
        other = NULL;

        /* Jump off of ground */
        other = player.ground;
        if (!other) {
                to = CVec_add(player.origin, CVec(0, JUMP_TRACE));
                trace = PEntity_trace(&player, to);
                other = trace.other;
        }
        if (other) {
                if (PEntity_event(other, GE_JUMPED_AWAY, &player))
                        return;
                if (player.velocity.y > 0)
                        player.velocity.y = 0;
                player.velocity.y = -JUMP_V;
        }

        /* Walljump off of right wall */
        if (!other) {
                other = player.rightWall;
                if (!other) {
                        to = CVec_add(player.origin, CVec(JUMP_TRACE, 0));
                        trace = PEntity_trace(&player, to);
                        other = trace.other;
                }
                if (other) {
                        if (PEntity_event(other, GE_JUMPED_AWAY, &player))
                                return;
                        if (player.velocity.x > 0)
                                player.velocity.x = 0;
                        player.velocity.x -= WALLJUMP_VX;
                        player.velocity.y -= WALLJUMP_VY;
                }
        }

        /* Walljump off of left wall */
        if (!other) {
                other = player.leftWall;
                if (!other) {
                        to = CVec_add(player.origin, CVec(-JUMP_TRACE, 0));
                        trace = PEntity_trace(&player, to);
                        other = trace.other;
                }
                if (other) {
                        if (PEntity_event(other, GE_JUMPED_AWAY, &player))
                                return;
                        if (player.velocity.x < 0)
                                player.velocity.x = 0;
                        player.velocity.x += WALLJUMP_VX;
                        player.velocity.y -= WALLJUMP_VY;
                }
        }

        /* Didn't jump */
        if (!other)
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

        /* Do nothing if paused or dead */
        if (!CLink_linked(&player.linkAll) || player.dead || p_speed <= 0)
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
        if (player.ground) {
                if (player.ground->friction >= 0 &&
                    player.ground->friction < 1)
                        accelX *= player.ground->friction;
                player.accel.x += accelX;
                RSprite_play(&playerBody,
                             g_control.x ? "playerBodyRun" : "playerBody");
        }

        /* In the air */
        else {
                player.accel.x += accelX * AIR_MOVE;
                RSprite_play(&playerBody, player.velocity.y < 0 ?
                                          "playerBodyUp" : "playerBodyDown");
        }

        /* Make sure player sprite depth is correct */
        playerHead.z = playerBody.z = playerWeapon.z = playerClass.z;

        /* Update camera */
        r_cameraTo = CVec_add(player.origin, CVec_divf(player.size, 2));
        r_cameraTo.x -= r_widthScaled / 2;
        r_cameraTo.y -= r_heightScaled / 2;

        /* Make sure camera is not looking outside the map area */
        r_cameraTo = CVec_max(p_topLeft, r_cameraTo);
        r_cameraTo = CVec_min(CVec_sub(p_bottomRight,
                                       CVec(r_widthScaled, r_heightScaled)),
                              r_cameraTo);

        /* Update aim vector */
        aim = CVec_add(r_cameraTo, constrainedMouse());

        /* Kill the player if he falls off the map */
        if (p_frame > 0 &&
            (player.origin.x <= p_topLeft.x ||
             player.origin.y <= p_topLeft.y ||
             player.origin.x + player.size.x >= p_bottomRight.x ||
             player.origin.y + player.size.y >= p_bottomRight.y)) {
                C_warning("Player fell off the map");
                PEntity_kill(&player);
        }

        /* If the player is dead, start a new game */
        if (player.dead) {
                C_debug("Player died");
                G_newGame();
        }
}

/******************************************************************************\
 Handles control events for the player entity.
\******************************************************************************/
bool G_dispatch_player(GEvent event)
{
        if (!CLink_linked(&player.linkAll))
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
                G_fireMissile(&player, "repelMissile", muzzle, aim);
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

        /* Init player sprites */
        RSprite_init(&playerHead, "playerHead");
        RSprite_init(&playerBody, "playerBody");
        RSprite_init(&playerWeapon, "repelCannon");

        /* Init HUD sprites */
        RSprite_init(&cursor, "cursor");
        cursor.z = G_Z_HUD;
        RText_init_range(&hudVelocity, "gfx/digits.png", '0', 4, 3, "0");
        hudVelocity.z = G_Z_HUD;

        /* Spawn player */
        C_zero(&player);
        player.entityClass = &playerClass;
        player.origin = origin;
        player.size = CVec(10, 16);
        player.mass = 1;
        player.friction = 5;
        player.elasticity = 0.25;
        player.eventFunc = (PEventFunc)playerEvent;
        player.impactOther = PIT_ENTITY;
        player.manualUpdate = FALSE;
        player.stepSize = 8;
        PEntity_spawn(&player, "Player");
        PEntity_impact(&player, PIT_ENTITY);
        G_depthSortEntity(&player);
}

