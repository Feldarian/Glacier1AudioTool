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
  uint32_t fileSizeWithoutHeader = 0;
  uint32_t fileSizeWithHeader = 0;
  uint32_t unk8 = 3; // always 3
  uint32_t unkC = 4; // always 4
};

inline constexpr std::array<uint32_t, 4> terminateBytes{0, 0, 0, 0};

union Hitman4WHDRecord
{
  struct Hitman4WHDRecordScene {
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
    uint32_t samplesPerBlock; // 0xCDCDCDCD for PCMs, 2041 for ADPCM, 0x004F3E93 for OGG...
    uint32_t nullBytes[4];

    HitmanSoundRecord ToHitmanSoundRecord() const;
    void FromHitmanSoundRecord(const HitmanSoundRecord& soundRecord);
  } scene;

  struct Hitman4WHDRecordStreamsAliased
  {
    uint32_t nullByte; // always 0
    uint32_t filePathOffset;
    uint16_t formatTag; // 1 - PCM 16-bit, 17 - IMA ADPCM, 4096 - OGG
    uint16_t dataInStreams; // either 0 when in scene data or 128 when in streams (if 128. filePathLength must be equal "PHO\0")
    uint32_t sampleRate;
    uint32_t bitsPerSample; // 0 for OGG
    uint32_t dataSizeUncompressed;
    uint32_t dataSize;
    uint32_t channels;
    uint32_t dataOffset; // may start with "LIP " (0x2050494C) chunk, ignore 0x1000 bytes
    uint32_t samplesCount;
    uint32_t blockAlign; // something weird for OGG!
    uint32_t samplesPerBlock; // 0xCDCDCDCD for PCMs, 2041 for ADPCM, 0x004F3E93 for OGG...
    uint32_t nullBytes[4];

    HitmanSoundRecord ToHitmanSoundRecord() const;
    void FromHitmanSoundRecord(const HitmanSoundRecord& soundRecord);
  } streamsAliased;

  struct Hitman4WHDRecordStreams
  {
    uint32_t id; // always "PHO\0" (0x004F4850)
    uint32_t filePathOffset;
    uint16_t formatTag; // 1 - PCM 16-bit, 17 - IMA ADPCM, 4096 - OGG
    uint16_t dataInStreams; // either 0 when in scene data or 128 when in streams (if 128. filePathLength must be equal "PHO\0")
    uint32_t unkC; // weird values in case filePathLength is "PHO\0" and dataInStreams is 128
    uint32_t bitsPerSample; // 0 for OGG
    uint32_t dataSizeUncompressed;
    uint32_t dataSize;
    uint32_t channels;
    uint32_t dataOffset; // may start with "LIP " (0x2050494C) chunk, ignore 0x1000 bytes
    uint32_t samplesCount;
    uint32_t unk18;
    uint32_t samplesPerBlock; // 0xCDCDCDCD for PCMs, 2041 for ADPCM, 0x004F3E93 for OGG...
    uint32_t unk2C;
    uint32_t nullBytes[3];

    HitmanSoundRecord ToHitmanSoundRecord() const;
    void FromHitmanSoundRecord(const HitmanSoundRecord& soundRecord);
  } streams;

  static_assert(sizeof(scene) == sizeof(streamsAliased));
  static_assert(sizeof(streamsAliased) == sizeof(streams));

  HitmanSoundRecord ToHitmanSoundRecord() const;
  void FromHitmanSoundRecord(const HitmanSoundRecord& soundRecord);
};

union Hitman4WAVHeader
{
  uint32_t unk0; // always 0
  uint32_t fileSizeWithHeader;
  uint32_t unk8; // always 3
  uint32_t unkC; // always 4
};

struct Hitman4LIPData
{
  uint32_t id; // compares to first 4 chars from "LIP "
  uint32_t unk4;
  uint32_t unk8;
  uint32_t unkC; // one of three values - 0x00, 0x4063, 0x660000 -- also observed 0x408F, which also source was checking... it is used in german version
  // some other data, at least 0x1000 in size
};

struct Hitman4STRHeader
{
  char id[0x0C] = {'I', 'O', 'I', 'S', 'N', 'D', 'S', 'T', 'R', 'E', 'A', 'M'};
  uint32_t version = 9; // always 0x09
  uint32_t offsetToEntryTable = 0; // points at the STR_Footer, right after string table ends
  uint32_t entriesCount = 0; // same as number of STR_Data entries in STR_Footer
  uint32_t dataBeginOffset = 0x0100; // offset to begining of data (or whole header size...)
  uint32_t unk1C = 0; // was 0x0 in samples
  uint32_t unk20 = 1; // was 0x1 in all samples
  uint32_t unkLanguageId = 1; // was 0x1 in english samples and 0x2 in german, maybe some language id?
  uint32_t unk28[0x100 - 0x28] = {};
};

