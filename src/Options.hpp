//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2

#pragma once

class CommonSettings
{
public:
  void Load(const toml::table &aInputRoot);
  void Save(toml::table &aOutputRoot) const;
  void ResetToDefaults();

  void DrawDialog();

  [[nodiscard]] auto operator<=>(const CommonSettings &) const = default;

  bool disableWarnings{false};
  bool checkOriginality{true};
  bool importOriginalFiles{false};
  bool convertToGameFormat{true};
  bool fixChannels{true};
  bool fixSampleRate{true};
  bool transcodeToOriginalFormat{true};
  bool directImport{false};
};

class Options : public Singleton<Options>
{
public:
  void Load();
  void Save() const;
  void ResetToDefaults();

  void DrawDialog();

  CommonSettings common{};
};
