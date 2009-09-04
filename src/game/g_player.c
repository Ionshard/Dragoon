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
#define JUMP_TRACE 0
#define WALLJUMP_VX (JUMP_V / C_SQRT2)
#define WALLJUMP_VY (JUMP_V / C_SQRT2)
#define GROUND_A 800
#define AIR_MOVE 0.2

/* Cannon parameters */
#define CANNON_DELAY 200

/* Melee weapon parameters */
#define SWORD_EXTEND 0.015
#define SWORD_REACH 15
#define SWORD_FORCE_PER_V 3
#define SWORD_BOOST (50 * SWORD_FORCE_PER_V)
#define SWORD_BOOST_TIME 150
#define SWORD_BOOST_FLASH 1000
#define SWORD_FORCE_PLAYER 0.5
#define SWORD_IMPACT_SLOW 500
#define SWORD_CLIPPING 0

/* Minimal distance the pointer keeps from the player */
#define CURSOR_DIST 18

/* Impact shake effect params */
#define SHAKE_SCALE 1
#define SHAKE_MIN 350

/* Glow effect params */
#define VEL_GLOW_MIN 500.f
#define VEL_GLOW_MAX 1000.f

/* Impact effect params */
#define IMPACT_DELAY 50

/* Player data */
PEntity g_player;
CVec g_aim, g_muzzle;

static GEntityClass playerClass;
static RSprite playerHead, playerBody, playerWeapon, playerGlow;
static float meleeProgress, clip_left, clip_right, clip_top, clip_bottom;
static int meleeTime, meleeDelay, fireDelay, jumpDelay;
static bool meleeHeld;

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
        PImpactEvent *impactEvent;
        float vel;
        bool mirror;

        switch (event) {
        case PE_FREE:
                return 1;
        case PE_UPDATE:

                /* Weapon muzzle */
                g_muzzle = CVec_add(g_player.origin,
                                    CVec_divf(g_player.size, 2));
                g_muzzle.y += G_MUZZLE_OFFSET;

                /* Position body parts */
                mirror = g_aim.x > g_player.origin.x + g_player.size.x / 2.f;
                RSprite_center(&playerBody, g_player.origin, g_player.size);
                RSprite_center(&playerWeapon, g_muzzle, CVec(0, 0));
                RSprite_center(&playerHead, g_player.origin, g_player.size);

                /* Position the head and weapon */
                RSprite_lookAt(&playerHead, g_aim);
                RSprite_lookAt(&playerWeapon, g_aim);

                /* Mirror the sprite */
                if (!(playerBody.mirror = playerHead.mirror =
                                          playerWeapon.mirror = mirror)) {
                        playerHead.angle += C_PI;
                        playerWeapon.angle += C_PI;
                }
                RSprite_center(&playerBody, g_player.origin, g_player.size);
                RSprite_center(&playerWeapon, g_muzzle, CVec(0, 0));
                RSprite_center(&playerHead, g_player.origin, g_player.size);

                /* Draw player */
                RSprite_draw(&playerBody);
                RSprite_draw(&playerHead);

                /* Draw weapon with clipping */
                if (SWORD_CLIPPING && meleeProgress >= 0) {
                        R_pushClip();
                        R_clip_left(clip_left);
                        R_clip_top(clip_top);
                        R_clip_right(clip_right);
                        R_clip_bottom(clip_bottom);
                        playerWeapon.z = G_Z_CHAR;
                        RSprite_draw_still(&playerWeapon);
                        R_clip_disable();
                        R_popClip();
                        playerWeapon.z = G_Z_REAR;
                } else
                        playerWeapon.z = G_Z_CHAR;
                RSprite_draw(&playerWeapon);

                /* Player glow effect */
                vel = CVec_len(g_player.velocity);
                if (vel < VEL_GLOW_MIN)
                        break;
                RSprite_center(&playerGlow, playerBody.origin, playerBody.size);
                playerGlow.modulate.a = (vel - VEL_GLOW_MIN) /
                                        (VEL_GLOW_MAX - VEL_GLOW_MIN);
                if (playerGlow.modulate.a > 1)
                        playerGlow.modulate.a = 1;
                playerGlow.angle = CVec_angle(g_player.velocity);
                RSprite_draw(&playerGlow);
                break;

        /* Shake camera on impact */
        case PE_IMPACT:
                impactEvent = args;
                if (impactEvent->impulse >= SHAKE_MIN) {
                        float shake;

                        shake = SHAKE_SCALE *
                                logf(impactEvent->impulse - SHAKE_MIN);
                        if (r_cameraShake < shake)
                                r_cameraShake = shake;
                        g_impactProgress = 0;
                        g_impactVelocity = CVec_len(g_player.velocity);
                }
                break;

        default:
                break;
        }
        return 0;
}

