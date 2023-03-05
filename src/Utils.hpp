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

template <typename InstanceType>
class Singleton
{
public:
  static InstanceType& Get()
  {
    static InstanceType instance;
    return instance;
  }
};

std::vector<char> ReadWholeBinaryFile(const std::filesystem::path &acpPath);

std::string ReadWholeTextFile(const std::filesystem::path &acpPath);

float GetAlignedItemWidth(int64_t acItemsCount);

const ImWchar *GetGlyphRanges();

std::wstring BrowseDirectoryDialog();

std::wstring OpenFileDialog(std::wstring_view filters, std::wstring fileName);
std::vector<std::wstring> OpenFileDialogMultiple(std::wstring_view filters, std::wstring fileName);

std::wstring SaveFileDialog(std::wstring_view filters, std::wstring fileName);

std::vector<std::filesystem::path> GetAllFilesInDirectory(const std::filesystem::path &directory,
                                                          std::wstring_view extension, bool recursive);

std::wstring GetTemporaryFilePath();

const std::wstring &GetProgramPath();

int32_t DisplayError(std::string_view message, std::string_view title = "", bool yesNo = false);

int32_t DisplayWarning(std::string_view message, std::string_view title = "", bool yesNo = false);

std::vector<std::wstring_view> GetPathStems(const std::filesystem::path& path);

std::filesystem::path ChangeExtension(const std::filesystem::path& path, std::wstring_view newExtension);
