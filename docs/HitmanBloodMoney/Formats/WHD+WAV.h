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
// The pair represents sound data for specific scene with same name. They can't be opened without each other.
// Scene WAV files do not contain any sort of header, only blobs of simple data. Scene WHD files contain all
// header information of all sound data required for the given scene. This data may be contained in scene or
// in stream STR file.
//
// Both of these scene files are completely aligned on 16 bytes.
//
// WHD file does not has any known direct pointers into at the time of writting. Best method may not be best after more
// of SND file is reverse engineered as it looks to contain some additional sound information in form of sound data graphs. 
// For now, below is very stable algorithm without sideeffects you can use to parse scene WHD+WAV pairs (TODO - already outdated,
// needs an update!). Algorithm was verified with ton of assertions, bunch of which are still left in G1AT for debug builds. Also, 
// great care was taken to make sure algorithm doesn't go out of bounds anywhere (only exception is aliased stream entry, which was 
// done for simplification and as this was proven to work without sideeffects).
//
// How to parse (TODO - already outdated, needs an update!):
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

// record with this header has its data in scene's WAV file at the given data offset with given format
// data has duplicate paths if not prefixed in some way, which may point to them being duplicates, needs to be checked
// data is never LIP encoded
// data of these records should never have other format than IMA ADPCM
struct WHD_Record_Scene {
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
  //uint32_t pad30[4]; // in WHD, has additional padding bytes filled with zeroes without exceptions
};

// sub record of WHD_Record_StreamsAliased, it has zero padding
struct WHD_Record_StreamsAliased_SubRecord
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
struct WHD_Record_StreamsAliased
{
  WHD_Record_StreamsAliased_SubRecord subRecords[2];
  //uint32_t pad34[3];  // in WHD, has additional padding bytes filled with zeroes without exceptions
};

// record with this header has its data in stream STR file somewhere at the given data offset with given format
// data may be LIP encoded
// data of this type should never have other format than IMA ADPCM, PCM S16 or OGG Vorbis
struct WHD_Record_Streams
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
  //uint32_t pad34[3]; // in WHD, has additional padding bytes filled with zeroes without exceptions
};

union WHDWAV_Header
{
  uint32_t unk0; // always 0
  uint32_t fileSizeWithHeader;
  uint32_t unk8; // always 3
  uint32_t unkC; // always 4
};
