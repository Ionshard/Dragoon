/******************************************************************************\
 Dragoon - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "c_private.h"

/******************************************************************************\
 Add a CLink object to a linked list with [root].
\******************************************************************************/
void CLink_add(CLink *link, CLink **root, void *data)
{
        if (link->root == root)
                return;
        C_assert(!link->root);
        link->data = data;
        link->root = root;
        link->prev = NULL;
        if ((link->next = *root))
                link->next->prev = link;
        *root = link;
}

void CLink_add_rear(CLink *link, CLink **root, void *data)
{
        CLink *end;

        if (!*root) {
                CLink_add(link, root, data);
                return;
        }
        if (link->root == root)
                return;
        C_assert(!link->root);
        for (end = *root; end->next; end = end->next);
        link->data = data;
        link->root = root;
        link->prev = end;
        link->next = NULL;
        end->next = link;
}

/******************************************************************************\
 Remove a CLink object from the linked list its in, if any.
\******************************************************************************/
void CLink_remove(CLink *link)
{
        if (!link->root)
                return;
        if (link->next)
                link->next->prev = link->prev;
        if (link->prev) {
                C_assert(*link->root != link);
                link->prev->next = link->next;
        } else {
                C_assert(*link->root == link);
                *link->root = link->next;
        }
        link->prev = link->next = NULL;
        link->root = NULL;
}

/******************************************************************************\
 Move a link back/forward in the list.
\******************************************************************************/
void CLink_back(CLink *link)
{
        if (!link->next)
                return;
        if ((link->next->prev = link->prev))
                link->prev->next = link->next;
        else
                *link->root = link->next;
        link->prev = link->next;
        if ((link->next = link->prev->next))
                link->next->prev = link;
        link->prev->next = link;
}

void CLink_forward(CLink *link)
{
        if (!link->prev)
                return;
        if ((link->prev->next = link->next))
                link->next->prev = link->prev;
        link->next = link->prev;
        if (!(link->prev = link->next->prev))
                *link->root = link;
        else
                link->prev->next = link;
        link->next->prev = link;
}

