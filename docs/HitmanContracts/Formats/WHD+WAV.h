//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: Unlicense
//
// This file is not licensed the same way as is the rest of the project. You may freely copy and use this file
// in any of your projects. See Unlicense license.
//
// Header partially defines structure of WHD+WAV files used by Hitman Contracts.
// It is specific for Hitman Contracts, it is written as simply as possible wihtout using anything fancy,
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

struct WHD_Record
{
  uint32_t type; // always 6
  uint32_t filePathOffset;
  uint16_t formatTag; // either 4096 == OGG, 17 == IMA ADPCM, 1 == PCM (S16)
  uint16_t dataInStreams; // either 0 or 32768, dataInStreams != 0 => data is in streams
  uint32_t sampleRate;
  uint32_t bitsPerSample;
  uint32_t dataSizeUncompressed; // uncompressed data size, equal dataSize when formatTag == 1
  uint32_t dataSize;
  uint32_t channels;
  uint32_t dataOffset;
  uint32_t samplesCount; // always half of dataSizeUncompressed
  uint32_t blockAlign;
  uint32_t fmtExtra;  // always 1 when formatTag == 1
};

struct WHDWAV_Header
{
  uint32_t unk0; // always 0
  uint32_t fileSizeWithHeader;
  uint32_t unk8; // always 3
  uint32_t unkC; // always 4
};
