//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: Unlicense
//
// This file is not licensed the same way as is the rest of the project. You may freely copy and use this file
// in any of your projects. See Unlicense license.
//
// Header partially defines structure of STR files used by Hitman Blood Money.
// It is specific for Hitman Blood Money, it is written as simply as possible wihtout using anything fancy,
// with any information I had about each field written next to it.
//
// It represents audio streams data, shared between multiple scenes.
// Streams readability may be dependent on WHD information also, similiarly to previous games, needs to be checked.
//
// String table is located right before entry table begins, it is not aligned and breaks structure alignment! Only
// table that seems to be unaligned in whole format...
//
// Data block is organized in such a way that it ends with aliased entries. Aliased entries point to same data offset, 
// there are always exactly three such pointers (2 defined in WHD, 1 is only defined in STR). There cannot be other 
// number of "duplicates" than 1 (none) or 3 (aiased entry, 1 entry split into 2 data-wise). It is almost certainly 
// split like this, as when one tries to compare this remove entry size, they find out that the size is the same as the sum
// of the other two. Offsets to specific data needs to be calculated in such a way that you subtract from last entry to first
// from complete WAV data block size. Each step, newly calculated offset should be anded with mask (~0xFF) so offset is aligned
// on 0x100 boundary from left. LIP data (if present) is non-zero remainder in front of data.
// 
// Completely all WAV data seems to be aligned, value seems to be same as STR_Header.dataBeginOffset but that may be
// a coincidence, relation unknown. Without aligning offsets though, mainly in case of data prepended with LIP, leads to
// broken exports. 
//
// For reading data starting with LIP, there is currently no better method (nor known if this is fully right) then to 
// treat such entries as concatenated and marking them as having LIP data.  This means getting whole WAV data block size
// and subtracting real data size from it, aligning offset on 0x100 from left.
//

#pragma once

struct STR_Header 
{
  char id[0x0C] = {'I', 'O', 'I', 'S', 'N', 'D', 'S', 'T', 'R', 'E', 'A', 'M'};
  uint32_t version = 9; // always 0x09
  uint32_t offsetToEntryTable = 0; // points at the STR_Footer, right after string table ends
  uint32_t entriesCount = 0; // same as number of STR_Data entries in STR_Footer
  uint32_t dataBeginOffset = 0x0100; // offset to begining of data (or whole header size... or data alignemnt, as all data seems to be aligned so far...)
  uint32_t unk1C = 0; // was 0x0 in samples
  uint32_t unk20 = 1; // was 0x1 in all samples
  uint32_t unkLanguageId = 1; // was 0x1 in english samples and 0x2 in german, maybe some language id?
  uint32_t unk28[0x100 - 0x28] = {};
};

struct STR_EntryHeader
{
  // all 0x02, 0x03, 0x04 and 0x11 entries have following bytes
  uint32_t headerType; // always one of 0x02, 0x03, 0x04, 0x11
  uint32_t unk4; // possibly data size
  uint32_t channels; // number of channels
  uint32_t sampleRate; // sample rate
  uint32_t bitsPerSample; // bits per sample

  // all 0x02, 0x03 and 0x11 entries have following bytes on top
  uint32_t blockAlignOrUnknown; // block alignment for 0x03, unknown for rest

  // all 0x03 entries have following bytes on top
  uint32_t samplesPerBlock; // samples per block
}

struct STR_Entry 
{
  uint64_t unk0; // some number, maybe id? doesnt seem to be too out of hand...
  uint64_t dataOffset; // offset to beginning of data, BEWARE OF ALIASED RECORDS!
  uint64_t dataSize; // data size
  uint64_t headerOffset; // offset to some table which seems to contain headers
  uint32_t sizeOfHeader; // size of header
  uint32_t unk24; // unknown number, doesnt seem to be an offset, maybe some size?
  uint64_t fileNameLength; // length of filename in string table
  uint64_t fileNameOffset; // offset to filename in string table
  uint32_t unk38; // was 0x4 in sample where LIP data was present, 0x0 in other
  uint32_t unk3C; // was 0x30 in sample where LIP data was present, 0x0 in other
  uint64_t unk40; // is not 0 always but in quite a few places was
};

// LIP chunk which may be at the start of some sound files data, unknown purpose and unknown how to properly detect size for now...
struct STR_LIPChunk
{
   uint32_t id; // compares to first 4 chars from "LIP "
   char unk4[]; // unknown contents, unknown size
};

// english steam version has data portion end at 0x6FD8C300 which marks begin of all header data
// then at offset 0x6FDA32D4 starts string table, which ends at begining of entry table which has recorded offset
