//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#pragma once

#include "ArchiveDialog.hpp"

using Glacier1AudioRecord = AudioRecord;

struct Glacier1AudioFile
{
  bool Import(AudioRecord soundRecord, const std::span<const char>& soundDataView, const Options& options);
  bool Import(const std::span<const char>& in, const Options& options);
  bool Import(const StringView8CI &importPath, const Options& options);

  bool ImportNative(const AudioRecord & soundRecord, const std::span<const char>& soundDataView, const std::span<const int16_t>& pcms16DataView, const Options& options);
  bool ImportNative(AudioRecord soundRecord, const std::span<const char>& soundDataView, bool allowConversions, const Options& options);
  bool ImportNative(const std::span<const char>& in, bool allowConversions, const Options& options);
  bool ImportNative(const StringView8CI &importPath, bool allowConversions, const Options& options);

  bool Export(std::vector<char> &outputBytes, const Options& options) const;

  bool ExportNative(std::vector<char> &outputBytes, const Options& options) const;

  StringView8CI path;
  Glacier1AudioRecord archiveRecord;
  Glacier1AudioRecord originalRecord;
  std::vector<char> data;
};

class Glacier1ArchiveDialog : public ArchiveDialog
{
public:
  bool Clear(bool retVal = false) override;

  bool IsSaveAllowed() const override;
  bool IsExportAllowed() const override;
  bool IsImportAllowed() const override;

  bool GenerateOriginalData(const Options &options = Options::Get());
  bool LoadOriginalData(const Options &options = Options::Get());
  int32_t ReloadOriginalData(bool reset = false, const Options &options = Options::Get());

  bool ImportSingleHitmanFile(Glacier1AudioFile &glacier1AudioFile, const std::span<const char> &data, bool allowConversions, const Options &options);
  bool ImportSingleHitmanFile(Glacier1AudioFile &glacier1AudioFile, const StringView8CI &importFilePath, const Options &options);

  bool ExportSingleHitmanFile(const Glacier1AudioFile &glacier1AudioFile, std::vector<char> &data, bool doConversion, const Options &options) const;
  bool ExportSingleHitmanFile(const Glacier1AudioFile &glacier1AudioFile, const StringView8CI &exportFolderPath, const Options &options) const;

  bool ExportSingle(const StringView8CI &exportFolderPath, const StringView8CI &exportFilePath, const Options &options) const override;

  int32_t DrawGlacier1ArchiveDialog();

  OrderedMap<StringView8CI, Glacier1AudioFile> fileMap;
  String8CI originalDataPathPrefix;
  uint64_t originalDataID = 0;
  uint64_t originalDataParentID = 0;
  bool needsOriginalDataReload = false;
  bool needsOriginalDataReset = false;
};
