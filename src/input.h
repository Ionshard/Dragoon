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
#include "log.h"
#include "var.h"
#include "Vec.h"

namespace dragoon {
namespace input {

/** Keyboard event */
class Key {
public:
  Key(int sym, int code, bool down);
  Vec<2> rel_motion() const { return motion_; }
  int sym() const { return sym_; }
  bool down() const { return sym_ && down_; }
  bool up() const { return sym_ && !down_; }
  bool select() const { return sym_ == SDLK_RETURN; }
  bool cancel() const { return sym_ == SDLK_ESCAPE || sym_ == SDLK_BACKSPACE; }

  /** Process events from SDL */
  static Key* Dispatch(const SDL_Event& ev);

  static Vec<2> motion() { return motion$; }
  static bool ctrl() { return ctrl$; }
  static bool alt() { return alt$; }
  static bool shift() { return shift$; }
  static void bind_up(int key) { bind_up$ = key; }
  static void bind_down(int key) { bind_down$ = key; }
  static void bind_left(int key) { bind_left$ = key; }
  static void bind_right(int key) { bind_right$ = key; }

private:
  Vec<2> motion_;
  int sym_;
  int code_;
  bool down_;

  static var::Int bind_up$;
  static var::Int bind_down$;
  static var::Int bind_left$;
  static var::Int bind_right$;
  static Vec<2> motion$;
  static bool ctrl$;
  static bool alt$;
  static bool shift$;
};

/** Mouse event */
class Mouse {
public:
  Mouse(int dx, int dy, int button = -1, bool down = false);
  Vec<2> rel_pointer() const { return pointer_; }
  int button() const { return button_; }
  bool down() const { return down_; }

  /** Process events from SDL */
  static Mouse* Dispatch(const SDL_Event& ev);

  static Vec<2> pointer() { return pointer$; }
  static bool button(int button) {
    ASSERT(button >= 0 && button < 3);
    return buttons$[button];
  }

private:
  Vec<2> pointer_;
  int button_;
  bool down_;

  static Vec<2> pointer$;
  static bool buttons$[3];
};

} // namespace input
} // namespace dragoon
