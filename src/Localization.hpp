//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2

#pragma once

template <typename TypeInput, typename... TypesToCompareWith>
concept IsAnyOfTypes = (std::same_as<TypeInput, TypesToCompareWith> || ...);

template<typename UTFCharType>
concept IsUTF8CharType = IsAnyOfTypes<UTFCharType, char, char8_t, int8_t, uint8_t>;

#ifdef _WIN32

template<typename UTFCharType>
concept IsUTF16CharType = IsAnyOfTypes<UTFCharType, wchar_t, char16_t, int16_t, uint16_t, UChar>;

template<typename UTFCharType>
concept IsUTF32CharType = IsAnyOfTypes<UTFCharType, char32_t, int32_t, uint32_t, UChar32>;

#else

template<typename UTFCharType>
concept IsUTF16CharType = IsAnyOfTypes<UTFCharType, char16_t, int16_t, uint16_t, UChar>;

template<typename UTFCharType>
concept IsUTF32CharType = IsAnyOfTypes<UTFCharType, wchar_t, char32_t, int32_t, uint32_t, UChar32>;

#endif

template<typename UTFCharType>
concept IsUTFCharType = IsUTF8CharType<UTFCharType> || IsUTF16CharType<UTFCharType> || IsUTF32CharType<UTFCharType>;

