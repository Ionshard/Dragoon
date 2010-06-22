/******************************************************************************\
 Dragoon - Copyright (C) 2009 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "log.h"
#include "Config.h"
#include "var.h"

namespace dragoon {
namespace var {

namespace {
  typedef std::map<std::string, String*> variables$T;

  ptr::Scope<variables$T> variables$;

  bool CheckBool(const char* s) {
    return !strcasecmp(s, "yes") || !strcasecmp(s, "true");
  }
}

String::String(const char* name, const char* value, const char* comment):
  name_(name), comment_(comment), string_(NULL)
{
  if (!variables$)
    variables$ = new variables$T();
  std::string name_string(name);
  if (variables$->count(name_string) > 0)
    ERROR("Redeclared variable '%s'", name);
  (*variables$)[name_string] = this;
  *this = value;
  string_default_ = string_ ? strdup(string_) : NULL;
}

String::~String() {
  if (variables$)
    variables$->erase(std::string(name_));
  Clear();
}

String* Get(const char* name) {
  return variables$->count(name) > 0 ? (*variables$)[name] : NULL;
}

void String::Clear() {
  free(string_);
  string_ = NULL;
  free(string_default_);
  string_default_ = NULL;
}

String& String::operator=(const char* s) {
  free(string_);
  string_ = s ? strdup(s) : NULL;
  return *this;
}

void String::Write(FILE* f) {
  if (comment_ && (((string_ != NULL) != (string_default_ != NULL)) ||
                   (string_ && strcmp(string_, string_default_)))) {
    if (comment_[0])
      fprintf(f, "\n# %s\n", comment_);
    fprintf(f, "%s \"%s\"\n", name_, c_str());
  }
}

const char* Float::c_str() {
  if (!string_) {
    string_ = (char *)malloc(16);
    snprintf(string_, 16, "%g", float_);
  }
  return string_;
}

void Float::Clear() {
  String::Clear();
  float_ = 0;
  float_default_ = 0;
}

void Float::Write(FILE* f) {
  if (comment_ && float_ != float_default_) {
    if (comment_[0])
      fprintf(f, "\n# %s\n", comment_);
    fprintf(f, "%s %g\n", name_, float_);
  }
}

Float& Float::operator=(const char* s) {
  if (s) {
    float_ = atof(string_ = strdup(s));
    if (CheckBool(string_))
      float_ = 1;
  }
  return *this;
}

Float& Float::operator=(float f) {
  if (string_) {
    free(string_);
    string_ = NULL;
  }
  float_ = f;
  return *this;
}

const char* Int::c_str() {
  if (!string_) {
    string_ = (char *)malloc(16);
    snprintf(string_, 16, "%d", int_);
  }
  return string_;
}

void Int::Clear() {
  String::Clear();
  int_ = 0;
  int_default_ = 0;
}

void Int::Write(FILE* f) {
  if (comment_ && int_ != int_default_) {
    if (comment_[0])
      fprintf(f, "\n# %s\n", comment_);
    fprintf(f, "%s %d\n", name_, int_);
  }
}

Int& Int::operator=(const char* s) {
  if (s) {
    char* end_ptr;
    int_ = strtol(string_ = strdup(s), &end_ptr, 0);
    if (CheckBool(string_))
      int_ = 1;
  }
  return *this;
}

Int& Int::operator=(float i) {
  if (string_) {
    free(string_);
    string_ = NULL;
  }
  int_ = i;
  return *this;
}

void LoadConfig(const char* path) {
  Config config(path);
  for (const Config::Node* n = config.root(); n; n = n->next()) {
    const char* name = n->token(0);
    String* var;
    if (n->child())
      WARN("%s:%d: Variable '%s' child block ignored", path, n->line(), name);
    if (!(var = Get(name)))
      WARN("%s:%d: Variable '%s' not found", path, n->line(), name);
    else if (n->size() == 2) {
      *var = n->token(1);
      DEBUG("%s = %s", name, n->token(1));
    } else if (n->size() > 2)
      WARN("%s:%d: Excess values in assignment to variable '%s'",
           path, n->line(), name);
    else if (n->size() < 2)
      WARN("%s:%d: No value assigned to variable '%s'", path, n->line(), name);
  }
}

void ParseArgs(int argc, char* argv[]) {
  for (int i = 1; i < argc; i += 2) {
    String* var = Get(argv[i]);
    if (!var)
      WARN("Command-line variable '%s' not found", argv[i]);
    else {
      if (i + 1 >= argc) {
        WARN("Command-line variable '%s' has no value", argv[i]);
        return;
      }
      *var = argv[i + 1];
    }
  }
}

void SaveConfig(const char* path) {
  FILE* f = fopen(path, "w");
  if (!f) {
    WARN("Failed to open configuration file '%s' for writing", path);
    return;
  }
  fputs("####################################################################"
        "############\n# Dragoon -- automatically generated configuration "
        "file\n##############################################################"
        "##################\n", f);
  for (variables$T::iterator it = variables$->begin(), end = variables$->end();
       it != end; ++it)
    it->second->Write(f);
  fclose(f);
  DEBUG("Saved configuration file '%s'", path);
}

} // namespace var
} // namespace dragoon
