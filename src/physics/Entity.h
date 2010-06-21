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
#include "Vec.h"

namespace dragoon {
namespace physics {

/** Base physics entity class */
class Entity {
public:

  /** The entity should free any allocated memory. Returning \c false will
      prevent the entity class itself from being free'd. */
  virtual bool OnFree() { return true; }

  /** The entity has been killed somehow and should remove itself from
      the game world */
  virtual void OnKill() {}

  /** The entity is about to participate in a collision. Returning false
      will prevent the collision processing. */
  virtual bool OnImpact(Entity* other) { return true; }

  /** Event sent out before physics are updated */
  virtual void PrePhysics() {}

  /** Event sent out after physics are updated */
  virtual void PostPhysics() {}

  /* The entity should render itself and update game mechanics */
  virtual void Update() {}

protected:
  Entity* ground_;        ///< Entity we are standing on
  Entity* ceiling_;       ///< Entity immediately above us
  Entity* right_wall_;    ///< Entity touching on the right
  Entity* left_wall_;     ///< Entity touching on the left
  Vec<2> origin_;         ///< Entity's upper-left hand corner position
  Vec<2> ground_origin_;  ///< Point on the ground entity we are standing on
  Vec<2> velocity_;       ///< Entity's current speed and direction
  Vec<2> accel_;          ///< Entity's acceleration for this frame

  /** Physical properties */
  class Properties {
  public:
    Properties():
      size_(1, 1), mass_(0), friction_(1), drag_(1), elasticity_(0),
      step_size_(0), gravity_(1), frame_skip_(0) {}

    Vec<2> size_;           ///< Bounding-box size of the entity
    float mass_;            ///< Mass of entity or zero for fixtures
    float friction_;        ///< Friction scale factor
    float drag_;            ///< Deceleration proportion in air
    float elasticity_;      ///< How bouncy the entity is
    float step_size_;       ///< Height of stairs this entity can walk over
    float gravity_;         ///< Proportion of gravity force affecting entity
    int frame_skip_;        ///< Wait this many frames between updates
    unsigned int impact_;   ///< Bit field of types for this entity
    unsigned int impacts_;  ///< Bit field of types this entity impacts
  } properties;

private:
  float lag_sec_;
  bool dead_;
};

} // namespace physics
} // namespace dragoon
