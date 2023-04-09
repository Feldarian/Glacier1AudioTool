//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#pragma once

template <typename TypeInput, typename... TypesToCompareWith>
concept IsAnyOfTypes = (std::same_as<TypeInput, TypesToCompareWith> || ...);

template <typename UTFCharType>
concept IsUTF8CharType = IsAnyOfTypes<UTFCharType, char, char8_t, int8_t, uint8_t>;

static_assert(std::same_as<char16_t, UChar>, "ICU UChar typedef is not char16_t!");

#ifdef _WIN32

template <typename UTFCharType>
concept IsUTF16CharType = IsAnyOfTypes<UTFCharType, wchar_t, char16_t, int16_t, uint16_t, UChar>;

template <typename UTFCharType>
concept IsUTF32CharType = IsAnyOfTypes<UTFCharType, char32_t, int32_t, uint32_t, UChar32>;

#else

static_assert(std::same_as<wchar_t, UChar32>, "STD wchar_t had unexpected size!");

template <typename UTFCharType>
concept IsUTF16CharType = IsAnyOfTypes<UTFCharType, char16_t, int16_t, uint16_t, UChar>;

template <typename UTFCharType>
concept IsUTF32CharType = IsAnyOfTypes<UTFCharType, wchar_t, char32_t, int32_t, uint32_t, UChar32>;

#endif

template <typename UTFCharType>
concept IsUTFNativeCharType = IsAnyOfTypes<UTFCharType, char, wchar_t>;

template <typename UTFCharType>
concept IsUTFCharType = IsUTF8CharType<UTFCharType> || IsUTF16CharType<UTFCharType> || IsUTF32CharType<UTFCharType> || IsUTFNativeCharType<UTFCharType>;

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

class UTFGlyphRangesBuilder : public Singleton<UTFGlyphRangesBuilder>
{
public:
  static constexpr auto CodepointInvalid{ 0u };
  static constexpr auto CodepointMax{ 0x10FFFFu };

  UTFGlyphRangesBuilder()
  {
    // Basic Latin (ASCII) + Latin-1 Supplement
    AddRange({0x0020, 0x00FF});

    // Invalid Unicode Character
    Add(0xFFFD);
  }

  void Clear()
  {
    glyphsInUse.reset();

    // Basic Latin (ASCII) + Latin-1 Supplement
    AddRange({0x0020, 0x00FF});

    // Invalid Unicode Character
    Add(0xFFFD);
  }

  bool NeedsBuild() const
  {
    return newGlyphsAdded;
  }

  std::vector<std::pair<uint32_t, uint32_t>> Build()
  {
    std::vector<std::pair<uint32_t, uint32_t>> glyphRanges;

    std::unique_lock lock(dataMutex);

    std::pair<uint32_t, uint32_t> glyphRange{0, 0};
    for (uint32_t glyph = 1; glyph <= CodepointMax; ++glyph)
    {
      if (glyphsInUse.test(glyph))
      {
        if (glyphRange.first == 0)
          glyphRange.first = glyph;

        glyphRange.second = glyph;
      }
      else
      {
        if (glyphRange.first != 0)
        {
          glyphRanges.emplace_back(glyphRange);
          glyphRange = {0, 0};
        }
      }
    }

    if (glyphRange.first != 0)
      glyphRanges.emplace_back(glyphRange);

    newGlyphsAdded = false;
    return glyphRanges;
  }

  void AddRange(std::pair<uint32_t, uint32_t> glyphRange)
  {
    for (auto glyph = glyphRange.first; glyph <= glyphRange.second; ++glyph)
      Add(glyph);
  }

  void Add(uint32_t glyph)
  {
    if (glyph == 0 || glyph > CodepointMax)
      return;

    std::unique_lock lock(dataMutex);
    newGlyphsAdded |= !glyphsInUse.test(glyph);
    glyphsInUse.set(glyph);
  }

