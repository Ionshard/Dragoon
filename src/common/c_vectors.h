/******************************************************************************\
 Dragoon - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Vector type declarations and static inline definitions for the vector
   operations. Requires math.h to be included. */

#pragma pack(push, 4)

typedef struct {
        GLfloat x, y;
} CVec;

typedef struct CColor {
        GLfloat r, g, b, a;
} CColor;

#pragma pack(pop)

/* Vector types can be converted to float arrays and vice versa */
#define C_ARRAYF(v) ((GLfloat *)&(v))

/******************************************************************************\
 Construct a vector from x/y parameters.
\******************************************************************************/
#define CVec(x, y) CVec_xy(x, y)
#define CVec_one() CVec_xy(1.f, 1.f)
#define CVec_zero() CVec_xy(0.f, 0.f)
static inline CVec CVec_xy(float x, float y)
{
        CVec result = { x, y };
        return result;
}

/******************************************************************************\
 Construct a random vector.
\******************************************************************************/
static inline CVec CVec_randf(float rx, float ry)
{
        return CVec_xy(rx * (2.f * rand() / RAND_MAX - 1),
                       ry * (2.f * rand() / RAND_MAX - 1));
}

static inline CVec CVec_rand(CVec r)
{
        return CVec_randf(r.x, r.y);
}

/******************************************************************************\
 Construct a color from 0-1 ranged integer values.
\******************************************************************************/
#define CColor(r, g, b, a) CColor_rgba(r, g, b, a)
#define CColor_white() CColor_rgba(1.f, 1.f, 1.f, 1.f)
#define CColor_black() CColor_rgba(0.f, 0.f, 0.f, 1.f)
#define CColor_trans() CColor_rgba(0.f, 0.f, 0.f, 0.f)
static inline CColor CColor_rgba(float r, float g, float b, float a)
{
        CColor result = { r, g, b, a };
        return result;
}

/******************************************************************************\
 Component-wise binary operators with another vector.
\******************************************************************************/
static inline CVec CVec_add(CVec a, CVec b)
{
        return CVec_xy(a.x + b.x, a.y + b.y);
}

static inline CVec CVec_sub(CVec a, CVec b)
{
        return CVec_xy(a.x - b.x, a.y - b.y);
}

static inline CVec CVec_scale(CVec a, CVec b)
{
        return CVec_xy(a.x * b.x, a.y * b.y);
}

static inline CVec CVec_div(CVec a, CVec b)
{
        return CVec_xy(a.x / b.x, a.y / b.y);
}

static inline CColor CColor_add(CColor a, CColor b)
{
        return CColor_rgba(a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a);
}

static inline CColor CColor_sub(CColor a, CColor b)
{
        return CColor_rgba(a.r - b.r, a.g - b.g, a.b - b.b, a.a - b.a);
}

static inline CColor CColor_scale(CColor a, CColor b)
{
        return CColor_rgba(a.r * b.r, a.g * b.g, a.b * b.b, a.a * b.a);
}

static inline CColor CColor_div(CColor a, CColor b)
{
        return CColor_rgba(a.r / b.r, a.g / b.g, a.b / b.b, a.a / b.a);
}

/******************************************************************************\
 Binary operators with a scalar.
\******************************************************************************/
static inline CVec CVec_addf(CVec a, float f)
{
        return CVec_xy(a.x + f, a.y + f);
}

static inline CVec CVec_subf(CVec a, float f)
{
        return CVec_xy(a.x - f, a.y - f);
}

static inline CVec CVec_scalef(CVec a, float f)
{
        return CVec_xy(a.x * f, a.y * f);
}

static inline CVec CVec_divf(CVec a, float f)
{
        return CVec_xy(a.x / f, a.y / f);
}

static inline CColor CColor_addf(CColor a, float f)
{
        return CColor_rgba(a.r + f, a.g + f, a.b + f, a.a + f);
}

static inline CColor CColor_subf(CColor a, float f)
{
        return CColor_rgba(a.r - f, a.g - f, a.b - f, a.a - f);
}

static inline CColor CColor_scalef(CColor a, float f)
{
        return CColor_rgba(a.r * f, a.g * f, a.b * f, a.a * f);
}

static inline CColor CColor_mod(CColor a, float f)
{
        return CColor_rgba(a.r * f, a.g * f, a.b * f, a.a);
}

static inline CColor CColor_divf(CColor a, float f)
{
        return CColor_rgba(a.r / f, a.g / f, a.b / f, a.a / f);
}

/******************************************************************************\
 Vector dot and cross products.
\******************************************************************************/
static inline float CVec_dot(CVec a, CVec b)
{
        return a.x * b.x + a.y * b.y;
}

static inline float CVec_cross(CVec a, CVec b)
{
        return a.y * b.x - a.x * b.y;
}

/******************************************************************************\
 Vector dimensions squared and summed. That is, magnitude without the square
 root operation.
\******************************************************************************/
static inline float CVec_square(CVec p)
{
        return p.x * p.x + p.y * p.y;
}

/******************************************************************************\
 Vector magnitude/length.
\******************************************************************************/
static inline float CVec_len(CVec p)
{
        return (float)sqrt(CVec_square(p));
}

/******************************************************************************\
 Vector normalization.
\******************************************************************************/
static inline CVec CVec_norm(CVec p)
{
        return CVec_divf(p, CVec_len(p));
}

