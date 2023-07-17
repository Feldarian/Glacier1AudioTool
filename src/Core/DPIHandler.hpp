//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#pragma once

#include "Window.hpp"

namespace App {

struct WindowSize {
  int width;
  int height;
};

class DPIHandler {
 public:
  [[nodiscard]] static float get_scale();

  [[nodiscard]] static WindowSize get_dpi_aware_window_size(const Window::Settings& settings);

  static void set_render_scale(SDL_Renderer* renderer);
  static void set_global_font_scaling(ImGuiIO* io);
};

}  // namespace App