/******************************************************************************\
 Disable melee weapon clipping.
\******************************************************************************/
static void disableClip(void) {
        clip_left = -R_CLIP_LIMIT;
        clip_top = -R_CLIP_LIMIT;
        clip_right = R_CLIP_LIMIT;
        clip_bottom = R_CLIP_LIMIT;
}

/******************************************************************************\
 Slash with the melee weapon.
\******************************************************************************/
static void startMelee(void)
{
        if (fireDelay)
                return;
        RSprite_play(&playerWeapon, "sword");
        meleeProgress = 0;
        meleeTime = p_timeMsec;
}

/******************************************************************************\
 Put away the melee weapon.
\******************************************************************************/
static void stopMelee(void)
{
        meleeProgress = -1;
        meleeTime = 0;
        RSprite_play(&playerWeapon, "repelCannon");
        disableClip();
}

/******************************************************************************\
 Reset control holds.
\******************************************************************************/
void G_resetHolds(void)
{
        meleeHeld = FALSE;
        stopMelee();
}

/******************************************************************************\
 Checks if the player can jump and handles the acceleration.
\******************************************************************************/
static void checkJump(void)
{
        PTrace trace;
        PEntity *other;
        CVec to;

        /* Can't jump yet */
        if ((jumpDelay -= p_frameMsec) < 0)
                jumpDelay = 0;
        else
                return;

        /* Don't want to jump */
        if (g_control.y >= 0)
                return;
        other = NULL;

        /* Jump off of ground */
        other = g_player.ground;
        if (!other && JUMP_TRACE) {
                to = CVec_add(g_player.origin, CVec(0, JUMP_TRACE));
                trace = PEntity_trace(&g_player, to);
                other = trace.other;
        }
        if (other) {
                if (PEntity_event(other, GE_JUMPED_AWAY, &g_player))
                        return;
                if (g_player.velocity.y > 0)
                        g_player.velocity.y = 0;
                g_player.velocity.y = -JUMP_V;
        }

        /* Walljump off of right wall */
        if (!other) {
                other = g_player.rightWall;
                if (!other && JUMP_TRACE) {
                        to = CVec_add(g_player.origin, CVec(JUMP_TRACE, 0));
                        trace = PEntity_trace(&g_player, to);
                        other = trace.other;
                }
                if (other) {
                        if (PEntity_event(other, GE_JUMPED_AWAY, &g_player))
                                return;
                        if (g_player.velocity.x > 0)
                                g_player.velocity.x = 0;
                        g_player.velocity.x -= WALLJUMP_VX;
                        g_player.velocity.y -= WALLJUMP_VY;
                }
        }

        /* Walljump off of left wall */
        if (!other) {
                other = g_player.leftWall;
                if (!other && JUMP_TRACE) {
                        to = CVec_add(g_player.origin, CVec(-JUMP_TRACE, 0));
                        trace = PEntity_trace(&g_player, to);
                        other = trace.other;
                }
                if (other) {
                        if (PEntity_event(other, GE_JUMPED_AWAY, &g_player))
                                return;
                        if (g_player.velocity.x < 0)
                                g_player.velocity.x = 0;
                        g_player.velocity.x += WALLJUMP_VX;
                        g_player.velocity.y -= WALLJUMP_VY;
                }
        }

        /* Didn't jump */
        if (!other)
                return;

        /* Jumped somehow */
        jumpDelay = JUMP_DELAY;
}

