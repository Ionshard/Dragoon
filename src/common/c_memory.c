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

/* When memory checking is enabled, this structure is prepended to every
   allocated block. There is also a no-mans-land chunk, filled with a specific
   byte, at the end of every allocated block as well. */
#define noMansLand_BYTE 0x5a
#define noMansLand_SIZE 64
typedef struct MemTag {
        struct MemTag *next;
        const char *allocFunc, *freeFunc;
        void *data;
        size_t size;
        bool freed;
        char noMansLand[noMansLand_SIZE];
} MemTag;

#if CHECKED
static MemTag *memRoot;
static size_t memBytes, memBytesMax;
static int memCalls;
#endif

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

/******************************************************************************\
 Get a named object from a linked list. If it is not in the list, a new entry
 can be added of [size]. Pass a zero or negative [size] to prevent creating
 a new object.
\******************************************************************************/
void *CNamed_get_full(const char *func, CNamed **root,
                      const char *name, int size)
{
        CNamed *named, *cur, *last;

        C_assert(size < 1 || size >= sizeof (CNamed));

        /* Go through the list */
        last = NULL;
        cur = *root;
        while (cur) {
                int cmp;

                if (!(cmp = strcmp(cur->name, name)))
                        return cur;
                if (cmp > 0)
                        break;
                last = cur;
                cur = cur->next;
        }

        /* Allocate a new object */
        if (size < 1)
                return NULL;
        named = C_calloc(size);
        named->next = cur;
        if (last)
                last->next = named;
        else
                *root = named;
        C_strncpy_buf(named->name, name);
        return named;
}

/******************************************************************************\
 Free the objects in a named linked list.
\******************************************************************************/
void CNamed_free(CNamed **root, CCallback cleanupFunc)
{
        CNamed *next;

        if (!*root)
                return;
        while (*root) {
                next = (*root)->next;
                if (cleanupFunc)
                        cleanupFunc(*root);
                C_free(*root);
                *root = next;
        }
        *root = NULL;
}

/******************************************************************************\
 Allocates new memory, similar to C_realloc_full().
\******************************************************************************/
#if CHECKED
static void *malloc_full(const char *func, size_t size)
{
        MemTag *tag;
        size_t realSize;

        realSize = size + sizeof (MemTag) + noMansLand_SIZE;
        tag = malloc(realSize);
        tag->data = (char *)tag + sizeof (MemTag);
        tag->size = size;
        tag->allocFunc = func;
        tag->freed = FALSE;
        memset(tag->noMansLand, noMansLand_BYTE, noMansLand_SIZE);
        memset((char *)tag->data + size, noMansLand_BYTE, noMansLand_SIZE);
        tag->next = NULL;
        if (memRoot)
                tag->next = memRoot;
        memRoot = tag;
        memBytes += size;
        memCalls++;
        if (memBytes > memBytesMax)
                memBytesMax = memBytes;
        return tag->data;
}
#endif

/******************************************************************************\
 Finds the memory tag that holds [ptr].
\******************************************************************************/
#if CHECKED
static MemTag *findTag(const void *ptr, MemTag **prevTag)
{
        MemTag *tag, *prev;

        prev = NULL;
        tag = memRoot;
        while (tag && tag->data != ptr) {
                prev = tag;
                tag = tag->next;
        }
        if (prevTag)
                *prevTag = prev;
        return tag;
}
#endif

/******************************************************************************\
 Reallocate [ptr] to [size] bytes large. Abort on error. String all the
 allocated chunks into a linked list and tracks information about the
 memory and where it was allocated from. This is used later in C_free() and
 C_check_leaks() to detect various errors.
\******************************************************************************/
#if CHECKED
void *C_realloc_full(const char *func, void *ptr, size_t size)
{
        MemTag *tag, *prevTag, *oldTag;
        size_t realSize;

        if (!ptr)
                return malloc_full(func, size);
        if (!(tag = findTag(ptr, &prevTag)))
                C_log(CLL_ERROR, func,
                      "Trying to reallocate unallocated address (0x%x)", ptr);
        oldTag = tag;
        realSize = size + sizeof (MemTag) + noMansLand_SIZE;
        tag = realloc((char *)ptr - sizeof (MemTag), realSize);
        if (!tag)
                C_error("Out of memory, %s() tried to allocate %d bytes",
                        func, size );
        if (prevTag)
                prevTag->next = tag;
        if (oldTag == memRoot)
                memRoot = tag;
        memBytes += size - tag->size;
        if (size > tag->size) {
                memCalls++;
                if (memBytes > memBytesMax)
                        memBytesMax = memBytes;
        }
        tag->size = size;
        tag->allocFunc = func;
        tag->data = (char *)tag + sizeof (MemTag);
        memset((char *)tag->data + size, noMansLand_BYTE, noMansLand_SIZE);
        return tag->data;
}
#endif

/******************************************************************************\
 Allocate zero'd memory.
\******************************************************************************/
#if CHECKED
void *C_recalloc_full(const char *func, void *ptr, size_t size)
{
        ptr = C_realloc_full(func, ptr, size);
        memset(ptr, 0, size);
        return ptr;
}
#endif

