/******************************************************************************\
 Dragoon - Copyright (C) 2010 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#pragma once
#include "Vec.h"

namespace dragoon {
namespace param {

#define PARAM(N, T, F, D)\
  class N {\
  public:\
    N(): F##_ D {}\
    T F() const { return F##_; }\
    void set_##F(T v) { F##_ = v; }\
    \
  protected:\
    T F##_;\
  };

PARAM(Angle, float, angle, (0))
PARAM(Enabled, bool, enabled, (true))
PARAM(Explode, Vec<2>, explode, (0, 0))
PARAM(Flip, bool, flip, (false))
PARAM(Mirror, bool, mirror, (false))
PARAM(Modulate, Color, modulate, (1, 1, 1, 1))
PARAM(Origin, Vec<2>, origin, (0, 0))
PARAM(Scale, Vec<2>, scale, (1, 1))
PARAM(Size, Vec<2>, size, (0, 0))
PARAM(Visible, bool, visible, (false))
PARAM(Z, float, z, (0))

#undef PARAM

class Jiggle {
public:
  Jiggle(): jiggle_speed_(0), jiggle_radius_(0) {}
  float jiggle_speed() const { return jiggle_speed_; }
  float jiggle_radius() const { return jiggle_radius_; }
  void set_jiggle_speed(float v) { jiggle_speed_ = v; }
  void set_jiggle_radius(float v) { jiggle_radius_ = v; }

protected:
  float jiggle_speed_;
  float jiggle_radius_;
};

} // namespace traits
} // namespace dragoon
