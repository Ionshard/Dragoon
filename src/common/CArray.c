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
 Initialize an array.
\******************************************************************************/
void CArray_init(CArray *array, size_t size, int cap)
{
        if (!array)
                return;
        array->size = (int)size;
        array->len = 0;
        if (cap < 0)
                cap = 0;
        array->cap = cap;
        array->data = cap > 0 ? C_malloc(cap * size) : 0;
}

/******************************************************************************\
 Clean up after the array.
\******************************************************************************/
void CArray_cleanup(CArray *array)
{
        if (!array)
                return;
        C_free(array->data);
        memset(array, 0, sizeof (*array));
}

/******************************************************************************\
 Adjust capacity so that enough space is allocated for [n] elements.
\******************************************************************************/
void CArray_reserve(CArray *array, int n)
{
        if (!array)
                return;
        if (n < array->len)
                n = array->len;
        if (array->cap == n)
                return;
        array->data = C_realloc(array->data, array->size * n);
        array->cap = n;
}

/******************************************************************************\
 Set the value of an array using memory copying. If the type is known it is
 preferable to use CArray_set().
\******************************************************************************/
void CArray_set_copy(CArray *array, int index, void *item)
{
        if (!array || !item)
                return;
        if (index < 0 || index >= array->len)
                C_error("Invalid index array");
        memcpy((char *)array->data + index * array->size, item, array->size);
}

/******************************************************************************\
 Append something to an array. [item] may be NULL or point to a structure of
 size [size] to copy into the new location. Returns the new item's index.
\******************************************************************************/
int CArray_append(CArray *array, void *item)
{
        if (!array)
                return -1;
        if (array->len >= array->cap) {
                if (array->len > array->cap)
                        C_error("Invalid array");
                if (array->cap < 1)
                        array->cap = 1;
                CArray_reserve(array, array->cap * 2);
        }
        CArray_set_copy(array, ++array->len - 1, item);
        return array->len - 1;
}

/******************************************************************************\
 Insert an element into the array. [item] may be NULL or point to a structure
 of size [size] to copy into the new location.
\******************************************************************************/
void CArray_insert(CArray *array, int index, void *item)
{
        if (!array)
                return;
        if (index < 0 || index > array->len)
                C_error("Invalid array index");
        if (index == array->len) {
                CArray_append(array, item);
                return;
        }
        CArray_append(array, NULL);
        memmove((char *)array->data + (index + 1) * array->size,
                (char *)array->data + index * array->size,
                (array->len - index) * array->size);
        CArray_set_copy(array, index, item);
}

/******************************************************************************\
 Remove an element from the array. Makes sure the array capacity is no
 greater than twice the array length.
\******************************************************************************/
void CArray_remove(CArray *array, int index)
{
        if (!array)
                return;
        if (index < 0 || index > array->len)
                C_error("Invalid array index");
        memmove((char *)array->data + index * array->size,
                (char *)array->data + (index + 1) * array->size,
                (--array->len - index) * array->size);
        if (array->cap > 2 * array->len)
                CArray_reserve(array, 2 * array->len);
}

/******************************************************************************\
 Reallocate the memory so the array doesn't have empty space at the end and
 return the pointer to the data. The array itself is cleaned up.
\******************************************************************************/
void *CArray_steal(CArray *array)
{
        void *result;

        if (!array)
                return NULL;
        result = C_realloc(array->data, array->len * array->size);
        C_zero(array);
        return result;
}

/******************************************************************************\
 Sort an array using standard library quicksort.
\******************************************************************************/
void CArray_sort(CArray *array, int (*compare)(const void *, const void *))
{
        if (!array)
                return;
        qsort(array->data, array->len, array->size, compare);
}

