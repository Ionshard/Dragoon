/******************************************************************************\
 Dragoon - Copyright (C) 2010 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "Vec.h"
#include "Mode.h"
#include "Sprite.h"

namespace dragoon {
namespace draw {

void rect(Vec<2> origin, float z, Vec<2> size, Color mod, Color add) {
  if (z < 0.f || (mod.a() <= 0 && add.a() <= 0))
    return;

  // Setup quad
  Texture::Deselect();
  Sprite::Vertex verts[4];
  verts[0].co = origin;
  verts[2].co = origin + size;
  verts[1].co = Vec<2>(verts[0].co.x(), verts[2].co.y());
  verts[3].co = Vec<2>(verts[2].co.x(), verts[0].co.y());
  verts[3].z = verts[2].z = verts[1].z = verts[0].z = z;

  // No need for depth testing if closest possible
  if (z <= 0)
    glDisable(GL_DEPTH_TEST);

  // Additive blending
  if (add.a() >= 0.f) {
    add.SelectAdd();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDisable(GL_ALPHA_TEST);
    glInterleavedArrays(Sprite::Vertex::FORMAT, 0, verts);
    glDrawArrays(GL_QUADS, 0, 4);
    Mode::faces$ += 2;
  }

  // Alpha blending
  if (mod.a() >= 0.f) {
    mod.Select();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_ALPHA_TEST);
    glInterleavedArrays(Sprite::Vertex::FORMAT, 0, verts);
    glDrawArrays(GL_QUADS, 0, 4);
    Mode::faces$ += 2;
  }

  /* Remember to re-enable depth testing */
  if (z <= 0)
    glEnable(GL_DEPTH_TEST);

  Mode::Check();
}

} // namespace draw
} // namespace dragoon