  template <typename UTFCharType>
  requires IsUTFCharType<UTFCharType>
  void AddText(const std::basic_string_view<UTFCharType> utf)
  {
    if constexpr (IsUTF32CharType<UTFCharType>)
    {
      for (const auto glyph : utf)
        Add(static_cast<uint32_t>(glyph));
    }
    else if constexpr (IsUTF16CharType<UTFCharType>)
    {
      const auto *utfData = utf.data();
      const auto utfSize = utf.size();
      for (size_t utfOffset = 0; utfOffset < utfSize;)
      {
        uint32_t glyph = 0;
        U16_NEXT(utfData, utfOffset, utfSize, glyph);

        Add(glyph);
      }
    }
    else if constexpr (IsUTF8CharType<UTFCharType>)
    {
      const auto *utfData = utf.data();
      const auto utfSize = utf.size();
      for (size_t utfOffset = 0; utfOffset < utfSize;)
      {
        uint32_t glyph = 0;
        U8_NEXT(utfData, utfOffset, utfSize, glyph);

        Add(glyph);
      }
    }
  }

  template <typename UTFCharType>
  requires IsUTFCharType<UTFCharType>
  void AddText(const std::basic_string<UTFCharType> &utf)
  {
    AddText(std::basic_string_view<UTFCharType>(utf));
  }

  template <typename UTFCharType, size_t UTFSize>
  requires IsUTFCharType<UTFCharType>
  void AddText(const UTFCharType (&utf)[UTFSize])
  {
    AddText(std::basic_string_view<UTFCharType>(utf, UTFSize - 1));
  }

  template <typename UTFCharType>
  requires IsUTFCharType<UTFCharType>
  void AddText(const UTFCharType *utf, const size_t length)
  {
    AddText(std::basic_string_view<UTFCharType>(utf, length));
  }

private:
  std::bitset<CodepointMax + 1> glyphsInUse;
  bool newGlyphsAdded = false;

  mutable std::mutex dataMutex;
};

template <typename UTFCharType>
requires IsUTFCharType<UTFCharType>
class UTFLocalizationInstance
{
public:
  inline static const std::basic_string<UTFCharType> Empty;

  void Clear()
  {
    localizationMap.clear();
  }

  bool Load(const toml::table &localizationTable)
  {
    UTFToTypeMapCI<UTFCharType, std::basic_string<UTFCharType>> addLocalizationMap;

    for (const auto &[key, value] : localizationTable)
    {
      if (!value.is_string())
        return false;

      auto keyUTF = ToUTF<UTFCharType>(key.str());
      auto valueUTF = ToUTF<UTFCharType>(**value.as_string());

      UTFGlyphRangesBuilder::Get().AddText(keyUTF);
      UTFGlyphRangesBuilder::Get().AddText(valueUTF);
      addLocalizationMap.insert_or_assign(std::move(keyUTF), std::move(valueUTF));
    }

    // if everything went well so far, merge loaded data with existing one

    for (auto &&[key, value] : addLocalizationMap)
      localizationMap.insert_or_assign(std::move(key), std::move(value));

    return true;
  }

  const std::basic_string<UTFCharType> &Localize(const std::basic_string_view<UTFCharType> key) const
  {
    return Localize(std::basic_string<UTFCharType>(key));
  }

  const std::basic_string<UTFCharType> &Localize(const std::basic_string<UTFCharType> &key) const
  {
    const auto localizationIt = localizationMap.find(key);
    if (localizationIt == localizationMap.end())
      return Empty;

    return localizationIt->second;
  }

  template <size_t UTFCharTypeSize>
  const std::basic_string<UTFCharType> &Localize(const UTFCharType (&key)[UTFCharTypeSize]) const
  {
    return Localize(std::basic_string<UTFCharType>(key, UTFCharTypeSize - 1));
  }

  const std::basic_string<UTFCharType> &Localize(const UTFCharType *key, const size_t length) const
  {
    return Localize(std::basic_string<UTFCharType>(key, length));
  }

