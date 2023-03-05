//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#pragma once

#include "HitmanDialog.hpp"

struct Hitman23WHDHeader
{
  uint32_t fileSizeWithoutHeader;
  uint32_t fileSizeWithHeader;
  uint32_t unk8; // always 3
  uint32_t unkC; // always 4
};

struct Hitman23WHDRecord
{
  uint32_t type = 6; // always 6
  uint32_t filePathOffset;
  uint16_t formatTag;
  uint16_t dataInStreams; // either 0 or 32768
  uint32_t sampleRate;
  uint32_t bitsPerSample;
  uint32_t dataSizeUncompressed; // uncompressed data size, equal dataSize when formatTag == 1
  uint32_t dataSize;
  uint32_t channels;
  uint32_t dataOffset;
  uint32_t samplesCount; // always half of dataSizeUncompressed
  uint32_t blockAlign;
  uint32_t fmtExtra;  // always 1 when formatTag == 1

  HitmanSoundRecord ToHitmanSoundRecord() const;
  void FromHitmanSoundRecord(const HitmanSoundRecord& soundRecord);
};

struct Hitman23WAVHeader
{
  uint32_t unk0 = 0; // always 0
  uint32_t fileSizeWithHeader;
  uint32_t unk8 = 3; // always 3
  uint32_t unkC = 4; // always 4
};

struct Hitman23WAVRecord
{
  std::reference_wrapper<std::vector<char>> data = *static_cast<std::vector<char> *>(nullptr);
  uint32_t newOffset = 0;
};

struct Hitman23WAVFile
{
  bool Clear(bool retVal = false);

  bool Load(const std::filesystem::path &loadPath, const UTFViewToTypeMapCI<wchar_t, Hitman23WHDRecord *> &whdRecordsMap,
            UTFViewToTypeMapCI<wchar_t, HitmanFile>& fileMap, bool isMissionWAV);

  bool Save(const std::filesystem::path &savePath);

  Hitman23WAVHeader *header = nullptr;
  std::map<uint32_t, Hitman23WAVRecord> recordMap;
  std::vector<std::vector<char>> extraData;
  std::filesystem::path path;
};

struct Hitman23WHDFile
{
  bool Clear(bool retVal = false);

  bool BuildArchivePaths(const std::filesystem::path &basePath, const std::filesystem::path &loadPath, PathSetCI& archivePaths);

  bool Load(const std::filesystem::path &basePath, PathSetCI& archivePaths, UTFViewToTypeMapCI<wchar_t, HitmanFile>& fileMap, UTFViewToTypeMapCI<wchar_t, std::vector<Hitman23WHDRecord *>>& whdRecordsMap);

  bool Save(const Hitman23WAVFile &streamsWAV, const Hitman23WAVFile &missionWAV, const std::filesystem::path &savePath);

  Hitman23WHDHeader *header = nullptr;
  UTFViewToTypeMapCI<wchar_t, Hitman23WHDRecord *> recordMap;
  std::vector<char> data;
  std::filesystem::path path;
};

class Hitman23Dialog final : public HitmanDialog, public Singleton<Hitman23Dialog>
{
public:
  bool Clear(bool retVal = false) override;

  bool ImportSingle(const std::filesystem::path &importFolderPath, const std::filesystem::path &importFilePath) override;

  bool LoadImpl(const std::filesystem::path &loadPath) override;

  bool SaveImpl(const std::filesystem::path &savePath) override;

  void DrawDialog() override;

  std::vector<Hitman23WHDFile> whdFiles;
  std::vector<Hitman23WAVFile> wavFiles;
  Hitman23WAVFile streamsWAV;

  UTFViewToTypeMapCI<wchar_t, std::vector<Hitman23WHDRecord *>> whdRecordsMap;

  std::filesystem::path basePath;
};
