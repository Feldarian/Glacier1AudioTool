//
// Created by Andrej Redeky.
// SPDX-License-Identifier: Unlicense
//
// Extended format information: https://reshax.com/topic/27-glacier-1-str-file-format
//
// V1 is for Hitman: Blood Money
// V2 is for Kane & Lynch: Dead Men and Mini Ninjas
// V3 is for Kane & Lynch 2: Dog Days (TODO - missing information + headers!)
//

#pragma once

enum class STR_LanguageID_v1 : uint32_t
{
  Default = 0,
  English = 1,
  German = 2,
  French = 3,
  Spanish = 4,
  Italian = 5,
  Dutch = 6
};

struct STR_Header_v1
{
  char id[0xC] = {'I', 'O', 'I', 'S', 'N', 'D', 'S', 'T', 'R', 'E', 'A', 'M'}; // always "IOISNDSTREAM"
  uint8_t unkC[0x4]; // always seems to be a sequence 09 00 00 00
  uint32_t offsetToEntryTable = 0; // points at the STR_Footer, right after string table ends
  uint32_t entriesCount = 0; // same as number of STR_Data entries in STR_Footer
  uint32_t dataBeginOffset = 0x100; // offset to beginning of data probably, but it is like this even for PC_Eng.str which does not have such size and has no data...
  uint8_t unk1C[0x8]; // always seems to be a sequence 00 00 00 00 01 00 00 00
  STR_LanguageID_v1 languageId = STR_LanguageID_v1::Default; // specifies which language data is contained within the archive
};

enum class STR_DataFormat_v1 : uint32_t
{
  INVALID = 0x00,
  PCM_S16 = 0x02,
  IMA_ADPCM = 0x03,
  OGG_VORBIS = 0x04,
  DISTANCE_BASED_MASTER = 0x11
};

// beware that this is really 3 different headers, as there is no padding... didn't know how to name things so left it like this for now..
struct STR_DataHeader_v1
{
  // PCM_S16, IMA_ADPCM, OGG_VORBIS and DISTANCE_BASED_MASTER have following bytes
  STR_DataFormat_v1 format; // specifies how data should be read
  uint32_t samplesCount; // samples count
  uint32_t channels; // number of channels
  uint32_t sampleRate; // sample rate
  uint32_t bitsPerSample; // bits per sample

  // all PCM_S16, IMA_ADPCM and DISTANCE_BASED_MASTER have following bytes on top
  uint32_t blockAlign; // block alignment

  // all IMA_ADPCM have following bytes on top
  uint32_t samplesPerBlock; // samples per block
};

struct STR_Entry_v1
{
  uint64_t id; // probably some ID, is less than total entries count, does not match its index
  uint64_t dataOffset; // offset to beginning of data, beware of the distance-based records which alias the same index!
  uint64_t dataSize; // data size
  uint64_t dataHeaderOffset; // offset to table containing header
  uint32_t dataHeaderSize; // size of STR_DataHeader_v1 (unused fields from the structure are left out)
  uint32_t unk24; // unknown number
  uint64_t fileNameLength; // length of filename in string table
  uint64_t fileNameOffset; // offset to filename in string table
  uint32_t hasLIP; // 0x04 when LIP data is present for current entry, 0x00 otherwise
  uint32_t unk3C; // unknown number
  uint64_t distanceBasedRecordOrder;  // if 0, entry is not distance-based, otherwise denotes data order of individual records in data block (or is simply non-zero for master record)
};

using STR_LanguageID_v2 = STR_LanguageID_v1;

struct STR_Header_v2
{
  char id[0xC] = {'I', 'O', 'I', 'S', 'N', 'D', 'S', 'T', 'R', 'E', 'A', 'M'}; // always "IOISNDSTREAM"
  uint8_t unkC[0xC]; // always seems to be a sequence 09 00 00 00 XX XX YY YY 00 00 00 00 where XX XX changes with language and game and YY YY is same for a game (Kane & Lynch: Dead Man has this sequence E1 46, Mini Ninjas has this sequence 4C 4A)
  uint32_t offsetToEntryTable = 0; // points at the STR_Footer, right after string table ends
  uint32_t entriesCount = 0; // same as number of STR_Data entries in STR_Footer
  uint32_t dataBeginOffset = 0x100; // offset to beginning of data probably, but it is like this even for PC_Eng.str which does not have such size and has no data...
  uint8_t unk24[0x8]; // always seems to be a sequence 00 00 00 00 01 00 00 00
  STR_LanguageID_v2 languageId = STR_LanguageID_v2::Default; // specifies which language data is contained within the archive
  uint8_t unk30[0x8]; // always some sequence 38 XX XX XX XX XX XX XX where XX is same for a game (Kane & Lynch: Dead Man has this sequence 00 A1 01 18 EE 90 7C, Mini Ninjas has this sequence 00 00 00 00 00 00 00)
};

enum class STR_DataFormat_v2 : uint32_t
{
  INVALID = 0x00,
  PCM_S16 = 0x02,
  IMA_ADPCM = 0x03,
  OGG_VORBIS = 0x04,
  UNKNOWN_MASTER = 0x1A
};

// beware that this is really 2 different headers, as there is no padding... didn't know how to name things so left it like this for now..
struct STR_DataHeader_v2
{
  // PCM_S16, IMA_ADPCM, OGG_VORBIS and UNKNOWN_MASTER have following bytes
  STR_DataFormat_v2 format; // specifies how data should be read
  uint32_t samplesCount; // samples count
  uint32_t channels; // number of channels
  uint32_t sampleRate; // sample rate
  uint32_t bitsPerSample; // bits per sample
  uint32_t unk14 = 0;
  uint32_t unk18 = 0;
  uint32_t blockAlign; // block alignment

  // all IMA_ADPCM have following bytes on top
  uint32_t samplesPerBlock; // samples per block
};

struct STR_Entry_v2
{
  uint64_t id; // probably some ID, is less than total entries count, does not match its index
  uint64_t dataOffset; // offset to beginning of data, beware of the distance-based records which alias the same index!
  uint64_t dataSize; // data size
  uint64_t dataHeaderOffset; // offset to table containing header
  uint32_t dataHeaderSize; // size of STR_DataHeader_v2 (unused fields from the structure are left out)
  uint32_t unk24; // unknown number
  uint64_t fileNameLength; // length of filename in string table
  uint64_t fileNameOffset; // offset to filename in string table
  uint32_t hasLIP; // 0x04 when LIP data is present for current entry, 0x00 otherwise
  uint32_t unk3C; // unknown number
  uint64_t unk40;  // OLD INFO: if 0, entry is not distance-based, otherwise denotes data order of individual records in data block (or is simply non-zero for master record)
};
