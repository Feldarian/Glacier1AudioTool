//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#include <Precompiled.hpp>

#include <Core/Resources.hpp>

namespace App {

static const std::string BASE_PATH{SDL_GetBasePath()};

std::filesystem::path Resources::resource_path(const std::filesystem::path& file_path) {
  std::filesystem::path font_path{BASE_PATH};
  font_path /= "../share";
  font_path /= "fonts" / file_path;
  return font_path;
}

std::filesystem::path Resources::font_path(const std::string_view& font_file) {
  return resource_path(font_file);
}

}  // namespace App
