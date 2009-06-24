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

/* Impact shake effect params */
#define SHAKE_SCALE 1
#define SHAKE_MIN 350

/* Impact render effect params */
#define IMPACT_RATE 7
#define IMPACT_RATE_V -0.01
#define IMPACT_DELAY 50

/* Cannon parameters */
#define CANNON_DELAY 300

/* Melee weapon parameters */
#define SWORD_EXTEND 0.015
#define SWORD_REACH 15
#define SWORD_FORCE_PER_V 3
#define SWORD_BOOST (50 * SWORD_FORCE_PER_V)
#define SWORD_FORCE_PLAYER 0.5
#define SWORD_IMPACT_SLOW 500
#define SWORD_CLIPPING 0

/* Minimal distance the pointer keeps from the player */
#define CURSOR_DIST 18

/* Velocity display params */
#define VEL_SCALE 0.001f
#define VEL_JIGGLE_SCALE 0.000005f
#define VEL_JIGGLE_SPEED 0.1f
#define VEL_NONE 50.f
#define VEL_MIN 200.f
#define VEL_WEAK 350.f
#define VEL_STRONG 500.f
#define VEL_GLOW_MIN 500.f
#define VEL_GLOW_MAX 1000.f

static GEntityClass playerClass;
static PEntity player;
static RSprite playerHead, playerBody, playerWeapon, playerGlow, cursor;
static RText hudVelocity;
static CVec aim, muzzle;
static float meleeProgress, impactProgress, impactVelocity,
             clip_left, clip_right, clip_top, clip_bottom;
static int meleeDelay, fireDelay, jumpDelay;
static bool meleeHeld, jumpHeld;

/******************************************************************************\
 Draw velocity meter.
\******************************************************************************/
static void drawVelocity(void)
{
        CColor color_a, color_b;
        float lerp, scale, rate;
        int vel, vel2;

        /* Impact velocity effect */
        if (impactProgress >= 0) {
                if ((rate = IMPACT_RATE + IMPACT_RATE_V * impactVelocity) < 1)
                        rate = 1;
                if ((impactProgress += rate * p_frameSec) > 1)
                        impactProgress = -1;
        }
        if (impactProgress >= 0) {
                scale = 1 + impactProgress;
                vel = (int)impactVelocity;
        } else {
                scale = 1;
                vel = (int)CVec_len(player.velocity);
        }
        vel2 = vel * vel;
        C_snprintf_buf(hudVelocity.string, "%d", vel);

        /* Color */
        if (vel <= VEL_MIN) {
                color_a = CColor(1, 1, 1, 0);
                color_b = CColor(1, 1, 1, 1);
                lerp = vel / VEL_MIN;
        } else if (vel <= VEL_WEAK) {
                color_a = CColor(1, 1, 1, 1);
                color_b = CColor(1, 1, 0, 1);
                lerp = (vel - VEL_MIN) / (VEL_WEAK - VEL_MIN);
        } else if (vel <= VEL_STRONG) {
                color_a = CColor(1, 1, 0, 1);
                color_b = CColor(1, 0, 0, 1);
                lerp = (vel - VEL_WEAK) / (VEL_STRONG - VEL_WEAK);
        } else {
                color_a = CColor(1, 0, 0, 1);
                color_b = CColor(1, 1, 1, 1);
                lerp = C_rand();
        }
        hudVelocity.modulate = CColor_lerp(color_a, lerp, color_b);

        /* Fade out impact velocity */
        if (impactProgress >= 0) {
                hudVelocity.modulate.a = 1 - impactProgress;
                hudVelocity.jiggleRadius = 0;
                hudVelocity.jiggleSpeed = 0;
        }

        /* Jiggle with speed */
        else {
                hudVelocity.jiggleRadius = vel * vel * VEL_JIGGLE_SCALE;
                hudVelocity.jiggleSpeed = VEL_JIGGLE_SPEED;
        }

        /* Scale with speed */
        scale *= 1 + vel * VEL_SCALE;
        hudVelocity.scale = CVec(scale, scale);

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
        PImpactEvent *impactEvent;
        float vel;
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
                        playerHead.angle += C_PI;
                        playerWeapon.angle += C_PI;
                }
                RSprite_center(&playerBody, player.origin,
                               player.size);
                RSprite_center(&playerWeapon, muzzle, CVec(0, 0));
                RSprite_center(&playerHead, player.origin,
                               player.size);

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
                vel = CVec_len(player.velocity);
                if (vel < VEL_GLOW_MIN)
                        break;
                RSprite_center(&playerGlow, playerBody.origin, playerBody.size);
                playerGlow.modulate.a = (vel - VEL_GLOW_MIN) /
                                        (VEL_GLOW_MAX - VEL_GLOW_MIN);
                if (playerGlow.modulate.a > 1)
                        playerGlow.modulate.a = 1;
                playerGlow.angle = CVec_angle(player.velocity);
                RSprite_draw(&playerGlow);
                break;

        /* Shake camera on impact */
        case PE_IMPACT:
                impactEvent = args;
                if (impactEvent->impulse >= SHAKE_MIN) {
                        CVec shake;
                        float scale;

                        scale = SHAKE_SCALE *
                                logf(impactEvent->impulse - SHAKE_MIN);
                        shake = CVec_scalef(impactEvent->dir, scale);
                        r_cameraShake = CVec_add(r_cameraShake, shake);
                        impactProgress = 0;
                        impactVelocity = CVec_len(player.velocity);
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
}

