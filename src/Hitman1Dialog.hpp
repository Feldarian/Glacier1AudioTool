//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#pragma once

#include "HitmanDialog.hpp"

class Hitman1Dialog final : public HitmanDialog, public Singleton<Hitman1Dialog>
{
public:
  bool Clear(bool retVal = false) override;

  bool ImportSingle(const std::filesystem::path &importFolderPath, const std::filesystem::path &importFilePath) override;

  bool LoadImpl(const std::filesystem::path &loadPath) override;

  bool SaveImpl(const std::filesystem::path &savePath) override;

  void DrawDialog() override;

  std::vector<std::wstring_view> indexToKey;

  UTFViewToTypeMapCI<wchar_t, std::wstring> lastModifiedDatesMap;
};
