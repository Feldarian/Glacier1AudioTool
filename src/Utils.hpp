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

std::vector<char> ReadWholeBinaryFile(const StringView8CI &acpPath);

String8 ReadWholeTextFile(const StringView8CI &acpPath);

float GetAlignedItemWidth(int64_t acItemsCount);

String8CI BrowseDirectoryDialog();

String8CI OpenFileDialog(const StringView8CI &filters, const StringView8CI &defaultFileName);

String8CI SaveFileDialog(const StringView8CI &filters, const StringView8CI &defaultFileName);

std::vector<String8CI> GetAllFilesInDirectory(const StringView8CI &directory, const StringView8CI &extension, bool recursive);

StringView8CI GetProgramPath();

int32_t DisplayError(const StringView8 &message, const StringView8 &title = LocalizationManager::Get().Localize("MESSAGEBOX_ERROR_GENERIC_TITLE"), bool yesNo = false);

int32_t DisplayWarning(const StringView8 &message, const StringView8 &title = LocalizationManager::Get().Localize("MESSAGEBOX_WARNING_GENERIC_TITLE"), bool yesNo = false, const Options &options = Options::Get());

std::vector<StringView8CI> GetPathStems(StringView8CI pathView);

String8CI ChangeExtension(const StringView8CI &path, const StringView8CI &newExtension);
