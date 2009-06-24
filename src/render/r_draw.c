/******************************************************************************\
 Dragoon - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "r_private.h"

/* Size of the clipping stack */
#define CLIP_STACK 32

/* Clipping stack */
static float clipValues[CLIP_STACK * 4];
static int clipStack = -1;

/******************************************************************************\
 Draw a colored rectangle on screen.
\******************************************************************************/
void R_drawRect(CVec origin, float z, CVec size, CColor add, CColor mod)
{
        RVertex verts[4];

        if (z < 0.f || !CVec_intersect(origin, size, CVec(0, 0),
                                       CVec(r_widthScaled, r_heightScaled)))
                return;

        /* Unbind texture */
        RTexture_deselect();

        /* Setup quad */
        verts[0].co = origin;
        verts[2].co = CVec_add(origin, size);
        verts[1].co = CVec(verts[0].co.x, verts[2].co.y);
        verts[3].co = CVec(verts[2].co.x, verts[0].co.y);
        verts[3].z = verts[2].z = verts[1].z = verts[0].z = z;

        /* Additive blending */
        if (add.a >= 0.f) {
                glColor4f(add.r * add.a, add.g * add.a, add.b * add.a, 1.f);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                glDisable(GL_ALPHA_TEST);
                glInterleavedArrays(RV_FORMAT, 0, verts);
                glDrawArrays(GL_QUADS, 0, 4);
                CCount_add(&r_countFaces, 2);
        }

        /* Alpha blending */
        if (mod.a >= 0.f) {
                glColor4f(mod.r, mod.g, mod.b, mod.a);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glEnable(GL_ALPHA_TEST);
                glInterleavedArrays(RV_FORMAT, 0, verts);
                glDrawArrays(GL_QUADS, 0, 4);
                CCount_add(&r_countFaces, 2);
        }

        R_checkErrors();
}

/******************************************************************************\
 Update shake effect.
\******************************************************************************/
void R_updateShake(CVec *offset, CVec *vel, float accel, float drag, float sec,
                   float limit)
{
        float scale;

        *vel = CVec_add(*vel, CVec_scalef(*offset, -accel * sec));
        if ((drag = 1 - drag * sec) < 0)
                drag = 0;
        *vel = CVec_scalef(*vel, drag);
        *offset = CVec_add(*offset, CVec_scalef(*vel, sec));
        if (limit >= 0 && (scale = CVec_len(*offset)) > limit)
                *offset = CVec_scalef(CVec_divf(*offset, scale), limit);
}

/******************************************************************************\
 Sets the OpenGL clipping planes to the strictest clipping in each direction.
 This only works in 2D mode. OpenGL takes plane equations as arguments. Points
 that are visible satisfy the following inequality:
 a * x + b * y + c * z + d >= 0
\******************************************************************************/
static void setClipping(void)
{
        GLdouble eqn[4];
        float left, top, right, bottom;
        int i;

        if (clipStack < 0)
                return;

        /* Find the most restrictive clipping values in each direction */
        left = clipValues[0];
        top = clipValues[1];
        right = clipValues[2];
        bottom = clipValues[3];
        for (i = 1; i <= clipStack; i++) {
                if (clipValues[4 * i] > left)
                        left = clipValues[4 * i];
                if (clipValues[4 * i + 1] > top)
                        top = clipValues[4 * i + 1];
                if (clipValues[4 * i + 2] < right)
                        right = clipValues[4 * i + 2];
                if (clipValues[4 * i + 3] < bottom)
                        bottom = clipValues[4 * i + 3];
        }

        /* Clip left */
        eqn[2] = 0.f;
        if (left > -R_CLIP_LIMIT) {
                eqn[0] = 1.f;
                eqn[1] = 0.f;
                eqn[3] = -left;
                glEnable(GL_CLIP_PLANE0);
                glClipPlane(GL_CLIP_PLANE0, eqn);
        } else
                glDisable(GL_CLIP_PLANE0);

        /* Clip top */
        if (top > -R_CLIP_LIMIT) {
                eqn[0] = 0.f;
                eqn[1] = 1.f;
                eqn[3] = -top;
                glEnable(GL_CLIP_PLANE1);
                glClipPlane(GL_CLIP_PLANE1, eqn);
        } else
                glDisable(GL_CLIP_PLANE1);

        /* Clip right */
        eqn[3] = 1.f;
        if (right < R_CLIP_LIMIT) {
                eqn[0] = -1.f;
                eqn[1] = 0.f;
                eqn[3] = right;
                glEnable(GL_CLIP_PLANE2);
                glClipPlane(GL_CLIP_PLANE2, eqn);
        } else
                glDisable(GL_CLIP_PLANE2);

        /* Clip bottom */
        if (bottom < R_CLIP_LIMIT) {
                eqn[0] = 0.f;
                eqn[1] = -1.f;
                eqn[3] = bottom;
                glEnable(GL_CLIP_PLANE3);
                glClipPlane(GL_CLIP_PLANE3, eqn);
        } else
                glDisable(GL_CLIP_PLANE3);
}

/******************************************************************************\
 Disables the clipping at the current stack level.
\******************************************************************************/
void R_clip_disable(void)
{
        clipValues[4 * clipStack] = -R_CLIP_LIMIT;
        clipValues[4 * clipStack + 1] = -R_CLIP_LIMIT;
        clipValues[4 * clipStack + 2] = R_CLIP_LIMIT;
        clipValues[4 * clipStack + 3] = R_CLIP_LIMIT;
        setClipping();
}

/******************************************************************************\
 Adjust the clip stack.
\******************************************************************************/
void R_pushClip(void)
{
        if (++clipStack >= CLIP_STACK)
                C_error("Clip stack overflow");
        R_clip_disable();
}

void R_popClip(void)
{
        if (--clipStack < -1)
                C_error("Clip stack underflow");
        setClipping();
}

/******************************************************************************\
 Clip in a specific direction.
\******************************************************************************/
void R_clip_left(float dist)
{
        clipValues[4 * clipStack] = dist;
        setClipping();
}

void R_clip_top(float dist)
{
        clipValues[4 * clipStack + 1] = dist;
        setClipping();
}

void R_clip_right(float dist)
{
        clipValues[4 * clipStack + 2] = dist;
        setClipping();
}

void R_clip_bottom(float dist)
{
        clipValues[4 * clipStack + 3] = dist;
        setClipping();
}

/******************************************************************************\
 Setup a clipping rectangle.
\******************************************************************************/
void R_clip(CVec origin, CVec size)
{
        clipValues[4 * clipStack] = origin.x;
        clipValues[4 * clipStack + 1] = origin.y;
        clipValues[4 * clipStack + 2] = origin.x + size.x;
        clipValues[4 * clipStack + 3] = origin.y + size.y;
        setClipping();
}

