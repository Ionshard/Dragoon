/******************************************************************************\
 Dragoon - Copyright (C) 2010 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "log.h"
#include "os.h"
#include "ui.h"
#include "input.h"
#include "Mode.h"
#include "Sprite.h"
#include "Text.h"

namespace dragoon {
  namespace {
    std::string config_name$;

    // Cleanup on exit
    void Cleanup() {
      try {

        // Only cleanup once no matter what
        static bool already_ran;
        if (already_ran)
          return;
        already_ran = true;

        DEBUG("Cleaning up");
        var::SaveConfig(config_name$.c_str());
        SDL_Quit();
      } catch (log::Exception e) {
        e.Print();
      }
    }

    // Called when the program quits via signal. Performs an orderly cleanup.
    void CaughtSignal(int signal) {
      Cleanup();
      ERROR("Caught signal %d", signal);
    }
  }
}

/** Program entry point */
int main(int argc, char* argv[]) {
  using namespace dragoon;
  try {

    // Debug paramters
    log::set_color(!WINDOWS);
    log::set_debug(CHECKED);
    log::set_detail(log::DETAIL_FILE | log::DETAIL_LINE | log::DETAIL_FUNC);

    DEBUG(PACKAGE_STRING);
    atexit(Cleanup);
    os::HandleSignals(CaughtSignal);

    // Register variables
    var::Bool debug_prints("debug.prints");
    var::String edit_map("debug.edit");
    var::String play_map("debug.play");

    // Load variables
    config_name$ = os::UserDir();
    config_name$ += "/autogen.cfg";
    var::LoadConfig("data/defaults.cfg");
    var::LoadConfig(config_name$.c_str());
    var::ParseArgs(argc, argv);

    // Test debug prints
    if (debug_prints) {
      DEBUG("DEBUG()");
      WARN("WARN()");
      ERROR("ERROR()");
    }

    // Seed random number generator
    srand(time(NULL));

    // Get compiled SDL library versions
    SDL_version sdl_compiled;
    SDL_VERSION(&sdl_compiled);
    SDL_version ttf_compiled;
    TTF_VERSION(&ttf_compiled);
    DEBUG("Compiled with SDL %d.%d.%d, SDL_ttf %d.%d.%d",
          sdl_compiled.major, sdl_compiled.minor, sdl_compiled.patch,
          ttf_compiled.major, ttf_compiled.minor, ttf_compiled.patch);
    const SDL_version *sdl_linked = SDL_Linked_Version();
    const SDL_version *ttf_linked = TTF_Linked_Version();
    if (sdl_linked && ttf_linked)
      DEBUG("Linked with SDL %d.%d.%d, SDL_ttf %d.%d.%d",
            sdl_linked->major, sdl_linked->minor, sdl_linked->patch,
            ttf_linked->major, ttf_linked->minor, ttf_linked->patch);

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0 || TTF_Init() < 0)
      ERROR("Failed to initialize SDL: %s", SDL_GetError());
    SDL_WM_SetCaption(PACKAGE_STRING, PACKAGE);
    SDL_ShowCursor(SDL_DISABLE);

    // Setup video mode, interface
    Mode::Set();
    ui::Init();
    ui::ShowMenu();

    // Status text
    Count throttled;
    Text status;

    // Test sprites
    Sprite::LoadConfig("data/test.cfg");
    Sprite test_sprite("test");

    // Main loop
    DEBUG("Entering main loop");
    for (;;) {
      SDL_Event ev;

      // Dispatch events
      while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT)
          return 0;

        // Key events
        if (ev.type == SDL_KEYDOWN) {

          // In checked mode, Escape quits
          if (CHECKED && ev.key.keysym.sym == SDLK_ESCAPE)
            return 0;
        }

        // Window resized
        if (ev.type == SDL_VIDEORESIZE) {
          DEBUG("Window resized to %dx%d", ev.resize.w, ev.resize.h);
          Mode::Set(ev.resize.w, ev.resize.h, Mode::fullscreen());
        }

        // Keyboard events
        input::Key* key = input::Key::Dispatch(ev);
        if (key) {
          ui::Dispatch(key);
          delete key;
        }

        // Mouse events
        input::Mouse* mouse = input::Mouse::Dispatch(ev);
        if (mouse) {
          delete mouse;
        }
      }

      // Update FPS counter
      if (CHECKED && throttled.Poll(2000)) {
        char buf[80];
        snprintf(buf, sizeof(buf), "%.1f fps (%.0f%% throt), %.0f faces",
                 throttled.Fps(), throttled.PerFrame() * 100,
                 Mode::faces$.PerFrame());
        throttled.Reset();
        Mode::faces$.Reset();
        status.SetText(buf);
      }

      // Frame
      Mode::Begin();
      ui::Update();
      test_sprite.Draw();
      if (CHECKED)
        status.Draw();
      Mode::End();
      Timer::Update();
    }
  } catch (log::Exception e) {
    e.Print();
  }
  return 0;
}
