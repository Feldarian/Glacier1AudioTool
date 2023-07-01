//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#pragma once

#include "ArchiveDialog.hpp"

struct HitmanSoundRecord
{
  uint64_t dataXXH3 = 0;
  uint32_t dataSizeUncompressed = 0;
  uint32_t dataSize = 0;
  uint32_t sampleRate = 0;
  uint16_t formatTag = 0;
  uint16_t bitsPerSample = 0;
  uint16_t channels = 0;
  uint16_t blockAlign = 0;
  uint16_t fmtExtra = 0;

  auto operator<=>(const HitmanSoundRecord & other) const = default;
};

struct HitmanFile
{
  bool Import(char inputBytes[], size_t inputBytesCount, const Options& options);
  bool Import(std::vector<char> &inputBytes, const Options& options);
  bool Import(const StringView8CI &importPath, const Options& options);

  bool ImportNative(char inputBytes[], size_t inputBytesCount, const Options& options, bool doHashing);
  bool ImportNative(std::vector<char> &inputBytes, const Options& options, bool doHashing);
  bool ImportNative(const StringView8CI &importPath, const Options& options, bool doHashing);

  bool Export(std::vector<char> &outputBytes) const;
  bool Export(const StringView8CI &exportPath, bool fixExtension) const;

  HitmanSoundRecord originalRecord;
  HitmanSoundRecord archiveRecord;
  std::vector<char> data;
};

class HitmanDialog : public ArchiveDialog
{
public:
  bool Clear(bool retVal = false) override;

  bool GenerateOriginalData();

  bool LoadOriginalData();

  bool ImportSingleHitmanFile(HitmanFile &hitmanFile, const StringView8CI &hitmanFilePath, std::vector<char> &data, bool doConversion, const Options &options);

  bool ImportSingleHitmanFile(HitmanFile &hitmanFile, const StringView8CI &hitmanFilePath, const StringView8CI &importFilePath, const Options &options);

  bool ExportSingle(const StringView8CI &exportFolderPath, const StringView8CI &exportFilePath, const Options &options) const override;

  void ReloadOriginalData();

  void DrawHitmanDialog(const StringView8CI &dialogName, const StringView8CI &filters, const StringView8CI &defaultFilename);

  OrderedMap<StringView8CI, HitmanFile> fileMap;
  String8CI originalDataPath;
  bool needsOriginalDataReload = false;
};