struct Hitman4STRRecordHeader1
{
  uint32_t magic; // always 0x04
  uint32_t unk4; // possibly data size
  uint32_t channels; // number of channels
  uint32_t sampleRate; // sample rate
  uint32_t bitsPerSample; // bits per sample
};

struct Hitman4STRRecordHeader2 : Hitman4STRRecordHeader1
{
  // magic == 0x02 || 0x11
  uint32_t unk; // was 2
};

struct Hitman4STRRecordHeader3 : Hitman4STRRecordHeader1
{
  // magic == 0x03
  uint32_t blockAlign; // block alignment
  uint32_t samplesPerBlock; // samples per block
};

struct Hitman4STRRecord
{
  uint64_t unk0; // some number, maybe id? doesnt seem to be too out of hand...
  uint64_t dataOffset; // offset to beginning of data
  uint64_t dataSize; // unknown number
  uint64_t headerOffset; // offset to some table which seems to contain headers
  uint32_t sizeOfHeader; // size of header
  uint32_t unk24; // unknown number, doesnt seem to be an offset, maybe some size?
  uint64_t fileNameLength; // length of filename in string table
  uint64_t fileNameOffset; // offset to filename in string table
  uint32_t unk38; // was 0x4 in sample where LIP data was present, 0x0 in other
  uint32_t unk3C; // was 0x30 in sample where LIP data was present, 0x0 in other
  uint64_t unk40; // is not 0 always but in quite a few places was
};

struct Hitman4WAVRecord
{
  std::vector<char>& data;
  uint32_t newOffset = 0;
  OptionalReference<std::vector<char>> lipData = std::nullopt;
};

struct Hitman4STRFile
{
  bool Clear(bool retVal = false);

  bool Load(Hitman4Dialog& archiveDialog, const std::vector<char> &wavData, const OrderedMap<StringView8CI, Hitman4WHDRecord *> &whdRecordsMap);
  bool Load(Hitman4Dialog& archiveDialog, const StringView8CI &loadPath, const OrderedMap<StringView8CI, Hitman4WHDRecord *> &whdRecordsMap);

  bool Save(const StringView8CI &savePath);

  OrderedMap<uint32_t, Hitman4WAVRecord> recordMap;
  std::list<std::vector<char>> extraData;

  Hitman4STRHeader               header;
  std::vector<std::vector<char>> wavDataTable;
  std::vector<std::vector<char>> lipDataTable;
  std::vector<std::vector<char>> wavHeaderTableData;
  std::vector<String8CI>         stringTable;
  std::vector<Hitman4STRRecord>  recordTable;

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

  bool Load(const std::vector<char> &wavData, const OrderedMap<StringView8CI, Hitman4WHDRecord *> &whdRecordsMap,
            OrderedMap<StringView8CI, HitmanFile>& fileMap);
  bool Load(const StringView8CI &loadPath, const OrderedMap<StringView8CI, Hitman4WHDRecord *> &whdRecordsMap,
            OrderedMap<StringView8CI, HitmanFile>& fileMap);

  bool Save(const StringView8CI &savePath);

  Hitman4WAVHeader *header = nullptr;
  OrderedMap<uint32_t, Hitman4WAVRecord> recordMap;
  std::list<std::vector<char>> extraData;
  String8CI path;
};

struct Hitman4WHDFile
{
  bool Clear(bool retVal = false);

  bool Load(Hitman4Dialog& archiveDialog, const StringView8CI &loadPathView);

  bool Save(const Hitman4STRFile &streamsWAV, const Hitman4WAVFile &missionWAV, const StringView8CI &savePath);

  Hitman4WHDHeader *header = nullptr;
  OrderedMap<StringView8CI, Hitman4WHDRecord *> recordMap;
  std::vector<char> data;
  String8CI path;
};

class Hitman4Dialog final : public HitmanDialog
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

  static const std::vector<std::pair<String8, String8CI>>& GetOpenFilter();

  const std::vector<std::pair<String8, String8CI>>& GetSaveFilter() const override;

  std::vector<Hitman4WHDFile> whdFiles;
  std::vector<Hitman4WAVFile> wavFiles;
  Hitman4STRFile streamsWAV;

  OrderedMap<StringView8CI, std::vector<Hitman4WHDRecord *>> whdRecordsMap;

  String8CI basePath;
};
