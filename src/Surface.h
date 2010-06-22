/******************************************************************************\
 Dragoon - Copyright (C) 2010 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "ptr.h"
#include "Vec.h"

namespace dragoon {

/** Wrapper adding functionality to SDL_Surface */
class Surface: public ptr::Scope<SDL_Surface, SDL_FreeSurface> {
public:

  /** Initialize with surface pointer */
  Surface(SDL_Surface* surf = NULL):
    ptr::Scope<SDL_Surface, SDL_FreeSurface>(surf), lock_(0) {}

  /** Loads a PNG file and allocates a new SDL surface for it.
      Based on tutorial implementation in the libpng manual:
      http://www.libpng.org/pub/png/libpng-1.2.5-manual.html */
  Surface(const char* filename);

  /** Allocate blank surface through SDL */
  Surface(int width, int height): lock_(0) { Alloc(width, height); }

  /** Reads in an area of the screen */
  Surface(int x, int y, int width, int height);

  /** Set a pixel on the surface to a specific color */
  void Put(int x, int y, Color color);

  /** Get the color value of a pixel on the surface */
  Color Get(int x, int y) const;

  /** Lock surface
   *  @returns true if lock succeeded
   */
  bool Lock();

  /** Unlock surface */
  void Unlock();

  /** Write the contents of a surface as a PNG file
   *  @returns true if a file was written
   */
  bool Save(const char* filename);

  /** Scan a surface and color in transparent pixel colors with the average
      of their neighbors to prevent seam glitches during rotation */
  void Deseam();

  /** Vertically flip surface pixels */
  void Flip();

  /** Scale blit a surface to a destination surface by an integer size */
  void Scale(Surface& dest, int scale_x, int scale_y, int dx, int dy);

  /** Fast surface blit */
  void Blit(Surface& dest, int sx, int sy, int sw, int sh, int dx, int dy);

  /** Resizing surface blit */
  void Blit(Surface& dest, int sx, int sy, int sw, int sh,
            int dx, int dy, int dw, int dh);

  /** Blit and add a shadow */
  void BlitShadowed(Surface& dest, int sx, int sy, int sw, int sh,
                    int dx, int dy, int sh_x, int sh_y, Color shadow);

  /** Returns true if the surface is valid */
  bool Valid() { return ptr_ != NULL && ptr_->w && ptr_->h; }

  /** Returns the size of the surface */
  Vec<2> size() const
    { return ptr_ ? Vec<2>(ptr_->w, ptr_->h) : Vec<2>(0, 0); };

private:
  void Alloc(int width, int height);
  void Validate(int x, int y) const;

  int lock_;
};

} // namespace dragoon
