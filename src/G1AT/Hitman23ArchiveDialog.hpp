//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#pragma once

#include "Glacier1ArchiveDialog.hpp"

class Hitman23ArchiveDialog;

struct Hitman23WHDHeader
{
  uint32_t fileSizeWithoutHeader = 0;
  uint32_t fileSizeWithHeader = 0;
  uint32_t unk8 = 3; // always 3
  uint32_t unkC = 4; // always 4
};

struct Hitman23WHDRecord
{
  uint32_t type = 6; // always 6
  uint32_t filePathOffset = 0;
  uint16_t formatTag = 0;
  uint16_t dataInStreams = 0; // either 0 or 32768
  uint32_t sampleRate = 0;
  uint32_t bitsPerSample = 0;
  uint32_t dataSizeUncompressed = 0; // uncompressed data size, equal dataSize when format == 1
  uint32_t dataSize = 0;
  uint32_t channels = 0;
  uint32_t dataOffset = 0;
  uint32_t samplesCount = 0; // always half of dataSizeUncompressed
  uint32_t blockAlign = 0;
  uint32_t samplesPerBlock = 0;  // always 1 when format == 1

  Glacier1AudioRecord ToHitmanSoundRecord() const;
  void FromHitmanSoundRecord(const Glacier1AudioRecord& soundRecord);
};

struct Hitman23WAVHeader
{
  uint32_t unk0 = 0; // always 0
  uint32_t fileSizeWithHeader = 0;
  uint32_t unk8 = 3; // always 3
  uint32_t unkC = 4; // always 4
};

struct Hitman2WAVHeader
{
  uint32_t unk0 = 0; // always 0
  uint32_t fileSizeWithHeader = 0;
  uint32_t unk8 = 3; // always 3
  uint32_t unkC = 4; // always 4
};

struct Hitman3WAVHeader
{
  uint32_t unk0 = 0; // always 0
  uint32_t fileSizeWithHeader = 0;
  uint32_t unk8 = 3; // always 3
  uint32_t unkC = 4; // always 4
  uint32_t unk10 = 5; // always 5
  uint32_t unk14 = 6; // always 6
  uint32_t unk18 = 7; // always 7
  uint32_t unk1C = 8; // always 8
};

struct Hitman23WAVRecord
{
  std::vector<char>& data;
  uint32_t newOffset = 0;
};

struct Hitman23WAVFile
{
  bool Clear(bool retVal = false);

  bool Load(const std::vector<char> &wavData, const OrderedMap<StringView8CI, Hitman23WHDRecord *> &whdRecordsMap,
            OrderedMap<StringView8CI, Glacier1AudioFile>& fileMap, bool isMissionWAV);
  bool Load(const StringView8CI &loadPath, const OrderedMap<StringView8CI, Hitman23WHDRecord *> &whdRecordsMap,
            OrderedMap<StringView8CI, Glacier1AudioFile>& fileMap, bool isMissionWAV);

  bool Save(const StringView8CI &savePath);

  Hitman23WAVHeader *header = nullptr;
  OrderedMap<uint32_t, Hitman23WAVRecord> recordMap;
  std::list<std::vector<char>> extraData;
  String8CI path;
};

struct Hitman23WHDFile
{
  bool Clear(bool retVal = false);

  bool Load(Hitman23ArchiveDialog& archiveDialog, const StringView8CI &loadPathView);

  bool Save(const Hitman23WAVFile &streamsWAV, const Hitman23WAVFile &missionWAV, const StringView8CI &savePath);

  Hitman23WHDHeader *header = nullptr;
  OrderedMap<StringView8CI, Hitman23WHDRecord *> recordMap;
  std::vector<char> data;
  String8CI path;
};

class Hitman23ArchiveDialog final : public Glacier1ArchiveDialog
{
public:
  bool Clear(bool retVal = false) override;

  bool ImportSingle(const StringView8CI &importFolderPath, const StringView8CI &importFilePath, const Options &options) override;

  bool LoadImpl(const StringView8CI &loadPath, const Options &options) override;

  bool SaveImpl(const StringView8CI &savePath, const Options &options) override;

  int32_t DrawDialog() override;

  static const std::vector<std::pair<StringView8CI, StringView8>>& GetOpenFilter();

  const std::vector<std::pair<StringView8CI, StringView8>>& GetSaveFilter() const override;

  std::vector<Hitman23WHDFile> whdFiles;
  std::vector<Hitman23WAVFile> wavFiles;
  Hitman23WAVFile streamsWAV;

  OrderedMap<StringView8CI, std::vector<Hitman23WHDRecord *>> whdRecordsMap;

  String8CI basePath;
};
