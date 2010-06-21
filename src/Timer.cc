/******************************************************************************\
 Dragoon - Copyright (C) 2009 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "log.h"
#include "Count.h"
#include "Timer.h"

namespace dragoon {

Count Timer::throttled_;
int Timer::time_msec_;
int Timer::frame_ = 1;
int Timer::frame_msec_;

unsigned int Timer::Poll() {
  static unsigned int last_msec;
  unsigned int elapsed = SDL_GetTicks() - last_msec;
  last_msec += elapsed;
  return elapsed;
}

void Timer::ThrottleFps(int max_fps) {
  if (max_fps < 1)
    return;
  int throttle_msec = 1000 / max_fps;
  if (frame_msec_ > throttle_msec)
    return;

  // SDL_Delay() will only work in 10ms increments on some systems so it
  // is necessary to break down our delays into bite-sized chunks
  static int wait_msec;
  wait_msec += throttle_msec - frame_msec_;
  int msec = (wait_msec / 10) * 10;
  if (msec > 0) {
    SDL_Delay(msec);
    wait_msec -= msec;
    throttled_ += msec;
  }
}

void Timer::Update() {
  static unsigned int last_msec;
  time_msec_ = SDL_GetTicks();
  frame_msec_ = time_msec_ - last_msec;
  last_msec = time_msec_;

  // Report when a frame takes an unusually long time
  if (CHECKED && frame_msec_ >= 100)
    DEBUG("Frame %d lagged, %d msec", frame_, frame_msec_);

  ++frame_;
}

} // namespace dragoon
