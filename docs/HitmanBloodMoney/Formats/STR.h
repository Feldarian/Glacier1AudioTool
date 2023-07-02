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

#pragma once

struct STR_Header {
  char id[0x10];
  uint32_t offsetToFooter; // points at the STR_Footer, right after string table ends
  uint32_t unk14[0x100 - 0x14];
};

struct STR_Footer {
};

// LIP chunk which may be at the start of some sound files data, it is aligned on 0x100 boundary
struct STR_LIPChunk
{
   uint32_t id; // compares to first 4 chars from "LIP "
   uint32_t dataSize;
   // follows dataSize, note that structure is aligned on 0x100 (same as header)
};
