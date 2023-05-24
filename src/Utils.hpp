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

std::vector<char> ReadWholeBinaryFile(const std::filesystem::path &acpPath);

std::string ReadWholeTextFile(const std::filesystem::path &acpPath);

float GetAlignedItemWidth(int64_t acItemsCount);

std::wstring BrowseDirectoryDialog();

std::wstring OpenFileDialog(std::wstring_view filters, std::wstring fileName);
std::vector<std::wstring> OpenFileDialogMultiple(std::wstring_view filters, std::wstring fileName);

std::wstring SaveFileDialog(std::wstring_view filters, std::wstring fileName);

std::vector<std::filesystem::path> GetAllFilesInDirectory(const std::filesystem::path &directory,
                                                          std::wstring_view extension, bool recursive);

const std::wstring &GetProgramPath();

int32_t DisplayError(StringView8 message, StringView8 title = "", bool yesNo = false);

int32_t DisplayWarning(StringView8 message, StringView8 title = "", bool yesNo = false);

std::vector<std::wstring_view> GetPathStems(const std::filesystem::path& path);

std::filesystem::path ChangeExtension(const std::filesystem::path& path, std::wstring_view newExtension);

template <typename UTFCharTypeOutput, typename UTFCharTypeInput>
requires IsUTFCharType<UTFCharTypeOutput> && IsUTFCharType<UTFCharTypeInput>
const std::basic_string<UTFCharTypeOutput> &ToUTF(std::basic_string<UTFCharTypeOutput> &buffer, const std::basic_string_view<UTFCharTypeInput> utf)
{
  UErrorCode errorCode = U_ZERO_ERROR;

  buffer.clear();
  buffer.resize(utf.size() * std::max(1ull, sizeof(UTFCharTypeInput) / sizeof(UTFCharTypeOutput)), 0);

  size_t offset = 0;
  int32_t length = 0;
  if constexpr (IsUTF8CharType<UTFCharTypeOutput>)
  {
    if constexpr (IsUTF8CharType<UTFCharTypeInput>)
    {
      std::memcpy(buffer.data(), utf.data(), utf.size());
      length = utf.size();
    }
    else if constexpr (IsUTF16CharType<UTFCharTypeInput>)
      u_strToUTF8(buffer.data(), buffer.size(), &length, reinterpret_cast<const UChar *>(utf.data()), utf.size(), &errorCode);
    else if constexpr (IsUTF32CharType<UTFCharTypeInput>)
    {
      auto *reinterpretedBuffer = reinterpret_cast<uint8_t *>(buffer.data());
      const auto reinterpretedBufferSize = buffer.size();
      for (const auto utf32Char : utf)
      {
        bool wasError = false;
        const auto codePoint = static_cast<uint32_t>(utf32Char);
        U8_APPEND(reinterpretedBuffer, offset, reinterpretedBufferSize, codePoint, wasError);
        length = offset;
      }
    }
  }
  else if constexpr (IsUTF16CharType<UTFCharTypeOutput>)
  {
    if constexpr (IsUTF16CharType<UTFCharTypeInput>)
    {
      std::memcpy(buffer.data(), utf.data(), utf.size() * sizeof UChar);
      length = utf.size();
    }
    else if constexpr (IsUTF8CharType<UTFCharTypeInput>)
      u_strFromUTF8(reinterpret_cast<UChar *>(buffer.data()), buffer.size(), &length, reinterpret_cast<const char *>(utf.data()), utf.size(), &errorCode);
    else if constexpr (IsUTF32CharType<UTFCharTypeInput>)
      u_strFromUTF32(reinterpret_cast<UChar *>(buffer.data()), buffer.size(), &length, reinterpret_cast<const UChar32 *>(utf.data()), utf.size(), &errorCode);
  }
  else if constexpr (IsUTF32CharType<UTFCharTypeOutput>)
  {
    if constexpr (IsUTF32CharType<UTFCharTypeInput>)
    {
      std::memcpy(buffer.data(), utf.data(), utf.size());
      length = utf.size();
    }
    else if constexpr (IsUTF8CharType<UTFCharTypeInput>)
    {
      auto *reinterpretedBuffer = reinterpret_cast<UChar32 *>(buffer.data());
      const auto reinterpretedBufferSize = buffer.size();
      const auto *inputBuffer = utf.data();
      const auto inputBufferSize = utf.size();
      for (size_t inputOffset = 0; inputOffset < inputBufferSize;)
      {
        auto &outputChar = reinterpretedBuffer[offset++];
        U8_NEXT(inputBuffer, inputOffset, inputBufferSize, outputChar);
      }
      length = offset;
    }
    else if constexpr (IsUTF16CharType<UTFCharTypeInput>)
    {
      auto *reinterpretedBuffer = reinterpret_cast<UChar32 *>(buffer.data());
      const auto reinterpretedBufferSize = buffer.size();
      const auto *inputBuffer = utf.data();
      const auto inputBufferSize = utf.size();
      for (size_t inputOffset = 0; inputOffset < inputBufferSize;)
      {
        auto &outputChar = reinterpretedBuffer[offset++];
        U16_NEXT(inputBuffer, inputOffset, inputBufferSize, outputChar);
      }
      length = offset;
    }
  }

  if (errorCode > U_ZERO_ERROR)
    length = 0;

  buffer.resize(length, 0);
  return buffer;
}

