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
#include "Variable.h"

namespace dragoon {

ptr::Scope<Variable::variables$T> Variable::variables$;

const char* Variable::c_str() {
  if (!string_) {
    string_ = (char *)malloc(16);
    snprintf(string_, 16, "%g", float_);
  }
  return string_;
}

void Variable::Clear() {
  free(string_);
  string_ = NULL;
  free(string_default_);
  string_default_ = NULL;
}

Variable& Variable::operator=(Variable& v) {
  free(string_);
  string_ = strdup(v.c_str());
  float_ = v;
  return *this;
}

Variable& Variable::operator=(const char* s) {
  free(string_);
  if (s) {
    float_ = atof(string_ = strdup(s));
    CheckBool();
  }
  return *this;
}

Variable& Variable::operator=(float f) {
  if (string_) {
    free(string_);
    string_ = NULL;
  }
  float_ = f;
  return *this;
}

void Variable::CheckBool() {
  if (!strcasecmp(string_, "yes") || !strcasecmp(string_, "true"))
    float_ = 1;
}

void Variable::Write(FILE* f) {
  if (comment_ && (float_ != float_default_ ||
                   ((string_ != NULL) != (string_default_ != NULL)) ||
                   (string_ && strcmp(string_, string_default_)))) {
    if (comment_[0])
      fprintf(f, "\n# %s\n", comment_);
    fprintf(f, "%s \"%s\"\n", name_, c_str());
  }
}

void Variable::LoadConfig(const char* path) {
  Config config(path);
  for (const Config::Node* n = config.root(); n; n = n->next()) {
    const char* name = n->token(0);
    Variable* var;
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

void Variable::ParseArgs(int argc, char* argv[]) {
  for (int i = 1; i < argc; i += 2) {
    Variable* var = Get(argv[i]);
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

void Variable::SaveConfig(const char* path) {
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

} // namespace dragoon
