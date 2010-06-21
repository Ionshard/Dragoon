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
#include "param.h"
#include "Config.h"
#include "Texture.h"

namespace dragoon {

/** 2D sprite rendered onto the screen */
class Sprite:
  public param::Angle, public param::Flip, public param::Mirror,
  public param::Modulate, public param::Origin, public param::Size,
  public param::Z {
public:

  /** Sprite data information */
  struct Data {

    /** Animation frame */
    struct Frame {
      Frame(const char* name, int msec): name_(name), msec_(msec) {}

      std::string name_;
      int msec_;
    };

    /** Tiling mode */
    enum Tile {
      TILE_SCALED,   ///< Scale the sprite when it is resized
      TILE_LOCAL,    ///< Tile the sprite relative to its origin
      TILE_GLOBAL,   ///< Tile the sprite relative to global origin
      TILE_PARALLAX, ///< Global tiling with parallax effect
    };

    /** Blending mode */
    enum Blend {
      BLEND_ALPHA, ///< Alpha-blending translucency
      BLEND_SOLID, ///< Solid color sprite, no blending
      BLEND_ADD,   ///< Additive blending
    };

    /** Creates an uninitialized Data object */
    Data();

    /** Initializes data structures from a sprite config block */
    void ParseFrame(const Config::Node*);

    /** Initializes data structures from an anim config block */
    void ParseAnim(const Config::Node*);

    /** Create and register a sprite from a configuration node */
    static Data* ParseNode(const Config::Node*);

    /** Returns the natural size of the sprite */
    Vec<2> size() const { return box_size_ * scale_; }

    Texture* texture_;
    ptr::Scope<Texture> tiled_;
    ptr::Scope<Texture> edges_[4];
    Color modulate_;
    Vec<2> box_origin_;
    Vec<2> box_size_;
    Vec<2> tile_origin_;
    Vec<2> center_;
    Vec<2> scale_;
    Vec<2> corner_;
    std::vector<Frame> anim_;
    std::string name_;
    Tile tile_;
    Blend blend_;
    float parallax_;
    float flicker_;
    int next_msec_;
    bool flip_;
    bool mirror_;
    bool up_scale_;
  };

  /** 2D vertex */
#pragma pack(push, 4)
  struct Vertex {
    enum { FORMAT = GL_T2F_V3F };

    Vec<2> uv;
    Vec<2> co;
    float z;
  };
#pragma pack(pop)

  /** Initialize a sprite by data pointer */
  Sprite(const Data* data = NULL): data_(data) {
    if (data)
      size_ = data->size();
  }

  /** Initialize a sprite by name */
  Sprite(const char* name) {
    if ((data_ = Get(name)))
      size_ = data_->size();
  }

  /** Get the sprite center point */
  Vec<2> Center() const;

  /** Draw the sprite on the screen */
  void Draw();

  /** Get sprite data by name */
  static const Data* Get(const char* name);

  /** Load sprite config file */
  static void LoadConfig(const char* filename);

  /** Create and register a sprite from a configuration node */
  static const Data* ParseNode(const Config::Node*);

private:
  typedef ptr::Scope<Data>::Map<const std::string> sprites$T;

  /** Renders a single quad sprite */
  void DrawQuad(bool smooth);

  /** Renders a window sprite. A window sprite is composed of a grid of nine
      quads where the corner quads have a fixed size and the connecting quads
      stretch to fill the rest of the sprite size. */
  void DrawWindow(bool smooth);

  static sprites$T sprites$;

  const Data *data_;
};

} // namespace dragoon