/******************************************************************************\
 Check weapon refire delay and melee impacts.
\******************************************************************************/
static void checkWeapon(void)
{
        PTrace trace;
        CVec dir, to;
        float vel, force;

        /* Start a slash if it was interrupted for some reason */
        if (meleeHeld && meleeProgress < 0)
                startMelee();

        /* Wait times */
        if ((fireDelay -= p_frameMsec) < 0)
                fireDelay = 0;
        if ((meleeDelay -= p_frameMsec) < 0)
                meleeDelay = 0;

        /* Check melee weapon */
        if (meleeProgress < 0)
                return;
        if ((meleeProgress += p_frameMsec * SWORD_EXTEND) > 1)
                meleeProgress = 1;
        dir = CVec_norm(CVec_sub(g_aim, g_muzzle));
        to = CVec_add(g_muzzle, CVec_scalef(dir, meleeProgress * SWORD_REACH));
        g_player.ignore = TRUE;
        trace = PTrace_line(g_muzzle, to, PIT_ENTITY);
        g_player.ignore = FALSE;
        disableClip();
        if (trace.prop >= 1)
                return;

        /* Melee weapon clipping */
        if (SWORD_CLIPPING) {
                if (trace.end.x == trace.other->origin.x - 1)
                        clip_right = trace.end.x + 1;
                if (trace.end.x == trace.other->origin.x + trace.other->size.x)
                        clip_left = trace.end.x;
                if (trace.end.y == trace.other->origin.y - 1)
                        clip_bottom = trace.end.y + 1;
                if (trace.end.y == trace.other->origin.y + trace.other->size.y)
                        clip_top = trace.end.y;
        }

        /* Weapon impact */
        force = (vel = CVec_len(g_player.velocity)) * SWORD_FORCE_PER_V;
        if (p_timeMsec - meleeTime < SWORD_BOOST_TIME) {
                force += SWORD_BOOST;
                if (force > G_VEL_MIN)
                        r_screenFlash = CColor(1, 1, 1, (force - G_VEL_MIN) /
                                                        SWORD_BOOST_FLASH);
        }
        if (force < G_VEL_NONE)
                return;
        C_debug("Impact force %d", (int)force);
        PEntity_impact_impulse(&g_player, trace.other, dir,
                               force * SWORD_FORCE_PLAYER, force);
        PEntity_slowImpulse(&g_player, SWORD_IMPACT_SLOW * p_frameSec);
        if (meleeDelay > 0)
                return;
        meleeDelay += IMPACT_DELAY;
        if (force < G_VEL_MIN * SWORD_FORCE_PER_V)
                G_spawn_at("swordImpact_min", trace.end);
        else if (force <= G_VEL_WEAK * SWORD_FORCE_PER_V)
                G_spawn_at("swordImpact_weak", trace.end);
        else if (force <= G_VEL_STRONG * SWORD_FORCE_PER_V)
                G_spawn_at("swordImpact_strong", trace.end);
        else
                G_spawn_at("swordImpact_max", trace.end);
}

