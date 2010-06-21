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
namespace draw {

void rect(Vec<2> origin, float z, Vec<2> size,
          Color mod = Color::white(), Color add = Color::none());

} // namespace draw
} // namespace dragoon
