/******************************************************************************\
 Dragoon - Copyright (C) 2010 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "log.h"
#include "math.h"
#include "Timer.h"
#include "Mode.h"
#include "Texture.h"

namespace dragoon {

Texture::textures$T Texture::textures$;

Texture* Texture::Load(const char* name) {
  std::string key(name);
  if (textures$.count(key))
    return textures$[key];
  Texture* pt = new Texture(key.c_str());
  textures$[std::string(name)] = pt;
  return pt;
}

void Texture::Reset() {
  int count = 0;
  for (textures$T::iterator it = textures$.begin(), end = textures$.end();
       it != end; ++it) {
    it->second->up_scale_ = false;
    if (WINDOWS) {
      glDeleteTextures(1, &it->second->gl_name_);
      it->second->gl_name_ = 0;
    }
    ++count;
  }
  DEBUG("Reset %d textures", count);
}

Texture::~Texture() {
  if (gl_name_)
    glDeleteTextures(1, &gl_name_);
}

Texture::Texture(int width, int height):
  surface_(width, height), gl_name_(0), frame_(0), up_scale_(false),
  tile_(false) {}

void Texture::Upload() {

  // Texture has no surface data
  if (!surface_)
    return;

  // Surface size can be upscaled or not
  int scale = 1;
  int real_width = surface_->w;
  int real_height = surface_->h;
  if (up_scale_) {
    real_width *= scale = Mode::scale();
    real_height *= scale;
  }

  // Tiled texture span the entire surface
  Surface* pow2_surface = NULL;
  if (tile_) {
    scale_uv_ = Vec<2>(1, 1);

    // Allocate power-of-two surface
    pow2_width_ = math::NextPow2(real_width);
    pow2_height_ = math::NextPow2(real_height);

    // We can just upload this surface if its already power-of-two
    // otherwise we need to blit onto a new surface
    if (pow2_width_ != surface_->w || pow2_height_ != surface_->h) {
      pow2_surface = new Surface(pow2_width_, pow2_height_);
      surface_.Blit(*pow2_surface, 0, 0, surface_->w, surface_->h,
                    0, 0, pow2_width_, pow2_height_);
    }
  }

  // Otherwise we use texture coords to isolate a piece and add a
  // one pixel border around the texture
  else {
    pow2_width_ = math::NextPow2(real_width + 1);
    pow2_height_ = math::NextPow2(real_height + 1);

    // We can just upload this surface if its already power-of-two
    // otherwise we need to blit onto a new surface
    if (pow2_width_ != surface_->w || pow2_height_ != surface_->h) {
      pow2_surface = new Surface(pow2_width_, pow2_height_);
      surface_.Scale(*pow2_surface, scale, scale, 1, 1);
    }

    // Save UV scale
    scale_uv_ = Vec<2>((float)real_width / pow2_width_,
                       (float)real_height / pow2_height_);
  }

  // Upload the texture to OpenGL and build mipmaps
  glBindTexture(GL_TEXTURE_2D, gl_name_);
  Surface& upload_surface = pow2_surface ? *pow2_surface : surface_;
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, upload_surface->w,
               upload_surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               upload_surface->pixels);
  frame_ = Timer::frame();

  // Repeat wrapping (not supported for NPOT textures)
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // Free memory
  delete pow2_surface;

  Mode::Check();
}

Texture* Texture::Extract(int x, int y, int w, int h) {
  if (!surface_)
    return NULL;

  // Allocate a new surface and copy the portion of the old surface onto it
  Texture* dest = new Texture(w, h);
  dest->tile_ = true;
  surface_.Blit(dest->surface_, x, y, w, h, 0, 0, w, h);
  dest->surface_.Deseam();
  return dest;
}

void Texture::Select(bool smooth) {
  if (!surface_) {
    glDisable(GL_TEXTURE_2D);
    return;
  }

  // Make sure the texture has been uploaded to OpenGL
  bool need_upload = false;
  if (!gl_name_) {
    glGenTextures(1, &gl_name_);
    need_upload = true;
  }

  // Make sure that any texture we want to smooth has been upscaled
  if (smooth && !up_scale_) {
    up_scale_ = true;
    need_upload = true;
  }

  // Stale textures must be uploaded again
  if (frame_ < Mode::init_frame())
    need_upload = true;

  // Upload texture to OpenGL
  if (need_upload)
    Upload();

  // Select texture
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, gl_name_);

  // Scale filters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                  smooth ? GL_LINEAR : GL_NEAREST);

  // Non-power-of-two textures are pasted onto larger textures that require a
  // texture coordinate transformation
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  if (!tile_)
    glTranslatef(1.f / pow2_width_, 1.f / pow2_height_, 0.f);
  glScalef(scale_uv_[0], scale_uv_[1], 1.f);
  glMatrixMode(GL_MODELVIEW);

  Mode::Check();
}

Texture::Texture(const char* filename):
  surface_(filename), name_(filename), gl_name_(0), frame_(0), up_scale_(false),
  tile_(false) { surface_.Deseam(); }

} // namespace dragoon