/******************************************************************************\
 Put away the melee weapon.
\******************************************************************************/
static void stopMelee(void)
{
        meleeProgress = -1;
        RSprite_play(&playerWeapon, "repelCannon");
        disableClip();
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
        if (!other && JUMP_TRACE) {
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
                if (!other && JUMP_TRACE) {
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
                if (!other && JUMP_TRACE) {
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
        dir = CVec_norm(CVec_sub(aim, muzzle));
        to = CVec_add(muzzle, CVec_scalef(dir, meleeProgress * SWORD_REACH));
        player.ignore = TRUE;
        trace = PTrace_line(muzzle, to, PIT_ENTITY);
        player.ignore = FALSE;
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
        vel = CVec_len(player.velocity);
        force = vel * SWORD_FORCE_PER_V;
        if (meleeProgress < 1)
                force += SWORD_BOOST;
        if (force < VEL_NONE)
                return;
        PEntity_impact_impulse(&player, trace.other, dir,
                               force * SWORD_FORCE_PLAYER, force);
        PEntity_slowImpulse(&player, SWORD_IMPACT_SLOW * p_frameSec);
        if (meleeDelay > 0)
                return;
        meleeDelay += IMPACT_DELAY;
        if (force < VEL_MIN * SWORD_FORCE_PER_V)
                G_spawn_at("swordImpact_min", trace.end);
        else if (force <= VEL_WEAK * SWORD_FORCE_PER_V)
                G_spawn_at("swordImpact_weak", trace.end);
        else if (force <= VEL_STRONG * SWORD_FORCE_PER_V)
                G_spawn_at("swordImpact_strong", trace.end);
        else
                G_spawn_at("swordImpact_max", trace.end);
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
        checkWeapon();

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
        playerHead.z = playerBody.z = playerWeapon.z =
                       playerGlow.z = playerClass.z;

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
                G_fireMissile(&player, "repelMissile", muzzle, aim);
                fireDelay += CANNON_DELAY;
        }

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
        RSprite_init_time(&playerHead, "playerHead", &p_timeMsec);
        RSprite_init_time(&playerBody, "playerBody", &p_timeMsec);
        RSprite_init_time(&playerGlow, "playerGlow", &p_timeMsec);
        RSprite_init_time(&playerWeapon, "repelCannon", &p_timeMsec);

        /* Init HUD sprites */
        RSprite_init_time(&cursor, "cursor", &p_timeMsec);
        cursor.z = G_Z_HUD;
        RText_init_range(&hudVelocity, "gfx/digits.png", '0', 4, 3, "0");
        hudVelocity.z = G_Z_HUD;
        hudVelocity.timeMsec = &p_timeMsec;

        /* Initial weapon state */
        meleeProgress = -1;
        impactProgress = -1;
        disableClip();

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
        PEntity_impactType(&player, PIT_ENTITY);
        G_depthSortEntity(&player);
}

