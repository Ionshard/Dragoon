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
#include "Timer.h"

namespace dragoon {

/** A counter for counting how often something happens per frame */
class Count {
public:
  Count():
    start_frame_(Timer::frame()), start_msec_(Timer::time()),
    last_msec_(0), value_(0) {}

  /** Average FPS while counter was running */
  float Fps() const {
    float seconds = (Timer::time() - start_msec_) * 0.001f;
    if (seconds <= 0.f)
      return 0.f;
    return (Timer::frame() - start_frame_) / seconds;
  }

  /** Per-frame count of a counter */
  float PerFrame() const {
    int frames = Timer::frame() - start_frame_;
    if (frames < 1)
      return 0.f;
    return value_ / frames;
  }

  /** Per-second count of a counter */
  float PerSec() const {
    float seconds = (Timer::time() - start_msec_) * 0.001f;
    if (seconds <= 0.f)
      return 0.f;
    return value_ / seconds;
  }

  /** Polls a counter to see if \c interval msec have elapsed since the
   *  last poll.
   *  @return  \c true if the counter is ready to be polled; sets the poll time
   */
  bool Poll(int interval) {
    if (Timer::time() - last_msec_ < interval)
      return false;
    last_msec_ = Timer::time();
    return true;
  }

  /** Resets counter */
  void Reset() {
    start_msec_ = last_msec_ = Timer::time();
    start_frame_ = Timer::frame();
    value_ = 0.f;
  }

  Count& operator=(int n) {
    value_ = n;
    return *this;
  }

  Count& operator+=(int n) {
    value_ += n;
    return *this;
  }

  Count& operator++(int n) {
    ++value_;
    return *this;
  }

  Count& operator++() {
    ++value_;
    return *this;
  }

private:
  int start_frame_;
  int start_msec_;
  int last_msec_;
  float value_;
};

} // namespace dragoon