template <typename UTFCharTypeOutput, typename UTFCharTypeInput>
requires IsUTFCharType<UTFCharTypeOutput> && IsUTFCharType<UTFCharTypeInput>
const std::basic_string<UTFCharTypeOutput> &ToUTF(std::basic_string<UTFCharTypeOutput> &buffer, const std::basic_string<UTFCharTypeInput> &utf)
{
  return ToUTF<UTFCharTypeOutput>(buffer, std::basic_string_view<UTFCharTypeInput>(utf));
}

template <typename UTFCharTypeOutput, typename UTFCharTypeInput, size_t UTFInputSize>
requires IsUTFCharType<UTFCharTypeOutput> && IsUTFCharType<UTFCharTypeInput>
const std::basic_string<UTFCharTypeOutput> &ToUTF(std::basic_string<UTFCharTypeOutput> &buffer, const UTFCharTypeInput (&utf)[UTFInputSize])
{
  return ToUTF<UTFCharTypeOutput>(buffer, std::basic_string_view<UTFCharTypeInput>(utf, UTFInputSize - 1));
}

template <typename UTFCharTypeOutput, typename UTFCharTypeInput>
requires IsUTFCharType<UTFCharTypeOutput> && IsUTFCharType<UTFCharTypeInput>
const std::basic_string<UTFCharTypeOutput> &ToUTF(std::basic_string<UTFCharTypeOutput> &buffer, const UTFCharTypeInput *utf, const size_t length)
{
  return ToUTF<UTFCharTypeOutput>(buffer, std::basic_string_view<UTFCharTypeInput>(utf, length));
}

template <typename UTFCharTypeOutput, typename UTFCharTypeInput>
requires IsUTFCharType<UTFCharTypeOutput> && IsUTFCharType<UTFCharTypeInput>
const std::basic_string<UTFCharTypeOutput> &ToUTF(const std::basic_string_view<UTFCharTypeInput> utf)
{
  thread_local static std::basic_string<UTFCharTypeOutput> utfOut;
  return ToUTF<UTFCharTypeOutput>(utfOut, utf);
}

template <typename UTFCharTypeOutput, typename UTFCharTypeInput>
requires IsUTFCharType<UTFCharTypeOutput> && IsUTFCharType<UTFCharTypeInput>
const std::basic_string<UTFCharTypeOutput> &ToUTF(const std::basic_string<UTFCharTypeInput> &utf)
{
  return ToUTF<UTFCharTypeOutput>(std::basic_string_view<UTFCharTypeInput>(utf));
}

template <typename UTFCharTypeOutput, typename UTFCharTypeInput, size_t UTFInputSize>
requires IsUTFCharType<UTFCharTypeOutput> && IsUTFCharType<UTFCharTypeInput>
const std::basic_string<UTFCharTypeOutput> &ToUTF(const UTFCharTypeInput (&utf)[UTFInputSize])
{
  return ToUTF<UTFCharTypeOutput>(std::basic_string_view<UTFCharTypeInput>(utf, UTFInputSize - 1));
}

template <typename UTFCharTypeOutput, typename UTFCharTypeInput>
requires IsUTFCharType<UTFCharTypeOutput> && IsUTFCharType<UTFCharTypeInput>
const std::basic_string<UTFCharTypeOutput> &ToUTF(const UTFCharTypeInput *utf, const size_t length)
{
  return ToUTF<UTFCharTypeOutput>(std::basic_string_view<UTFCharTypeInput>(utf, length));
}

