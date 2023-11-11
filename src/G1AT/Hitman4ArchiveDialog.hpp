//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#pragma once

#include "Glacier1ArchiveDialog.hpp"

class Hitman4ArchiveDialog;

inline constexpr std::array<uint32_t, 4> terminateBytes{0, 0, 0, 0};

struct Hitman4WAVRecord
{
  std::vector<char>& data;
  uint32_t newOffset = 0;
  OptionalReference<std::vector<char>> lipData = std::nullopt;
};

struct Hitman4STRFile
{
  bool Clear(bool retVal = false);

  bool Load(Hitman4ArchiveDialog& archiveDialog, const std::vector<char> &wavData);
  bool Load(Hitman4ArchiveDialog& archiveDialog, const StringView8CI &loadPath);

  bool Save(const StringView8CI &savePath);

  OrderedMap<uint64_t, Hitman4WAVRecord> recordMap;
  std::list<std::vector<char>> extraData;

  STR::v1::Header                  header;
  std::vector<std::vector<char>>   wavDataTable;
  std::vector<std::vector<char>>   lipDataTable;
  std::vector<STR::v1::DataHeader> wavHeaderTable;
  std::vector<String8CI>           stringTable;
  std::vector<STR::v1::Entry>      recordTable;

  OrderedMap<StringView8CI, size_t> fileNameToIndex;

  //uint64_t wavDataTableBeginOffset = 0; // == header.dataBeginOffset
  uint64_t wavHeaderTableBeginOffset = 0;
  uint64_t stringTableBeginOffset = 0;
  //uint64_t recordTableBeginOffset = 0; // == header.offsetToEntryTable

  String8CI path;
};

struct Hitman4WAVFile
{
  bool Clear(bool retVal = false);

  bool Load(const std::vector<char> &wavData, const OrderedMap<StringView8CI, WHD::v2::EntryScenes *> &whdRecordsMap,
            OrderedMap<StringView8CI, Glacier1AudioFile>& fileMap);
  bool Load(const StringView8CI &loadPath, const OrderedMap<StringView8CI, WHD::v2::EntryScenes *> &whdRecordsMap,
            OrderedMap<StringView8CI, Glacier1AudioFile>& fileMap);

  bool Save(const StringView8CI &savePath);

  WAV::v2::Header *header = nullptr;
  OrderedMap<uint32_t, Hitman4WAVRecord> recordMap;
  std::list<std::vector<char>> extraData;
  String8CI path;
};

struct Hitman4WHDFile
{
  bool Clear(bool retVal = false);

  bool Load(Hitman4ArchiveDialog& archiveDialog, const StringView8CI &loadPathView);

  bool Save(const Hitman4STRFile &streamsWAV, const Hitman4WAVFile &missionWAV, const StringView8CI &savePath);

  WHD::v2::Header *header = nullptr;
  OrderedMap<StringView8CI, WHD::v2::EntryScenes *> recordMap;
  std::vector<char> data;
  String8CI path;
};

class Hitman4ArchiveDialog final : public Glacier1ArchiveDialog
{
public:
  bool Clear(bool retVal = false) override;

  bool ExportSingle(const StringView8CI &exportFolderPath, const StringView8CI &exportFilePath, const Options &options) const override;

  bool ImportSingle(const StringView8CI &importFolderPath, const StringView8CI &importFilePath, const Options &options) override;

  bool LoadImpl(const StringView8CI &loadPath, const Options &options) override;

  bool SaveImpl(const StringView8CI &savePath, const Options &options) override;

  int32_t DrawDialog() override;

  bool IsSaveAllowed() const override;
  bool IsExportAllowed() const override;
  bool IsImportAllowed() const override;

  static const std::vector<std::pair<StringView8CI, StringView8>>& GetOpenFilter();

  const std::vector<std::pair<StringView8CI, StringView8>>& GetSaveFilter() const override;

  std::vector<Hitman4WHDFile> whdFiles;
  std::vector<Hitman4WAVFile> wavFiles;
  Hitman4STRFile streamsWAV;

  OrderedMap<StringView8CI, std::vector<WHD::v2::EntryScenes *>> whdRecordsMap;

  String8CI basePath;
};
