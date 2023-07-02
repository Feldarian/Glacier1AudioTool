//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: Unlicense
//
// This file is not licensed the same way as is the rest of the project. You may freely copy and use this file
// in any of your projects. See Unlicense license.
//
// Header partially defines structure of WAV files used by Hitman Silent Assassin.
// It is specific for Hitman Silent Assassin, it is written as simply as possible wihtout using anything fancy,
// with any information I had about each field written next to it.
//
// It represents audio streams data, shared between multiple scenes. 
// Streams readability may be dependent on WHD information also, similiarly to other Glacier 1 games, needs to be checked.
//

#pragma once

struct StreamsWAV_Header
{
  uint32_t unk0; // footer offset?
  uint32_t fileSizeWithHeader;
  uint32_t unk8; // always 3
  uint32_t unkC; // always 4
};
