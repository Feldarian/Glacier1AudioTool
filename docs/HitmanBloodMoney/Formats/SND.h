//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: Unlicense
//
// This file is not licensed the same way as is the rest of the project. You may freely copy and use this file
// in any of your projects. See Unlicense license.
//
// Header partially defines structure of SND files used by Hitman Blood Money.
// It is specific for Hitman Blood Money, it is written as simply as possible wihtout using anything fancy,
// with any information I had about each field written next to it.
//
// Files represent a sound graph for specific scene with same name. They are contained in scene's ZIP file.
//

#pragma once

// always just 16 bytes like this
struct SND_Header
{
  uint32_t offsetToFooter; // points at the SND_Footer
  uint32_t fileSize; // total file size including header
  uint32_t unk8; // always 3
  uint32_t unkC; // always 4
};

// footer is always at the end of the file, always of size 0x1E0
struct SND_Footer
{
  uint32_t offsetToReservedTable1; // table always filled with nulls
  uint32_t offsetToStreamsFilename; // always 0x10 as this is always right after basic header
  uint32_t offsetToRootGroupEntry; // seems to point to some SND_EntryGroup... maybe some root group? if null it is not present in file
  char unkC[0x74 - 0xC];
  uint32_t offsetsToTableOfSoundEntries; // all offsets points to SND_EntrySound, if null it is not present in file
  uint32_t offsetsToTableOfSetEntries; // all offsets points to SND_EntrySet, if null it is not present in file
  uint32_t offsetsToTableOfSequenceEntries; // all offsets points to SND_EntrySequence, if null it is not present in file
  uint32_t offsetsToTableOfWaveEntries; // all offsets points to SND_EntryWave, if null it is not present in file
  uint32_t offsetsToTableOfDefineEntries; // all offsets points to SND_EntryDefine, if null it is not present in file
  uint32_t unk_offsetsToTableOfEntriesWithType_6; // unknown, assume that this is type table for 6 entries, all offsets points to SNDENTRY, if null it is not present in file
  uint32_t offsetsToTableOfUnknownOffsets1; // all offsets points to seemingly null bytes, at least always a single row of 16-bytes with 0, had not observed more than 16-bytes yet
  char unk90[0x1E0 - 0x90];
};

// all observed types, types seem to be aligned on 16-byte boundary
enum SND_EntryType
{
  INVALID = 0x00, // if it does not exists... maybe it is used as some unknown or something
  SOUND = 0x01, // looks like "audio:<ID>#sound"
  SET = 0x02, // looks like "audio:<ID>#set"
  SEQUENCE = 0x03, // looks like "audio:<ID>#seq"
  WAVE = 0x04, // does not have ID used it seems, but can be seen in file as "audio:<ID>#waves"
  DEFINE = 0x05, // does not have ID used it seems, but can be seen in file as "audio:<ID>#def"
  GROUP = 0x17, // looks like "audio:<ID>#group"
};

// seems to be before SND_UNKTYPE1
struct SND_UNKTYPE2
{
  uint32_t unk_size; // possibly number of bytes that follows? seems to be 12 in test cases I looked at
  uint32_t offsetToGroupEntry; // offset to SNDENTRY of group this belongs to
  uint32_t nullsOrReservedOrUnknown[2];
};

struct SND_UNKTYPE1
{
  uint32_t offsetToUnknown2; // offset to SNDUNKTYPE2
  uint32_t nullsOrReservedOrUnknown[3];
};

struct SND_EntrySound
{
  uint32_t type; // enum SND_EntryType::SOUND
  uint32_t unk4; // seems to be 0xCDCDCDCD in test files, check what it does, may be reserved or padding
  uint32_t offsetToIdentifier; // aligned on 16 bytes, null-terminated string, seems to always be some kind of string ID aligned like specified
  uint16_t unkC; // equals 1 for example
  uint16_t unkE; // equals 0xCDCD in test files, check what it does, may be reserved

  // TODO - these may or may not belong to the entry, unsure of sizes
  uint32_t offsetToUnknown1; // some offset to some 16-byte type
  uint32_t offsetToWaveEntry; // offset to wave entry SND_EntryWave, if null then not present in file
  char unk18[0x30 - 0x18]; // unknown, doesn't seem important for now... 
};

struct SND_EntrySet
{
  uint32_t type; // enum SND_EntryType::SET
  uint32_t unk4; // seems to be 0xCDCDCDCD in test files, check what it does, may be reserved or padding
  uint32_t offsetToIdentifier; // aligned on 16 bytes, null-terminated string, seems to always be some kind of string ID aligned like specified
  uint16_t unkC; // equals 2 for example
  uint16_t unkE; // equals 0xCDCD in test files, check what it does, may be reserved

