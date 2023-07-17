//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#include <Precompiled.hpp>

#include <Core/Application.hpp>
#include <Core/Debug/Instrumentor.hpp>
#include <Core/Log.hpp>

#include <Config/Config.hpp>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
  try {
    APP_PROFILE_BEGIN_SESSION_WITH_FILE("App", "profile.json");

    {
      APP_PROFILE_SCOPE("Test scope");
      App::Application app{G1AT_TITLE};
      app.run();
    }

    APP_PROFILE_END_SESSION();
  } catch (std::exception& e) {
    APP_ERROR("Main process terminated with: {}", e.what());
  }

  return 0;
}