/******************************************************************************\
 Vector comparison.
\******************************************************************************/
static inline int CVec_eq(CVec a, CVec b)
{
        return a.x == b.x && a.y == b.y;
}

static inline int CVec_geq(CVec a, CVec b)
{
        return a.x >= b.x && a.y >= b.y;
}

static inline int CVec_gt(CVec a, CVec b)
{
        return a.x > b.x && a.y > b.y;
}

static inline int CVec_leq(CVec a, CVec b)
{
        return a.x <= b.x && a.y <= b.y;
}

static inline int CVec_lt(CVec a, CVec b)
{
        return a.x < b.x && a.y < b.y;
}

/******************************************************************************\
 Vector interpolation.
\******************************************************************************/
static inline CVec CVec_lerp(CVec a, float lerp, CVec b)
{
        return CVec_xy(a.x + lerp * (b.x - a.x), a.y + lerp * (b.y - a.y));
}

static inline CColor CColor_lerp(CColor a, float lerp, CColor b)
{
        return CColor_rgba(a.r + lerp * (b.r - a.r),
                       a.g + lerp * (b.g - a.g),
                       a.b + lerp * (b.b - a.b),
                       a.a + lerp * (b.a - a.a));
}

/******************************************************************************\
 Truncate vector values down to whole numbers. Negative values are also
 truncated down (-2.1 is rounded to -3).
\******************************************************************************/
static inline float C_clamp(float value, float unit)
{
        return value < 0 ? (int)(value / -unit) * -unit :
                           (int)(value / unit) * unit;
}

static inline CVec CVec_clamp(CVec v, float u)
{
        return CVec_xy(C_clamp(v.x, u), C_clamp(v.y, u));
}

/******************************************************************************\
 Returns the vector with absolute valued members.
\******************************************************************************/
static inline CVec CVec_abs(CVec v)
{
        if (v.x < 0.f)
                v.x = -v.x;
        if (v.y < 0.f)
                v.y = -v.y;
        return v;
}

/******************************************************************************\
 Returns the vector with the min/max members of each vector.
\******************************************************************************/
static inline CVec CVec_min(CVec a, CVec b)
{
        return CVec_xy(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y);
}

static inline CVec CVec_max(CVec a, CVec b)
{
        return CVec_xy(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y);
}

/******************************************************************************\
 Returns TRUE if the two rectangles intersect.
\******************************************************************************/
static inline int CVec_intersect(CVec o1, CVec s1, CVec o2, CVec s2)
{
        return o1.x < o2.x + s2.x && o1.y < o2.y + s2.y &&
               o1.x + s1.x > o2.x && o1.y + s1.y > o2.y;
}

/******************************************************************************\
 Returns TRUE if the rectangle contains the point.
\******************************************************************************/
static inline int CVec_contains(CVec o, CVec s, CVec p)
{
        return p.x >= o.x && p.y >= o.y && p.x < o.x + s.x && p.y < o.y + s.y;
}

/******************************************************************************\
 Slope of the vector -- rise over run.
\******************************************************************************/
static inline float CVec_slope(CVec v)
{
        if (!v.x)
                return 10000;
        return v.y / v.x;
}

/******************************************************************************\
 Angle of the vector.
\******************************************************************************/
static inline float CVec_angle(CVec v)
{
        return atan2f(v.y, v.x);
}

/******************************************************************************\
 Return the vector component in [dir].
\******************************************************************************/
static inline CVec CVec_in(CVec v, CVec dir)
{
        return CVec_scalef(dir, CVec_dot(v, dir));
}

/******************************************************************************\
 Average two vectors.
\******************************************************************************/
static inline CVec CVec_avg(CVec a, CVec b)
{
        return CVec_scalef(CVec_add(a, b), 0.5f);
}

/******************************************************************************\
 Limit a color to valid range.
\******************************************************************************/
static inline CColor CColor_limit(CColor c)
{
        int i;

        for (i = 0; i < 4; i++) {
                if (C_ARRAYF(c)[i] < 0.f)
                        C_ARRAYF(c)[i] = 0.f;
                if (C_ARRAYF(c)[i] > 1.f)
                        C_ARRAYF(c)[i] = 1.f;
        }
        return c;
}

/******************************************************************************\
 Blend a color onto another color.
\******************************************************************************/
static inline CColor CColor_blend(CColor dest, CColor src)
{
        src.r *= src.a;
        src.g *= src.a;
        src.b *= src.a;
        return CColor_add(CColor_scalef(dest, 1.f - src.a), src);
}

/******************************************************************************\
 Returns color luma ranging from 0 to 1.
\******************************************************************************/
static inline float CColor_luma(CColor color)
{
        return color.a * (0.21f * color.r + 0.72f * color.g + 0.07f * color.b);
}

/******************************************************************************\
 Construct a color from 0-255 ranged integer values.
\******************************************************************************/
static inline CColor CColor_bytes(unsigned char r, unsigned char g,
                                  unsigned char b, unsigned char a)
{
        CColor result = { r, g, b, a };
        return CColor_divf(result, 255.f);
}

/******************************************************************************\
 Construct a color from a 32-bit ARGB integer.
\******************************************************************************/
static inline CColor CColor_32(unsigned int v)
{
        return CColor_rgba((v & 0xff0000) >> 16, (v & 0xff00) >> 8, v & 0xff,
                           (v & 0xff000000) >> 24);
}

