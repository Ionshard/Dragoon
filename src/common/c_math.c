/******************************************************************************\
 Dragoon - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Math functions not provided by the standard library */

#include "c_private.h"

/******************************************************************************\
 Calculate the next power of two for any integer.
\******************************************************************************/
int C_nextPow2(int n)
{
        int p;

        if (n < 1)
                return 0;
        for (p = 1; p < n; p <<= 1);
        return p;
}

/******************************************************************************\
 Convenience function to ensure a value is within a range.
\******************************************************************************/
void C_limit_float(float *value, float min, float max)
{
        if (!value)
                return;
        if (*value < min)
                *value = min;
        if (*value > max)
                *value = max;
}

void C_limit_int(int *value, int min, int max)
{
        if (!value)
                return;
        if (*value < min)
                *value = min;
        if (*value > max)
                *value = max;
}

/******************************************************************************\
 Dice function for DnD nerds.
\******************************************************************************/
int C_rollDice(int num, int sides)
{
        int i, total;

        for (total = 0, i = 0; i < num; i++)
                total += 1 + rand() % sides;
        return total;
}

/******************************************************************************\
 Solve the quadratic equation in a numerically stable way. The first solution
 (x1) will always be the farthest from zero.
 http://en.wikipedia.org/wiki/Quadratic_equation#Floating_point_implementation
\******************************************************************************/
void C_quadratic(float a, float b, float c, float *x1, float *x2)
{
        float t, bSign;

        bSign = -(b < 0);
        t = (b + bSign * sqrtf(b * b - 4 * a * c)) / -2;
        if (x1)
                *x1 = t / a;
        if (x2)
                *x2 = t ? c / t : 0;
}

/******************************************************************************\
 Convenience function to gradually fade values from 0 to 1. Returns TRUE if
 the fade value is non-zero.
\******************************************************************************/
bool C_fade_full(float *value, float from, float to, bool up, float rate)
{
        if (up) {
                if ((*value += rate * c_frameSec) > to)
                        *value = to;
        } else if ((*value -= rate * c_frameSec) <= from) {
                *value = from;
                return FALSE;
        }
        return TRUE;
}

/******************************************************************************\
 Deterministic random number generator.
\******************************************************************************/
int C_randDet(int last)
{
        return (1664525 * last + 1013904223) % RAND_MAX;
}

/******************************************************************************\
 Update an exponentially decaying shake.
\******************************************************************************/
CVec C_shake(float *amount, float decay, float sec)
{
        CVec v;

        v = CVec_randf(*amount, *amount);
        if ((*amount /= 1 + decay * sec) < 0.01)
                *amount = 0;
        return v;
}

