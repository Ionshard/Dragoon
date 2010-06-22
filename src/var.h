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
#include "ptr.h"

namespace dragoon {
namespace var {

/** Basic string class for storing configuration-file variables */
class String {
public:

  /** Initializes a variable class and inserts it into variable map
   *  @param name     Name used to fetch variable
   *  @param value    Default value
   *  @param comment  Comment attached to the variable
   */
  String(const char* name, const char* value = NULL,
         const char* comment = NULL);

  /** If a variable goes out of scope, it is removed from the database */
  ~String();

  /** Returns the last string assigned to this variable */
  virtual const char* c_str() { return string_; }

  /** Clear value */
  virtual void Clear();

  /** Write variable out to a configuration file */
  virtual void Write(FILE*);

  /** Variables own their strings */
  virtual String& operator=(const char*);

protected:
  const char* name_;
  const char* comment_;
  char* string_;
  char* string_default_;
};

/** Floating-point class for storing configuration-file variables */
class Float: public String {
public:

  /** Initializes a variable class and inserts it into variable map
   *  @param name     Name used to fetch variable
   *  @param value    Default value
   *  @param comment  Comment attached to the variable
   */
  Float(const char* name, float value = 0, const char* comment = NULL):
    String(name, NULL, comment), float_(value), float_default_(value) {}

  /** Returns the last string assigned to this variable */
  virtual const char* c_str();

  /** Clear value */
  virtual void Clear();

  /** Write variable out to a configuration file */
  virtual void Write(FILE*);

  /** Variables own their strings */
  virtual Float& operator=(const char*);

  /** Does not regenerate the string representation until c_str() is called */
  virtual Float& operator=(float);

  /** Cast to value */
  operator float() { return float_; }

private:
  float float_;
  float float_default_;
};

/** Integer class for storing configuration-file variables */
class Int: public String {
public:

  /** Initializes a variable class and inserts it into variable map
   *  @param name     Name used to fetch variable
   *  @param value    Default value
   *  @param comment  Comment attached to the variable
   */
  Int(const char* name, int value = 0, const char* comment = NULL):
    String(name, NULL, comment), int_(value), int_default_(value) {}

  /** Returns the last string assigned to this variable */
  virtual const char* c_str();

  /** Clear value */
  virtual void Clear();

  /** Write variable out to a configuration file */
  virtual void Write(FILE*);

  /** Variables own their strings */
  virtual Int& operator=(const char*);

  /** Does not regenerate the string representation until c_str() is called */
  virtual Int& operator=(float);

  /** Cast to value */
  operator int() { return int_; }

private:
  int int_;
  int int_default_;
};

/** Booleans are just integers */
typedef Int Bool;

/** Gets a variable by name */
String* Get(const char* name);

/** Loads a configuration file */
void LoadConfig(const char* path);

/** Reads variables from command-line argument list */
void ParseArgs(int argc, char* argv[]);

/** Saves a configuration file. Only variables with non-empty comments are
    saved. */
void SaveConfig(const char* path);

} // namespace var
} // namespace dragoon
