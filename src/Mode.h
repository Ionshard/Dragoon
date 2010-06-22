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
#include "var.h"
#include "Count.h"

namespace dragoon {

/** Static class for setting OpenGL state and video mode */
class Mode {
public:

  /** Check OpenGL for errors */
#if CHECKED
  static void Check();
#else
  static void Check() {}
#endif

  /** Sets the video mode with a specific resolution */
  static void Set(int width, int height, bool fullscreen);

  /** Sets the video mode using variables */
  static void Set() { Set(width$, height$, fullscreen$); }

  /** Begin rendering frame */
  static void Begin();

  /** End rendering frame */
  static void End();

  static int height() { return height_scaled$; }
  static int width() { return width_scaled$; }
  static int scale() { return scale$; }
  static int init_frame() { return init_frame$; }
  static bool fullscreen() { return fullscreen$; }

  /*
  void clip(Vec<2> origin, CVec size);
  void R_clip_disable(void);
  void R_clip_left(float);
  void R_clip_top(float);
  void R_clip_right(float);
  void R_clip_bottom(float);
  void R_pushClip(void);
  void R_popClip(void);
  */

  /** Counter for rendered faces */
  static Count faces$;

private:
  static var::Int target_height$;
  static var::Int height$;
  static var::Int width$;
  static var::Bool clear$;
  static var::Bool fullscreen$;
  static int height_scaled$;
  static int init_frame$;
  static int scale$;
  static int width_scaled$;
};

} // namespace dragoon
