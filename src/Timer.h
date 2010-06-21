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

namespace dragoon {

class Count;

/** Static class for tracking frame and time information */
class Timer {
public:

  /** The current frame number */
  static int frame() { return frame_; }

  /** Duration of the last frame in milliseconds */
  static int frame_msec() { return frame_msec_; }

  /** Duration of the last frame in seconds */
  static float frame_sec() { return frame_msec_ * 0.001f; }

  /** Returns the time since the last call to poll(). Useful for measuring
      the efficiency of sections of code. */
  static unsigned int Poll();

  /** Return counter for time spent throttled this frame */
  static const Count& throttled() { return throttled_; }

  /** Time since program started in milliseconds */
  static int time() { return time_msec_; }

  /** Time since program started in seconds */
  static float time_sec() { return time_msec_ * 0.001f; }

  /** Updates the current time. This needs to be called exactly once
      per frame. */
  static void Update();

  /** Throttle framerate if vsync is off or broken so we don't burn the CPU
   *  for no reason
   *  @param max_fps  Highest desired frames-per-second
   */
  static void ThrottleFps(int max_fps);

private:
  Timer() {}

  static int frame_;
  static int frame_msec_;
  static int time_msec_;
  static Count throttled_;
};

} // namespace dragoon
