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
#include "../ptr.h"
#include "../Text.h"

namespace dragoon {
namespace ui {

/** Navigatable menus */
class Menu: public param::Origin, public param::Visible {
public:

  /** Entry in a menu */
  class Entry: public param::Enabled {
  public:
    struct Option {
      Option(const char* text, float value):
        text_(text), fade_(0), value_(value) {}

      Text text_;
      float fade_;
      float value_;
    };

    /** Initialize entry with label */
    Entry(const char* label, void (*on_activate)(float) = NULL):
      label_(label), on_activate_(on_activate) {}

    /** Activate menu item */
    void Activate(bool next = true);

    /** Add an option to the end of the options list */
    void Append(const char* text, float value = 0)
      { options_.push_back(new Option(text, value)); }

    /** Add an option to the beginning of the options list */
    void Prepend(const char* text, float value = 0)
      { options_.insert(options_.begin(), new Option(text, value)); }

    /** Override to control behavior on activation */
    virtual void OnActivate(Option* option) {}

  protected:
    friend class Menu;

    void Update(float menu_fade, float menu_width, Vec<2> explode,
                bool selected);
    float Position(Vec<2> origin, float width);

  private:
    ptr::Scope<Option>::Vector options_;
    Text label_;
    void (*on_activate_)(float);
    float fade_;
    int selected_;
  };

  Menu(float width): size_(width, 0), selected_(0) {}

  /** Activate the selected entry.
   *  @param next  If entry has options, select the next option
   */
  void Activate(bool next = true) {
    if (entries_.size() > 0)
      entries_[selected_]->Activate(next);
  }

  /** Activate the last entry in the menu */
  void ActivateLast() {
    if (entries_.size() > 0)
      entries_[entries_.size() - 1]->Activate();
  }

  /** Add an entry class to the menu */
  void Add(Entry*, float margin = 0);

  /** Scroll selection in the menu */
  void Scroll(bool up = false);

  /** Update child menu entries */
  void Update();

  Vec<2> size() const { return size_; }

private:
  ptr::Scope<Entry>::Vector entries_;
  Vec<2> size_;
  float fade_;
  int selected_;
  bool hide_left_;
};

} // namespace ui
} // namespace dragoon
