/******************************************************************************\
 Dragoon - Copyright (C) 2009 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "../math.h"
#include "../Variable.h"
#include "Menu.h"

namespace dragoon {
namespace ui {

namespace {

  // Visual params for menu entries
  Variable jiggle_rate$("menu.jiggle_rate", 4);
  Variable jiggle_radius$("menu.jiggle_radius", 1);
  Variable jiggle_speed$("menu.jiggle_speed", 0.01);
  Variable entry_fade$("menu.entry_fade", 0.5);
  Variable disabled_fade$("menu.disabled_fade", 0.15);
  Variable fade$("menu.fade", 1);
}

void Menu::Entry::Activate(bool next) {
  if (options_.size()) {
    if (next && selected_ < (int)options_.size() - 1) {
      ++selected_;
      if (on_activate_)
        on_activate_(options_[selected_]->value_);
    }
    if (!next && selected_ > 0) {
      --selected_;
      if (on_activate_)
        on_activate_(options_[selected_]->value_);
    }
  } else if (on_activate_)
    on_activate_(-1);
}

void Menu::Entry::Update(float menu_fade, float menu_width, Vec<2> menu_explode,
                         bool selected) {
  Color color;

  // Entry label
  if (!enabled()) {
    color = Color(1, 1, 1, disabled_fade$);
    label_.set_jiggle_radius(0);
  } else {
    math::Fade(fade_, selected, jiggle_rate$);
    label_.set_jiggle_radius(fade_ * jiggle_radius$);
    label_.set_jiggle_speed(jiggle_speed$);
    color = Color(1, 1, 1, 1 - fade_ * (1 - fade_));
  }
  color[3] *= menu_fade;
  label_.set_modulate(color);
  label_.set_explode(menu_explode);
  label_.Draw();

  // Render options
  if (options_.size() > 0) {
    if (!enabled()) {
      Text& text = options_[selected_]->text_;
      text.set_origin(Vec<2>(menu_width - text.size().x(),
                             label_.origin().y()));
      text.set_modulate(color);
      text.Draw();
    } else {
      Vec<2> explode = Vec<2>(-1, 0);
      for (int i = 0; i < (int)options_.size(); ++i) {
        Option& opt = *options_[i];
        math::Fade(opt.fade_, i == selected_, 4);
        Color mod = color;
        mod[3] *= opt.fade_;
        opt.text_.set_origin(Vec<2>(menu_width - opt.text_.size().x(),
                                    label_.origin().y()));
        if (i == selected_) {
          opt.text_.set_jiggle_radius(label_.jiggle_radius());
          opt.text_.set_jiggle_speed(label_.jiggle_speed());
          if (opt.text_.explode().x() > 0)
            opt.text_.set_explode(Vec<2>(1, 0));
          opt.text_.set_explode(opt.text_.explode() * 10 * (1 - opt.fade_));
          explode = Vec<2>(1, 0);
        } else {
          math::Fade(mod[3], false, fade$);
          opt.text_.set_jiggle_radius(0);
          opt.text_.set_explode(0);
          explode = explode * 10 * (1 - opt.fade_);
        }
        opt.text_.set_modulate(mod);
        opt.text_.set_explode(opt.text_.explode() + menu_explode);
        opt.text_.Draw();
      }
    }
  }
}

float Menu::Entry::Position(Vec<2> origin, float width) {
  Vec<2> size = label_.size();

  // Center entries that don't have any options
  if (!options_.size())
    origin[0] = (width - size.x()) / 2;

  label_.set_origin(origin);
  return size.y();
}

void Menu::Add(Entry* entry, float margin) {
  float height = entry->Position(Vec<2>(0, size_[1] += margin), size_.x());
  size_[1] += height;
  entries_.push_back(entry);
}

void Menu::Scroll(bool up) {
  for (int i = selected_ + 1; i < selected_; up ? ++i : --i) {
    math::Wrap(i, 0, (int)entries_.size() - 1);
    if (entries_[i]->enabled()) {
      selected_ = i;
      break;
    }
  }
}

void Menu::Update() {
  if (math::Fade(fade_, visible_, fade$) && entries_.size()) {

    // Explode effect
    Vec<2> explode = Vec<2>(20, 0) * (1 - fade_);
    if (hide_left_)
      explode[0] = -explode[0];

    // Do not select a disabled item
    if (!entries_[selected_]->enabled())
      Scroll();

    // Render the menu
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef(origin_.x(), origin_.y() - size_.y() / 2, 0);
    for (int i = 0; i < (int)entries_.size(); ++i)
      entries_[i]->Update(fade_, size_.x(), explode, selected_ == i);
    glPopMatrix();
    Mode::Check();
  }
}

} // namespace ui
} // namespace dragoon
