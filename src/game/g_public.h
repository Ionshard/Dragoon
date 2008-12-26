/******************************************************************************\
 Dragoon - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* g_editor.c */
void G_initEditor(void);

extern char g_edit[C_NAME_MAX];

/* GEntity.c */
void G_cleanupEntities(void);
void G_parseEntityCfg(const char *filename);

/* g_input.c */
void G_dispatch(const SDL_Event *);
void G_update(void);

/* g_map.c */
void G_loadMap(const char *filename, CVec offset);
void G_saveMap(const char *filename);

/* g_menu.c */
void G_initMenu(void);
void G_cleanupMenu(void);
void G_showMenu(void);

/* g_test.c */
void G_loadTest(void);