  // TODO - these may or may not belong to the entry, unsure of sizes
  uint32_t unk10; // maybe offset or size, was 0x40 in test case
  uint32_t offsetToTableOfSoundEntries; // offset to table of offsets to SND_EntrySound entries, if null then not present in file, if entry in table is null then that is the table end, 16-byte aligned, probably any size possible, took 32 bytes in test case
  uint16_t unk18; // was 6 in example, same as number of entries in table...
  uint16_t unk1A; // was 6 in example, same as number of entries in table...
  uint32_t unk1C; // was 0 in example
  uint32_t unk20; // was 2 in example
  char unk24[0x30 - 0x24]; // unknown, doesn't seem important for now... seem like nulls anyway
};

struct SND_EntrySequence
{
  uint32_t type; // enum SND_EntryType::SEQUENCE
  uint32_t unk4; // seems to be 0xCDCDCDCD in test files, check what it does, may be reserved or padding
  uint32_t offsetToIdentifier; // aligned on 16 bytes, null-terminated string, seems to always be some kind of string ID aligned like specified
  uint16_t unkC; // equals 1 for example
  uint16_t unkE; // equals 0xCDCD in test files, check what it does, may be reserved

  // TODO - these may or may not belong to the entry, unsure of sizes
  uint32_t unk10; // maybe offset or size, was 0x30 in test case
  uint32_t offsetToSetEntry; // offset to wave entry SND_EntrySet, if null then not present in file
  uint16_t unk18; // was 2 in example
  uint16_t unk1A; // was 2 in example
  uint16_t unk1C; // was 0x64 in example
  uint16_t unk1E; // was 0xCDCD in example
};

struct SND_EntryWave
{
  uint32_t type; // enum SND_EntryType::WAVE
  uint32_t waveFileNameLength; // length of wave file name contained in the following offset, without null-terminator
  uint32_t waveFileNameOffset; // offset to null-terminated wave file name, aligned on 16-bytes
  uint32_t unkC; // maybe reserved, seens to be 0, check

  // TODO - these may or may not belong to the entry, unsure of sizes
  uint32_t unk10; // maybe reserved, seens to be 0, check
  uint32_t offsetToSetEntryID; // offset to ID of sound set, in form "audio:<ID>#set"
  uint32_t unk_timestamp32bit; // may be timestamp time_t in 32-bit format, check
  uint32_t unk_nullMaybeReserved; // unknown, seems to be null
};

struct SND_EntryDefine
{
  uint32_t type; // enum SND_EntryType::DEFINE
  uint32_t unk4; // seems to be 0xCDCDCDCD in test files, check what it does, may be reserved or padding
  uint32_t offsetToIdentifier; // aligned on 16 bytes, null-terminated string, seems to always be some kind of string ID aligned like specified
  uint16_t unkC; // equals 0 for example
  uint16_t unkE; // equals 0xCDCD in test files, check what it does, may be reserved

  // TODO - these may or may not belong to the entry, unsure of sizes
  uint32_t unk10; // maybe offset, was 0 in test case
  uint32_t offsetToUnknown; // all offsets points to seemingly null bytes, at least always a single row of 16-bytes with 0, had not observed more than 16-bytes yet, possibly same offset as some which can be found in SND_Footer::offsetsToTableOfUnknownOffsets1 table
  uint16_t unk18; // was 16 in example
  uint16_t unk1A; // was 16 in example
  uint32_t unk1C; // was 0 in example
};

struct SND_EntryGroup
{
  uint32_t type; // enum SND_EntryType::GROUP
  uint32_t unk4; // (seems to be 0xCDCDCDCD in test files, check what it does, may be reserved or padding)
  uint32_t offsetToIdentifier; // (aligned on 16 bytes, null-terminated string, seems to always be some kind of string ID aligned like specified)
  uint16_t unkC; // equals 0 for example
  uint16_t unkE; // equals 0xCDCD in test files, check what it does, may be reserved

  // TODO - these may or may not belong to the entry, unsure of sizes
  uint32_t unk10; // was 0 in example
  uint32_t offsetToTableOfChildGroupEntries; // offset to table of offsets to other SND_EntryGroup entries, if null then not present in file, unsure if some fixed size or alignemnt, assume 16-byte alignemnt, maybe fixed size of 0xA0, may contain NULLs (no childs either)
  char unk18[0x70 - 0x18]; // unknown, doesn't seem important for now... possibly 0x30 instead of 0x70 as in SNDENTRY_SOUND, but not known...
};
