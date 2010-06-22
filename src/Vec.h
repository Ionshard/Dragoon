/******************************************************************************\
 Dragoon - Copyright (C) 2009 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#pragma once

namespace dragoon {

/** Generic vector class */
#pragma pack(push, 4)
template<int D> class VecBase {
public:

// Generic component-wise vector operations
#define COMMON_VECBASE__OP(OP)\
  VecBase operator OP(const VecBase<D>& v) const {\
    VecBase r;\
    for (int i = 0; i < D; ++i)\
      r[i] = v_[i] OP v.v_[i];\
    return r;\
  }\
  VecBase& operator OP##=(const VecBase<D>& v) {\
    for (int i = 0; i < D; ++i)\
      v_[i] OP##= v.v_[i];\
    return *this;\
  }\
  VecBase operator OP(float f) const {\
    VecBase r;\
    for (int i = 0; i < D; ++i)\
      r[i] = v_[i] OP f;\
    return r;\
  }\
  VecBase& operator OP##=(float f) {\
    for (int i = 0; i < D; ++i)\
      v_[i] OP##= f;\
    return *this;\
  }\
  friend VecBase operator OP(float f, const VecBase<D>& v) {\
    VecBase r;\
    for (int i = 0; i < D; ++i)\
      r[i] = f OP v.v_[i];\
    return r;\
  }
  COMMON_VECBASE__OP(+)
  COMMON_VECBASE__OP(-)
  COMMON_VECBASE__OP(*)
  COMMON_VECBASE__OP(/)
#undef COMMON_VECBASE__OP

  // Equality and inequality are special cases
  bool operator ==(const VecBase<D>& v) const {
    for (int i = 0; i < D; ++i)
      if (v_[i] != v.v_[i])
        return false;
    return true;
  }
  bool operator ==(float f) const {
    for (int i = 0; i < D; ++i)
      if (v_[i] != f)
        return false;
    return true;
  }
  bool operator !=(const VecBase<D>& v) const {
    for (int i = 0; i < D; ++i)
      if (v_[i] == v.v_[i])
        return false;
    return true;
  }
  bool operator !=(float f) const {
    for (int i = 0; i < D; ++i)
      if (v_[i] == f)
        return false;
    return true;
  }

  /** Generic dot product */
  float Dot(const VecBase& v) const {
    float f = 0;
    for (int i = 0; i < D; ++i)
      f += v_[i] * v.v_[i];
    return f;
  }

  /** Generic norm */
  float Len() const {
    float f = 0;
    for (int i = 0; i < D; ++i)
      f += v_[i] * v_[i];
    return sqrtf(f);
  }

  /** Tests if vector is all zeros */
  bool Zero() {
    for (int i = 0; i < D; ++i)
      if (v_[i])
        return true;
    return false;
  }

  /** String representation */
  std::string ToString() {
    std::string s = "{";
    s.reserve(D * 8 + 2);
    char buf[32];
    for (int i = 0; i < D; ++i) {
      snprintf(buf, sizeof (buf), "%g", v_[i]);
      s.append(buf);
      if (i < D - 1)
        s.append(", ");
    }
    s.append("}");
    return s;
  }

  /** Const accessor */
  float get(unsigned int i) const { return v_[i]; }

  /** Return float array */
  float* array() { return v_; }

  /** Element accessor */
  float& operator[](int i) { return v_[i]; }

protected:
  VecBase() {}

  float v_[D];
};
#pragma pack(pop)

/** Generic vector class */
template<int D> class Vec: public VecBase<D> {
public:
  Vec() {
    for (int i = 0; i < D; ++i)
      VecBase<D>::v_[i] = 0;
  }

  /** Compatibility with base class */
  Vec(const VecBase<D>& v) {
    for (int i = 0; i < D; ++i)
      VecBase<D>::v_[i] = v.get[i];
  }
};

/** 2D vector class */
template<> class Vec<2>: public VecBase<2> {
public:
  Vec(float x = 0, float y = 0) {
    v_[0] = x;
    v_[1] = y;
  }

  /** Compatibility with base class */
  Vec(const VecBase<2>& v) {
    v_[0] = v.get(0);
    v_[1] = v.get(1);
  }

  /** Named accessors */
  float x() const { return v_[0]; }
  float y() const { return v_[1]; }
};

/** RGBA color class */
class Color: public Vec<4> {
public:

  /** Create color from unit-ranged values */
  Color(float r = 0, float g = 0, float b = 0, float a = 0) {
    v_[0] = r;
    v_[1] = g;
    v_[2] = b;
    v_[3] = a;
  }

  /** Compatibility with base class */
  Color(const VecBase<4>& v) {
    v_[0] = v.get(0);
    v_[1] = v.get(1);
    v_[2] = v.get(2);
    v_[3] = v.get(3);
  }

  /** Named accessor */
  float r() const { return v_[0]; }
  float g() const { return v_[1]; }
  float b() const { return v_[2]; }
  float a() const { return v_[3]; }

  /** Select for use with alpha-blended OpenGL */
  void Select() { glColor4f(v_[0], v_[1], v_[2], v_[3]); }

  /** Select for use with additive OpenGL */
  void SelectAdd()
    { glColor4f(v_[0] * v_[3], v_[1] * v_[3], v_[2] * v_[3], 1); }

  /** Blend onto another color */
  Color Blend(Color bg) {
    float w = (1 - v_[3]) * bg.v_[3];
    float a = v_[3] + w;
    if (a)
      return Color((v_[0] * v_[3] + w * bg.v_[0]) / a,
                   (v_[1] * v_[3] + w * bg.v_[1]) / a,
                   (v_[2] * v_[3] + w * bg.v_[2]) / a, a);
    return Color(0, 0, 0, 0);
  }

  /** Named colors */
  static Color black() { return Color(0, 0, 0, 1); }
  static Color white() { return Color(1, 1, 1, 1); }
  static Color none() { return Color(0, 0, 0, 0); }
};

} // namespace dragoon
