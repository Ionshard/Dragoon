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
 Free a named object.
\******************************************************************************/
void CNamed_free(CNamed *named)
{
        if (!named)
                return;
        if (named->cleanupFunc)
                named->cleanupFunc(named);
        C_free(named);
}

/******************************************************************************\
 Allocate a named object in a linked list. If it is already in the list, NULL
 is returned.
\******************************************************************************/
void *CNamed_alloc_full(const char *func, CNamed **root, const char *name,
                        int size, CCallback cleanupFunc, CNamedPolicy policy)
{
        CNamed *named, *cur, *last;

        C_assert(size >= sizeof (CNamed));

        /* Go through the list */
        last = NULL;
        cur = *root;
        while (cur) {
                int cmp;

                if (!cur->name)
                        break;
                if (name && !(cmp = strcmp(cur->name, name))) {
                        if (policy == CNP_NULL)
                                return NULL;
                        else if (policy == CNP_OVERWRITE) {
                                cur = (named = cur)->next;
                                CNamed_free(named);
                                break;
                        } else if (policy == CNP_RETURN) {
                                if (cur->size < size)
                                        C_error("Fetched object too small");
                                return cur;
                        }
                        C_error("Invalid policy %d", policy);
                }
                if (name && cmp > 0)
                        break;
                last = cur;
                cur = cur->next;
        }

        /* Allocate a new object */
        named = C_recalloc_full(func, NULL, size);
        named->size = size;
        named->next = cur;
        named->cleanupFunc = cleanupFunc;
        if (last)
                last->next = named;
        else
                *root = named;
        C_strncpy_buf(named->name, name);
        return named;
}

/******************************************************************************\
 Get a named object from a linked list. If it is not in the list, NULL is
 returned.
\******************************************************************************/
void *CNamed_get(CNamed *cur, const char *name)
{
        int cmp;

        for (cmp = -1; cur && cmp <= 0; cur = cur->next)
                if (!(cmp = strcmp(cur->name, name)))
                        return cur;
        return NULL;
}

/******************************************************************************\
 Free the objects in a named linked list.
\******************************************************************************/
void CNamed_freeAll(CNamed **root)
{
        CNamed *next;

        if (!*root)
                return;
        while (*root) {
                next = (*root)->next;
                CNamed_free(*root);
                *root = next;
        }
        *root = NULL;
}

