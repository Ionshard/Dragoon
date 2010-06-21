/******************************************************************************\
 Dragoon - Copyright (C) 2010 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "../Mode.h"
#include "../Sprite.h"

namespace dragoon {

void Sprite::DrawWindow(bool smooth) {

  // If the window dimensions are too small to fit the corners in,
  // we need to trim the corner size a little
  Vec<2> corner = data_->corner_;
  if (size_.x() <= corner.x() * 2)
    corner[0] = size_.x() / 2;
  if (size_.y() <= corner.y() * 2)
    corner[1] = size_.y() / 2;
  Vec<2> surface_sz = data_->texture_->size();
  Vec<2> uv_sz = data_->box_size_ / surface_sz;
  Vec<2> corner_uv = corner / surface_sz;
  corner /= size_;

  // The vertex array is indexed this way:
  //
  //   0---2------4---6
  //   |   |      |   |
  //   1---3------5---7
  //   |   |      |   |
  //   |   |      |   |
  //   11--10-----9---8
  //   |   |      |   |
  //   12--13-----14--15
  //
  Vertex verts[16];

  // Vertex coordinates
  verts[0].co = Vec<2>(-0.5, -0.5);
  verts[1].co = Vec<2>(-0.5, -0.5 + corner.y());
  verts[2].co = Vec<2>(-0.5 + corner.x(), -0.5);
  verts[3].co = Vec<2>(-0.5 + corner.x(), -0.5 + corner.y());
  verts[4].co = Vec<2>(0.5 - corner.x(), -0.5);
  verts[5].co = Vec<2>(0.5 - corner.x(), -0.5 + corner.y());
  verts[6].co = Vec<2>(0.5, -0.5);
  verts[7].co = Vec<2>(0.5, -0.5 + corner.y());
  verts[8].co = Vec<2>(0.5, 0.5 - corner.y());
  verts[9].co = Vec<2>(0.5 - corner.x(), 0.5 - corner.y());
  verts[10].co = Vec<2>(-0.5 + corner.x(), 0.5 - corner.y());
  verts[11].co = Vec<2>(-0.5, 0.5 - corner.y());
  verts[12].co = Vec<2>(-0.5, 0.5);
  verts[13].co = Vec<2>(-0.5 + corner.x(), 0.5);
  verts[14].co = Vec<2>(0.5 - corner.x(), 0.5);
  verts[15].co = Vec<2>(0.5, 0.5);

  // Vertex UVs
  verts[0].uv = data_->box_origin_ / surface_sz;
  verts[1].uv = Vec<2>(verts[0].uv.x(), verts[0].uv.y() + corner_uv.y());
  verts[2].uv = Vec<2>(verts[0].uv.x()+ corner_uv.x(), verts[0].uv.y());
  verts[3].uv = Vec<2>(verts[2].uv.x(), verts[1].uv.y());
  verts[4].uv = Vec<2>(verts[0].uv.x()+ uv_sz.x()- corner_uv.x(),
                       verts[0].uv.y());
  verts[5].uv = Vec<2>(verts[4].uv.x(), verts[1].uv.y());
  verts[6].uv = Vec<2>(verts[0].uv.x()+ uv_sz.x(), verts[0].uv.y());
  verts[7].uv = Vec<2>(verts[6].uv.x(), verts[1].uv.y());
  verts[8].uv = Vec<2>(verts[6].uv.x(),
                       verts[0].uv.y() + uv_sz.y() - corner_uv.y());
  verts[9].uv = Vec<2>(verts[4].uv.x(), verts[8].uv.y());
  verts[10].uv = Vec<2>(verts[2].uv.x(), verts[8].uv.y());
  verts[11].uv = Vec<2>(verts[0].uv.x(), verts[8].uv.y());
  verts[12].uv = Vec<2>(verts[0].uv.x(), verts[0].uv.y() + uv_sz.y());
  verts[13].uv = Vec<2>(verts[2].uv.x(), verts[12].uv.y());
  verts[14].uv = Vec<2>(verts[4].uv.x(), verts[12].uv.y());
  verts[15].uv = Vec<2>(verts[6].uv.x(), verts[12].uv.y());

  // Untiled quads can be rendered in one call
  if (!data_->tile_) {
    unsigned short indices[] = {
      0, 1, 3, 2,
      2, 3, 5, 4,
      4, 5, 7, 6,
      1, 11, 10, 3,
      3, 10, 9, 5,
      5, 9, 8, 7,
      11, 12, 13, 10,
      10, 13, 14, 9,
      9, 14, 15, 8,
    };
    data_->texture_->Select(smooth);
    glInterleavedArrays(Vertex::FORMAT, 0, verts);
    glDrawElements(GL_QUADS, 36, GL_UNSIGNED_SHORT, indices);
  }

  // The tiled portions of the window must be rendered separately
  else {

    // Corner proportion of tiled texture
    Vec<2> corner_prop = size_ / 2 / data_->corner_;
    if (corner_prop.x()> 1)
      corner_prop[0] = 1;
    if (corner_prop.y() > 1)
      corner_prop[1] = 1;

    // First render the non-tiling portions
    unsigned short indices[] = {
      0, 1, 3, 2,
      4, 5, 7, 6,
      11, 12, 13, 10,
      9, 14, 15, 8,
    };
    data_->texture_->Select(smooth);
    glInterleavedArrays(Vertex::FORMAT, 0, verts);
    glDrawElements(GL_QUADS, 16, GL_UNSIGNED_SHORT, indices);

    // Top quad
    data_->edges_[0]->Select(smooth);
    Vec<2> corners = data_->corner_ * 2;
    uv_sz = (size_ - corners) / (data_->box_size_ - corners);
    Vertex t_verts[4];
    t_verts[0].co = verts[2].co;
    t_verts[1].co = verts[3].co;
    t_verts[2].co = verts[5].co;
    t_verts[3].co = verts[4].co;
    t_verts[0].uv = Vec<2>(0, 0);
    t_verts[1].uv = Vec<2>(0, corner_prop.y());
    t_verts[2].uv = Vec<2>(uv_sz.x(), corner_prop.y());
    t_verts[3].uv = Vec<2>(uv_sz.x(), 0);
    glInterleavedArrays(Vertex::FORMAT  , 0, t_verts);
    glDrawArrays(GL_QUADS, 0, 4);

    // Bottom quad
    data_->edges_[3]->Select(smooth);
    t_verts[0].co = verts[10].co;
    t_verts[1].co = verts[13].co;
    t_verts[2].co = verts[14].co;
    t_verts[3].co = verts[9].co;
    glInterleavedArrays(Vertex::FORMAT, 0, t_verts);
    glDrawArrays(GL_QUADS, 0, 4);

    // Left quad
    data_->edges_[1]->Select(smooth);
    t_verts[0].co = verts[1].co;
    t_verts[1].co = verts[11].co;
    t_verts[2].co = verts[10].co;
    t_verts[3].co = verts[3].co;
    t_verts[1].uv = Vec<2>(0, uv_sz.y());
    t_verts[2].uv = Vec<2>(corner_prop.x(), uv_sz.y());
    t_verts[3].uv = Vec<2>(corner_prop.x(), 0);
    glInterleavedArrays(Vertex::FORMAT, 0, t_verts);
    glDrawArrays(GL_QUADS, 0, 4);

    // Right quad
    data_->edges_[2]->Select(smooth);
    t_verts[0].co = verts[5].co;
    t_verts[1].co = verts[9].co;
    t_verts[2].co = verts[8].co;
    t_verts[3].co = verts[7].co;
    glInterleavedArrays(Vertex::FORMAT, 0, t_verts);
    glDrawArrays(GL_QUADS, 0, 4);

    // Middle quad
    data_->tiled_->Select(smooth);
    t_verts[0].co = verts[3].co;
    t_verts[1].co = verts[10].co;
    t_verts[2].co = verts[9].co;
    t_verts[3].co = verts[5].co;
    t_verts[1].uv = Vec<2>(0, uv_sz.y());
    t_verts[2].uv = Vec<2>(uv_sz.x(), uv_sz.y());
    t_verts[3].uv = Vec<2>(uv_sz.x(), 0);
    glInterleavedArrays(Vertex::FORMAT, 0, t_verts);
    glDrawArrays(GL_QUADS, 0, 4);
  }

  if (CHECKED)
    Mode::faces$ += 18;
  Mode::Check();
}

} // namespace dragoon
