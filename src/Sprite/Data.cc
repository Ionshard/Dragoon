/******************************************************************************\
 Dragoon - Copyright (C) 2010 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "../log.h"
#include "../Sprite.h"

namespace dragoon {

Sprite::Data::Data():
  texture_(NULL),
  modulate_(1, 1, 1, 1),
  scale_(1, 1),
  tile_(TILE_SCALED),
  blend_(BLEND_ALPHA),
  parallax_(0),
  flicker_(0),
  flip_(false),
  mirror_(false),
  up_scale_(false)
  {}

void Sprite::Data::ParseFrame(const Config::Node* n) {
  bool have_box = false;
  bool have_center = false;
  for (n = n->child(); n; n = n->next()) {
    const Config::Node* c = n->child();

    // Texture file
    if (n->Match(0, "file"))
      texture_ = Texture::Load(n->token(1));

    // Modulation color
    else if (n->Match("color")) {
      if (c)
        modulate_ = Color(atof(c->token(0)) / 255.f, atof(c->token(1)) / 255.f,
                          atof(c->token(2)) / 255.f, atof(c->token(3)) / 255.f);
      else
        WARN("Expected child block for color in %s:%d",
             n->filename(), n->line());
    }

    // Flip and mirror
    else if (n->Match("flip"))
      flip_ = true;
    else if (n->Match("mirror"))
      mirror_ = true;

    // Bounding box origin and size
    else if (n->Match("box")) {
      if (c) {
        box_origin_ = Vec<2>(atof(c->token(0)), atof(c->token(1)));
        box_size_ = Vec<2>(atof(c->token(2)), atof(c->token(3)));
        have_box = true;
      } else
        WARN("Expected child block for box in %s:%d", n->filename(), n->line());
    }

    // Bounding box center
    else if (n->Match("center")) {
      if (c) {
        center_ = Vec<2>(atof(c->token(0)), atof(c->token(1)));
        have_center = true;
      } else
        WARN("Expected child block for center in %s:%d",
             n->filename(), n->line());
    }

    // Scale factor
    else if (n->Match(0, "scale"))
      scale_ = c ? Vec<2>(atof(c->token(0)), atof(c->token(1)))
                 : Vec<2>(atof(n->token(1)), atof(n->token(1)));

    // Force upscale
    else if (n->Match("upscale"))
      up_scale_ = true;

    // Window sprite
    else if (n->Match("window")) {
      if (c)
        corner_ = Vec<2>(atof(c->token(0)), atof(c->token(1)));
      else
        WARN("Expected child block for window in %s:%d",
             n->filename(), n->line());
    }

    // Flicker alpha
    else if (n->Match(0, "flicker"))
      flicker_ = atof(n->token(1));

    // Tile
    else if (n->Match(0, "tile")) {
      if (n->size() == 1) {
        tile_ = TILE_LOCAL;
        if (c)
          tile_origin_ = Vec<2>(atof(c->token(0)), atof(c->token(1)));
      } else if (n->Match(1, "global")) {
        tile_ = TILE_GLOBAL;
        if (c) {
          tile_origin_ = Vec<2>(atof(c->token(0)), atof(c->token(1)));
          parallax_ = atof(n->token(3));
        }
      } else
        WARN("Unrecognized tile command '%s' in %s:%d",
             n->c_str(), n->filename(), n->line());
    }

    // Blend mode
    else if (n->Match(0, "blend")) {
      if (n->Match(1, "solid"))
        blend_ = BLEND_SOLID;
      else if (n->Match(1, "alpha"))
        blend_ = BLEND_ALPHA;
      else if (n->Match(1, "add"))
        blend_ = BLEND_ADD;
      else
        WARN("Unrecognized blend command '%s' in %s:%d",
             n->c_str(), n->filename(), n->line());
    }

    // Unrecognized command
    else
      WARN("Unrecognized sprite command '%s' in %s:%d",
           n->c_str(), n->filename(), n->line());
  }

  // Defaults
  if (!have_box && texture_)
    box_size_ = texture_->size();
  if (!have_center)
    center_ = box_size_ / 2.f;

  // Create tiled window subtextures
  if (tile_) {

    // Window tiling
    if (corner_[0] || corner_[1]) {
      Vec<2> origin = box_origin_ + corner_;
      Vec<2> size = box_size_ - corner_ * 2;
      tiled_ = texture_->Extract(origin[0], origin[1], size[0], size[1]);

      // Extract edges: top, left, right, bottom
      edges_[0] = texture_->Extract(origin[0], box_origin_[1],
                                    size[0], corner_[1]);
      edges_[1] = texture_->Extract(box_origin_[0], origin[1],
                                    corner_[0], size[1]);
      edges_[2] = texture_->Extract(box_origin_[0] + size[0], origin[1],
                                    corner_[0], size[1]);
      edges_[3] = texture_->Extract(origin[0], origin[1] + size[1],
                                    size[0], corner_[1]);
    }

    // Non-window tiling
    else
      tiled_ = texture_->Extract(box_origin_[0], box_origin_[1],
                                 box_size_[0], box_size_[1]);
  }
}

void Sprite::Data::ParseAnim(const Config::Node* n) {
  for (n = n->child(); n; n = n->next())
    anim_.push_back(Frame(n->token(0), atoi(n->token(1))));
}

Sprite::Data* Sprite::Data::ParseNode(const Config::Node* n) {

  // Animations and sprite frames are parsed differently
  Data* d = new Data();
  ASSERT(n->Match(0, "sprite") || n->Match(0, "anim"));
  if (n->Match(0, "anim"))
    d->ParseAnim(n);
  else if (n->Match(0, "sprite"))
    d->ParseFrame(n);

  // Assign name
  if (n->size() > 1)
    d->name_ = n->token(1);

  return d;
}

} // namespace dragoon
