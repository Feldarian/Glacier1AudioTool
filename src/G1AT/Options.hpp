//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2

#pragma once

#include "Singleton.hpp"

class CommonSettings
{
public:
  void Load(const toml::table &aInputRoot);
  void Save(toml::table &aOutputRoot) const;
  void ResetToDefaults();

  void DrawDialog();

  [[nodiscard]] auto operator<=>(const CommonSettings &) const = default;

  bool disableWarnings{false};
  bool importSameFiles{false};
  bool fixChannels{true};
  bool fixSampleRate{true};
  bool transcodeToOriginalFormat{true};
  bool directImport{false};
  bool transcodeToPlayableFormat{true};
  bool transcodeOGGToPCM{false};
};

class Hitman4Settings
{
public:
  void Load(const toml::table &aInputRoot);
  void Save(toml::table &aOutputRoot) const;
  void ResetToDefaults();

  void DrawDialog();

  [[nodiscard]] auto operator<=>(const Hitman4Settings &) const = default;

  bool exportWithLIPData{false};
};

class Options : public Singleton<Options>
{
public:
  void Load();
  void Save() const;
  void ResetToDefaults();

  void DrawDialog();

  [[nodiscard]] auto operator<=>(const Options &other) const
  {
    if (const auto cmp = common <=> other.common; cmp != std::strong_ordering::equal)
      return cmp;

    if (const auto cmp = hitman4 <=> other.hitman4; cmp != std::strong_ordering::equal)
      return cmp;

    return std::strong_ordering::equal;
  }

  [[nodiscard]] bool operator==(const Options &other) const
  {
    return (*this <=> other) == std::strong_ordering::equal;
  }

  CommonSettings common;
  Hitman4Settings hitman4;
  bool opened = false;
};
