//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: Unlicense
//
// This file is not licensed the same way as is the rest of the project. You may freely copy and use this file
// in any of your projects. See Unlicense license.
//
// Header partially defines structure of WHD+WAV files used by Hitman Blood Money.
// It is specific for Hitman Blood Money, it is written as simply as possible wihtout using anything fancy,
// with any information I had about each field written next to it.
//
// It represents sound data for specific scene with same name. They can't be opened without each other.
// It is required to properly parse data in scene WAV files. Scene WAV files do not contain any sort of header 
// information and need information from WHD to be usable.
//
// Whole file is aligned on 16 bytes. This applies to both, WHD and WAV scene files.
//
// The file does not has any known direct pointers into at the time of writting. Best method may not be best after more
// of SND file is reverse engineered. For now, below is very stable algorithm without sideeffects you can use to parse
// scene WHD+WAV pairs. Algorithm was verified with ton of assertions, bunch of which are still left in G1AT for debug 
// builds. Also, great care was taken to make sure algorithm doesn't go out of bounds anywhere (only exception is aliased
// stream entry, which was done for simplification and as this was proven to work without sideeffects).
//
// How to parse:
// - read in WHD_Header and validate known fields
// - while not WHD_Header.fileSizeWithHeader offset reached
//   - go to the end of a string, including single byte for null terminator
//   - make sure you ended up on aligned address on 16 byte boundary and if not, add enough bytes to offset to be aligned
//   - try to read in WHD_Record
//     - if any of the last 12 bytes is not null, queue checkValidHeader()
//     - if another 4 bytes from end are not null and first four bytes are not 'PHO\0', queue checkValidHeader()
//     - if queued checkValidHeader(), check that first four bytes are null
//       - if not, set reset read offset to the point before "try to read in WHD_Record" and go back to 
//         "while not WHD_Header.fileSizeWithHeader offset reached"
//       - if yes, subtract 12 bytes from read offset and continue
//     - either check passed with 12 subtracted bytes or this is another iteration of the loop where nothing failed
//     - we can read in first 4 bytes of WHD_Record now and determine record subtype
//     - if after read offset points to null byte, repeat single time from "try to read in WHD_Record"
//     - otherwise, if this is second iteration of block "try to read in WHD_Record", subtract 8 bytes from read offset
// - WHD file parsing finished
//

#pragma once

struct WHD_Header
{
  uint32_t fileSizeWithoutHeader;
  uint32_t fileSizeWithHeader;
  uint32_t unk8; // always 3
  uint32_t unkC; // always 4
};

union WHD_Record
{
  // record with this header has its data in scene's WAV file at the given data offset with given format
  struct WHD_RecordScene {
    uint32_t filePathLength;
    uint32_t filePathOffset;
    uint16_t formatTag; // 1 - PCM 16-bit, 17 - IMA ADPCM, 4096 - OGG
    uint16_t dataInStreams; // always 0, 0xFF00 mask is always 0
    uint32_t sampleRate;
    uint32_t bitsPerSample; // 0 for OGG
    uint32_t dataSizeUncompressed;
    uint32_t dataSize;
    uint32_t channels;
    uint32_t dataOffset;
    uint32_t samplesCount;
    uint32_t blockAlign; // something weird for OGG!
    uint32_t samplesPerBlock; // 0xCDCDCDCD for PCMs, 0x07F9 for ADPCM, 0x004F3E93 for OGG...
    uint32_t pad30[4]; // all are always 0 without exception
  };

  // record with this header has its data in stream STR file somewhere at the given data offset with given format
  // it needs to be parsed specially, as it may contain LIP data before actual data, and it contains 2 datas concated
  // together, aligned on 0x0100
  // data of this type should never have other format than PCM S16
  // records of this type need to know full data block size in STR file and calculate offsets to real data from end based on data size
  // dont forget the mentioned alignment between data segments!
  // realistically, this structure is 16 bytes less or is of structure 2x this minus 16 bytes for each! for ease of use and parsing though, we can pretend it has same size and ignore padding bytes, it is always safe
  struct WHD_RecordStreamsAliased
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
    uint32_t pad34[3]; // these may not be null, if they are not null, there is some other aliased entry behind, there is always block of 16 bytes zeroed after last entry
  };

  // record with this header has its data in stream STR file somewhere at the given data offset with given format
  // it needs to be parsed specially in STR, as it may contain LIP data before actual data, aligned on 0x0100
  // records of this type need to know full data block size in STR file and calculate offsets to real data from end based on data size
  // dont forget the mentioned alignment between data segments!
  // data of this type should never have other format than IMA ADPCM or PCM S16
  struct WHD_RecordStreams
  {
    uint32_t id; // always "PHO\0" (0x004F4850)
    uint32_t filePathOffset;
    uint16_t formatTag; // 1 - PCM 16-bit, 17 - IMA ADPCM, 4096 - OGG
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
    uint32_t pad34[3]; // all are always 0 without exception
  };

  // sizes of all different record types must match! (unless mentioned different variant of WHD_RecordStreamsAliased is used)
  static_assert(sizeof(WHD_RecordScene) == sizeof(WHD_RecordStreamsAliased));
  static_assert(sizeof(WHD_RecordScene) == sizeof(WHD_RecordStreams));
};

union WHDWAV_Header
{
  uint32_t unk0; // always 0
  uint32_t fileSizeWithHeader;
  uint32_t unk8; // always 3
  uint32_t unkC; // always 4
};