  template <typename UTFCharTypeOther>
  requires IsUTFCharType<UTFCharTypeOther> && !std::same_as<UTFCharType, UTFCharTypeOther>
  const std::basic_string<UTFCharType> &Localize(const std::basic_string_view<UTFCharTypeOther> key) const
  {
    return Localize(ToUTF<UTFCharType>(key));
  }

  template <typename UTFCharTypeOther>
  requires IsUTFCharType<UTFCharTypeOther> && !std::same_as<UTFCharType, UTFCharTypeOther>
  const std::basic_string<UTFCharType> &Localize(const std::basic_string<UTFCharTypeOther> &key) const
  {
    return Localize(ToUTF<UTFCharType>(key));
  }

  template <typename UTFCharTypeOther, size_t UTFCharTypeOtherSize>
  requires IsUTFCharType<UTFCharTypeOther> && !std::same_as<UTFCharType, UTFCharTypeOther>
  const std::basic_string<UTFCharType> &Localize(const UTFCharTypeOther (&key)[UTFCharTypeOtherSize]) const
  {
    return Localize(ToUTF<UTFCharType>(key));
  }

  template <typename UTFCharTypeOther>
  requires IsUTFCharType<UTFCharTypeOther> && !std::same_as<UTFCharType, UTFCharTypeOther>
  const std::basic_string<UTFCharType> &Localize(const UTFCharTypeOther *key, const size_t length) const
  {
    return Localize(ToUTF<UTFCharType>(key, length));
  }

private:
  UTFToTypeMapCI<UTFCharType, std::basic_string<UTFCharType>> localizationMap;
  std::basic_string<UTFCharType> localizationLanguage;
};

template <typename UTFCharType>
requires IsUTFCharType<UTFCharType>
class UTFLocalizationManager : public Singleton<UTFLocalizationManager<UTFCharType>>
{
  using LocalizationInstance = UTFLocalizationInstance<UTFCharType>;

public:
  bool LoadLocalization(const std::filesystem::path &localizationPath)
  {
    if (!exists(localizationPath))
      return false;

    if (UTFCaseInsensitiveCompare(localizationPath.extension().native(), L".toml") != 0)
      return false;

    const auto localizationFile = toml::parse_file(localizationPath.native());
    if (localizationFile.failed())
      return false;

    const auto& fileName = ToUTF<UTFCharType>(localizationPath.stem().native());
    const auto languageNameSeparatorPosition = fileName.rfind(static_cast<UTFCharType>('_'));
    auto languageName = languageNameSeparatorPosition == std::basic_string<UTFCharType>::npos ? fileName : fileName.substr(languageNameSeparatorPosition + 1);

    const auto languageGroup = localizationFile["language"];
    if (languageGroup.is_table())
    {
      const auto &languageNameValue = languageGroup["name"];
      if (languageNameValue.is_string())
        languageName = ToUTF<UTFCharType>(**languageNameValue.as_string());
    }

    if (languageName.empty())
      return false;

    UTFGlyphRangesBuilder::Get().AddText(languageName);

    const auto localizationTableValue = localizationFile["localization"];
    if (!localizationTableValue.is_table())
      return false;

    const auto localizationTable = *localizationTableValue.as_table();

    std::unique_lock lock(dataMutex);

    const auto [localizationInstanceIt, localizationInstanceEmplaced] = localizationInstancesMap.try_emplace(languageName, LocalizationInstance{});
    const auto localizationLoaded = localizationInstanceIt->second.Load(localizationTable);

    if (localizationInstanceEmplaced)
    {
      if (!localizationLoaded)
        localizationInstancesMap.erase(localizationInstanceIt);

      SetDefaultLanguage(defaultLocalizationLanguage);
      SetLanguage(localizationLanguage);
    }

    return localizationLoaded;
  }

  const std::vector<std::basic_string<UTFCharType>> GetAvailableLanguages() const
  {
    std::shared_lock lock(dataMutex);

    const auto languagesRange = localizationInstancesMap | std::views::keys;
    return {languagesRange.begin(), languagesRange.end()};
  }

