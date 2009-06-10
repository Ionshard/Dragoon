/******************************************************************************\
 Dragoon - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "../common/c_public.h"
#include "p_public.h"

/* Limit the length of time a physics frame will simulate to account for
   lagged frames and gdb debugging */
#define P_FRAME_MSEC_MAX (1000 / 30)

/* Positions outside of this limit are assumed to be broken entities */
#define P_POSITION_LIMIT 100000

/* PEntity_physics.c */
void PEntity_physics(PEntity *entity, float delT);

