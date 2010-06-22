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

Text::Font::Font(const char* filename, float size, float scale):
  scale_(scale), first_(' '), cols_(16), rows_(6)
{
  // Load font file
  TTF_Font* font = TTF_OpenFont(filename, size);
  if (!font) {
    WARN("Failed to load font '%s' at %dpt", filename, size);
    return;
  }

  // Scan for largest glyph size
  widths_ = new int[cols_ * rows_];
  int w = 0, h = 0, w2, h2;
  for (int c = first_; c < first_ + cols_ * rows_; ++c) {
    char text[2] = { c, 0 };
    TTF_SizeUTF8(font, text, &w2, &h2);
    if (w2 > w)
      w = w2;
    if (h2 > h)
      h = h2;
    widths_[c - first_] = w2 + 1;
  }
  box_size_ = Vec<2>(w, h) + 1;
  w += 2;
  h += 2;

  // Render a font sheet
  ptr_ = new Texture(w * cols_, h * rows_);
  SDL_Color white = { 255, 255, 255, 255 };
  for (int c = first_; c < first_ + cols_ * rows_; ++c) {
    char text[2] = { c, 0 };
    TTF_SizeUTF8(font, text, &w2, &h2);
    Surface surf(TTF_RenderUTF8_Blended(font, text, white));
    if (!surf) {
      WARN("TTF_RenderUTF8_Blended() failed: %s", TTF_GetError());
      return;
    }
    surf.BlitShadowed(ptr_->surface(), 0, 0, surf->w, surf->h,
                      ((c - first_) % cols_) * w, (c - first_) / cols_ * h,
                      1, 1, Color::black());
  }

  TTF_CloseFont(font);
}

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
  sprite_data.scale_ = scale_ * font_->scale_;
  sprite_data.center_ = font_->box_size_ / 2;

  // Prepare effects
  int seed = (int)(size_t)this;
  float explode_norm = explode_.Zero() ? sqrtf(explode_.Len()) : 0;

  // Draw letters
  glMatrixMode(GL_MODELVIEW);
  Vec<2> offset_sz = font_->box_size_ + 1;
  float x = 0;
  int ch_max = font_->rows_ * font_->cols_;
  for (unsigned int i = 0; i < string_.length(); i++) {

    // Check character range
    int ch = string_[i] - font_->first_;
    if (ch < 0 || ch >= ch_max)
      continue;

    // Setup sprite data for this character
    Vec<2> origin = origin_ + Vec<2>(x, 0);
    Vec<2> coords = Vec<2>(ch % font_->cols_, ch / font_->cols_);
    sprite_data.box_size_ = font_->size(font_->first_ + ch);
    sprite_data.box_origin_ = offset_sz * coords;
    Sprite& sprite = sprites_[i] = Sprite(&sprite_data);
    sprite.set_size(sprite.size() * scale_);
    x += sprite_data.box_size_.x() * scale_.x();

    // Letter explode effect
    if (explode_norm) {
      seed = math::Rand(seed);
      Vec<2> diff = Vec<2>(explode_norm, explode_norm);
      diff[0] *= (float)(seed = math::Rand(seed)) / RAND_MAX - 0.5f;
      diff[1] *= (float)(seed = math::Rand(seed)) / RAND_MAX - 0.5f;
      diff += explode_;
      origin += diff * diff.Len();
    }

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

Vec<2> Text::Size() {
  Vec<2> size = Vec<2>(0, font_->size().y());
  for (int i = 0; i < (int)string_.length(); ++i)
    size[0] += font_->size(string_[i]).x();
  return size * scale_;
}

} // namespace dragoon
