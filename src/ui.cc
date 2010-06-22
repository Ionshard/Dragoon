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
#include "draw.h"
#include "str.h"
#include "ui.h"
#include "ui/Menu.h"

namespace dragoon {
namespace ui {

namespace {
  var::String font_name$("ui.font_name", "data/ui/font/blemished.ttf");
  var::Int font_size$("ui.font_size", 8);
  var::Int menu_height$("ui.menu_height", 128);
  var::Float bg_fade_rate$("ui.bg_fade_rate", 2);
  Menu main_menu$(160);
  Menu* visible_menu$ = NULL;
  std::list<Menu*> menus$;
  float bg_fade$;

  // Main menu
  void OnNewGame(float) {}
  void OnOptions(float) {}
  void OnExit(float) { exit(0); }
}

void Init() {

  // Default font
  Text::Font* def_font;
  if (str::EndsWith(font_name$.c_str(), ".png", true))
    def_font = new Text::Font(Texture::Load(font_name$.c_str()), ' ', 16, 6);
  else
    def_font = new Text::Font(font_name$.c_str(), font_size$);
  Text::set_default_font(def_font);

  // Main menu
  main_menu$.Add(new Menu::Entry("New Game", OnNewGame));
  Menu::Entry* entry = new Menu::Entry("Continue");
  entry->set_enabled(false);
  main_menu$.Add(entry);
  main_menu$.Add(new Menu::Entry("Options", OnOptions));
  main_menu$.Add(new Menu::Entry("Quit", OnExit), 16);
  menus$.push_back(&main_menu$);
}

void ShowMenu() {
  main_menu$.set_visible(true);
  visible_menu$ = &main_menu$;
}

void HideMenu() {
  if (visible_menu$) {
    visible_menu$->set_visible(false);
    visible_menu$ = NULL;
  }
}

bool Dispatch(const input::Key* key) {
  if (key->down()) {
    if (visible_menu$) {

      // Navigate menu
      if (key->rel_motion().y() > 0)
        visible_menu$->Scroll(true);
      else if (key->rel_motion().y() < 0)
        visible_menu$->Scroll(false);
      else if (key->rel_motion().x() > 0 || key->select())
        visible_menu$->Activate(true);
      else if (key->rel_motion().x() < 0 || key->cancel())
        visible_menu$->Activate(false);

      // TODO: Hide menu via key

      // TODO: Show menu via key

      return true;
    }
  }
  return false;
}

bool Dispatch(const input::Mouse* mouse) { return visible_menu$; }

void Update() {

  // Let some time go by before showing the menu for the first time
  if (Timer::time() < 1000)
    return;

  // Background
  math::Fade(bg_fade$, visible_menu$, bg_fade_rate$);
  draw::rect(Vec<2>(0, Mode::height() * 2 / 3 - menu_height$ / 2), 0,
             Vec<2>(Mode::width(), menu_height$),
             Color(0, 0, 0, 0.5 * bg_fade$));

  // Update menus
  for (std::list<Menu*>::iterator it = menus$.begin(), end = menus$.end();
       it != end; ++it) {
    (*it)->set_origin(Vec<2>(Mode::width() / 2 - (*it)->size().x() / 2,
                             Mode::height() * 2 / 3));
    (*it)->Update();
  }
}

} // namespace ui
} // namespace dragoon
