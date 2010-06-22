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
#include "log.h"
#include "ptr.h"
#include "Sprite.h"

namespace dragoon {

/** Text rendered onto the screen */
class Text:
  public param::Explode, public param::Jiggle, public param::Modulate,
  public param::Origin, public param::Scale, public param::Z {
public:

  /** Bitmap font sheet */
  class Font: public ptr::Wrap<Texture> {
  public:

    /** Initialize font sheet from grid texture.
     *  @param texture  Font texture
     *  @param first    First character on the font sheet
     *  @param cols     Font sheet columns
     *  @param rows     Font sheet rows
     */
    Font(Texture* texture, int first, int cols, int rows, float scale = 1):
      ptr::Wrap<Texture>(texture), box_size_(0, 0), scale_(scale),
      first_(first), cols_(cols), rows_(rows)
    {
      if (texture->Valid())
        box_size_ = texture->size() / Vec<2>(cols, rows) - 1;
      else
        WARN("Font texture '%s' invalid", texture ? texture->name() : "(null)");
    }

    /** Initialize font sheet from monospace TTF font file.
     *  @param filename  Font file path
     */
    Font(const char* filename, float size, float scale = 1);

    /** Get character dimensions */
    Vec<2> size(int ch = 0) const {
      Vec<2> size = box_size_;
      if (widths_ && ch >= first_ && ch < first_ + cols_ * rows_)
        size[0] = widths_[ch - first_];
      return size * scale_;
    }

  protected:
    friend class Text;

    Vec<2> box_size_;
    ptr::Scope<int, ptr::DeleteArray> widths_;
    float scale_;
    int first_;
    int cols_;
    int rows_;
  };

  /** Initialize text */
  Text(const char* string = "", const Font* font = NULL): font_(font) {
    SetFont(font);
    SetText(string);
  }

  /** Center the text object on a point */
  void CenterOn(Vec<2> origin) { origin_ = origin - Size() / 2; }

  /** Draw the text object on screen */
  void Draw();

  /** Set font */
  void SetFont(const Font* font) {
    font_ = font ? font : default_font_;
    SetText(string_.c_str());
  }

  /** Set text */
  void SetText(const char* string);

  /** Returns the dimensions of the text object */
  Vec<2> Size();

  /** Specify font object used when no font is given to a Text object */
  static void set_default_font(Font* font) { default_font_ = font; }

private:
  static ptr::Scope<Font> default_font_;

  const Font* font_;
  std::string string_;
  ptr::Scope<Sprite, ptr::DeleteArray> sprites_;
};

} // namespace dragoon
