/******************************************************************************\
 Dragoon - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "g_private.h"

/* Speed of camera movement */
#define CAM_SPEED -250

/* Filename of map being edited */
char g_edit[C_NAME_MAX];

/* Entity being edited */
static GEntityClass *editClass;
static PEntity *editEntity;
static CVec editOffset;
static bool editSizing;

/* Camera control */
static RText camStatus;
static CVec mouseWorld;

/******************************************************************************\
 Load a map for editing.
\******************************************************************************/
void G_initEditor(void)
{
        if (!g_edit[0])
                return;
        G_loadMap(g_edit, CVec(0, 0));
        editEntity = NULL;
        editClass = NULL;

        /* Pause physics */
        p_speed = 0.f;

        /* Clear the screen */
        r_clear = TRUE;

        /* Show cursor */
        SDL_ShowCursor(SDL_ENABLE);
}

/******************************************************************************\
 Pick an existing entity to edit.
\******************************************************************************/
static void pickEntity(void)
{
        GEntityClass *class_i, *class_nearest;
        PEntity *ents[64];
        int i, len, nearest;

        if (editEntity) {
                editEntity = NULL;
                editClass = NULL;
                editSizing = FALSE;
                return;
        }
        if (!(len = P_entsInBox_buf(mouseWorld, CVec(1, 1), PIT_ALL, ents)))
                return;

        /* Find the nearest entity */
        class_nearest = ents[0]->entityClass;
        for (nearest = 0, i = 1; i < len; i++) {
                class_i = ents[i]->entityClass;
                if (!class_i ||
                    (class_nearest && class_i->z < class_nearest->z))
                        continue;
                class_nearest = class_i;
                nearest = i;
        }
        editEntity = ents[nearest];
        editClass = class_nearest;
        editOffset = CVec_clamp(CVec_sub(editEntity->origin, mouseWorld), 1);
}

/******************************************************************************\
 Select a new entity to place.
\******************************************************************************/
static bool selectEntity(int key)
{
        GEntityClass *entClass;

        /* Select the next entity class */
        if (editClass && editClass->editorKey == key) {
                GEntityClass *firstClass;
                bool found;

                found = FALSE;
                firstClass = NULL;
                for (entClass = (GEntityClass *)g_classRoot; entClass;
                     entClass = (GEntityClass *)entClass->named.next) {
                        if (entClass->editorKey != key)
                                continue;
                        if (!firstClass)
                                firstClass = entClass;
                        if (found)
                                break;
                        found = editClass == entClass;
                }
                if (!entClass)
                        entClass = firstClass;
        }

        /* Select a new entity class */
        else
                for (entClass = (GEntityClass *)g_classRoot;
                     entClass && entClass->editorKey != key;
                     entClass = (GEntityClass *)entClass->named.next);

        /* Class not found */
        if (!entClass)
                return FALSE;

        /* Kill the old entity being edited */
        PEntity_kill(editEntity);
        editEntity = NULL;
        editSizing = FALSE;

        /* Spawn new edit entity */
        if ((editEntity = G_spawn(entClass->named.name))) {
                editEntity->origin = mouseWorld;
                editEntity->size = entClass->size;
                editClass = entClass;
        } else
                editClass = NULL;
        editOffset = CVec_clamp(CVec_divf(editEntity->size, -2), 1);
        editEntity->origin = CVec_add(editEntity->origin, editOffset);
        return TRUE;
}

/******************************************************************************\
 Delete the entity under the mouse.
\******************************************************************************/
static void deleteEntity(void)
{
        if (!editEntity) {
                pickEntity();
                if (!editEntity)
                        return;
        }
        PEntity_kill(editEntity);
        editEntity = NULL;
        editClass = NULL;
}

/******************************************************************************\
 Move or resize the selected entity.
\******************************************************************************/
static void resizeEntity(void)
{
        if (!editEntity)
                return;
        if (editSizing) {
                editEntity->size = CVec_sub(mouseWorld, editEntity->origin);
                if (editEntity->size.x < 1)
                        editEntity->size.x = 1;
                if (editEntity->size.y < 1)
                        editEntity->size.y = 1;
        } else
                editEntity->origin = CVec_add(editOffset, mouseWorld);
}

/******************************************************************************\
 Handle key-presses in the editor.
\******************************************************************************/
bool G_dispatch_editor(GEvent event)
{
        if (!g_edit[0])
                return FALSE;

        /* Select an entity class to place using keys */
        if (event == GE_KEY_DOWN) {

                /* PgDn/Up changes list order of current entity */
                if (g_key == SDLK_PAGEUP)
                        G_pushBackEntity(editEntity);
                else if (g_key == SDLK_PAGEDOWN)
                        G_pushForwardEntity(editEntity);

                /* All other keys can be used to select an entity */
                else if (!selectEntity(g_key))
                        G_controlEvent(event);

                return TRUE;
        }

        /* Stop moving camera */
        else if (event == GE_KEY_UP) {
                G_controlEvent(event);
                return TRUE;
        }

        /* Mouse clicks */
        else if (event == GE_MOUSE_DOWN) {

                /* Left-click to place */
                if (g_button == SDL_BUTTON_LEFT) {
                        editSizing = FALSE;
                        pickEntity();
                }

                /* Middle-click to resize */
                else if (g_button == SDL_BUTTON_MIDDLE) {
                        editSizing = TRUE;
                        pickEntity();
                }

                /* Right-click to remove */
                else if (g_button == SDL_BUTTON_RIGHT)
                        deleteEntity();

                /* Mouse-wheel changes list order of current entity */
                if (g_button == SDL_BUTTON_WHEELUP)
                        G_pushBackEntity(editEntity);
                if (g_button == SDL_BUTTON_WHEELDOWN)
                        G_pushForwardEntity(editEntity);
        }

        /* Update camera */
        else if (event == GE_UPDATE) {
                if (g_control.x || g_control.y) {
                        CVec diff;

                        diff = CVec_scalef(g_control, CAM_SPEED * c_frameSec);
                        r_cameraTo = CVec_sub(r_cameraTo, diff);
                        RText_init(&camStatus, NULL,
                                   C_va("%.1f, %.1f", r_camera.x, r_camera.y));
                        camStatus.origin.y = r_heightScaled -
                                             camStatus.boxSize.y * 2;
                }
                RText_draw(&camStatus);

                /* Update the position/size of the edit entity */
                mouseWorld = CVec_clamp(CVec_add(g_mouse, r_camera),
                                        g_shift ? 16 : 1);
                resizeEntity();
        }

        return FALSE;
}

