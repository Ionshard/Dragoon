/******************************************************************************\
 Dragoon - Copyright (C) 2010 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "Mode.h"
#include "input.h"

namespace dragoon {
namespace input {

// Key
Variable Key::bind_up$("bind_up", 'w', "Up key code");
Variable Key::bind_down$("bind_down", 's', "Down key code");
Variable Key::bind_left$("bind_left", 'a', "Left key code");
Variable Key::bind_right$("bind_right", 'd', "Right key code");
Vec<2> Key::motion$;
bool Key::ctrl$(false);
bool Key::alt$(false);
bool Key::shift$(false);

// Mouse
Vec<2> Mouse::pointer$(0, 0);
bool Mouse::buttons$[3] = { false, false, false };

Key::Key(int sym, int code, bool down): sym_(sym), code_(code), down_(down) {
  if (sym == bind_left$ || sym == SDLK_LEFT)
    motion_ = Vec<2>(-1, 0);
  else if (sym == bind_right$ || sym == SDLK_RIGHT)
    motion_ = Vec<2>(1, 0);
  else if (sym == bind_up$ || sym == SDLK_UP || sym == ' ')
    motion_ = Vec<2>(0, -1);
  else if (sym == bind_down$ || sym == SDLK_DOWN)
    motion_ = Vec<2>(0, 1);
}

Key* Key::Dispatch(const SDL_Event& ev) {

  // Update modifiers
  SDLMod mod = SDL_GetModState();
  shift$ = mod & KMOD_SHIFT;
  alt$ = mod & KMOD_ALT;
  ctrl$ = mod & KMOD_CTRL;

  Key* event = NULL;
  switch (ev.type) {
  case SDL_KEYDOWN:
    event = new Key((int)ev.key.keysym.sym,
                    (int)ev.key.keysym.scancode, true);
    motion$ += event->rel_motion();
    //DEBUG("Key down: %s", event->rel_motion().ToString().c_str());
    return event;
  case SDL_KEYUP:
    event = new Key((int)ev.key.keysym.sym,
                    (int)ev.key.keysym.scancode, false);
    motion$ -= event->rel_motion();
    return event;
  default:
    return NULL;
  }
}

Mouse::Mouse(int dx, int dy, int button, bool down):
  pointer_(Vec<2>(dx, dy) / Mode::scale()), button_(button), down_(down) {}

Mouse* Mouse::Dispatch(const SDL_Event& ev) {
  switch (ev.type) {
  case SDL_MOUSEMOTION:
    return new Mouse(ev.motion.x, ev.motion.y);
  case SDL_MOUSEBUTTONDOWN:
    if (ev.button.button >= 0 && ev.button.button < 3)
      return new Mouse(0, 0, ev.button.button,
                       buttons$[ev.button.button] = true);
    return NULL;
  case SDL_MOUSEBUTTONUP:
    if (ev.button.button >= 0 && ev.button.button < 3)
      return new Mouse(0, 0, ev.button.button,
                       buttons$[ev.button.button] = false);
  default:
    return NULL;
  }
}

} // namespace input
} // namespace dragoon