template <typename UTFCharTypeOutput, typename UTFCharTypeInput>
requires IsUTFCharType<UTFCharTypeOutput> && IsUTFCharType<UTFCharTypeInput>
const std::basic_string<UTFCharTypeOutput>& ToUTF(std::basic_string<UTFCharTypeOutput>& buffer, const std::basic_string_view<UTFCharTypeInput> utf)
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
      auto* reinterpretedBuffer = reinterpret_cast<uint8_t *>(buffer.data());
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
      u_strFromUTF8(reinterpret_cast<UChar *>(buffer.data()), buffer.size(), &length, reinterpret_cast<const char*>(utf.data()), utf.size(), &errorCode);
    else if constexpr (IsUTF32CharType<UTFCharTypeInput>)
      u_strFromUTF32(reinterpret_cast<UChar *>(buffer.data()), buffer.size(), &length, reinterpret_cast<const UChar32*>(utf.data()), utf.size(), &errorCode);
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
      auto* reinterpretedBuffer = reinterpret_cast<UChar32 *>(buffer.data());
      const auto reinterpretedBufferSize = buffer.size();
      const auto* inputBuffer = utf.data();
      const auto inputBufferSize = utf.size();
      for (size_t inputOffset = 0; inputOffset < inputBufferSize;) {
        auto& outputChar = reinterpretedBuffer[offset++];
        U8_NEXT(inputBuffer, inputOffset, inputBufferSize, outputChar);
      }
      length = offset;
    }
    else if constexpr (IsUTF16CharType<UTFCharTypeInput>)
    {
      auto* reinterpretedBuffer = reinterpret_cast<UChar32 *>(buffer.data());
      const auto reinterpretedBufferSize = buffer.size();
      const auto* inputBuffer = utf.data();
      const auto inputBufferSize = utf.size();
      for (size_t inputOffset = 0; inputOffset < inputBufferSize;) {
        auto& outputChar = reinterpretedBuffer[offset++];
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
const std::basic_string<UTFCharTypeOutput>& ToUTF(std::basic_string<UTFCharTypeOutput>& buffer, const std::basic_string<UTFCharTypeInput>& utf)
{
  return ToUTF<UTFCharTypeOutput>(buffer, std::basic_string_view<UTFCharTypeInput>(utf));
}

template <typename UTFCharTypeOutput, typename UTFCharTypeInput, size_t UTFInputSize>
requires IsUTFCharType<UTFCharTypeOutput> && IsUTFCharType<UTFCharTypeInput>
const std::basic_string<UTFCharTypeOutput>& ToUTF(std::basic_string<UTFCharTypeOutput>& buffer, const UTFCharTypeInput (& utf)[UTFInputSize])
{
  return ToUTF<UTFCharTypeOutput>(buffer, std::basic_string_view<UTFCharTypeInput>(utf, UTFInputSize - 1));
}

template <typename UTFCharTypeOutput, typename UTFCharTypeInput>
requires IsUTFCharType<UTFCharTypeOutput> && IsUTFCharType<UTFCharTypeInput>
const std::basic_string<UTFCharTypeOutput>& ToUTF(std::basic_string<UTFCharTypeOutput>& buffer, const UTFCharTypeInput* utf, const size_t length)
{
  return ToUTF<UTFCharTypeOutput>(buffer, std::basic_string_view<UTFCharTypeInput>(utf, length));
}

template <typename UTFCharTypeOutput, typename UTFCharTypeInput>
requires IsUTFCharType<UTFCharTypeOutput> && IsUTFCharType<UTFCharTypeInput>
const std::basic_string<UTFCharTypeOutput>& ToUTF(const std::basic_string_view<UTFCharTypeInput> utf)
{
  thread_local static std::basic_string<UTFCharTypeOutput> utfOut;
  return ToUTF<UTFCharTypeOutput>(utfOut, utf);
}

template <typename UTFCharTypeOutput, typename UTFCharTypeInput>
requires IsUTFCharType<UTFCharTypeOutput> && IsUTFCharType<UTFCharTypeInput>
const std::basic_string<UTFCharTypeOutput>& ToUTF(const std::basic_string<UTFCharTypeInput>& utf)
{
  return ToUTF<UTFCharTypeOutput>(std::basic_string_view<UTFCharTypeInput>(utf));
}

template <typename UTFCharTypeOutput, typename UTFCharTypeInput, size_t UTFInputSize>
requires IsUTFCharType<UTFCharTypeOutput> && IsUTFCharType<UTFCharTypeInput>
const std::basic_string<UTFCharTypeOutput>& ToUTF(const UTFCharTypeInput (& utf)[UTFInputSize])
{
  return ToUTF<UTFCharTypeOutput>(std::basic_string_view<UTFCharTypeInput>(utf, UTFInputSize - 1));
}

template <typename UTFCharTypeOutput, typename UTFCharTypeInput>
requires IsUTFCharType<UTFCharTypeOutput> && IsUTFCharType<UTFCharTypeInput>
const std::basic_string<UTFCharTypeOutput>& ToUTF(const UTFCharTypeInput* utf, const size_t length)
{
  return ToUTF<UTFCharTypeOutput>(std::basic_string_view<UTFCharTypeInput>(utf, length));
}

// TODO - iterate codepoints and compare manually for better efficiency
template <typename UTFCharTypeLeft, typename UTFCharTypeRight = UTFCharTypeLeft>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const std::basic_string_view<UTFCharTypeLeft> left, const std::basic_string_view<UTFCharTypeRight> right) {
  thread_local static std::basic_string<UChar> leftConverted;
  const auto* leftData = ToUTF(leftConverted, left).c_str();

  thread_local static std::basic_string<UChar> rightConverted;
  const auto* rightData = ToUTF(rightConverted, right).c_str();

  UErrorCode errorCode = U_ZERO_ERROR;
  return u_strCaseCompare(leftData, leftConverted.size(), rightData, rightConverted.size(), 0, &errorCode);
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight = UTFCharTypeLeft>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const std::basic_string_view<UTFCharTypeLeft> left, const std::basic_string<UTFCharTypeRight>& right) {
  return UTFCaseInsensitiveCompare(left, std::basic_string_view<UTFCharTypeRight>(right));
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight = UTFCharTypeLeft, size_t UTFSizeRight>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const std::basic_string_view<UTFCharTypeLeft> left, const UTFCharTypeRight (& right)[UTFSizeRight]) {
  return UTFCaseInsensitiveCompare(left, std::basic_string_view<UTFCharTypeRight>(right, UTFSizeRight - 1));
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight = UTFCharTypeLeft>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const std::basic_string_view<UTFCharTypeLeft> left, const UTFCharTypeRight* right, const size_t rightLength) {
  return UTFCaseInsensitiveCompare(left, std::basic_string_view<UTFCharTypeRight>(right, rightLength));
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight = UTFCharTypeLeft>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const std::basic_string<UTFCharTypeLeft>& left, const std::basic_string_view<UTFCharTypeRight> right) {
  return UTFCaseInsensitiveCompare(std::basic_string_view<UTFCharTypeLeft>(left), right);
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight = UTFCharTypeLeft>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const std::basic_string<UTFCharTypeLeft>& left, const std::basic_string<UTFCharTypeRight>& right) {
  return UTFCaseInsensitiveCompare(std::basic_string_view<UTFCharTypeLeft>(left), std::basic_string_view<UTFCharTypeRight>(right));
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight = UTFCharTypeLeft, size_t UTFSizeRight>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const std::basic_string<UTFCharTypeLeft>& left, const UTFCharTypeRight (& right)[UTFSizeRight]) {
  return UTFCaseInsensitiveCompare(std::basic_string_view<UTFCharTypeLeft>(left), std::basic_string_view<UTFCharTypeRight>(right, UTFSizeRight - 1));
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight = UTFCharTypeLeft>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const std::basic_string<UTFCharTypeLeft>& left, const UTFCharTypeRight* right, const size_t rightLength) {
  return UTFCaseInsensitiveCompare(std::basic_string_view<UTFCharTypeLeft>(left), std::basic_string_view<UTFCharTypeRight>(right, rightLength));
}

template <typename UTFCharTypeLeft, size_t UTFSizeLeft, typename UTFCharTypeRight = UTFCharTypeLeft>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const UTFCharTypeLeft (& left)[UTFSizeLeft], const std::basic_string_view<UTFCharTypeRight> right) {
  return UTFCaseInsensitiveCompare(std::basic_string_view<UTFCharTypeLeft>(left, UTFSizeLeft - 1), right);
}

