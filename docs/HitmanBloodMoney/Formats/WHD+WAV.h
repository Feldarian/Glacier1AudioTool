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
// Streams readability may be dependent on WHD information also, similiarly to other Glacier 1 games, needs to be checked.
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
  struct WHD_RecordScene {
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
    uint32_t fmtExtra; // 0xCDCDCDCD for PCMs, 0x07F9 for ADPCM, 0x004F3E93 for OGG...
    uint32_t nullBytes[4];
  };

  struct WHD_RecordSceneAliased
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
    uint32_t dataOffset;
    uint32_t samplesCount;
    uint32_t blockAlign; // something weird for OGG!
    uint32_t fmtExtra; // 0xCDCDCDCD for PCMs, 0x07F9 for ADPCM, 0x004F3E93 for OGG...
    uint32_t nullBytes[4];
  };

  struct WHD_RecordStreams
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
    uint32_t dataOffset; // may start with "LIP " (0x2050494C) chunk, ignore lip->dataSize bytes aligned on 0x100 boundary
    uint32_t samplesCount;
    uint32_t unk18;
    uint32_t fmtExtra; // 0xCDCDCDCD for PCMs, 0x07F9 for ADPCM, 0x004F3E93 for OGG...
    uint32_t unk2C;
    uint32_t nullBytes[3];
  };

  // sizes of all different record types must match!
  static_assert(sizeof(WHD_RecordScene) == sizeof(WHD_RecordSceneAliased));
  static_assert(sizeof(WHD_RecordScene) == sizeof(WHD_RecordStreams));
};

union WHDWAV_Header
{
  uint32_t unk0; // always 0
  uint32_t fileSizeWithHeader;
  uint32_t unk8; // always 3
  uint32_t unkC; // always 4
};
