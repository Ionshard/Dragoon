/******************************************************************************\
 Dragoon - Copyright (C) 2010 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#pragma once

namespace dragoon {
namespace ptr {

/** Free pointer */
template<class T> void Free(T* ptr) { free(ptr); }

/** Delete object */
template<class T> void Delete(T* ptr) { delete ptr; }

/** Delete object array */
template<class T> void DeleteArray(T* ptr) { delete[] ptr; }

/** Base class that can be dereferenced into a wrapped pointer type */
template<class T> class Wrap {
public:

  /** Create reference */
  Wrap(T* ptr = 0): ptr_(ptr) {}

  /** Assignment from raw pointer type */
  void operator=(T* ptr) { ptr_ = ptr; }

  /** Scope pointer can be cast to type */
  operator T*() const { return ptr_; }

  /** Can be dereferenced */
  T& operator*() const { return *ptr_; }

  /** Can be member dereferenced */
  T* operator->() const { return ptr_; }

  /** Can be used as an array */
  T& operator[](int i) const { return ptr_[i]; }

protected:
  T* ptr_;
};

/** Pointer class that owns a raw pointer and its memory and deletes it
    when it goes out of scope. Do not put scoped classes into containers,
    they will be double-free'd! Used scoped containers instead. */
template<class T, void (*F)(T*) = ptr::Delete<T> > class Scope: public Wrap<T> {
public:
  using Wrap<T>::ptr_;

  /** Scoped vector container */
  class Vector: public std::vector<T*> {
  public:
    ~Vector() {
      for (typename std::vector<T*>::iterator
           it = std::vector<T*>::begin(), end = std::vector<T*>::end();
           it != end; ++it)
        F(*it);
    }
  };

  /** Scoped map container */
  template <class K> class Map: public std::map<K, T*> {
  public:
    ~Map() {
      for (typename std::map<K, T*>::iterator
           it = std::map<K, T*>::begin(), end = std::map<K, T*>::end();
           it != end; ++it)
        F(it->second);
    }
  };

  /** Create reference */
  Scope(T* ptr = 0): Wrap<T>(ptr) {}

  /** Owned memory is deleted when scope pointer moves out of scope */
  ~Scope() { Release(); }

  /** Release pointer */
  void Release() {
    F(ptr_);
    ptr_ = NULL;
  }

  /** Assignment from raw pointer type does not free current pointer */
  void operator=(T* ptr) { ptr_ = ptr; }
};

} // namespace ptr
} // namespace dragoon
