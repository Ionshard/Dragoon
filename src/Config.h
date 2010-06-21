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

/** Configuration file reader */
class Config {
public:

  /** Class representing a list of tokens and an optional block */
  class Node {
  public:
    ~Node();

    /** Match a string token (case-insensitive) */
    bool Match(unsigned int i, const char* s) const {
      if (i >= tokens_.size())
        return s == NULL;
      return !strcasecmp(tokens_[i].c_str(), s);
    }

    /** Match the entire string */
    bool Match(const char* s) const { return !strcasecmp(string_.c_str(), s); }

    /** Get filename */
    const char* filename() const { return filename_; };

    /** Get node line number */
    int line() const { return line_; }

    /** Get next member of block */
    const Node* next() const { return next_; }

    /** Get child block */
    const Node* child() const { return child_; }

    /** Length of tokens list */
    int size() const { return tokens_.size(); }

    /** Return the entire string */
    const char* c_str() const { return string_.c_str(); }

    /** Return a token as a string */
    const char* token(unsigned int i) const {
      return i < tokens_.size() ? tokens_[i].c_str() : "";
    }

    /** Build a node graph by reading from a file stream */
    static Node* Parse(FILE* stream, const char* filename = NULL);

  private:
    Node(): next_(NULL), child_(NULL) {}

    // Some helper functions for parsing
    static char Getch(FILE*, int& line);
    static int SkipSpace(FILE*, int& line);
    static void Token(FILE*, std::string&, int& line);

    std::vector<std::string> tokens_;
    std::string string_;
    Node* next_;
    Node* child_;
    const char* filename_;
    int line_;
  };

  /** Read in and parse configuration file */
  Config(const char* filename);

  /** Get the root node */
  const Node* root() { return root_; }

private:
  ptr::Scope<Node> root_;
  const std::string filename_;
};

} // namespace dragoon
