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

  bool ImportSingle(const StringView8CI &importFolderPath, const StringView8CI &importFilePath, const Options &options) override;

  bool LoadImpl(const StringView8CI &loadPath, const Options &options) override;

  bool SaveImpl(const StringView8CI &savePath, const Options &options) override;

  void DrawDialog() override;

  std::vector<StringView8CI> indexToKey;

  OrderedMap<StringView8CI, String8> lastModifiedDatesMap;
};