  bool SetDefaultLanguage(const std::basic_string<UTFCharType> &language)
  {
    if (language.empty())
      return false;

    std::unique_lock lock(dataMutex);

    auto localizationInstanceIt = localizationInstancesMap.find(language);
    if (localizationInstanceIt == localizationInstancesMap.cend())
      return false;

    defaultLocalizationLanguage = localizationInstanceIt->first;
    defaultLocalizationInstance = &localizationInstanceIt->second;
    return true;
  }

  const std::basic_string<UTFCharType> &GetDefaultLanguage() const
  {
    std::shared_lock lock(dataMutex);

    return defaultLocalizationLanguage;
  }

  bool SetLanguage(const std::basic_string<UTFCharType> &language)
  {
    if (language.empty())
      return false;

    std::unique_lock lock(dataMutex);

    auto localizationInstanceIt = localizationInstancesMap.find(language);
    if (localizationInstanceIt == localizationInstancesMap.cend())
      return false;

    localizationLanguage = localizationInstanceIt->first;
    localizationInstance = &localizationInstanceIt->second;
    return true;
  }

  const std::basic_string<UTFCharType> &GetLanguage() const
  {
    std::shared_lock lock(dataMutex);

    return localizationLanguage.empty() ? GetDefaultLanguage() : localizationLanguage;
  }

  const std::basic_string<UTFCharType> &Localize(const std::basic_string_view<UTFCharType> key) const
  {
    const auto &localized = localizationInstance ? localizationInstance->Localize(key) : LocalizationInstance::Empty;
    if (!localized.empty())
      return localized;

    return defaultLocalizationInstance ? defaultLocalizationInstance->Localize(key) : LocalizationInstance::Empty;
  }

  const std::basic_string<UTFCharType> &Localize(const std::basic_string<UTFCharType> &key) const
  {
    return Localize(std::basic_string_view<UTFCharType>(key));
  }

  template <size_t UTFCharTypeSize>
  const std::basic_string<UTFCharType> &Localize(const UTFCharType (&key)[UTFCharTypeSize]) const
  {
    return Localize(std::basic_string_view<UTFCharType>(key, UTFCharTypeSize - 1));
  }

  const std::basic_string<UTFCharType> &Localize(const UTFCharType *key, const size_t length) const
  {
    return Localize(std::basic_string_view<UTFCharType>(key, length));
  }

  template <typename UTFCharTypeOther>
  requires IsUTFCharType<UTFCharTypeOther> && !std::same_as<UTFCharType, UTFCharTypeOther>
  const std::basic_string<UTFCharType> &Localize(const std::basic_string_view<UTFCharTypeOther> key) const
  {
    return Localize(ToUTF<UTFCharType>(key));
  }

  template <typename UTFCharTypeOther>
  requires IsUTFCharType<UTFCharTypeOther> && !std::same_as<UTFCharType, UTFCharTypeOther>
  const std::basic_string<UTFCharType> &Localize(const std::basic_string<UTFCharTypeOther> &key) const
  {
    return Localize(ToUTF<UTFCharType>(key));
  }

  template <typename UTFCharTypeOther, size_t UTFCharTypeOtherSize>
  requires IsUTFCharType<UTFCharTypeOther> && !std::same_as<UTFCharType, UTFCharTypeOther>
  const std::basic_string<UTFCharType> &Localize(const UTFCharTypeOther (&key)[UTFCharTypeOtherSize]) const
  {
    return Localize(ToUTF<UTFCharType>(key));
  }

  template <typename UTFCharTypeOther>
  requires IsUTFCharType<UTFCharTypeOther> && !std::same_as<UTFCharType, UTFCharTypeOther>
  const std::basic_string<UTFCharType> &Localize(const UTFCharTypeOther *key, const size_t length) const
  {
    return Localize(ToUTF<UTFCharType>(key, length));
  }