// TODO - iterate codepoints and compare manually for better efficiency
template <typename UTFCharTypeLeft, typename UTFCharTypeRight = UTFCharTypeLeft>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const std::basic_string_view<UTFCharTypeLeft> left, const std::basic_string_view<UTFCharTypeRight> right)
{
  thread_local static std::basic_string<UChar> leftConverted;
  const auto *leftData = ToUTF(leftConverted, left).c_str();

  thread_local static std::basic_string<UChar> rightConverted;
  const auto *rightData = ToUTF(rightConverted, right).c_str();

  UErrorCode errorCode = U_ZERO_ERROR;
  return u_strCaseCompare(leftData, leftConverted.size(), rightData, rightConverted.size(), 0, &errorCode);
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight = UTFCharTypeLeft>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const std::basic_string_view<UTFCharTypeLeft> left, const std::basic_string<UTFCharTypeRight> &right)
{
  return UTFCaseInsensitiveCompare(left, std::basic_string_view<UTFCharTypeRight>(right));
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight = UTFCharTypeLeft, size_t UTFSizeRight>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const std::basic_string_view<UTFCharTypeLeft> left, const UTFCharTypeRight (&right)[UTFSizeRight])
{
  return UTFCaseInsensitiveCompare(left, std::basic_string_view<UTFCharTypeRight>(right, UTFSizeRight - 1));
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight = UTFCharTypeLeft>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const std::basic_string_view<UTFCharTypeLeft> left, const UTFCharTypeRight *right, const size_t rightLength)
{
  return UTFCaseInsensitiveCompare(left, std::basic_string_view<UTFCharTypeRight>(right, rightLength));
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight = UTFCharTypeLeft>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const std::basic_string<UTFCharTypeLeft> &left, const std::basic_string_view<UTFCharTypeRight> right)
{
  return UTFCaseInsensitiveCompare(std::basic_string_view<UTFCharTypeLeft>(left), right);
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight = UTFCharTypeLeft>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const std::basic_string<UTFCharTypeLeft> &left, const std::basic_string<UTFCharTypeRight> &right)
{
  return UTFCaseInsensitiveCompare(std::basic_string_view<UTFCharTypeLeft>(left), std::basic_string_view<UTFCharTypeRight>(right));
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight = UTFCharTypeLeft, size_t UTFSizeRight>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const std::basic_string<UTFCharTypeLeft> &left, const UTFCharTypeRight (&right)[UTFSizeRight])
{
  return UTFCaseInsensitiveCompare(std::basic_string_view<UTFCharTypeLeft>(left), std::basic_string_view<UTFCharTypeRight>(right, UTFSizeRight - 1));
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight = UTFCharTypeLeft>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const std::basic_string<UTFCharTypeLeft> &left, const UTFCharTypeRight *right, const size_t rightLength)
{
  return UTFCaseInsensitiveCompare(std::basic_string_view<UTFCharTypeLeft>(left), std::basic_string_view<UTFCharTypeRight>(right, rightLength));
}

template <typename UTFCharTypeLeft, size_t UTFSizeLeft, typename UTFCharTypeRight = UTFCharTypeLeft>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const UTFCharTypeLeft (&left)[UTFSizeLeft], const std::basic_string_view<UTFCharTypeRight> right)
{
  return UTFCaseInsensitiveCompare(std::basic_string_view<UTFCharTypeLeft>(left, UTFSizeLeft - 1), right);
}

template <typename UTFCharTypeLeft, size_t UTFSizeLeft, typename UTFCharTypeRight = UTFCharTypeLeft>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const UTFCharTypeLeft (&left)[UTFSizeLeft], const std::basic_string<UTFCharTypeRight> &right)
{
  return UTFCaseInsensitiveCompare(std::basic_string_view<UTFCharTypeLeft>(left, UTFSizeLeft - 1), std::basic_string_view<UTFCharTypeRight>(right));
}

template <typename UTFCharTypeLeft, size_t UTFSizeLeft, typename UTFCharTypeRight = UTFCharTypeLeft, size_t UTFSizeRight>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const UTFCharTypeLeft (&left)[UTFSizeLeft], const UTFCharTypeRight (&right)[UTFSizeRight])
{
  return UTFCaseInsensitiveCompare(std::basic_string_view<UTFCharTypeLeft>(left, UTFSizeLeft - 1), std::basic_string_view<UTFCharTypeRight>(right, UTFSizeRight - 1));
}

