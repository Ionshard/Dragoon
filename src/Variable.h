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

/** Class for storing configuration-file variables */
class Variable {
public:
  typedef std::map<std::string, Variable*> variables$T;

  /** Initializes a variable class and inserts it into variable map
   *  @param name     Name used to fetch variable
   *  @param value    Default value
   *  @param comment  Comment attached to the variable
   */
  template<class T> Variable(const char* name, T value,
                             const char* comment = NULL):
    name_(name), comment_(comment), string_(NULL)
  {
    if (!variables$)
      variables$ = new variables$T();
    (*variables$)[std::string(name)] = this;
    *this = value;
    string_default_ = string_ ? strdup(string_) : NULL;
    float_default_ = float_;
  }

  /** If a Variable goes out of scope, it is removed from the database */
  ~Variable() {
    if (!variables$)
      variables$ = new variables$T();
    variables$->erase(std::string(name_));
    Clear();
  }

  /** Returns the last string assigned to this variable. If the floating
      point value has been updated since then, the string representation is
      updated and returned. */
  const char* c_str();

  /** Clear value */
  void Clear();

  /** Assignment from another variable */
  Variable& operator=(Variable&);

  /** Variables own their strings */
  Variable& operator=(const char* s);

  /** Assigning a float to a variable does not update the string */
  Variable& operator=(float f);

  /** Returns the last numeric value assigned to this variable */
  operator float() { return float_; }

  /** Gets a variable by name */
  static Variable* Get(const char* name)
    { return variables$->count(name) > 0 ? (*variables$)[name] : NULL; }

  /** Loads a configuration file */
  static void LoadConfig(const char* path);

  /** Reads variables from command-line argument list */
  static void ParseArgs(int argc, char* argv[]);

  /** Saves a configuration file. Only variables with non-empty comments are
      saved. */
  static void SaveConfig(const char* path);

private:

  /** Propagate boolean values from string to int */
  void CheckBool();

  /** Write variable out to a configuration file */
  void Write(FILE*);

  const char* name_;
  const char* comment_;
  char* string_;
  char* string_default_;
  float float_;
  float float_default_;

  static ptr::Scope<variables$T> variables$;
};

} // namespace dragoon