  template <typename... FormatArgs>
  requires IsUTFNativeCharType<UTFCharType>
  const std::basic_string<UTFCharType> &LocalizeFormat(const std::basic_string_view<UTFCharType> key, FormatArgs &&...args) const
  {
    std::shared_lock lock(dataMutex);

    thread_local static std::basic_string<UTFCharType> utfOut;
    return LocalizeFormatInternal(utfOut, key, std::forward<FormatArgs>(args)...);
  }

  template <typename... FormatArgs>
  requires IsUTFNativeCharType<UTFCharType>
  const std::basic_string<UTFCharType> &LocalizeFormat(const std::basic_string<UTFCharType> &key, FormatArgs &&...args) const
  {
    return LocalizeFormat(std::basic_string_view<UTFCharType>(key), std::forward<FormatArgs>(args)...);
  }

  template <size_t UTFCharTypeSize, typename... FormatArgs>
  requires IsUTFNativeCharType<UTFCharType>
  const std::basic_string<UTFCharType> &LocalizeFormat(const UTFCharType (&key)[UTFCharTypeSize], FormatArgs &&...args) const
  {
    return LocalizeFormat(std::basic_string_view<UTFCharType>(key, UTFCharTypeSize - 1), std::forward<FormatArgs>(args)...);
  }

  template <typename UTFCharTypeOther, typename... FormatArgs>
  requires IsUTFNativeCharType<UTFCharType> && IsUTFCharType<UTFCharTypeOther> && !std::same_as<UTFCharType, UTFCharTypeOther>
  const std::basic_string<UTFCharType> &LocalizeFormat(const std::basic_string_view<UTFCharTypeOther> key, FormatArgs &&...args) const
  {
    return LocalizeFormat(ToUTF<UTFCharType>(key), std::forward<FormatArgs>(args)...);
  }

  template <typename UTFCharTypeOther, typename... FormatArgs>
  requires IsUTFNativeCharType<UTFCharType> && IsUTFCharType<UTFCharTypeOther> && !std::same_as<UTFCharType, UTFCharTypeOther>
  const std::basic_string<UTFCharType> &LocalizeFormat(const std::basic_string<UTFCharTypeOther> &key, FormatArgs &&...args) const
  {
    return LocalizeFormat(ToUTF<UTFCharType>(key), std::forward<FormatArgs>(args)...);
  }

  template <typename UTFCharTypeOther, size_t UTFCharTypeOtherSize, typename... FormatArgs>
  requires IsUTFNativeCharType<UTFCharType> && IsUTFCharType<UTFCharTypeOther> && !std::same_as<UTFCharType, UTFCharTypeOther>
  const std::basic_string<UTFCharType> &LocalizeFormat(const UTFCharTypeOther (&key)[UTFCharTypeOtherSize], FormatArgs &&...args) const
  {
    return LocalizeFormat(ToUTF<UTFCharType>(key), std::forward<FormatArgs>(args)...);
  }

private:
  template <typename... FormatArgs>
  const std::basic_string<UTFCharType> &LocalizeFormatInternal(std::basic_string<UTFCharType> &buffer, const std::basic_string_view<UTFCharType> key, FormatArgs &&...args) const
  {
    const auto &localizedFormat = Localize(key);
    if (localizedFormat.empty())
      return localizedFormat;

    buffer.clear();

    try
    {
      std::vformat_to(std::back_inserter(buffer), localizedFormat, std::make_format_args(std::forward<FormatArgs>(args)...));
    }
    catch ([[maybe_unused]] std::format_error error)
    {
      return LocalizationInstance::Empty;
    }

    return buffer;
  }

  UTFToTypeMapCI<UTFCharType, LocalizationInstance> localizationInstancesMap;
  std::basic_string<UTFCharType> defaultLocalizationLanguage;
  LocalizationInstance *defaultLocalizationInstance = nullptr;
  std::basic_string<UTFCharType> localizationLanguage;
  LocalizationInstance *localizationInstance = nullptr;

  mutable std::shared_mutex dataMutex;
};
