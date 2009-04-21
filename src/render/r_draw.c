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

/******************************************************************************\
 Draw a colored rectangle on screen.
\******************************************************************************/
void R_drawRect(CVec origin, float z, CVec size, CColor add, CColor mod)
{
        RVertex verts[4];

        if (z > 0.f || !CVec_intersect(origin, size, CVec(0, 0),
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

