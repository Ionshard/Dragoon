/******************************************************************************\
 Dragoon - Copyright (C) 2010 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "math.h"
#include "Sprite.h"
#include "Text.h"

namespace dragoon {

ptr::Scope<Text::Font> Text::default_font_;

void Text::SetText(const char *string) {
  string_ = string;
  sprites_.Release();
  if (font_) {
    sprites_ = new Sprite[string_.length()];
    for (unsigned int i = 0; i < string_.length(); i++)
      sprites_[i].set_size(font_->box_size_ * font_->scale_);
  } else
    sprites_ = NULL;
}

void Text::Draw() {
  if (!font_ || !(*font_)->Valid() || !sprites_)
    return;

  // Setup sprite data for all characters
  Sprite::Data sprite_data;
  sprite_data.texture_ = *font_;
  sprite_data.modulate_ = modulate_;
  sprite_data.box_size_ = font_->box_size_;
  sprite_data.scale_ = scale_ * font_->scale_;
  sprite_data.center_ = font_->box_size_ / 2;

  // Prepare effects
  int seed = (int)(size_t)this;
  float explode_norm = explode_.Zero() ? sqrtf(explode_.Len()) : 0;

  // Draw letters
  glMatrixMode(GL_MODELVIEW);
  Vec<2> offset_sz = font_->box_size_ + 1;
  int ch_max = font_->rows_ * font_->cols_;
  for (unsigned int i = 0; i < string_.length(); i++) {
    Vec<2> origin = origin_;
    origin[0] += font_->box_size_.x() * font_->scale_ * i * scale_.x();

    // Check character range
    int ch = string_[i] - font_->first_;
    if (ch < 0 || ch >= ch_max)
      continue;

    // Letter explode effect
    if (explode_norm) {
      seed = math::Rand(seed);
      Vec<2> diff = Vec<2>(explode_norm, explode_norm);
      diff[0] *= (float)(seed = math::Rand(seed)) / RAND_MAX - 0.5f;
      diff[1] *= (float)(seed = math::Rand(seed)) / RAND_MAX - 0.5f;
      diff += explode_;
      origin += diff * diff.Len();
    }

    // Setup sprite data for this character
    Vec<2> coords = Vec<2>(ch % font_->cols_, ch / font_->cols_);
    sprite_data.box_origin_ = offset_sz * coords;
    Sprite& sprite = sprites_[i] = Sprite(&sprite_data);
    sprite.set_size(sprite.size() * scale_);

    // Letter jiggle effect
    if (jiggle_radius_ != 0) {
      float time = Timer::time() * jiggle_speed_;
      origin += Vec<2>(sin(time + 787 * i), cos(time + 386 * i))
                * jiggle_radius_;
      sprite.set_angle(0.1 * jiggle_radius_ * sin(time + 911 * i));
    } else
      sprite.set_angle(0);

    // Draw character sprite
    glPushMatrix();
    glTranslatef(origin.x(), origin.y(), z_);
    sprite.Draw();
    glPopMatrix();
  }

  Mode::Check();
}

} // namespace dragoon
