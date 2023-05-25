//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#pragma once

struct RIFFHeaderPCM
{
  char riffId[4] = {'R', 'I', 'F', 'F'};
  uint32_t riffSize = 0x24;
  char waveId[4] = {'W', 'A', 'V', 'E'};
  char fmtId[4] = {'f', 'm', 't', ' '};
  uint32_t fmtSize = 0x10;
  uint16_t fmtFormat = 1;
  uint16_t fmtChannels = 0;
  uint32_t fmtSampleRate = 0;
  uint32_t fmtAvgBytesRate = 0;
  uint16_t fmtBlockAlign = 0;
  uint16_t fmtBitsPerSample = 0;
  char dataId[4] = {'d', 'a', 't', 'a'};
  uint32_t dataSize = 0;
};

struct RIFFHeaderADPCM
{
  char riffId[4] = {'R', 'I', 'F', 'F'};
  uint32_t riffSize = 0x34;
  char waveId[4] = {'W', 'A', 'V', 'E'};
  char fmtId[4] = {'f', 'm', 't', ' '};
  uint32_t fmtSize = 0x14;
  uint16_t fmtFormat = 17;
  uint16_t fmtChannels = 0;
  uint32_t fmtSampleRate = 0;
  uint32_t fmtAvgBytesRate = 0;
  uint16_t fmtBlockAlign = 0;
  uint16_t fmtBitsPerSample = 0;
  uint16_t fmtExtraSize = 0x02;
  uint16_t fmtExtra = 1017;
  char factId[4] = {'f', 'a', 'c', 't'};
  uint32_t factSize = 0x04;
  uint32_t factSamplesCount = 0;
  char dataId[4] = {'d', 'a', 't', 'a'};
  uint32_t dataSize = 0;
};

std::vector<char> ReadWholeBinaryFile(StringView8CI acpPath);

String8 ReadWholeTextFile(StringView8CI acpPath);

float GetAlignedItemWidth(int64_t acItemsCount);

String8CI BrowseDirectoryDialog();

String8CI OpenFileDialog(StringView8CI filters, StringView8CI defaultFileName);

String8CI SaveFileDialog(StringView8CI filters, StringView8CI defaultFileName);

std::vector<String8CI> GetAllFilesInDirectory(StringView8CI directory, StringView8CI extension, bool recursive);

StringView8CI GetProgramPath();

int32_t DisplayError(StringView8 message, StringView8 title = "", bool yesNo = false);

int32_t DisplayWarning(StringView8 message, StringView8 title = "", bool yesNo = false);

std::vector<StringView8CI> GetPathStems(StringView8CI pathView);

String8CI ChangeExtension(StringView8CI path, StringView8CI newExtension);