template <typename UTFCharTypeLeft, size_t UTFSizeLeft, typename UTFCharTypeRight = UTFCharTypeLeft>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const UTFCharTypeLeft (& left)[UTFSizeLeft], const std::basic_string<UTFCharTypeRight>& right) {
  return UTFCaseInsensitiveCompare(std::basic_string_view<UTFCharTypeLeft>(left, UTFSizeLeft - 1), std::basic_string_view<UTFCharTypeRight>(right));
}

template <typename UTFCharTypeLeft, size_t UTFSizeLeft, typename UTFCharTypeRight = UTFCharTypeLeft, size_t UTFSizeRight>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const UTFCharTypeLeft (& left)[UTFSizeLeft], const UTFCharTypeRight (& right)[UTFSizeRight]) {
  return UTFCaseInsensitiveCompare(std::basic_string_view<UTFCharTypeLeft>(left, UTFSizeLeft - 1), std::basic_string_view<UTFCharTypeRight>(right, UTFSizeRight - 1));
}

template <typename UTFCharTypeLeft, size_t UTFSizeLeft, typename UTFCharTypeRight = UTFCharTypeLeft>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const UTFCharTypeLeft (& left)[UTFSizeLeft], const UTFCharTypeRight* right, const size_t rightLength) {
  return UTFCaseInsensitiveCompare(std::basic_string_view<UTFCharTypeLeft>(left, UTFSizeLeft - 1), std::basic_string_view<UTFCharTypeRight>(right, rightLength));
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight = UTFCharTypeLeft>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const UTFCharTypeLeft* left, const size_t leftLength, const std::basic_string_view<UTFCharTypeRight> right) {
  return UTFCaseInsensitiveCompare(std::basic_string_view<UTFCharTypeLeft>(left, leftLength), right);
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight = UTFCharTypeLeft>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const UTFCharTypeLeft* left, const size_t leftLength, const std::basic_string<UTFCharTypeRight>& right) {
  return UTFCaseInsensitiveCompare(std::basic_string_view<UTFCharTypeLeft>(left, leftLength), std::basic_string_view<UTFCharTypeRight>(right));
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight = UTFCharTypeLeft, size_t UTFSizeRight>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const UTFCharTypeLeft* left, const size_t leftLength, const UTFCharTypeRight (& right)[UTFSizeRight]) {
  return UTFCaseInsensitiveCompare(std::basic_string_view<UTFCharTypeLeft>(left, leftLength), std::basic_string_view<UTFCharTypeRight>(right, UTFSizeRight - 1));
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight = UTFCharTypeLeft>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
int32_t UTFCaseInsensitiveCompare(const UTFCharTypeLeft* left, const size_t leftLength, const UTFCharTypeRight* right, const size_t rightLength) {
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
  bool operator()(const std::basic_string<UTFCharTypeLeft>& left, const std::basic_string<UTFCharTypeRight>& right) const
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

template <typename UTFCharType>
requires IsUTFCharType<UTFCharType>
class UTFLocalizationInstance
{
public:
  inline static const std::basic_string<UTFCharType> Empty;

  const std::basic_string<UTFCharType>& GetLanguage() const
  {
    return localizationLanguage;
  }

  bool Load(const std::filesystem::path& localizationPath)
  {
    localizationMap.clear();
    localizationLanguage.clear();

    if (!exists(localizationPath))
      return false;

    if (UTFCaseInsensitiveCompare(localizationPath.extension().native(), ".toml") != 0)
      return false;

    const auto& fileName = ToUTF<char>(localizationPath.stem().native());
    const auto languageSeparatorPosition = fileName.rfind('_');
    const auto language = languageSeparatorPosition == std::string::npos ? fileName : fileName.substr(languageSeparatorPosition + 1);
    if (language.empty())
      return false;

    const auto localizationFile = toml::parse_file(localizationPath.native());
    if (localizationFile.failed())
      return false;

    const auto localizationGroup = localizationFile["localization"];
    if (!localizationGroup.is_table())
        return false;

    for (auto&& [key, value] : *localizationGroup.as_table())
    {
      if (!value.is_string())
      {
        localizationMap.clear();
        return false;
      }

      if (!localizationMap.try_emplace(std::string(key.str()), *value.as_string()).second)
      {
        localizationMap.clear();
        return false;
      }
    }

    localizationLanguage = std::move(language);
    return true;
  }

  const std::basic_string<UTFCharType>& Localize(const std::basic_string_view<UTFCharType> key) const
  {
    return Localize(std::basic_string<UTFCharType>(key));
  }
  
  const std::basic_string<UTFCharType>& Localize(const std::basic_string<UTFCharType>& key) const
  {
    const auto localizationIt = localizationMap.find(key);
    if (localizationIt == localizationMap.end())
      return Empty;

    return localizationIt->second;
  }
  
  template <size_t UTFCharTypeSize>
  const std::basic_string<UTFCharType>& Localize(const UTFCharType (& key)[UTFCharTypeSize]) const
  {
    return Localize(std::basic_string<UTFCharType>(key, UTFCharTypeSize - 1));
  }
  
  const std::basic_string<UTFCharType>& Localize(const UTFCharType* key, const size_t length) const
  {
    return Localize(std::basic_string<UTFCharType>(key, length));
  }

  template <typename UTFCharTypeOther>
  requires IsUTFCharType<UTFCharTypeOther> && !std::same_as<UTFCharType, UTFCharTypeOther>
  const std::basic_string<UTFCharType>& Localize(const std::basic_string_view<UTFCharTypeOther> key) const
  {
    return Localize(ToUTF<UTFCharType>(key));
  }
  
  template <typename UTFCharTypeOther>
  requires IsUTFCharType<UTFCharTypeOther> && !std::same_as<UTFCharType, UTFCharTypeOther>
  const std::basic_string<UTFCharType>& Localize(const std::basic_string<UTFCharTypeOther>& key) const
  {
    return Localize(ToUTF<UTFCharType>(key));
  }
  
  template <typename UTFCharTypeOther, size_t UTFCharTypeOtherSize>
  requires IsUTFCharType<UTFCharTypeOther> && !std::same_as<UTFCharType, UTFCharTypeOther>
  const std::basic_string<UTFCharType>& Localize(const UTFCharTypeOther (& key)[UTFCharTypeOtherSize]) const
  {
    return Localize(ToUTF<UTFCharType>(key));
  }
  
  template <typename UTFCharTypeOther>
  requires IsUTFCharType<UTFCharTypeOther> && !std::same_as<UTFCharType, UTFCharTypeOther>
  const std::basic_string<UTFCharType>& Localize(const UTFCharTypeOther* key, const size_t length) const
  {
    return Localize(ToUTF<UTFCharType>(key, length));
  }

private:
  UTFToTypeMapCI<UTFCharType, std::basic_string<UTFCharType>> localizationMap;
  std::basic_string<UTFCharType> localizationLanguage;
};

template <typename UTFCharType>
requires IsUTFCharType<UTFCharType>
class UTFLocalizationManager
{
  using LocalizationInstance = UTFLocalizationInstance<UTFCharType>;

public:
  bool LoadLocalization(const std::filesystem::path& localizationPath, bool reload = false)
  {
    auto [localizationInstanceIt, emplaced] = localizationInstancesMap.try_emplace(localizationPath, LocalizationInstance{});
    if (!emplaced && !reload)
      return true;
    
    auto& localizationInstance = localizationInstanceIt->second;
    auto originalLanguage = localizationInstance.GetLanguage();
    assert(emplaced || !originalLanguage.empty());
    
    if (!exists(localizationPath))
    {
      if (!emplaced)
        std::ranges::remove(languageToLocalizationPathsMap[originalLanguage], localizationPath);
      localizationInstancesMap.erase(localizationInstanceIt);
      return false;
    }

    if (!localizationInstance.Load(localizationPath))
    {
      if (!emplaced)
        std::ranges::remove(languageToLocalizationPathsMap[originalLanguage], localizationPath);
      localizationInstancesMap.erase(localizationInstanceIt);
      return false;
    }

    if (!emplaced)
    {
      if (UTFCaseInsensitiveCompare(originalLanguage, localizationInstance.GetLanguage()) != 0)
      {
        std::ranges::remove(languageToLocalizationPathsMap[originalLanguage], localizationPath);
        languageToLocalizationPathsMap[localizationInstance.GetLanguage()].emplace_back(localizationPath);
      }
    }
    else
      languageToLocalizationPathsMap[localizationInstance.GetLanguage()].emplace_back(localizationPath);

    return true;
  }

  bool UnloadLocalization(const std::filesystem::path& localizationPath)
  {
    const auto localizationInstanceIt = localizationInstancesMap.find(localizationPath);
    if (localizationInstanceIt == localizationInstancesMap.cend())
      return false;
    
    std::ranges::remove(languageToLocalizationPathsMap[localizationInstanceIt->second.GetLanguage()], localizationPath);
    localizationInstancesMap.erase(localizationInstanceIt);
    return true;
  }

  void SetDefaultLanguage(const std::basic_string_view<UTFCharType> language)
  {
    defaultLanguage = language;
  }

  const std::basic_string<UTFCharType>& GetDefaultLanguage() const
  {
    return defaultLanguage;
  }

  void SetLanguage(const std::basic_string_view<UTFCharType> language)
  {
    currentLanguage = language;
  }

  const std::vector<std::basic_string<UTFCharType>> GetAvailableLanguages() const
  {
    const auto languagesRange = languageToLocalizationPathsMap | std::views::keys;
    return { languagesRange.begin(), languagesRange.end() };
  }

  const std::basic_string<UTFCharType>& GetLanguage() const
  {
    return currentLanguage.empty() ? defaultLanguage : currentLanguage;
  }

  const std::basic_string<UTFCharType>& GetLocalizationLanguage(const std::filesystem::path& localizationPath) const
  {
    return LocalizationInstance::Empty;
  }
  
  const std::basic_string<UTFCharType>& Localize(const std::basic_string_view<UTFCharType> key) const
  {
    const auto& localized = LocalizeInternal(key, currentLanguage);
    if (!localized.empty())
      return localized;

    return LocalizeInternal(key, defaultLanguage);
  }

  const std::basic_string<UTFCharType>& Localize(const std::basic_string<UTFCharType>& key) const
  {
    return Localize(std::basic_string_view<UTFCharType>(key));
  }
  
  template <size_t UTFCharTypeSize>
  const std::basic_string<UTFCharType>& Localize(const UTFCharType (& key)[UTFCharTypeSize]) const
  {
    return Localize(std::basic_string_view<UTFCharType>(key, UTFCharTypeSize - 1));
  }
  
  const std::basic_string<UTFCharType>& Localize(const UTFCharType* key, const size_t length) const
  {
    return Localize(std::basic_string_view<UTFCharType>(key, length));
  }

  template <typename UTFCharTypeOther>
  requires IsUTFCharType<UTFCharTypeOther> && !std::same_as<UTFCharType, UTFCharTypeOther>
  const std::basic_string<UTFCharType>& Localize(const std::basic_string_view<UTFCharTypeOther> key) const
  {
    return Localize(ToUTF<UTFCharType>(key));
  }
  
  template <typename UTFCharTypeOther>
  requires IsUTFCharType<UTFCharTypeOther> && !std::same_as<UTFCharType, UTFCharTypeOther>
  const std::basic_string<UTFCharType>& Localize(const std::basic_string<UTFCharTypeOther>& key) const
  {
    return Localize(ToUTF<UTFCharType>(key));
  }
  
  template <typename UTFCharTypeOther, size_t UTFCharTypeOtherSize>
  requires IsUTFCharType<UTFCharTypeOther> && !std::same_as<UTFCharType, UTFCharTypeOther>
  const std::basic_string<UTFCharType>& Localize(const UTFCharTypeOther (& key)[UTFCharTypeOtherSize]) const
  {
    return Localize(ToUTF<UTFCharType>(key));
  }
  
  template <typename UTFCharTypeOther>
  requires IsUTFCharType<UTFCharTypeOther> && !std::same_as<UTFCharType, UTFCharTypeOther>
  const std::basic_string<UTFCharType>& Localize(const UTFCharTypeOther* key, const size_t length) const
  {
    return Localize(ToUTF<UTFCharType>(key, length));
  }

  template <typename... FormatArgs>
  const std::basic_string<UTFCharType>& LocalizeFormat(const std::basic_string_view<UTFCharType> key, FormatArgs&&... args) const
  {
    thread_local static std::basic_string<UTFCharType> utfOut;
    return LocalizeFormatInternal(utfOut, key, std::forward<FormatArgs>(args)...);
  }
  
  template <typename... FormatArgs>
  const std::basic_string<UTFCharType>& LocalizeFormat(const std::basic_string<UTFCharType>& key, FormatArgs&&... args) const
  {
    return LocalizeFormat(std::basic_string_view<UTFCharType>(key), std::forward<FormatArgs>(args)...);
  }
  
  template <size_t UTFCharTypeSize, typename... FormatArgs>
  const std::basic_string<UTFCharType>& LocalizeFormat(const UTFCharType (& key)[UTFCharTypeSize], FormatArgs&&... args) const
  {
    return LocalizeFormat(std::basic_string_view<UTFCharType>(key, UTFCharTypeSize - 1), std::forward<FormatArgs>(args)...);
  }
  
  template <typename UTFCharTypeOther, typename... FormatArgs>
  requires IsUTFCharType<UTFCharTypeOther> && !std::same_as<UTFCharType, UTFCharTypeOther>
  const std::basic_string<UTFCharType>& LocalizeFormat(const std::basic_string_view<UTFCharTypeOther> key, FormatArgs&&... args) const
  {
    return LocalizeFormat(ToUTF<UTFCharType>(key), std::forward<FormatArgs>(args)...);
  }
  
  template <typename UTFCharTypeOther, typename... FormatArgs>
  requires IsUTFCharType<UTFCharTypeOther> && !std::same_as<UTFCharType, UTFCharTypeOther>
  const std::basic_string<UTFCharType>& LocalizeFormat(const std::basic_string<UTFCharTypeOther>& key, FormatArgs&&... args) const
  {
    return LocalizeFormat(ToUTF<UTFCharType>(key), std::forward<FormatArgs>(args)...);
  }
  
  template <typename UTFCharTypeOther, size_t UTFCharTypeOtherSize, typename... FormatArgs>
  requires IsUTFCharType<UTFCharTypeOther> && !std::same_as<UTFCharType, UTFCharTypeOther>
  const std::basic_string<UTFCharType>& LocalizeFormat(const UTFCharTypeOther (& key)[UTFCharTypeOtherSize], FormatArgs&&... args) const
  {
    return LocalizeFormat(ToUTF<UTFCharType>(key), std::forward<FormatArgs>(args)...);
  }

private:
  const std::basic_string<UTFCharType>& LocalizeInternal(const std::basic_string_view<UTFCharType> key, const std::basic_string<UTFCharType>& language) const
  {
    if (language.empty())
      return LocalizationInstance::Empty;

    auto languageToLocalizationPathIt = languageToLocalizationPathsMap.find(language);
    if (languageToLocalizationPathIt == languageToLocalizationPathsMap.cend())
      return LocalizationInstance::Empty;

    for (const auto& localizationPath : languageToLocalizationPathIt->second)
    {
      const auto localizationInstanceIt = localizationInstancesMap.find(localizationPath);
      if (localizationInstanceIt == localizationInstancesMap.cend())
        continue;
      
      const auto& localized = localizationInstanceIt->second.Localize(key);
      if (!localized.empty())
        return localized;
    }

    return LocalizationInstance::Empty;
  }

  template <typename... FormatArgs>
  const std::basic_string<UTFCharType>& LocalizeFormatInternal(std::basic_string<UTFCharType>& buffer, const std::basic_string_view<UTFCharType> key, FormatArgs&&... args) const
  {
    const auto& localizedFormat = Localize(key);
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

  UTFToTypeMapCI<UTFCharType, std::vector<std::filesystem::path>> languageToLocalizationPathsMap;
  PathToTypeMapCI<LocalizationInstance> localizationInstancesMap;
  std::basic_string<UTFCharType> defaultLanguage;
  std::basic_string<UTFCharType> currentLanguage;
};
