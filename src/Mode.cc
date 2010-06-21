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
#include "Timer.h"
#include "Variable.h"
#include "Texture.h"
#include "Mode.h"

namespace dragoon {

Variable Mode::fullscreen$("fullscreen", false, "Render fullscreen window");
Variable Mode::width$("width", 1024, "Screen/window resolution width");
Variable Mode::height$("height", 768, "Screen/window resolution height");
Variable Mode::clear$("clear", true);
Variable Mode::width_target$("width_target", 640);
Variable Mode::height_target$("height_target", 480);
Count Mode::faces$;
int Mode::init_frame$;
int Mode::scale$;
int Mode::width_scaled$;
int Mode::height_scaled$;

#if CHECKED
void Mode::Check() {
  int error = glGetError();
  if (error != GL_NO_ERROR)
    ERROR("OpenGL error %d: %s", error, gluErrorString(error));
}
#endif

void Mode::Set(int width, int height, bool fullscreen) {
  int flags;

  // Ensure a minimum render size
  if (width < width_target$)
    width = width_target$;
  if (height < height_target$)
    height = height_target$;

  // Reset textures for resolution change
  Texture::Reset();

  // Create a new window
  SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
  flags = SDL_OPENGL | SDL_DOUBLEBUF | SDL_ANYFORMAT | SDL_RESIZABLE;
  if (fullscreen)
    flags |= SDL_FULLSCREEN;
  const SDL_Surface* video = SDL_SetVideoMode(width, height, 0, flags);
  if (!video)
    ERROR("Failed to set video mode: %s", SDL_GetError());

  // Update values
  width$ = width;
  height$ = height;
  fullscreen$ = fullscreen;

  // Get the actual screen size
  scale$ = (video->h + height_target$ - 1) / height_target$;
  height_scaled$ = video->h / scale$;
  height_scaled$ += (video->h - scale$ * height_scaled$) / scale$;
  width_scaled$ = video->w * height_scaled$ / video->h;
  DEBUG("Set %s mode %dx%d (%dx%d scaled), scale factor %d",
        fullscreen ? "fullscreen" : "windowed", video->w, video->h,
        width_scaled$, height_scaled$, scale$);

  // Update reset frame so we reinitialize textures as necessary
  init_frame$ = Timer::frame();

  // Screen viewport
  glViewport(0, 0, width$, height$);

  // Orthogonal projection
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.f, width_scaled$, height_scaled$, 0.f, 0.f, -1.f);

  // Identity model matrix
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Minimal alpha testing
  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GREATER, 1 / 255.f);

  // Background clear color
  glClearColor(1.0f, 0.0f, 1.0f, 1.f);

  // No texture by default
  glDisable(GL_TEXTURE_2D);

  // We use lines to do 2D edge anti-aliasing although there is probably
  // a better way so we need to always smooth lines (requires alpha
  // blending to be on to work)
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

  // Sprites are depth-tested
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  Check();
}

void Mode::Begin() {
  int clear_flags = GL_DEPTH_BUFFER_BIT;
  if (clear$)
    clear_flags |= GL_COLOR_BUFFER_BIT;
  glClear(clear_flags);
}

void Mode::End() {
  SDL_GL_SwapBuffers();
  Check();
}

} // namespace dragoon