/******************************************************************************\
 Player must be updated outside of normal entities once per frame because the
 g_player entity's motion affects the camera.
\******************************************************************************/
void G_updatePlayer(void)
{
        float accelX;

        /* Do nothing if paused or dead */
        if (!CLink_linked(&g_player.linkAll) || g_player.dead || p_speed <= 0)
                return;

        checkJump();
        checkWeapon();

        /* Horizontal movement */
        accelX = 0;
        if (g_control.x > 0)
                accelX += GROUND_A;
        else if (g_control.x < 0)
                accelX -= GROUND_A;

        /* On the ground */
        if (g_player.ground) {
                if (g_player.ground->friction >= 0 &&
                    g_player.ground->friction < 1)
                        accelX *= g_player.ground->friction;
                g_player.accel.x += accelX;
                RSprite_play(&playerBody,
                             g_control.x ? "playerBodyRun" : "playerBody");
        }

        /* In the air */
        else {
                g_player.accel.x += accelX * AIR_MOVE;
                RSprite_play(&playerBody, g_player.velocity.y < 0 ?
                                          "playerBodyUp" : "playerBodyDown");
        }

        /* Make sure g_player sprite depth is correct */
        playerHead.z = playerBody.z = playerWeapon.z =
                       playerGlow.z = playerClass.z;

        /* Update camera */
        r_cameraTo = CVec_add(g_player.origin, CVec_divf(g_player.size, 2));
        r_cameraTo.x -= r_widthScaled / 2;
        r_cameraTo.y -= r_heightScaled / 2;

        /* Make sure camera is not looking outside the map area */
        r_cameraTo = CVec_max(p_topLeft, r_cameraTo);
        r_cameraTo = CVec_min(CVec_sub(p_bottomRight,
                                       CVec(r_widthScaled, r_heightScaled)),
                              r_cameraTo);

        /* Update aim vector */
        g_aim = CVec_add(r_cameraTo, constrainedMouse());

        /* Kill the g_player if he falls off the map */
        if (p_frame > 0 &&
            (g_player.origin.x <= p_topLeft.x ||
             g_player.origin.y <= p_topLeft.y ||
             g_player.origin.x + g_player.size.x >= p_bottomRight.x ||
             g_player.origin.y + g_player.size.y >= p_bottomRight.y)) {
                C_warning("Player fell off the map");
                PEntity_kill(&g_player);
        }

        /* If the g_player is dead, start a new game */
        if (g_player.dead) {
                C_debug("Player died");
                G_newGame();
        }
}

/******************************************************************************\
 Handles control events for the g_player entity.
\******************************************************************************/
bool G_dispatch_player(GEvent event)
{
        if (!CLink_linked(&g_player.linkAll))
                return FALSE;

        /* Swing sword */
        if (g_button == SDL_BUTTON_LEFT) {
                if (event == GE_MOUSE_DOWN) {
                        startMelee();
                        meleeHeld = TRUE;
                }
                if (event == GE_MOUSE_UP) {
                        stopMelee();
                        meleeHeld = FALSE;
                }
        }

        /* Fire cannon */
        else if (event == GE_MOUSE_DOWN && g_button == SDL_BUTTON_RIGHT &&
                 fireDelay <= 0) {
                stopMelee();
                G_fireMissile(&g_player, "repelMissile", g_muzzle, g_aim);
                fireDelay += CANNON_DELAY;
        }

        return TRUE;
}

/******************************************************************************\
 Spawn the g_player within the world.
\******************************************************************************/
void G_spawnPlayer(CVec origin)
{
        /* Init player class */
        C_zero(&playerClass);
        C_strncpy_buf(playerClass.named.name, "Player");
        playerClass.z = G_Z_CHAR;

        /* Init player sprites */
        RSprite_init_time(&playerHead, "playerHead", &p_timeMsec);
        RSprite_init_time(&playerBody, "playerBody", &p_timeMsec);
        RSprite_init_time(&playerGlow, "playerGlow", &p_timeMsec);
        RSprite_init_time(&playerWeapon, "repelCannon", &p_timeMsec);

        /* Initial weapon state */
        meleeProgress = -1;
        disableClip();

        /* Spawn player */
        C_zero(&g_player);
        g_player.entityClass = &playerClass;
        g_player.origin = origin;
        g_player.size = CVec(10, 16);
        g_player.mass = 1;
        g_player.friction = 5;
        g_player.elasticity = 0.25;
        g_player.eventFunc = (PEventFunc)playerEvent;
        g_player.impactOther = PIT_ENTITY;
        g_player.manualUpdate = FALSE;
        g_player.stepSize = 8;
        PEntity_spawn(&g_player, "Player");
        PEntity_impactType(&g_player, PIT_ENTITY);
        G_depthSortEntity(&g_player);
}

