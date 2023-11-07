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

// record with this header has its data in scene's WAV file at the given data offset with given format
// data has duplicate paths if not prefixed in some way, which may point to them being duplicates, needs to be checked
// data is never LIP encoded
// data of these records should never have other format than IMA ADPCM
struct Hitman4WHDRecordScene
{
  uint32_t filePathLength;
  uint32_t filePathOffset;
  uint16_t formatTag; // always 0x11 IMA ADPCM
  uint16_t dataInStreams; // always 0
  uint32_t sampleRate;
  uint32_t bitsPerSample; // always 4
  uint32_t dataSizeUncompressed;
  uint32_t dataSize;
  uint32_t channels;
  uint32_t dataOffset;
  uint32_t samplesCount;
  uint32_t blockAlign; // always seems to be 0x0400
  uint32_t samplesPerBlock; // always seems to be 0x07F9
  uint32_t pad30[4]; // in WHD, has additional padding bytes filled with zeroes without exceptions

  HitmanSoundRecord ToHitmanSoundRecord() const;
  void FromHitmanSoundRecord(const HitmanSoundRecord& soundRecord);
};

// sub record of WHD_Record_StreamsAliased, it has zero padding
struct Hitman4WHDRecordStreamsAliasedSubRecord
{
  uint32_t id; // always 0
  uint32_t filePathOffset;
  uint16_t formatTag; // always 0x01 PCM
  uint16_t dataInStreams; // always has 0x80 set, 0xFF00 mask differs, either 0x8200 or 0x0100, other values not found, exact purpose yet unknown, but 0x8200 comes first (padding not zeroed), 0x0100 second (padding zeroed)
  uint32_t sampleRate;
  uint32_t bitsPerSample; // always 16
  uint32_t dataSizeUncompressed;
  uint32_t dataSize;
  uint32_t channels;
  uint32_t dataOffset; // may start with "LIP " (0x2050494C) chunk, read like described under STR format (LIP data is ONLY in STR files)
  uint32_t samplesCount;
  uint32_t blockAlign; // always 0
  uint32_t samplesPerBlock; // always 0
  uint32_t unk30; // not always 0, purpose unknown
};

// records within this header are aliasing same data block (they have same data offset) somewhere in stream STR file
// data description for these records is hidden in one of the STR file entries which does not have set extension,
// file will have exactly same data offset as both of these records
// their data may be LIP encoded
// data of these records should never have other format than PCM S16
struct Hitman4WHDRecordStreamsAliased
{
  Hitman4WHDRecordStreamsAliasedSubRecord subRecords[2];
  uint32_t pad34[2];  // in WHD, has additional padding bytes filled with zeroes without exceptions
};

// record with this header has its data in stream STR file somewhere at the given data offset with given format
// data may be LIP encoded
// data of this type should never have other format than IMA ADPCM, PCM S16 or OGG Vorbis
struct Hitman4WHDRecordStreams
{
  uint32_t id; // always "PHO\0" (0x004F4850)
  uint32_t filePathOffset;
  uint16_t formatTag; // 0x01 - PCM 16-bit, 0x11 - IMA ADPCM, 0x1000 - OGG
  uint16_t dataInStreams; // always 0x80, 0xFF00 mask is always 0
  uint32_t sampleRate;
  uint32_t bitsPerSample; // 0 for OGG
  uint32_t dataSizeUncompressed;
  uint32_t dataSize;
  uint32_t channels;
  uint32_t dataOffset; // may start with "LIP " (0x2050494C) chunk, read like described under STR format (LIP data is ONLY in STR files)
  uint32_t samplesCount;
  uint32_t blockAlignOrUnknown; // unknown value for OGG, block align for PCM adn ADPCM
  uint32_t samplesPerBlock; // 0xCDCDCDCD for PCMs, 0x07f9 for ADPCM, 0x004F3E93 for OGG...
  uint16_t hasLIP; // this is either 0x00 or 0x04, if it is 0x04 then LIP data is present
  uint16_t unk32; // this always has some value when LIP data is present, 0x0000 otherwise
  uint32_t pad34[3]; // in WHD, has additional padding bytes filled with zeroes without exceptions
};

struct Hitman4WAVHeader
{
  uint32_t unk0; // always 0
  uint32_t fileSizeWithHeader;
  uint32_t unk8; // always 3
  uint32_t unkC; // always 4
};

// exact contents of LIP segment header are unknown, but first segment header always has 4 byte magic "LIP "
// size is 0xF00, header is aligned on min(0x100, data alignment)
struct Hitman4LIPSegmentHeader
{
   uint32_t id; // compares to first 4 chars from "LIP " (0x2050494C) for first segment
   char unk4[0x0F00 - 0x04]; // unknown contents, but there are a lot of visible patterns...
};

