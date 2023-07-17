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
// Streams can be read on its own, it contains all of the required data. Only problem is that I had not found reliable way to
// discard aliased stream entries without data gathered from all WHD files, as unused data in STR can be discarded by no presence.
// in WHD (without WHD referencing the record, record is unused).
//
// There are some extra data that are not related to aliased entries. These should be valid for export, but had not tested it yet.
// They may also contain LIP data segments before actual data so beware.
//
// Data in file can be separated into following sections, some have clear indices some can be implicitly infered:
// - header
// - block of WAV data (also contains LIP segments before some data)
// - block of WAV headers (different format than WHD, not fully parsed, is much simpler and more concise)
// - file name table (file names match those in WHD and SND records, there are some extras though so this is not full subset!)
// - records table (start marked in header along with records count)
//
// Data seems to be aligned on 0x0100 (which coincidentaly seems to also be size of header and offset to first WAV data block...)
// Alignment breaks after block of WAV data, as block of WAV headers is not aligned this way, same seems to be the case for file
// name table and records table... Alignment is only valid for part of the file, but it must be respected for that part of the file!
//
// Any WAV data block may be prepended with LIP segment ('LIP ' magic). This segment has unknown size and exact purpose, extraction of
// these entries is therefore currently "best effort" only. It is always prepended before the actual data.
//
// Block of WAV data is organized in such a way that it has all simple entries at the beginning and all aliased entries at the end.
// There is no clear block of LIP data, it seems to be mixed randomly inbetween the entries so no reliable distinction in the block.
// Aliased entries point to same data offset, there are always exactly three such pointers (2 defined in WHD, 1 is only defined in STR). 
// There cannot be other number of "duplicates" than 1 (none) or 3 (aiased entry, 1 dummy and 2 data-wise). Unknown exact purpose of third
// entry nor how to detect this seemingly bogus entry without comparing with WHD datas. Size of it though seems to be always equal size of
// of the other two chunks, but that seems to be some coincidense or it may be deliberate... Fact is though that the 2 datas are not
// of the same exact type, sample rates may differ for example.
//
// Due to all this, recomennded way to get to the actual data is to precalculate all individual WAV data block sizes and subtract data sizes
// of all entries to specific data offset mapped in WHD (and not just in STR due to mentioned "bogus" entries). After each subtraction, make
// sure you ended up on 0x0100 boundary and if not, subtract more bytes until you are. After reaching the boundary, subtract again, until
// there is nothing to subtract. Order of entries subtractions is dictated by STR_EntryHeader.blockAlignOrUnknown (all entries have 
// STR_EntryHeader.headerType 0x02 or 0x11 so they have this uint32_t field). This field dictates order of these two datas (NOTE: this seems
// to be 0x02 for some "bogus" entries, which aliases valid "index"... possible that "bogus" entry could be detected by size, as they will be
// inevitably bigger than the valid entry with same size). Logically, subtract items from biggest index to the smallest. You should be left 
// with either offset 0 or some non-zero number aligned on 0x0100. This is the expected LIP segment size. If 0, it is not present. You should 
// never end up with non-zero value and WAV data block without LIP segment. If this happens, something is broken in your parser. Not respecting
// alignments may lead to broken data exports.
//
// Below is best current process for parsing the STR file. Algorithm was verified with ton of assertions, bunch of which are still left in G1AT
// for debug builds. Also, great care was taken to make sure algorithm doesn't go out of bounds anywhere, no exceptions to this rule.
//
// How to parse:
// - read in STR_Header and validate known fields
// - jump to the STR_Header.offsetToEntryTable offset
// - make some variable holding a set of all unique data offsets 
// - make some variable which would hold offset to the start of block of WAV headers so we know where block of WAV data ends
//   - pre-initialize with STR_Header.offsetToEntryTable, it will never be bigger... and it is a valid offset in case of some bad failure
// - while not STR_Header.entriesCount entries read
//   - read in STR_Entry and validate known fields
//   - retrieve STR_EntryHeader or file name or data as required based on data in STR_Entry, but do not use them yet as you miss information!
//   - if offset to start of block of WAV headers is bigger than offset to WAV header in current record, assign new block of WAV headers start 
//     offset
//   - save data offset in set of all data offsets you made before looping
//     - mark attempts to add already present value to mark up aliased records
// - (optional for easier calculations) place offset to the start of block of WAV headers into the set
// - split block of WAV data based on offsets (don't forget that last data block is until the start of the block of WAV headers!)
// - you may now parse individual data blocks for each record based on all of the previous information gathered (meant for both, data from this
//   parsing algorithm and knowledge written before it)
//
// Note that format information of data, along with data sizes, offsets, names, etc. are all same as in equivalent WHD record. So there is no need
// to reference WHDs for this info, they are mostly required for reliably detecting and ignoring "bogus" entries.
//

#pragma once

struct STR_Header 
{
  char id[0x0C] = {'I', 'O', 'I', 'S', 'N', 'D', 'S', 'T', 'R', 'E', 'A', 'M'}; // always "IOISNDSTREAM"
  uint32_t version = 9; // always 0x09
  uint32_t offsetToEntryTable = 0; // points at the STR_Footer, right after string table ends
  uint32_t entriesCount = 0; // same as number of STR_Data entries in STR_Footer
  uint32_t dataBeginOffset = 0x0100; // offset to begining of data (or whole header size... or data alignemnt, as all data seems to be aligned so far...)
  uint32_t unk1C = 0; // was 0x0 in samples
  uint32_t unk20 = 1; // was 0x1 in all samples
  uint32_t unkLanguageId = 1; // was 0x1 in english samples and 0x2 in german, maybe some language id?
  uint32_t unk28[0x100 - 0x28] = {};
};

// BEWARE this is really 3 different headers, as there is no padding... didn't know how to name things so left it like this for now..
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