template <typename UTFCharTypeLeft, size_t UTFSizeLeft, typename UTFCharTypeRight = UTFCharTypeLeft>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const UTFCharTypeLeft (&left)[UTFSizeLeft], const UTFCharTypeRight *right, const size_t rightLength)
{
  return UTFCaseInsensitiveCompare(std::basic_string_view<UTFCharTypeLeft>(left, UTFSizeLeft - 1), std::basic_string_view<UTFCharTypeRight>(right, rightLength));
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight = UTFCharTypeLeft>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const UTFCharTypeLeft *left, const size_t leftLength, const std::basic_string_view<UTFCharTypeRight> right)
{
  return UTFCaseInsensitiveCompare(std::basic_string_view<UTFCharTypeLeft>(left, leftLength), right);
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight = UTFCharTypeLeft>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const UTFCharTypeLeft *left, const size_t leftLength, const std::basic_string<UTFCharTypeRight> &right)
{
  return UTFCaseInsensitiveCompare(std::basic_string_view<UTFCharTypeLeft>(left, leftLength), std::basic_string_view<UTFCharTypeRight>(right));
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight = UTFCharTypeLeft, size_t UTFSizeRight>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const UTFCharTypeLeft *left, const size_t leftLength, const UTFCharTypeRight (&right)[UTFSizeRight])
{
  return UTFCaseInsensitiveCompare(std::basic_string_view<UTFCharTypeLeft>(left, leftLength), std::basic_string_view<UTFCharTypeRight>(right, UTFSizeRight - 1));
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight = UTFCharTypeLeft>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const UTFCharTypeLeft *left, const size_t leftLength, const UTFCharTypeRight *right, const size_t rightLength)
{
  return UTFCaseInsensitiveCompare(std::basic_string_view<UTFCharTypeLeft>(left, leftLength), std::basic_string_view<UTFCharTypeRight>(right, rightLength));
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight = UTFCharTypeLeft>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
struct UTFViewInsensitiveCompareLess
{
  bool operator()(const std::basic_string_view<UTFCharTypeLeft> left, const std::basic_string_view<UTFCharTypeRight> right) const
  {
    return UTFCaseInsensitiveCompare(left, right) < 0;
  }
};

template <typename UTFCharType, typename Type>
requires IsUTFCharType<UTFCharType>
using UTFViewToTypeMapCI = std::map<std::basic_string_view<UTFCharType>, Type, UTFViewInsensitiveCompareLess<UTFCharType>>;

template <typename UTFCharType, typename Type>
requires IsUTFCharType<UTFCharType>
using UTFViewToTypeMap = std::map<std::basic_string_view<UTFCharType>, Type>;

template <typename UTFCharType>
requires IsUTFCharType<UTFCharType>
using UTFViewSetCI = std::set<std::basic_string_view<UTFCharType>, UTFViewInsensitiveCompareLess<UTFCharType>>;

template <typename UTFCharType>
requires IsUTFCharType<UTFCharType>
using UTFViewSet = std::set<std::basic_string_view<UTFCharType>>;

template <typename UTFCharTypeLeft, typename UTFCharTypeRight = UTFCharTypeLeft>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
struct UTFInsensitiveCompareLess
{
  bool operator()(const std::basic_string<UTFCharTypeLeft> &left, const std::basic_string<UTFCharTypeRight> &right) const
  {
    return UTFCaseInsensitiveCompare(left, right) < 0;
  }
};

template <typename UTFCharType, typename Type>
requires IsUTFCharType<UTFCharType>
using UTFToTypeMapCI = std::map<std::basic_string<UTFCharType>, Type, UTFInsensitiveCompareLess<UTFCharType>>;

template <typename UTFCharType, typename Type>
requires IsUTFCharType<UTFCharType>
using UTFToTypeMap = std::map<std::basic_string<UTFCharType>, Type>;

template <typename UTFCharType>
requires IsUTFCharType<UTFCharType>
using UTFSetCI = std::set<std::basic_string<UTFCharType>, UTFInsensitiveCompareLess<UTFCharType>>;

template <typename UTFCharType>
requires IsUTFCharType<UTFCharType>
using UTFSet = std::set<std::basic_string<UTFCharType>>;

struct PathCaseInsensitiveCompareLess
{
  bool operator()(const std::filesystem::path &left, const std::filesystem::path &right) const
  {
    return UTFCaseInsensitiveCompare(left.native(), right.native()) < 0;
  }
};

template <typename Type>
using PathToTypeMapCI = std::map<std::filesystem::path, Type, PathCaseInsensitiveCompareLess>;

template <typename Type>
using PathToTypeMap = std::map<std::filesystem::path, Type>;

using PathSetCI = std::set<std::filesystem::path, PathCaseInsensitiveCompareLess>;

using PathSet = std::set<std::filesystem::path>;