// LIP segment which may be used to store sound data, it has variadic size which is unknown how it can be retrieved at the moment.
// First segment always has 4 byte magic 'LIP ' in its header. There is always segment header, then possibly padding (depends on if
// header is aligned the same way data is), then variable number of blocks of data. All segments apart from last one are aligned on
// 0x100 or data alignment, whatever is bigger.
union Hitman4LIPSegment
{
  struct {
   Hitman4LIPSegmentHeader header;
   char data[]; // data of variadic size, always aligned the same way as is block alignment
  } aligned0x100;

  struct {
   Hitman4LIPSegmentHeader header;
   char alignmentPadding[0x100];
   char data[]; // data of variadic size, always aligned the same way as is block alignment
  } aligned0x1000;
};

struct Hitman4STRHeader
{
  char id[0x0C] = {'I', 'O', 'I', 'S', 'N', 'D', 'S', 'T', 'R', 'E', 'A', 'M'};
  uint32_t version = 9; // always 0x09
  uint32_t unk10[2];
  uint32_t offsetToEntryTable = 0; // points at the STR_Footer, right after string table ends
  uint32_t entriesCount = 0; // same as number of STR_Data entries in STR_Footer
  uint32_t dataBeginOffset = 0x0100; // offset to begining of data (or whole header size...)
  uint32_t unk24[0x100 - 0x24] = {};
};

struct Hitman4STRRecordHeader
{
  // magic == 0x02 || 0x03 || 0x04 || 0x11
  uint32_t magic = 0;
  uint32_t samplesCount = 0; // possibly data size
  uint32_t channels = 0; // number of channels
  uint32_t sampleRate = 0; // sample rate
  uint32_t bitsPerSample = 0; // bits per sample
  // magic == 0x02 || 0x03 || 0x11
  uint32_t unk14 = 0; // block alignment for 0x02
  // magic == 0x02 || 0x03 || 0x11
  uint32_t unk18 = 0; // samples per block
  uint32_t blockAlign = 0;
  // magic == 0x03 || 0x11
  uint32_t samplesPerBlock = 0; // samples per block

  HitmanSoundRecord ToHitmanSoundRecord() const;
  void FromHitmanSoundRecord(const HitmanSoundRecord& soundRecord);
};

struct Hitman4STRRecord
{
  uint64_t id; // probably some ID, is less than total entries count, does not match its index
  uint64_t dataOffset; // offset to beginning of data
  uint64_t dataSize; // unknown number
  uint64_t headerOffset; // offset to some table which seems to contain headers
  uint32_t sizeOfHeader; // size of header
  uint32_t unk24; // unknown number, doesnt seem to be an offset, maybe some size?
  uint64_t fileNameLength; // length of filename in string table
  uint64_t fileNameOffset; // offset to filename in string table
  uint32_t hasLIP; // unknown purpose, seen 0x00 and 0x04
  uint32_t unk3C; // unknown purpose, seen 0x00 and 0x30
  uint64_t aliasedEntryOrderOrReference; // if 0, entry is not aliased, otherwise it denotes entry order in aliased data block or reference to one
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

  bool Load(Hitman4Dialog& archiveDialog, const std::vector<char> &wavData);
  bool Load(Hitman4Dialog& archiveDialog, const StringView8CI &loadPath);

  bool Save(const StringView8CI &savePath);

  OrderedMap<uint64_t, Hitman4WAVRecord> recordMap;
  std::list<std::vector<char>> extraData;

  Hitman4STRHeader                    header;
  std::vector<std::vector<char>>      wavDataTable;
  std::vector<std::vector<char>>      lipDataTable;
  std::vector<Hitman4STRRecordHeader> wavHeaderTable;
  std::vector<String8CI>              stringTable;
  std::vector<Hitman4STRRecord>       recordTable;

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

  bool Load(const std::vector<char> &wavData, const OrderedMap<StringView8CI, Hitman4WHDRecordScene *> &whdRecordsMap,
            OrderedMap<StringView8CI, HitmanFile>& fileMap);
  bool Load(const StringView8CI &loadPath, const OrderedMap<StringView8CI, Hitman4WHDRecordScene *> &whdRecordsMap,
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
  OrderedMap<StringView8CI, Hitman4WHDRecordScene *> recordMap;
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

  static const std::vector<std::pair<StringView8CI, StringView8>>& GetOpenFilter();

  const std::vector<std::pair<StringView8CI, StringView8>>& GetSaveFilter() const override;

  std::vector<Hitman4WHDFile> whdFiles;
  std::vector<Hitman4WAVFile> wavFiles;
  Hitman4STRFile streamsWAV;

  OrderedMap<StringView8CI, std::vector<Hitman4WHDRecordScene *>> whdRecordsMap;

  String8CI basePath;
};
