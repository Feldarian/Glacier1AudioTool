//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#pragma once

#include "HitmanDialog.hpp"

class Hitman4Dialog;

struct Hitman4WHDHeader
{
  uint32_t fileSizeWithoutHeader;
  uint32_t fileSizeWithHeader;
  uint32_t unk8; // always 3
  uint32_t unkC; // always 4
};

inline constexpr std::array<uint32_t, 4> terminateBytes{0, 0, 0, 0};

union Hitman4WHDRecord
{
  struct Hitman4WHDRecordMission {
    uint32_t filePathLength;
    uint32_t filePathOffset;
    uint16_t formatTag; // 1 - PCM 16-bit, 17 - IMA ADPCM, 4096 - OGG
    uint16_t dataInStreams; // always 0
    uint32_t sampleRate;
    uint32_t bitsPerSample; // 0 for OGG
    uint32_t dataSizeUncompressed;
    uint32_t dataSize;
    uint32_t channels;
    uint32_t dataOffset;
    uint32_t samplesCount;
    uint32_t blockAlign; // something weird for OGG!
    uint32_t fmtExtra; // 0xCDCDCDCD for PCMs, 2041 for ADPCM, 0x004F3E93 for OGG...
    uint32_t nullBytes[4] = {0};

    HitmanSoundRecord ToHitmanSoundRecord() const;
    void FromHitmanSoundRecord(const HitmanSoundRecord& soundRecord);
  } mission;

  struct Hitman4WHDRecordConcatenated
  {
    uint32_t nullByte; // always 0
    uint32_t filePathOffset;
    uint16_t formatTag; // 1 - PCM 16-bit, 17 - IMA ADPCM, 4096 - OGG
    uint16_t dataInStreams; // either 0 when in mission data or 128 when in streams (if 128. filePathLength must be equal "PHO\0")
    uint32_t sampleRate;
    uint32_t bitsPerSample; // 0 for OGG
    uint32_t dataSizeUncompressed;
    uint32_t dataSize;
    uint32_t channels;
    uint32_t dataOffset; // may start with "LIP " (0x2050494C) chunk, ignore 0x1000 bytes
    uint32_t samplesCount;
    uint32_t blockAlign; // something weird for OGG!
    uint32_t fmtExtra; // 0xCDCDCDCD for PCMs, 2041 for ADPCM, 0x004F3E93 for OGG...
    uint32_t nullBytes[4] = {0};

    HitmanSoundRecord ToHitmanSoundRecord() const;
    void FromHitmanSoundRecord(const HitmanSoundRecord& soundRecord);
  } concatenated;

  struct Hitman4WHDRecordStreams
  {
    uint32_t id; // always "PHO\0" (0x004F4850)
    uint32_t filePathOffset;
    uint16_t formatTag; // 1 - PCM 16-bit, 17 - IMA ADPCM, 4096 - OGG
    uint16_t dataInStreams; // either 0 when in mission data or 128 when in streams (if 128. filePathLength must be equal "PHO\0")
    uint32_t unkC; // weird values in case filePathLength is "PHO\0" and dataInStreams is 128
    uint32_t bitsPerSample; // 0 for OGG
    uint32_t dataSizeUncompressed;
    uint32_t dataSize;
    uint32_t channels;
    uint32_t dataOffset; // may start with "LIP " (0x2050494C) chunk, ignore 0x1000 bytes
    uint32_t samplesCount;
    uint32_t unk18;
    uint32_t fmtExtra; // 0xCDCDCDCD for PCMs, 2041 for ADPCM, 0x004F3E93 for OGG...
    uint32_t unk2C;
    uint32_t nullBytes[3] = {0};

    HitmanSoundRecord ToHitmanSoundRecord() const;
    void FromHitmanSoundRecord(const HitmanSoundRecord& soundRecord);
  } streams;

  static_assert(sizeof(mission) == sizeof(concatenated));
  static_assert(sizeof(concatenated) == sizeof(streams));

  HitmanSoundRecord ToHitmanSoundRecord() const;
  void FromHitmanSoundRecord(const HitmanSoundRecord& soundRecord);
};

union Hitman4WAVHeader
{
  struct Hitman4WAVHeaderWHD {
    uint32_t unk0 = 0; // always 0
    uint32_t fileSizeWithHeader;
    uint32_t unk8 = 3; // always 3
    uint32_t unkC = 4; // always 4
  } whd;

  struct Hitman4WAVHeaderStreams {
    char id[0x10];
    uint32_t stringTableEndOffset;
    uint32_t padding[0x100 - 0x14];
  } streams;
};

struct Hitman4LIPData
{
   uint32_t id; // compares to first 4 chars from "LIP "
   uint32_t unk4;
   uint32_t unk8;
   uint32_t unkB;
   // dataSize array of bytes follows
};

struct Hitman4WAVRecord
{
  std::vector<char>& data;
  uint32_t newOffset = 0;
  OptionalReference<std::vector<char>> lipData = std::nullopt;
};

struct Hitman4WAVFile
{
  bool Clear(bool retVal = false);

  bool Load(StringView8CI loadPath, const OrderedMap<StringView8CI, Hitman4WHDRecord *> &whdRecordsMap,
            OrderedMap<StringView8CI, HitmanFile>& fileMap, bool isMissionWAV);

  bool Save(StringView8CI savePath);

  Hitman4WAVHeader *header = nullptr;
  OrderedMap<uint32_t, Hitman4WAVRecord> recordMap;
  std::list<std::vector<char>> lipsData;
  std::list<std::vector<char>> extraData;
  String8CI path;
};

struct Hitman4WHDFile
{
  bool Clear(bool retVal = false);

  bool Load(Hitman4Dialog& archiveDialog, StringView8CI loadPathView);

  bool Save(const Hitman4WAVFile &streamsWAV, const Hitman4WAVFile &missionWAV, StringView8CI savePath);

  Hitman4WHDHeader *header = nullptr;
  OrderedMap<StringView8CI, Hitman4WHDRecord *> recordMap;
  std::vector<char> data;
  String8CI path;
};

class Hitman4Dialog final : public HitmanDialog, public Singleton<Hitman4Dialog>
{
public:
  bool Clear(bool retVal = false) override;

  bool ImportSingle(StringView8CI importFolderPath, StringView8CI importFilePath, const Options &options) override;

  bool LoadImpl(StringView8CI loadPath, const Options &options) override;

  bool SaveImpl(StringView8CI savePath, const Options &options) override;

  void DrawDialog() override;

  std::vector<Hitman4WHDFile> whdFiles;
  std::vector<Hitman4WAVFile> wavFiles;
  Hitman4WAVFile streamsWAV;

  OrderedMap<StringView8CI, std::vector<Hitman4WHDRecord *>> whdRecordsMap;

  String8CI basePath;
};
