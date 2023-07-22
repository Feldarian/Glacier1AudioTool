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
// field for LIP encoding use which you may check without need to compare magic of each data block. For aliased entries, you will have to
// look into referenced entry to see if LIP encoding is used. There may be multiple LIP segments in the data block, but only first one
// has magic in first four bytes. Detection method for LIP segments currently relies on the fact that archive is aligned and that we can
// calculate exact size of each data block. We also know that each LIP segment starts at some fixed offset (relative to given data offset)
// which must be aligned on 0x1000. We start by subtracting true data size from LIP encoded entry and then mask result with ~0xFFull which 
// gives us size of all additional data included by LIP encoding. We can then infer number of LIP segments by dividing this number with 
// header size (note that in case 0xF00 is not aligned the same way as WAV data blocks are, you have to use the block alignment instead!)
// When we know number of expected LIP segments, we may try to search for pattern filled with zeroes which has half the size of header 0x780.
// First match of the pattern after 0x1000 should be the interesting point. You should then align yourself on 0x100 to the right and continue
// adding and checking next 16 bytes until some of the bytes is not zero. Verify you found correct offset and if not, correct yourself (you 
// shouldn't be on an offset which is not divisible by 0x100 and when you subtract either header (or block alignment size, depending on the case),
// the offset you end up on should be the variadic size of the LIP segment encoding for this WAV data).
//
// Last LIP segment is never aligned in some special way other than appending zeroes to fit into 0x100 alignment requirement, but LIP doesn't
// seem to make any special restrictions in this regard. 
//
// Aliased entries always have STR_EntryHeader of header type 0x02. 
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

// BEWARE this is really 3 different headers, as there is no padding... didn't know how to name things so left it like this for now..
struct STR_EntryHeader
{
  // all 0x02, 0x03, 0x04 and 0x11 entries have following bytes
  uint32_t headerType; // always one of 0x02, 0x03, 0x04, 0x11
  uint32_t samplesCount; // samples count
  uint32_t channels; // number of channels
  uint32_t sampleRate; // sample rate
  uint32_t bitsPerSample; // bits per sample

  // all 0x02, 0x03 and 0x11 entries have following bytes on top
  uint32_t blockAlign; // block alignment

  // all 0x03 and 0x11 entries have following bytes on top
  uint32_t samplesPerBlock; // samples per block
}

struct STR_Entry 
{
  uint64_t id; // probably some ID, is less than total entries count, does not match its index
  uint64_t dataOffset; // offset to beginning of data, BEWARE OF ALIASED RECORDS!
  uint64_t dataSize; // data size
  uint64_t headerOffset; // offset to some table which seems to contain headers
  uint32_t sizeOfHeader; // size of header
  uint32_t unk24; // unknown number, doesnt seem to be an offset, maybe some size?
  uint64_t fileNameLength; // length of filename in string table
  uint64_t fileNameOffset; // offset to filename in string table
  uint32_t hasLIP; // 0x04 when LIP data is present for current entry, 0x00 otherwise
  uint32_t unk3C; // was 0x30 in sample where LIP data was present, 0x0 in other
  uint64_t aliasedEntryOrderOrReference; // if 0, entry is not aliased, otherwise it denotes entry order in aliased data block or reference to one
};

// LIP segment which may be used to store sound data, it has variadic size which is unknown how it can be retrieved at the moment.
// First segment always starts with 4 byte magic 'LIP '. There is always block of 0xF00 extra data at the beginning of each segment,
// followed by block of data. Last segment is not aligned, only on 0x100 as is the rest of the data-related blocks. 
struct STR_LIPSegment
{
   uint32_t id; // compares to first 4 chars from "LIP " for first segment
   char unk4[0x0F00 - 0x04]; // unknown contents, but there are a lot of visible patterns...
   // NOTE - there may be additional block of 0x0100 or similar due to block alignment of data, which when bigger, forces some additional padding bytes inbetween the data
   char data[]; // data of variadic size, always aligned the same way as is block alignment
};
