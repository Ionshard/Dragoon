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

namespace dragoon {

Config::Node::~Node() {
  delete child_;
  delete next_;
}

char Config::Node::Getch(FILE* f, int& line) {
  int ch;

  // Skip windows carriage returns
  do {
    ch = fgetc(f);

    // Slash-newline continuation
    if (ch == '\\') {
      ch = fgetc(f);
      if (ch == '\n') {
        ++line;
        continue;
      }
    }

  } while (ch == '\r');

  // Skip comment lines
  if (ch == '#')
    while (ch != EOF && (ch = fgetc(f)) != '\n');

  // Convert tabs to spaces
  if (ch == '\t')
    ch = ' ';

  // Convert EOF to zero
  if (ch == EOF)
    return 0;

  // Count newlines
  if (ch == '\n')
    ++line;

  return (char)ch;
}

int Config::Node::SkipSpace(FILE* f, int& line) {
  int ch;
  for (ch = Getch(f, line); ch == ' '; ch = Getch(f, line));
  return ch;
}

void Config::Node::Token(FILE* f, std::string& out, int& line) {
  out.clear();
  char ch = SkipSpace(f, line);
  if (!ch)
    return;

  // Read quote
  if (ch == '"')
    for (ch = Getch(f, line); ch && ch != '"'; ch = Getch(f, line)) {
      if (ch == '\\') {
        ch = Getch(f, line);
        if (ch == 'n')
          ch = '\n';
      }
      out.push_back(ch);
    }

  // Read brace
  else if (ch == '{' || ch == '}' || ch == '\n')
    out.push_back(ch);

  // Read identifier
  else
    for (; ch; ch = Getch(f, line)) {
      if (ch == ' ' || ch == '{' || ch == '}' || ch == '\n') {
        ungetc(ch, f);
        break;
      }
      out.push_back(ch);
    }
}

Config::Node* Config::Node::Parse(FILE* f, const char* filename) {
  std::string t;
  Node* cur = NULL;
  Node* prev = NULL;
  Node* root = NULL;
  int line = 1;
  for (Token(f, t, line); t.size(); Token(f, t, line)) {
    if (t == "}")
      break;
    if (t == "\n") {
      if (cur) {
        prev = cur;
        cur = NULL;
      }
      continue;
    }
    if (!cur) {
      cur = new Node();
      cur->filename_ = filename;
      cur->line_ = line;
      if (prev)
        prev->next_ = cur;
      if (!root)
        root = cur;
    }
    if (t == "{")
      cur->child_ = Parse(f, filename);
    else {
      cur->tokens_.push_back(t);
      if (cur->string_.size())
        cur->string_ += " ";
      cur->string_ += t;
    }
  }
  return root;
}

Config::Config(const char* filename): filename_(filename) {
  FILE* f = fopen(filename, "r");
  if (!f)
    WARN("Failed to open configuration file '%s'", filename);
  else {
    DEBUG("Parsing configuration file '%s'", filename);
    root_ = Node::Parse(f, filename_.c_str());
    fclose(f);
  }
}

} // namespace dragoon
