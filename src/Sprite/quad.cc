/******************************************************************************\
 Dragoon - Copyright (C) 2010 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "../log.h"
#include "../Mode.h"
#include "../Sprite.h"

namespace dragoon {

void Sprite::DrawQuad(bool smooth) {

  // Select texture
  Texture* tex = data_->texture_;
  if (data_->tile_) {
    tex = data_->tiled_;
    ASSERT(tex);

    // Non-power-of-two tiles need to be upscaled
    smooth |= data_->tile_ && tex->pow2_size() != tex->size();
  }
  if (tex)
    tex->Select(smooth);

  // Setup textured quad vertex positions
  Vertex verts[4];
  verts[0].co = Vec<2>(-0.5f, -0.5f);
  verts[0].z = 0.f;
  verts[2].co = Vec<2>(0.5f, 0.5f);
  verts[2].z = 0.f;
  verts[1].co = Vec<2>(-0.5f, 0.5f);
  verts[1].z = 0.f;
  verts[3].co = Vec<2>(0.5f, -0.5f);
  verts[3].z = 0.f;

  // Setup vertex UV coordinates
  Vec<2> origin;
  Vec<2> surface_sz(0, 0);
  if (tex)
    surface_sz = tex->size();
  if (data_->tile_ == Data::TILE_GLOBAL) {
    origin = origin_ + data_->tile_origin_;
    verts[0].uv = origin / surface_sz;
    verts[2].uv = (origin + size_) / surface_sz;
  } else if (data_->tile_ == Data::TILE_PARALLAX) {
    origin = origin + data_->tile_origin_; //- camera$ * data_->parallax_;
    verts[0].uv = origin / surface_sz;
    verts[2].uv = (origin + size_) / surface_sz;
  } else if (data_->tile_) {
    verts[0].uv = Vec<2>(0, 0);
    verts[2].uv = size_ / surface_sz;
  } else {
    verts[0].uv = data_->box_origin_ / surface_sz;
    verts[2].uv = (data_->box_origin_ + data_->box_size_) / surface_sz;
  }

  // Scale UV for tiled sprites
  if (data_->tile_) {
    verts[0].uv /= data_->scale_;
    verts[2].uv /= data_->scale_;
  }

  verts[1].uv[0] = verts[0].uv[0];
  verts[1].uv[1] = verts[2].uv[1];
  verts[3].uv[0] = verts[2].uv[0];
  verts[3].uv[1] = verts[0].uv[1];

  // Render textured quad
  if (CHECKED)
    Mode::faces$ += 2;
  glInterleavedArrays(Vertex::FORMAT, 0, verts);
  const unsigned short indices[] = { 0, 1, 2, 3, 0 };
  glDrawElements(GL_QUADS, 4, GL_UNSIGNED_SHORT, indices);

  Mode::Check();
}

} // namespace dragoon
