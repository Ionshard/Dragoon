/******************************************************************************\
 Dragoon - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "r_private.h"

/* Visual params for menu entries */
#define JIGGLE_RATE 4
#define JIGGLE_RADIUS 0.5
#define JIGGLE_SPEED 0.01
#define ENTRY_FADE 0.5
#define DISABLED_FADE 0.1

/******************************************************************************\
 Add an entry to a menu.
\******************************************************************************/
void RMenu_add(RMenu *menu, RMenuEntry *newEntry, float margin)
{
        RMenuEntry *entry;

        /* Center entries that don't have any options */
        newEntry->label.origin = CVec(0, menu->size.y += margin);
        if (!newEntry->options)
                newEntry->label.origin.x = menu->size.x / 2 -
                                           RText_size(&newEntry->label).x / 2;

        /* Add the entry to the menu linked list */
        newEntry->next = NULL;
        menu->size.y += newEntry->label.boxSize.y;
        if (!menu->entries) {
                menu->selected = menu->entries = newEntry;
                return;
        }
        for (entry = menu->entries; entry->next; entry = entry->next);
        entry->next = newEntry;
}

/******************************************************************************\
 Free memory used by a menu's child objects.
\******************************************************************************/
void RMenu_cleanup(RMenu *menu)
{
        RMenuEntry *entry, *entry_next;
        RMenuOption *option, *option_next;

        for (entry = menu->entries; entry; entry = entry_next) {
                entry_next = entry->next;
                for (option = entry->options; option; option = option_next) {
                        option_next = option->next;
                        C_free(option);
                }
                C_free(entry);
        }
        menu->entries = NULL;
}

/******************************************************************************\
 Render and update menu text effects.
\******************************************************************************/
void RMenu_update(RMenu *menu)
{
        RMenuEntry *entry;
        CVec menuExplode;

        if (!C_fade(&menu->fade, menu->shown, R_MENU_FADE))
                return;
        menuExplode = CVec_scalef(CVec(20, 0), 1 - menu->fade);
        if (menu->hideLeft)
                menuExplode.x = -menuExplode.x;

        /* Render the menu */
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glTranslatef(menu->origin.x, menu->origin.y - menu->size.y / 2,
                     menu->z);
        for (entry = menu->entries; entry; entry = entry->next) {
                RMenuOption *option;
                RText *text;
                CColor color;
                CVec explode;

                /* Entry label */
                if (!entry->enabled)
                        color = CColor(1, 1, 1, DISABLED_FADE);
                else {
                        C_fade(&entry->fade, menu->selected == entry,
                               JIGGLE_RATE);
                        entry->label.jiggleRadius = entry->fade * JIGGLE_RADIUS;
                        entry->label.jiggleSpeed = JIGGLE_SPEED;
                        color = CColor(1, 1, 1,
                                       1 - ENTRY_FADE * (1 - entry->fade));
                }
                color.a *= menu->fade;
                entry->label.modulate = color;
                entry->label.explode = menuExplode;
                RText_draw(&entry->label);

                /* Render options */
                if (!entry->selected)
                        continue;
                if (!entry->enabled) {
                        text = &entry->selected->text;
                        text->origin = CVec(menu->size.x - RText_size(text).x,
                                            entry->label.origin.y);
                        text->modulate = color;
                        RText_draw(text);
                        continue;
                }
                explode = CVec(-1, 0);
                for (option = entry->options; option; option = option->next) {
                        text = &option->text;
                        text->modulate = color;
                        C_fade(&option->fade, option == entry->selected,
                               4);
                        text->modulate.a *= option->fade;
                        text->origin = CVec(menu->size.x - RText_size(text).x,
                                            entry->label.origin.y);
                        if (option == entry->selected) {
                                text->jiggleRadius = entry->label.jiggleRadius;
                                text->jiggleSpeed = entry->label.jiggleSpeed;
                                if (text->explode.x > 0)
                                        explode = CVec(1, 0);
                                text->explode = CVec_scalef(explode, 10 *
                                                            (1 - option->fade));
                                explode = CVec(1, 0);
                        } else {
                                C_fade(&text->modulate.a, FALSE, R_MENU_FADE);
                                text->jiggleRadius = 0;
                                text->explode = CVec_scalef(explode, 10 *
                                                            (1 - option->fade));
                        }
                        text->explode = CVec_add(text->explode, menuExplode);
                        RText_draw(text);
                }
        }
        glPopMatrix();
        R_checkErrors();
}

/******************************************************************************\
 Add a new option to a menu entry.
\******************************************************************************/
RMenuOption *RMenuEntry_add(RMenuEntry *entry, const char *text, float value)
{
        RMenuOption *option, *newOption;

        C_new(&newOption);
        RText_init(&newOption->text, NULL, text);
        newOption->value = value;
        newOption->next = NULL;
        if (!entry->options)
                entry->selected = entry->options = newOption;
        else {
                for (option = entry->options; option->next;
                     option = option->next);
                option->next = newOption;
        }
        return newOption;
}

/******************************************************************************\
 Allocate a new menu entry.
\******************************************************************************/
RMenuEntry *RMenuEntry_new(const char *label, CCallback onActivate)
{
        RMenuEntry *entry;

        C_new(&entry);
        RText_init(&entry->label, NULL, label);
        entry->onActivate = onActivate;
        entry->enabled = TRUE;
        return entry;
}

/******************************************************************************\
 Change menu selection up or down.
\******************************************************************************/
void RMenu_select(RMenu *menu, bool up)
{
        RMenuEntry *entry, *enabled;

        if (!menu || !menu->entries)
                return;

        /* Going down */
        if (!up) {
                for (entry = menu->selected->next; ; entry = entry->next) {
                        if (!entry)
                                entry = menu->entries;
                        if (entry->enabled) {
                                menu->selected = entry;
                                break;
                        }
                }
                return;
        }

        /* Going up */
        for (entry = menu->selected->next; entry; entry = entry->next)
                if (entry->enabled)
                        enabled = entry;
        for (entry = menu->entries; entry->next; entry = entry->next) {
                if (entry == menu->selected)
                        break;
                if (entry->enabled)
                        enabled = entry;
        }
        if (enabled)
                menu->selected = enabled;
}

/******************************************************************************\
 Activate the current menu item.
\******************************************************************************/
void RMenu_activate(RMenu *menu, bool next)
{
        RMenuEntry *entry;

        if (!menu || !(entry = menu->selected))
                return;

        /* Select next option */
        if (entry->options && next) {
                if (entry->selected->next) {
                        if (entry->selected->fade < 1)
                                entry->selected->fade = 1;
                        entry->selected = entry->selected->next;
                }
        } else if (entry->options && !next) {
                RMenuOption *option;

                for (option = entry->options; option->next;
                     option = option->next)
                        if (option->next == entry->selected) {
                                if (entry->selected->fade < 1)
                                        entry->selected->fade = 1;
                                entry->selected = option;
                                break;
                        }
        }

        /* Notify of the activation */
        if (entry->onActivate)
                entry->onActivate(entry);
}

