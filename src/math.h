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
namespace math {

/** Calculate the next power of two for any integer */
static inline int NextPow2(int n) {
  if (n < 1)
    return 0;
  int p;
  for (p = 1; p < n; p <<= 1);
  return p;
}

/** Limit a value to a range */
template<typename T> void Limit(T& f, T min, T max) {
  if (f < min)
    f = min;
  if (f > max)
    f = max;
}

/** Lightweight deterministic random function
 *  @returns  Pseudo-random integer from 0 to RAND_MAX
 */
static inline int Rand(int last)
  { return (1664525 * last + 1013904223) % RAND_MAX; }

/** Lightweight deterministic random function
 *  @returns  Pseudo-random integer from 0 to 1
 */
static inline int UnitRand(int last) { return (float)Rand(last) / RAND_MAX; }

/** Limit a value to a range with wrapping */
template<typename T> void Wrap(T& f, T min, T max) {
  if (f < min)
    f = max;
  if (f > max)
    f = min;
}

/** Interpolate a floating-point value for the frame
 *  @returns \c false if value is at or below minimum
 */
static inline bool Fade(float& value, bool up, float rate,
                        float min = 0, float max = 1) {
  if (up) {
    if ((value += rate * Timer::frame_sec()) > max)
      value = max;
  } else if ((value -= rate * Timer::frame_sec()) <= min) {
    value = min;
    return false;
  }
  return true;
}

/** Convert radians to degrees */
static inline float RadToDeg(float rad) { return rad * 180.f / M_PI; }

/** Convert degrees to radians */
static inline float DegToRad(float deg) { return deg * M_PI / 180.f; }

/** Floating-point random number 0-1 */
static inline float UnitRand() { return (float)rand() / RAND_MAX; }

} // namespace math
} // namespace dragoon
