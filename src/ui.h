/******************************************************************************\
 Dragoon - Copyright (C) 2010 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "input.h"

namespace dragoon {
namespace ui {

/** Initialize interface resources */
void Init();

/** Show main menu */
void ShowMenu();

/** Hide open menu */
void HideMenu();

/** Dispatch a key event
 *  @returns  \c true if the event should cease propagation
 */
bool Dispatch(const input::Key*);

/** Dispatch a mouse event
 *  @returns  \c true if the event should cease propagation
 */
bool Dispatch(const input::Mouse*);

/** Render user interface */
void Update();

} // namespace ui
} // namespace dragoon
