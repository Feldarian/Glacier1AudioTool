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
// Streams can be read on its own, it contains all of the required data. It should actually be read before any scenes
// when someone wants to do anything audio-related to have best support.
//
// There are some extra data that are not related to aliased entries. These should be valid for export, but had not tested it yet.
// They may also contain LIP data segments instead of regular data dumps so beware.
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
// Any WAV data block may be encoded in LIP segments which have variable length and header of size 0xF00 ('LIP ' magic). Record contains
// field which can be checked if it uses LIP encoding without need to compare magic of each data block. For aliased entries, you will have to
// look into referenced entry to see if LIP encoding is used. There may be multiple LIP segments in the data block, but only first one
// has magic in first four bytes. Due to varialble data length, we have to find out the right size LIP segment first before parsing. It 
// seems to appear roughly every ~4 seconds, but naive formula of `average byte rate * 4` just roughly yields where the end of LIP segment is.
// Size is for all LIP segments (excluding last one) aligned on 0x1000.
//
// Current detection method for LIP segments relies on the fact that archive is aligned, we know roughly where the offset should be and that
// we can calculate exact size of each data block. There is also additional observation to be made that nearly all LIP segments seem to have
// around half of their data filled with zeroes. We can also notice that when we subtract real data size aligned on 0x100 from whole data block
// size, we get amount of bytes belonging to LIP segments. We can then calculate from this size amount of LIP segments in the data block. There
// may be only one, which does not require us to do any magic - we just have to skip past the header and read real data size bytes, ignoring any
// alignment paddings. If there are more segments, we can proceed with calculation of segment size (as it is not equal whole block size).
//
// As mentioned before, we rougly know when each of LIP segments appears in the audio file (it is roughly equivalent to 4 seconds, leaving
// last block unaligned most of the time with smaller size). We should try to pattern match buffer of size 0x780 filled with zeroes, masking
// each found offset with ~0xFFFull which will left-align on 0x1000 and taking closest offset to the one we predicted. We then read in minimum
// of data block bytes left to read and this found LIP segment offset, skip aligned LIP header size and copy each part of the segment into its
// own buffer. In the end, we are left with complete LIP data and complete WAV data.
//
// Block of WAV data is organized in such a way that it has all simple entries at the beginning and all aliased entries at the end.
// There is no clear block of LIP data, it seems to be mixed randomly inbetween the entries so no reliable distinction in the block.
// Aliased entries point to same data offset, there are always exactly three such pointers (2 defined in WHD, 1 is only defined in STR). 
// There cannot be other number of "duplicates" than 1 (none) or 3 (aiased entry, 1 for selection and 2 templates). Third entry we mentioned
// is STR file only is the true data definition. It defines reference to aliased entry being used (same field as for aliased entry order). 
// If LIP data is present, reference record has appropriate flag set. Note that reference record should not really be used for other things,
// as its parameters are not exactly the same always and correct ones are located directly in the entry. 
//
// Due to all this, recomennded way to get to the actual data is to precalculate all individual WAV data block sizes and resolve aliased records. 
// 
// Below is best current process for parsing the STR file (TODO - already outdated, needs an update!). Algorithm was verified with ton of assertions, 
// bunch of which are still left in G1AT for debug builds. Also, great care was taken to make sure algorithm doesn't go out of bounds anywhere, 
// no exceptions to this rule.
//
// How to parse (TODO - already outdated, needs an update!):
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
// to reference WHDs for any sort of information.
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

enum STR_EntryHeaderFormat
{
  PCM_S16 = 0x02,
  ADPCM = 0x03,
  VORBIS = 0x04,
  ALIASED_MASTER = 0x11
}

// BEWARE this is really 3 different headers, as there is no padding... didn't know how to name things so left it like this for now..
struct STR_EntryHeader
{
  // PCM_S16, ADPCM, VORBIS and ALIASED_MASTER have following bytes
  uint32_t headerFormat; // always one of enum STR_EntryHeaderFormat options
  uint32_t samplesCount; // samples count
  uint32_t channels; // number of channels
  uint32_t sampleRate; // sample rate
  uint32_t bitsPerSample; // bits per sample

  // all PCM_S16, ADPCM and ALIASED_MASTER have following bytes on top
  uint32_t blockAlign; // block alignment

  // all ADPCM have following bytes on top
  uint32_t samplesPerBlock; // samples per block
}

struct STR_Entry 
{
  uint64_t id; // probably some ID, is less than total entries count, does not match its index
  uint64_t dataOffset; // offset to beginning of data, BEWARE OF ALIASED RECORDS!
  uint64_t dataSize; // data size
  uint64_t headerOffset; // offset to some table which seems to contain headers
  uint32_t sizeOfHeader; // size of STR_EntryHeader (unused fields are left out)
  uint32_t unk24; // unknown number
  uint64_t fileNameLength; // length of filename in string table
  uint64_t fileNameOffset; // offset to filename in string table
  uint32_t hasLIP; // 0x04 when LIP data is present for current entry, 0x00 otherwise
  uint32_t unk3C; // unknown number
  uint64_t aliasedEntryOrderOrReference; // if 0, entry is not aliased, otherwise it denotes entry order in aliased data block or reference to one
};

// exact contents of LIP segment header are unknown, but first segment header always has 4 byte magic "LIP "
// size is 0xF00, header is aligned on min(0x100, data alignment)
struct STR_LIPSegmentHeader
{
   uint32_t id; // compares to first 4 chars from "LIP " (0x2050494C) for first segment
   char unk4[0x0F00 - 0x04]; // unknown contents, but there are a lot of visible patterns...
};

// LIP segment which may be used to store sound data, it has variadic size which is unknown how it can be retrieved at the moment.
// First segment always has 4 byte magic 'LIP ' in its header. There is always segment header, then possibly padding (depends on if 
// header is aligned the same way data is), then variable number of blocks of data. All segments apart from last one are aligned on 
// 0x100 or data alignment, whatever is bigger. 
struct STR_LIPSegment
{
   STR_LIPSegmentHeader header;
   // NOTE - there may be additional block of 0x0100 or similar due to block alignment of data, which when bigger, forces some additional padding bytes inbetween the data
   // char alignmentPadding[/*0x100*/];
   char data[]; // data of variadic size, always aligned the same way as is block alignment
};