/******************************************************************************\
 Checks if a no-mans-land region has been corrupted.
\******************************************************************************/
#if CHECKED
static int checkNoMansLand(const char *ptr)
{
        int i;

        for (i = 0; i < noMansLand_SIZE; i++)
                if (ptr[i] != noMansLand_BYTE)
                        return FALSE;
        return TRUE;
}
#endif

/******************************************************************************\
 Frees memory. If memory checking is enabled, will check the following:
 - [ptr] was never allocated
 - [ptr] was already freed
 - [ptr] no-mans-land (upper or lower) was corrupted
\******************************************************************************/
#if CHECKED
void C_free_full(const char *func, void *ptr)
{
        MemTag *tag, *prevTag, *oldTag;

        if (!ptr)
                return;
        if (!(tag = findTag(ptr, &prevTag)))
                C_log(CLL_ERROR, func,
                      "Trying to free unallocated address (0x%x)", ptr);
        if (tag->freed)
                C_log(CLL_ERROR, func,
                      "Address (0x%x), %d bytes allocated by %s(), already "
                      "freed by %s()",
                      ptr, tag->size, tag->allocFunc, tag->freeFunc);
        if (!checkNoMansLand(tag->noMansLand))
                C_log(CLL_ERROR, func,
                      "Address (0x%x), %d bytes allocated by "
                      "%s(), overran lower boundary",
                      ptr, tag->size, tag->allocFunc);
        if (!checkNoMansLand((char *)ptr + tag->size))
                C_log(CLL_ERROR, func,
                      "Address (0x%x), %d bytes allocated by "
                      "%s(), overran upper boundary",
                      ptr, tag->size, tag->allocFunc);
        tag->freed = TRUE;
        tag->freeFunc = func;
        oldTag = tag;
        tag = realloc(tag, sizeof (*tag));
        if (prevTag)
                prevTag->next = tag;
        if (oldTag == memRoot)
                memRoot = tag;
        memBytes -= tag->size;
}
#endif

/******************************************************************************\
 If memory checking is enabled, checks for memory leaks and prints warnings.
\******************************************************************************/
#if CHECKED
void C_checkLeaks(void)
{
        MemTag *tag;
        int tags;

        tags = 0;
        for (tag = memRoot; tag; tag = tag->next) {
                unsigned int i;

                tags++;
                if (tag->freed)
                        continue;
                C_warning("%s() leaked %d bytes (0x%08x)",
                          tag->allocFunc, tag->size, tag->data);

                /* If we leaked a string, we can print it */
                if (tag->size < 1)
                        continue;
                for (i = 0; C_isPrint(((char *)tag->data)[i]); i++) {
                        char buf[128];

                        if (i >= tag->size - 1 || i >= sizeof (buf) - 1 ||
                            !((char *)tag->data)[i + 1]) {
                                C_strncpy_buf(buf, tag->data);
                                C_debug("Looks like a string: '%s'", buf);
                                break;
                        }
                }
        }
        C_debug("%d allocation calls, high mark %.1fmb, %d tags",
                memCalls, memBytesMax / 1048576.f, tags);
}
#endif

/******************************************************************************\
 Run some test to see if memory checking actually works. Note that code here
 is intentionally poor, so do not fix "bugs" here they are there for a reason.
\******************************************************************************/
#if CHECKED
void C_testMemCheck(int test)
{
        char *ptr;
        int i;

        switch (test) {
        case 0:
        case 1: return;
        case 2: C_debug("Normal operation, shouldn't fail");
                ptr = C_malloc(1024);
                C_free(ptr);
                ptr = C_calloc(1024);
                C_realloc(ptr, 2048);
                C_realloc(ptr, 512);
                C_free(ptr);
                return;
        case 3: C_debug("Intentionally leaking memory");
                ptr = C_malloc(1024);
                return;
        case 4: C_debug("Freeing unallocated memory");
                C_free((void *)0x12345678);
                break;
        case 5: C_debug("Double freeing memory");
                ptr = C_malloc(1024);
                C_free(ptr);
                C_free(ptr);
                break;
        case 6: C_debug("Simulating memory underrun");
                ptr = C_malloc(1024);
                for (i = 0; i > -noMansLand_SIZE / 2; i--)
                        ptr[i] = 42;
                C_free(ptr);
                break;
        case 7: C_debug("Simulating memory overrun");
                ptr = C_malloc(1024);
                for (i = 1024; i < 1024 + noMansLand_SIZE / 2; i++)
                        ptr[i] = 42;
                C_free(ptr);
                break;
        case 8: C_debug("Reallocating unallocated memory");
                ptr = C_realloc((void *)0x12345678, 1024);
                break;
        case 9: C_debug("Intentionally leaking string");
                ptr = C_malloc(1024);
                strncpy(ptr, "This string was leaked", 1024);
                return;
        default:
                C_error("Unknown memory check test %d", test);
        }
        C_error("Memory check test %d failed", test);
}
#endif

