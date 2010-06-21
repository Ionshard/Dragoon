/******************************************************************************\
 Dragoon - Copyright (C) 2010 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "log.h"
#include "math.h"
#include "Mode.h"
#include "Sprite.h"

namespace dragoon {

Sprite::sprites$T Sprite::sprites$;

Vec<2> Sprite::Center() const {
  if (!data_ || !size_.x() || !size_.y() ||
      !data_->box_size_.x() || !data_->box_size_.y())
    return Vec<2>(0, 0);
  Vec<2> center = data_->center_;
  if (mirror_)
    center[0] = data_->box_size_.x() - center[0];
  if (flip_)
    center[1] = data_->box_size_.y() - center[1];
  return center * size_ / data_->box_size_;
}

void Sprite::Draw() {
  if (!data_ || z_ < 0.f || modulate_.a() <= 0.f)
    return;

  // TODO: Check if sprite is on-screen
  //if (!CVec_intersect(r_cameraOn ? r_camera : CVec(0, 0),
  //                    CVec(r_widthScaled, r_heightScaled),
  //                    sprite->origin, sprite->size))
  //        return;

  // Setup transformation matrix
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  Vec<2> c = size_ / 2;
  glTranslatef(origin_.x() + c.x(), origin_.y() + c.y(), z_);
  bool smooth = angle_ != 0.f;
  if (smooth) {
    Vec<2> trans = Center() - c;
    glTranslatef(trans.x(), trans[1], 0);
    glRotatef(math::RadToDeg(angle_), 0.0, 0.0, 1.0);
    glTranslatef(-trans.x(), -trans[1], 0);
  }

  // Flip/mirror texture
  glScalef(mirror_ ^ data_->mirror_ ? -size_.x() : size_.x(),
           flip_ ^ data_->flip_ ? -size_.y() : size_.y(), 0);

  // Additive blending
  if (data_->blend_ == Data::BLEND_ADD) {
    glEnable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  }

  // Solid color
  else if (data_->blend_ == Data::BLEND_SOLID) {
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
  }

  // Alpha blending
  else {
    glEnable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

  // Modulate color
  Color modulate = modulate_ * data_->modulate_;
  if (data_->flicker_ > 0)
    modulate[3] = modulate[3] * (1 - data_->flicker_) +
                  data_->flicker_ * math::UnitRand();
  if (data_->blend_ == Data::BLEND_ADD) {
    modulate *= modulate[3];
    modulate[3] = 1;
  } else if (data_->blend_ == Data::BLEND_SOLID)
    modulate[3] = 1;
  modulate.Select();

  // Render the sprite quad(s)
  smooth |= data_->up_scale_;
  if (data_->corner_.x() || data_->corner_.y())
    DrawWindow(smooth);
  else
    DrawQuad(smooth);

  // Cleanup
  glPopMatrix();
  Mode::Check();
}

const Sprite::Data* Sprite::Get(const char* name) {
  std::string key(name);
  if (sprites$.count(key))
    return sprites$[key];
  WARN("Sprite '%s' not found", name);
  return NULL;
}

void Sprite::LoadConfig(const char* filename) {
  Config config(filename);
  for (const Config::Node* n = config.root(); n; n = n->next())
    ParseNode(n);
}

const Sprite::Data* Sprite::ParseNode(const Config::Node* node) {
  Data* data = Data::ParseNode(node);
  if (data && data->name_.size()) {
    if (sprites$.count(data->name_)) {
      WARN("Redeclared sprite '%s'", data->name_.c_str());
      delete data;
      return sprites$[data->name_];
    }
    sprites$[data->name_] = data;
  }
  return data;
}

} // namespace dragoon
