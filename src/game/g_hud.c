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

/* Velocity display params */
#define VEL_SCALE 0.001f
#define VEL_JIGGLE_SCALE 0.000005f
#define VEL_JIGGLE_SPEED 0.1f

/* Impact render effect params */
#define IMPACT_RATE 7
#define IMPACT_RATE_V -0.01

/* Impact effect variables */
float g_impactProgress, g_impactVelocity;

static RSprite cursor;
static RText hudVelocity;

/******************************************************************************\
 Draw velocity meter.
\******************************************************************************/
static void drawVelocity(void)
{
        CColor color_a, color_b;
        float lerp, scale, rate;
        int vel, vel2;

        /* Impact velocity effect */
        if (g_impactProgress >= 0) {
                if ((rate = IMPACT_RATE + IMPACT_RATE_V * g_impactVelocity) < 1)
                        rate = 1;
                if ((g_impactProgress += rate * p_frameSec) > 1)
                        g_impactProgress = -1;
        }
        if (g_impactProgress >= 0) {
                scale = 1 + g_impactProgress;
                vel = (int)g_impactVelocity;
        } else {
                scale = 1;
                vel = (int)CVec_len(g_player.velocity);
        }
        vel2 = vel * vel;
        C_snprintf_buf(hudVelocity.string, "%d", vel);

        /* Color */
        if (vel <= G_VEL_MIN) {
                color_a = CColor(1, 1, 1, 0);
                color_b = CColor(1, 1, 1, 1);
                lerp = vel / G_VEL_MIN;
        } else if (vel <= G_VEL_WEAK) {
                color_a = CColor(1, 1, 1, 1);
                color_b = CColor(1, 1, 0, 1);
                lerp = (vel - G_VEL_MIN) / (G_VEL_WEAK - G_VEL_MIN);
        } else if (vel <= G_VEL_STRONG) {
                color_a = CColor(1, 1, 0, 1);
                color_b = CColor(1, 0, 0, 1);
                lerp = (vel - G_VEL_WEAK) / (G_VEL_STRONG - G_VEL_WEAK);
        } else {
                color_a = CColor(1, 0, 0, 1);
                color_b = CColor(1, 1, 1, 1);
                lerp = p_speed * C_rand();
        }
        hudVelocity.modulate = CColor_lerp(color_a, lerp, color_b);

        /* Fade out impact velocity */
        if (g_impactProgress >= 0) {
                hudVelocity.modulate.a = 1 - g_impactProgress;
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
 Initialize the HUD.
\******************************************************************************/
void G_initHud(void)
{
        RSprite_init_time(&cursor, "cursor", &p_timeMsec);
        cursor.z = G_Z_HUD;
        RText_init_range(&hudVelocity, "gfx/digits.png", '0', 4, 3, "0");
        hudVelocity.z = G_Z_HUD;
        hudVelocity.timeMsec = &p_timeMsec;
}

/******************************************************************************\
 Draw the HUD.
\******************************************************************************/
void G_drawHud(void)
{
        drawVelocity();

        /* Cursor */
        RSprite_lookAt(&cursor, CVec_sub(g_muzzle, r_camera));
        RSprite_center(&cursor, CVec_sub(g_aim, r_camera), CVec(0, 0));
        RSprite_draw(&cursor);
}

