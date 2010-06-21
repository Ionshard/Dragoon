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
#include "ptr.h"
#include "Vec.h"
#include "Mode.h"
#include "Surface.h"

namespace dragoon {

/** Class holding a texture reference */
class Texture {
public:
  ~Texture();

  /** Allocate a blank texture */
  Texture(int width, int height);

  /** If the texture's SDL surface has changed, the image must be reloaded
      into OpenGL. This function will do this. It is assumed that the texture
      surface format has not changed since the texture was created. */
  void Upload();

  /** Cut a tilable chunk out of an already loaded texture */
  Texture* Extract(int x, int y, int width, int height);

  /** Selects (binds) a texture for rendering in OpenGL. Also sets whatever
      options are necessary to get the texture to show up properly. */
  void Select(bool smooth = false);

  /** Returns true if the texture is valid */
  bool Valid() { return this && surface_.Valid(); }

  /** Returns the dimensions of the texture surface */
  Vec<2> size() const { return surface_.size(); }

  /** Smallest power-of-two dimensions that contain the surface */
  Vec<2> pow2_size() const { return Vec<2>(pow2_width_, pow2_height_); }

  /** Get texture name */
  const char* name() const { return name_.c_str(); }

  /** Deselect current OpenGL texture */
  static void Deselect() { glDisable(GL_TEXTURE_2D); }

  /** Load a texture from disk or return a reference if already loaded */
  static Texture* Load(const char* name);

  /** Reset textures */
  static void Reset();

protected:
  Texture(const char* name);

private:
  typedef ptr::Scope<Texture>::Map<std::string> textures$T;

  static textures$T textures$;

  Vec<2> scale_uv_;
  Surface surface_;
  std::string name_;
  unsigned int gl_name_;
  int pow2_width_;
  int pow2_height_;
  int frame_;
  bool up_scale_;
  bool tile_;
};

} // namespace dragoon
