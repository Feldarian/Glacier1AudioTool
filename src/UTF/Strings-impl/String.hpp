//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//
// TODO - verify null-terminated flag for std::string

#pragma once

namespace UTF
{

template <typename UTFStorageType, typename UTFCharType, bool CaseSensitive>
requires IsUTFCharType<UTFCharType> && IsAnyOfTypes<UTFStorageType, std::basic_string<UTFCharType>, std::basic_string_view<UTFCharType>>
class StringWrapper
{
public:
  inline static constexpr auto npos{ UTFStorageType::npos };

  StringWrapper() = default;

  template <typename UTFCharTypeInput>
  StringWrapper(const std::basic_string_view<UTFCharTypeInput> other, const bool nullTerminated = false)
    requires IsUTFCharType<UTFCharTypeInput> && (std::same_as<UTFStorageType, std::basic_string<UTFCharType>> || IsSameUTFCharType<UTFCharType, UTFCharTypeInput>)
  {
    *this = other;

    if constexpr (std::same_as<UTFStorageType, std::basic_string_view<UTFCharType>>)
      this->nullTerminated = nullTerminated;
  }

  template <typename UTFCharTypeInput>
  StringWrapper(const std::basic_string<UTFCharTypeInput> &other)
    requires IsUTFCharType<UTFCharTypeInput> && (std::same_as<UTFStorageType, std::basic_string<UTFCharType>> || IsSameUTFCharType<UTFCharType, UTFCharTypeInput>)
    : StringWrapper{ std::basic_string_view<UTFCharTypeInput>(other), true }
  {}

  StringWrapper(std::basic_string<UTFCharType> &&other) noexcept
    requires std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
    : utfData{ std::move(other) }
    , nullTerminated{ true }
  {}

  template <typename UTFCharTypeInput, size_t UTFSizeInput>
  StringWrapper(const UTFCharTypeInput (&other)[UTFSizeInput])
    requires IsUTFCharType<UTFCharTypeInput> && (std::same_as<UTFStorageType, std::basic_string<UTFCharType>> || IsSameUTFCharType<UTFCharType, UTFCharTypeInput>)
    : StringWrapper{ std::basic_string_view<UTFCharTypeInput>(other, other[UTFSizeInput - 1] ==  UTFCharTypeInput(0) ? UTFSizeInput - 1 : UTFSizeInput), other[UTFSizeInput - 1] ==  UTFCharTypeInput(0) }
  {}

  template <typename UTFCharTypeInput>
  StringWrapper(const UTFCharTypeInput *other, const size_t length)
    requires IsUTFCharType<UTFCharTypeInput> && (std::same_as<UTFStorageType, std::basic_string<UTFCharType>> || IsSameUTFCharType<UTFCharType, UTFCharTypeInput>)
    : StringWrapper{ std::basic_string_view<UTFCharTypeInput>(other, other[length - 1] ==  UTFCharTypeInput(0) ? length - 1 : length), other[length - 1] ==  UTFCharTypeInput(0) }
  {}

  StringWrapper(const std::filesystem::path &other)
    requires std::same_as<UTFStorageType, std::basic_string<UTFCharType>> || IsSameUTFCharType<UTFCharType, wchar_t>
    : StringWrapper{ std::basic_string_view<wchar_t>(other.native()), true }
  {}

  template <typename UTFStorageTypeInput, typename UTFCharTypeInput, bool CaseSensitiveInput>
  StringWrapper(const StringWrapper<UTFStorageTypeInput, UTFCharTypeInput, CaseSensitiveInput>& other)
    requires IsUTFCharType<UTFCharTypeInput> && IsAnyOfTypes<UTFStorageTypeInput, std::basic_string<UTFCharTypeInput>, std::basic_string_view<UTFCharTypeInput>> && (std::same_as<UTFStorageType, std::basic_string<UTFCharType>> || IsSameUTFCharType<UTFCharType, UTFCharTypeInput>)
    : StringWrapper{ std::basic_string_view<UTFCharTypeInput>(other.native()), other.IsNullTerminated() }
  {}

  StringWrapper(const StringWrapper& other)
    : StringWrapper{ std::basic_string_view<UTFCharType>(other.utfData), other.nullTerminated }
  {}

  StringWrapper(StringWrapper&& other) noexcept
    : utfData{ std::move(other.utfData) }
    , nullTerminated{ other.nullTerminated }
  {}

  template <typename UTFCharTypeInput>
  StringWrapper& operator=(const std::basic_string_view<UTFCharTypeInput> other)
    requires IsSameUTFCharType<UTFCharType, UTFCharTypeInput>
  {
    if (other.empty())
      utfData = {};
    else
      utfData = {reinterpret_cast<const UTFCharType *>(other.data()), other.size()};

    if constexpr (std::same_as<UTFStorageType, std::basic_string_view<UTFCharType>>)
      nullTerminated = false;

    return *this;
  }

  template <typename UTFCharTypeInput>
  StringWrapper& operator=(const std::basic_string_view<UTFCharTypeInput> other)
    requires IsUTF8CharType<UTFCharType> && IsUTF16CharType<UTFCharTypeInput> && std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  {
    UErrorCode errorCode = U_ZERO_ERROR;

    resize(other.size() * std::max(1ull, sizeof(UTFCharTypeInput) / sizeof(UTFCharType)), static_cast<UTFCharType>(0));

    int32_t length = 0;

    u_strToUTF8(reinterpret_cast<char *>(data()), size(), &length, reinterpret_cast<const UChar *>(other.data()), other.size(), &errorCode);

    if (errorCode > U_ZERO_ERROR)
      length = 0;

    resize(length, static_cast<UTFCharType>(0));

    return *this;
  }

  template <typename UTFCharTypeInput>
  StringWrapper& operator=(const std::basic_string_view<UTFCharTypeInput> other)
    requires IsUTF8CharType<UTFCharType> && IsUTF32CharType<UTFCharTypeInput> && std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  {
    UErrorCode errorCode = U_ZERO_ERROR;

    resize(other.size() * std::max(1ull, sizeof(UTFCharTypeInput) / sizeof(UTFCharType)), static_cast<UTFCharType>(0));

    size_t offset = 0;
    int32_t length = 0;

    auto *reinterpretedData = reinterpret_cast<uint8_t *>(data());
    const auto reinterpretedDataSize = size();
    for (const auto otherCodePoint : other)
    {
      [[maybe_unused]] bool wasError = false;
      const auto codePoint = static_cast<uint32_t>(otherCodePoint);
      U8_APPEND(reinterpretedData, offset, reinterpretedDataSize, codePoint, wasError);
      length = offset;
    }

    if (errorCode > U_ZERO_ERROR)
      length = 0;

    resize(length, static_cast<UTFCharType>(0));

    return *this;
  }

  template <typename UTFCharTypeInput>
  StringWrapper& operator=(const std::basic_string_view<UTFCharTypeInput> other)
    requires IsUTF16CharType<UTFCharType> && IsUTF8CharType<UTFCharTypeInput> && std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  {
    UErrorCode errorCode = U_ZERO_ERROR;

    resize(other.size() * std::max(1ull, sizeof(UTFCharTypeInput) / sizeof(UTFCharType)), static_cast<UTFCharType>(0));

    int32_t length = 0;

    u_strFromUTF8(reinterpret_cast<UChar *>(data()), size(), &length, reinterpret_cast<const char *>(other.data()), other.size(), &errorCode);

    if (errorCode > U_ZERO_ERROR)
      length = 0;

    resize(length, static_cast<UTFCharType>(0));

    return *this;
  }

  template <typename UTFCharTypeInput>
  StringWrapper& operator=(const std::basic_string_view<UTFCharTypeInput> other)
    requires IsUTF16CharType<UTFCharType> && IsUTF32CharType<UTFCharTypeInput> && std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  {
    UErrorCode errorCode = U_ZERO_ERROR;

    resize(other.size() * std::max(1ull, sizeof(UTFCharTypeInput) / sizeof(UTFCharType)), static_cast<UTFCharType>(0));

    int32_t length = 0;

    u_strFromUTF32(reinterpret_cast<UChar *>(data()), size(), &length, reinterpret_cast<const UChar32 *>(other.data()), other.size(), &errorCode);

    if (errorCode > U_ZERO_ERROR)
      length = 0;

    resize(length, static_cast<UTFCharType>(0));

    return *this;
  }

  template <typename UTFCharTypeInput>
  StringWrapper& operator=(const std::basic_string_view<UTFCharTypeInput> other)
    requires IsUTF32CharType<UTFCharType> && IsUTF8CharType<UTFCharTypeInput> && std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  {
    UErrorCode errorCode = U_ZERO_ERROR;

    resize(other.size() * std::max(1ull, sizeof(UTFCharTypeInput) / sizeof(UTFCharType)), static_cast<UTFCharType>(0));

    size_t offset = 0;
    int32_t length = 0;

    auto *reinterpretedData = reinterpret_cast<UChar32 *>(data());
    const auto *inputData = other.data();
    const auto inputDataSize = other.size();
    for (size_t inputOffset = 0; inputOffset < inputDataSize;)
    {
      auto &outputChar = reinterpretedData[offset++];
      U8_NEXT(inputData, inputOffset, inputDataSize, outputChar);
    }
    length = offset;

    if (errorCode > U_ZERO_ERROR)
      length = 0;

    resize(length, static_cast<UTFCharType>(0));

    return *this;
  }

  template <typename UTFCharTypeInput>
  StringWrapper& operator=(const std::basic_string_view<UTFCharTypeInput> other)
    requires IsUTF32CharType<UTFCharType> && IsUTF16CharType<UTFCharTypeInput> && std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  {
    UErrorCode errorCode = U_ZERO_ERROR;

    resize(other.size() * std::max(1ull, sizeof(UTFCharTypeInput) / sizeof(UTFCharType)), static_cast<UTFCharType>(0));

    size_t offset = 0;
    int32_t length = 0;

    auto *reinterpretedData = reinterpret_cast<UChar32 *>(data());
    const auto *inputData = other.data();
    const auto inputDataSize = other.size();
    for (size_t inputOffset = 0; inputOffset < inputDataSize;)
    {
      auto &outputChar = reinterpretedData[offset++];
      U16_NEXT(inputData, inputOffset, inputDataSize, outputChar);
    }
    length = offset;

    if (errorCode > U_ZERO_ERROR)
      length = 0;

    resize(length, static_cast<UTFCharType>(0));

    return *this;
  }

  template <typename UTFCharTypeInput>
  StringWrapper& operator=(const std::basic_string<UTFCharTypeInput> &other)
    requires IsUTFCharType<UTFCharTypeInput> && (std::same_as<UTFStorageType, std::basic_string<UTFCharType>> || std::same_as<UTFCharType, UTFCharTypeInput>)
  {
    *this = std::basic_string_view<UTFCharTypeInput>(other);

    if constexpr (std::same_as<UTFStorageType, std::basic_string_view<UTFCharType>>)
      nullTerminated = false;

    return *this;
  }

  template <typename UTFCharTypeInput, size_t UTFSizeInput>
  StringWrapper& operator=(const UTFCharTypeInput (&other)[UTFSizeInput])
    requires IsUTFCharType<UTFCharTypeInput> && (std::same_as<UTFStorageType, std::basic_string<UTFCharType>> || std::same_as<UTFCharType, UTFCharTypeInput>)
  {
    *this = std::basic_string_view<UTFCharTypeInput>(other, other[UTFSizeInput - 1] ==  UTFCharTypeInput(0) ? UTFSizeInput - 1 : UTFSizeInput);

    if constexpr (std::same_as<UTFStorageType, std::basic_string_view<UTFCharType>>)
      nullTerminated = other[UTFSizeInput - 1] ==  UTFCharTypeInput(0);

    return *this;
  }

  StringWrapper& operator=(const std::filesystem::path& other)
    requires std::same_as<UTFStorageType, std::basic_string<UTFCharType>> || IsSameUTFCharType<UTFCharType, wchar_t>
  {
    return *this = other.native();
  }

  template <typename UTFStorageTypeInput, typename UTFCharTypeInput, bool CaseSensitiveInput>
  requires IsUTFCharType<UTFCharTypeInput> && IsAnyOfTypes<UTFStorageTypeInput, std::basic_string<UTFCharTypeInput>, std::basic_string_view<UTFCharTypeInput>> && (std::same_as<UTFStorageType, std::basic_string<UTFCharType>> || IsSameUTFCharType<UTFCharType, UTFCharTypeInput>)
  StringWrapper& operator=(const StringWrapper<UTFStorageTypeInput, UTFCharTypeInput, CaseSensitiveInput>& other)
  {
    *this = other.native();
    nullTerminated = other.IsNullTerminated();

    return *this;
  }

  StringWrapper& operator=(const StringWrapper& other)
  {
    if (this == &other)
      return *this;

    utfData = other.utfData;
    nullTerminated = other.nullTerminated;

    return *this;
  }

  StringWrapper& operator=(StringWrapper&& other) noexcept
  {
    if (this == &other)
      return *this;

    utfData = std::move(other.utfData);
    nullTerminated = other.nullTerminated;

    return *this;
  }

  [[nodiscard]] UTFCharType& operator[](const size_t index)
  {
    return utfData[index];
  }

  [[nodiscard]] UTFCharType operator[](const size_t index) const
  {
    return utfData[index];
  }

  template <typename UTFCharTypeInput>
  [[nodiscard]] auto operator<=>(const std::basic_string_view<UTFCharTypeInput> other) const
    requires IsUTF8CharType<UTFCharType> && IsUTF8CharType<UTFCharTypeInput>
  {
    auto *reinterpretedData = reinterpret_cast<const uint8_t *>(data());
    const auto reinterpretedDataSize = size();
    size_t reinterpretedDataOffset = 0;
    auto *reinterpretedInput = reinterpret_cast<const uint8_t *>(other.data());
    const auto reinterpretedInputSize = other.size();
    size_t reinterpretedInputOffset = 0;
    auto glyph = CodepointInvalid;
    auto glyphInput = CodepointInvalid;
    while (reinterpretedDataOffset < reinterpretedDataSize && reinterpretedInputOffset < reinterpretedInputSize)
    {
      U8_NEXT(reinterpretedData, reinterpretedDataOffset, reinterpretedDataSize, glyph);
      U8_NEXT(reinterpretedInput, reinterpretedInputOffset, reinterpretedInputSize, glyphInput);

      if constexpr (!CaseSensitive)
      {
        glyph = u_tolower(glyph);
        glyphInput = u_tolower(glyphInput);
      }

      if (glyph != glyphInput)
        return glyph <=> glyphInput;
    }

    if (reinterpretedDataOffset != reinterpretedDataSize)
      U8_NEXT(reinterpretedData, reinterpretedDataOffset, reinterpretedDataSize, glyph);
    else
      glyph = CodepointInvalid;

    if (reinterpretedInputOffset != reinterpretedInputSize)
      U8_NEXT(reinterpretedInput, reinterpretedInputOffset, reinterpretedInputSize, glyphInput);
    else
      glyphInput = CodepointInvalid;

    if constexpr (!CaseSensitive)
    {
      glyph = u_tolower(glyph);
      glyphInput = u_tolower(glyphInput);
    }

    return glyph <=> glyphInput;
  }

  template <typename UTFCharTypeInput>
  [[nodiscard]] auto operator<=>(const std::basic_string_view<UTFCharTypeInput> other) const
    requires IsUTF8CharType<UTFCharType> && IsUTF16CharType<UTFCharTypeInput>
  {
    auto *reinterpretedData = reinterpret_cast<const uint8_t *>(data());
    const auto reinterpretedDataSize = size();
    size_t reinterpretedDataOffset = 0;
    auto *reinterpretedInput = reinterpret_cast<const UChar *>(other.data());
    const auto reinterpretedInputSize = other.size();
    size_t reinterpretedInputOffset = 0;
    auto glyph = CodepointInvalid;
    auto glyphInput = CodepointInvalid;
    while (reinterpretedDataOffset < reinterpretedDataSize && reinterpretedInputOffset < reinterpretedInputSize)
    {
      U8_NEXT(reinterpretedData, reinterpretedDataOffset, reinterpretedDataSize, glyph);
      U16_NEXT(reinterpretedInput, reinterpretedInputOffset, reinterpretedInputSize, glyphInput);

      if constexpr (!CaseSensitive)
      {
        glyph = u_tolower(glyph);
        glyphInput = u_tolower(glyphInput);
      }

      if (glyph != glyphInput)
        return glyph <=> glyphInput;
    }

    if (reinterpretedDataOffset != reinterpretedDataSize)
      U8_NEXT(reinterpretedData, reinterpretedDataOffset, reinterpretedDataSize, glyph);
    else
      glyph = CodepointInvalid;

    if (reinterpretedInputOffset != reinterpretedInputSize)
      U16_NEXT(reinterpretedInput, reinterpretedInputOffset, reinterpretedInputSize, glyphInput);
    else
      glyphInput = CodepointInvalid;

    if constexpr (!CaseSensitive)
    {
      glyph = u_tolower(glyph);
      glyphInput = u_tolower(glyphInput);
    }

    return glyph <=> glyphInput;
  }

  template <typename UTFCharTypeInput>
  [[nodiscard]] auto operator<=>(const std::basic_string_view<UTFCharTypeInput> other) const
    requires IsUTF8CharType<UTFCharType> && IsUTF32CharType<UTFCharTypeInput>
  {
    auto *reinterpretedData = reinterpret_cast<const uint8_t *>(data());
    const auto reinterpretedDataSize = size();
    size_t reinterpretedDataOffset = 0;
    size_t inputOffset = 0;
    auto glyph = CodepointInvalid;
    auto glyphInput = CodepointInvalid;
    while (reinterpretedDataOffset < reinterpretedDataSize && inputOffset < other.size())
    {
      U8_NEXT(reinterpretedData, reinterpretedDataOffset, reinterpretedDataSize, glyph);
      glyphInput = static_cast<uint32_t>(other[inputOffset++]);

      if constexpr (!CaseSensitive)
      {
        glyph = u_tolower(glyph);
        glyphInput = u_tolower(glyphInput);
      }

      if (glyph != glyphInput)
        return glyph <=> glyphInput;
    }

    if (reinterpretedDataOffset != reinterpretedDataSize)
      U8_NEXT(reinterpretedData, reinterpretedDataOffset, reinterpretedDataSize, glyph);
    else
      glyph = CodepointInvalid;

    if (inputOffset != other.size())
      glyphInput = static_cast<uint32_t>(other[inputOffset]);
    else
      glyphInput =  CodepointInvalid;

    if constexpr (!CaseSensitive)
    {
      glyph = u_tolower(glyph);
      glyphInput = u_tolower(glyphInput);
    }

    return glyph <=> glyphInput;
  }

  template <typename UTFCharTypeInput>
  [[nodiscard]] auto operator<=>(const std::basic_string_view<UTFCharTypeInput> other) const
    requires IsUTF16CharType<UTFCharType> && IsUTF8CharType<UTFCharTypeInput>
  {
    auto *reinterpretedData = reinterpret_cast<const UChar *>(data());
    const auto reinterpretedDataSize = size();
    size_t reinterpretedDataOffset = 0;
    auto *reinterpretedInput = reinterpret_cast<const uint8_t *>(other.data());
    const auto reinterpretedInputSize = other.size();
    size_t reinterpretedInputOffset = 0;
    auto glyph = CodepointInvalid;
    auto glyphInput = CodepointInvalid;
    while (reinterpretedDataOffset < reinterpretedDataSize && reinterpretedInputOffset < reinterpretedInputSize)
    {
      U16_NEXT(reinterpretedData, reinterpretedDataOffset, reinterpretedDataSize, glyph);
      U8_NEXT(reinterpretedInput, reinterpretedInputOffset, reinterpretedInputSize, glyphInput);

      if constexpr (!CaseSensitive)
      {
        glyph = u_tolower(glyph);
        glyphInput = u_tolower(glyphInput);
      }

      if (glyph != glyphInput)
        return glyph <=> glyphInput;
    }

    if (reinterpretedDataOffset != reinterpretedDataSize)
      U16_NEXT(reinterpretedData, reinterpretedDataOffset, reinterpretedDataSize, glyph);
    else
      glyph = CodepointInvalid;

    if (reinterpretedInputOffset != reinterpretedInputSize)
      U8_NEXT(reinterpretedInput, reinterpretedInputOffset, reinterpretedInputSize, glyphInput);
    else
      glyphInput = CodepointInvalid;

    if constexpr (!CaseSensitive)
    {
      glyph = u_tolower(glyph);
      glyphInput = u_tolower(glyphInput);
    }

    return glyph <=> glyphInput;
  }

  template <typename UTFCharTypeInput>
  [[nodiscard]] auto operator<=>(const std::basic_string_view<UTFCharTypeInput> other) const
    requires IsUTF16CharType<UTFCharType> && IsUTF16CharType<UTFCharTypeInput>
  {
    auto *reinterpretedData = reinterpret_cast<const UChar *>(data());
    const auto reinterpretedDataSize = size();
    size_t reinterpretedDataOffset = 0;
    auto *reinterpretedInput = reinterpret_cast<const UChar *>(other.data());
    const auto reinterpretedInputSize = other.size();
    size_t reinterpretedInputOffset = 0;
    auto glyph = CodepointInvalid;
    auto glyphInput = CodepointInvalid;
    while (reinterpretedDataOffset < reinterpretedDataSize && reinterpretedInputOffset < reinterpretedInputSize)
    {
      U16_NEXT(reinterpretedData, reinterpretedDataOffset, reinterpretedDataSize, glyph);
      U16_NEXT(reinterpretedInput, reinterpretedInputOffset, reinterpretedInputSize, glyphInput);

      if constexpr (!CaseSensitive)
      {
        glyph = u_tolower(glyph);
        glyphInput = u_tolower(glyphInput);
      }

      if (glyph != glyphInput)
        return glyph <=> glyphInput;
    }

    if (reinterpretedDataOffset != reinterpretedDataSize)
      U16_NEXT(reinterpretedData, reinterpretedDataOffset, reinterpretedDataSize, glyph);
    else
      glyph = CodepointInvalid;

    if (reinterpretedInputOffset != reinterpretedInputSize)
      U16_NEXT(reinterpretedInput, reinterpretedInputOffset, reinterpretedInputSize, glyphInput);
    else
      glyphInput = CodepointInvalid;

    if constexpr (!CaseSensitive)
    {
      glyph = u_tolower(glyph);
      glyphInput = u_tolower(glyphInput);
    }

    return glyph <=> glyphInput;
  }

  template <typename UTFCharTypeInput>
  [[nodiscard]] auto operator<=>(const std::basic_string_view<UTFCharTypeInput> other) const
    requires IsUTF16CharType<UTFCharType> && IsUTF32CharType<UTFCharTypeInput>
  {
    auto *reinterpretedData = reinterpret_cast<const UChar *>(data());
    const auto reinterpretedDataSize = size();
    size_t reinterpretedDataOffset = 0;
    size_t inputOffset = 0;
    auto glyph = CodepointInvalid;
    auto glyphInput = CodepointInvalid;
    while (reinterpretedDataOffset < reinterpretedDataSize && inputOffset < other.size())
    {
      U16_NEXT(reinterpretedData, reinterpretedDataOffset, reinterpretedDataSize, glyph);
      glyphInput = static_cast<uint32_t>(other[inputOffset++]);

      if constexpr (!CaseSensitive)
      {
        glyph = u_tolower(glyph);
        glyphInput = u_tolower(glyphInput);
      }

      if (glyph != glyphInput)
        return glyph <=> glyphInput;
    }

    if (reinterpretedDataOffset != reinterpretedDataSize)
      U16_NEXT(reinterpretedData, reinterpretedDataOffset, reinterpretedDataSize, glyph);
    else
      glyph = CodepointInvalid;

    if (inputOffset != other.size())
      glyphInput = static_cast<uint32_t>(other[inputOffset]);
    else
      glyphInput =  CodepointInvalid;

    if constexpr (!CaseSensitive)
    {
      glyph = u_tolower(glyph);
      glyphInput = u_tolower(glyphInput);
    }

    return glyph <=> glyphInput;
  }

  template <typename UTFCharTypeInput>
  [[nodiscard]] auto operator<=>(const std::basic_string_view<UTFCharTypeInput> other) const
    requires IsUTF32CharType<UTFCharType> && IsUTF8CharType<UTFCharTypeInput>
  {
    auto *reinterpretedInput = reinterpret_cast<const uint8_t *>(other.data());
    const auto reinterpretedInputSize = other.size();
    size_t reinterpretedInputOffset = 0;
    size_t dataOffset = 0;
    auto glyph = CodepointInvalid;
    auto glyphInput = CodepointInvalid;
    while (dataOffset < size() && reinterpretedInputOffset < reinterpretedInputSize)
    {
      glyph = static_cast<uint32_t>(utfData[dataOffset++]);
      U8_NEXT(reinterpretedInput, reinterpretedInputOffset, reinterpretedInputSize, glyphInput);

      if constexpr (!CaseSensitive)
      {
        glyph = u_tolower(glyph);
        glyphInput = u_tolower(glyphInput);
      }

      if (glyph != glyphInput)
        return glyph <=> glyphInput;
    }

    if (dataOffset != size())
      glyph = static_cast<uint32_t>(utfData[dataOffset]);
    else
      glyph = CodepointInvalid;

    if (reinterpretedInputOffset != reinterpretedInputSize)
      U8_NEXT(reinterpretedInput, reinterpretedInputOffset, reinterpretedInputSize, glyphInput);
    else
      glyphInput = CodepointInvalid;

    if constexpr (!CaseSensitive)
    {
      glyph = u_tolower(glyph);
      glyphInput = u_tolower(glyphInput);
    }

    return glyph <=> glyphInput;
  }

  template <typename UTFCharTypeInput>
  [[nodiscard]] auto operator<=>(const std::basic_string_view<UTFCharTypeInput> other) const
    requires IsUTF32CharType<UTFCharType> && IsUTF16CharType<UTFCharTypeInput>
  {
    auto *reinterpretedInput = reinterpret_cast<const UChar *>(other.data());
    const auto reinterpretedInputSize = other.size();
    size_t reinterpretedInputOffset = 0;
    size_t dataOffset = 0;
    auto glyph = CodepointInvalid;
    auto glyphInput = CodepointInvalid;
    while (dataOffset < size() && reinterpretedInputOffset < reinterpretedInputSize)
    {
      glyph = static_cast<uint32_t>(utfData[dataOffset++]);

      U16_NEXT(reinterpretedInput, reinterpretedInputOffset, reinterpretedInputSize, glyphInput);

      if constexpr (!CaseSensitive)
      {
        glyph = u_tolower(glyph);
        glyphInput = u_tolower(glyphInput);
      }

      if (glyph != glyphInput)
        return glyph <=> glyphInput;
    }

    if (dataOffset != size())
      glyph = static_cast<uint32_t>(utfData[dataOffset]);
    else
      glyph = CodepointInvalid;

    if (reinterpretedInputOffset != reinterpretedInputSize)
      U16_NEXT(reinterpretedInput, reinterpretedInputOffset, reinterpretedInputSize, glyphInput);
    else
      glyphInput = CodepointInvalid;

    if constexpr (!CaseSensitive)
    {
      glyph = u_tolower(glyph);
      glyphInput = u_tolower(glyphInput);
    }

    return glyph <=> glyphInput;
  }

  template <typename UTFCharTypeInput>
  [[nodiscard]] auto operator<=>(const std::basic_string_view<UTFCharTypeInput> other) const
    requires IsUTF32CharType<UTFCharType> && IsUTF32CharType<UTFCharTypeInput>
  {
    size_t inputOffset = 0;
    size_t dataOffset = 0;
    auto glyph = CodepointInvalid;
    auto glyphInput = CodepointInvalid;
    while (dataOffset < size() && inputOffset < other.size())
    {
      glyph = static_cast<uint32_t>(utfData[dataOffset++]);
      glyphInput = static_cast<uint32_t>(other[inputOffset++]);

      if constexpr (!CaseSensitive)
      {
        glyph = u_tolower(glyph);
        glyphInput = u_tolower(glyphInput);
      }

      if (glyphInput != glyph)
        return glyphInput <=> glyph;
    }

    if (dataOffset != size())
      glyph = static_cast<uint32_t>(utfData[dataOffset]);
    else
      glyph = CodepointInvalid;

    if (inputOffset != other.size())
      glyphInput = static_cast<uint32_t>(other[inputOffset]);
    else
      glyphInput = CodepointInvalid;

    if constexpr (!CaseSensitive)
    {
      glyph = u_tolower(glyph);
      glyphInput = u_tolower(glyphInput);
    }

    return glyph <=> glyphInput;
  }

  template <typename UTFCharTypeInput>
  [[nodiscard]] auto operator<=>(const std::basic_string<UTFCharTypeInput>& other) const
    requires IsUTFCharType<UTFCharTypeInput>
  {
    return *this <=> std::basic_string_view<UTFCharTypeInput>(other);
  }

  template <typename UTFCharTypeInput, size_t UTFSizeInput>
  [[nodiscard]] auto operator<=>(const UTFCharTypeInput (&other)[UTFSizeInput]) const
    requires IsUTFCharType<UTFCharTypeInput>
  {
    return *this <=> std::basic_string_view<UTFCharTypeInput>(other, other[UTFSizeInput - 1] ==  UTFCharTypeInput(0) ? UTFSizeInput - 1 : UTFSizeInput);
  }

  [[nodiscard]] auto operator<=>(const std::filesystem::path& other) const
  {
    return *this <=> other.native();
  }

  template <typename UTFStorageTypeInput, typename UTFCharTypeInput>
  [[nodiscard]] auto operator<=>(const StringWrapper<UTFStorageTypeInput, UTFCharTypeInput, true>& other) const
    requires IsUTFCharType<UTFCharTypeInput> && IsAnyOfTypes<UTFStorageTypeInput, std::basic_string<UTFCharTypeInput>, std::basic_string_view<UTFCharTypeInput>>
  {
    return *this <=> other.native();
  }

  template <typename UTFStorageTypeInput, typename UTFCharTypeInput>
  [[nodiscard]] auto operator<=>(const StringWrapper<UTFStorageTypeInput, UTFCharTypeInput, false>& other) const
    requires IsUTFCharType<UTFCharTypeInput> && IsAnyOfTypes<UTFStorageTypeInput, std::basic_string<UTFCharTypeInput>, std::basic_string_view<UTFCharTypeInput>>
  {
    const auto result = other <=> native();
    if (result == 0)
      return result;

    return result < 0 ? std::strong_ordering::greater : std::strong_ordering::less;
  }

  [[nodiscard]] auto operator<=>(const StringWrapper& other) const
  {
    return *this <=> other.utfData;
  }

  template <typename UTFCharTypeInput>
  [[nodiscard]] bool operator==(const std::basic_string_view<UTFCharTypeInput> other) const
    requires IsUTFCharType<UTFCharTypeInput>
  {
    return (*this <=> other) == std::weak_ordering::equivalent;
  }

  template <typename UTFCharTypeInput>
  [[nodiscard]] bool operator==(const std::basic_string<UTFCharTypeInput>& other) const
    requires IsUTFCharType<UTFCharTypeInput>
  {
    return (*this <=> other) == std::weak_ordering::equivalent;
  }

  template <typename UTFCharTypeInput, size_t UTFSizeInput>
  [[nodiscard]] bool operator==(const UTFCharTypeInput (&other)[UTFSizeInput]) const
    requires IsUTFCharType<UTFCharTypeInput>
  {
    return (*this <=> other) == std::weak_ordering::equivalent;
  }

  [[nodiscard]] bool operator==(const std::filesystem::path& other) const
  {
    return (*this <=> other) == std::weak_ordering::equivalent;
  }

  template <typename UTFStorageTypeInput, typename UTFCharTypeInput, bool CaseSensitiveInput>
  [[nodiscard]] bool operator==(const StringWrapper<UTFStorageTypeInput, UTFCharTypeInput, CaseSensitiveInput>& other) const
    requires IsUTFCharType<UTFCharTypeInput> && IsAnyOfTypes<UTFStorageTypeInput, std::basic_string<UTFCharTypeInput>, std::basic_string_view<UTFCharTypeInput>>
  {
    return (*this <=> other.native()) == std::weak_ordering::equivalent;
  }

  [[nodiscard]] bool operator==(const StringWrapper& other) const
  {
    return (*this <=> other.utfData) == std::weak_ordering::equivalent;
  }

  template <typename UTFCharTypeInput>
  StringWrapper& operator+=(const std::basic_string_view<UTFCharTypeInput> other)
    requires IsUTFCharType<UTFCharTypeInput> && std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  {
    return *this += StringWrapper<std::basic_string_view<UTFCharTypeInput>, UTFCharTypeInput, CaseSensitive>{other};
  }

  template <typename UTFCharTypeInput>
  StringWrapper& operator+=(const std::basic_string<UTFCharTypeInput>& other)
    requires IsUTFCharType<UTFCharTypeInput> && std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  {
    return *this += StringWrapper<std::basic_string_view<UTFCharTypeInput>, UTFCharTypeInput, CaseSensitive>{other};
  }

  template <typename UTFCharTypeInput, size_t UTFSizeInput>
  StringWrapper& operator+=(const UTFCharTypeInput (&other)[UTFSizeInput])
    requires IsUTFCharType<UTFCharTypeInput> && std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  {
    return *this += StringWrapper<std::basic_string_view<UTFCharTypeInput>, UTFCharTypeInput, CaseSensitive>{other};
  }

  StringWrapper& operator+=(const std::filesystem::path& other)
    requires std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  {
    return *this += StringWrapper<std::basic_string_view<wchar_t>, wchar_t, CaseSensitive>{other};
  }

  template <typename UTFStorageTypeInput, typename UTFCharTypeInput, bool CaseSensitiveInput>
  StringWrapper& operator+=(const StringWrapper<UTFStorageTypeInput, UTFCharTypeInput, CaseSensitiveInput>& other)
    requires IsSameUTFCharType<UTFCharType, UTFCharTypeInput> && std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  {
    if (!other.empty())
      utfData.append(reinterpret_cast<const UTFCharType *>(other.data()), other.size());

    return *this;
  }

  template <typename UTFStorageTypeInput, typename UTFCharTypeInput, bool CaseSensitiveInput>
  StringWrapper& operator+=(const StringWrapper<UTFStorageTypeInput, UTFCharTypeInput, CaseSensitiveInput>& other)
    requires (!IsSameUTFCharType<UTFCharType, UTFCharTypeInput>) && std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  {
    return *this += StringWrapper{other};
  }

  StringWrapper& operator+=(const StringWrapper& other)
    requires std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  {
    if (!other.empty())
      utfData.append(other.data(), other.size());

    return *this;
  }

  template <typename UTFCharTypeInput>
  [[nodiscard]] StringWrapper<std::basic_string<UTFCharType>, UTFCharType, CaseSensitive> operator+(const std::basic_string_view<UTFCharTypeInput> other)
    requires IsUTFCharType<UTFCharTypeInput>
  {
    return *this + StringWrapper<std::basic_string_view<UTFCharTypeInput>, UTFCharTypeInput, CaseSensitive>{other};
  }

  template <typename UTFCharTypeInput>
  [[nodiscard]] StringWrapper<std::basic_string<UTFCharType>, UTFCharType, CaseSensitive> operator+(const std::basic_string<UTFCharTypeInput>& other)
    requires IsUTFCharType<UTFCharTypeInput>
  {
    return *this + StringWrapper<std::basic_string_view<UTFCharTypeInput>, UTFCharTypeInput, CaseSensitive>{other};
  }

  template <typename UTFCharTypeInput, size_t UTFSizeInput>
  [[nodiscard]] StringWrapper<std::basic_string<UTFCharType>, UTFCharType, CaseSensitive> operator+(const UTFCharTypeInput (&other)[UTFSizeInput])
    requires IsUTFCharType<UTFCharTypeInput>
  {
    return *this + StringWrapper<std::basic_string_view<UTFCharTypeInput>, UTFCharTypeInput, CaseSensitive>{other};
  }

  [[nodiscard]] StringWrapper<std::basic_string<UTFCharType>, UTFCharType, CaseSensitive> operator+(const std::filesystem::path& other)
  {
    return *this + StringWrapper<std::basic_string_view<wchar_t>, wchar_t, CaseSensitive>{other};
  }

  template <typename UTFStorageTypeInput, typename UTFCharTypeInput, bool CaseSensitiveInput>
  [[nodiscard]] StringWrapper<std::basic_string<UTFCharType>, UTFCharType, CaseSensitive> operator+(const StringWrapper<UTFStorageTypeInput, UTFCharTypeInput, CaseSensitiveInput>& other)
    requires IsUTFCharType<UTFCharTypeInput> && IsAnyOfTypes<UTFStorageTypeInput, std::basic_string<UTFCharTypeInput>, std::basic_string_view<UTFCharTypeInput>>
  {
    StringWrapper<std::basic_string<UTFCharType>, UTFCharType, CaseSensitive> result{*this};
    return result += other;
  }

  [[nodiscard]] StringWrapper<std::basic_string<UTFCharType>, UTFCharType, CaseSensitive> operator+(const StringWrapper& other)
  {
    StringWrapper<std::basic_string<UTFCharType>, UTFCharType, CaseSensitive> result{*this};
    return result += other;
  }

  [[nodiscard]] auto begin()
    requires std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  {
    return utfData.begin();
  }

  [[nodiscard]] auto begin() const
  {
    return utfData.cbegin();
  }

  [[nodiscard]] auto cbegin() const
  {
    return utfData.cbegin();
  }

  [[nodiscard]] auto end()
    requires std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  {
    return utfData.end();
  }

  [[nodiscard]] auto end() const
  {
    return utfData.cend();
  }

  [[nodiscard]] auto cend() const
  {
    return utfData.cend();
  }

  [[nodiscard]] UTFCharType* data()
    requires std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  {
    return utfData.data();
  }

  [[nodiscard]] const UTFCharType* data() const
  {
    return utfData.data();
  }

  [[nodiscard]] const UTFCharType* c_str() const
  {
    if constexpr (std::same_as<UTFStorageType, std::basic_string<UTFCharType>>)
      return utfData.c_str();
    else
    {
      if (nullTerminated)
        return utfData.data();

      utfDataSource = std::make_unique<std::basic_string<UTFCharType>>();
      utfDataSource->assign(utfData.data(), utfData.size());
      utfData = *utfDataSource;
      nullTerminated = true;

      return utfData.data();
    }
  }

  [[nodiscard]] UTFCharType& front()
    requires std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  {
    return utfData.front();
  }

  [[nodiscard]] const UTFCharType& front() const
  {
    return utfData.front();
  }

  [[nodiscard]] UTFCharType& back()
    requires std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  {
    return utfData.back();
  }

  [[nodiscard]] const UTFCharType& back() const
  {
    return utfData.back();
  }

  [[nodiscard]] bool empty() const
  {
    return utfData.empty();
  }

  [[nodiscard]] size_t size() const
  {
    return utfData.size();
  }

  [[nodiscard]] size_t length() const
  {
    return utfData.length();
  }

  [[nodiscard]] auto& native()
  {
    return utfData;
  }

  [[nodiscard]] const auto& native() const
  {
    return utfData;
  }

  [[nodiscard]] std::filesystem::path path() const
  {
    // TODO - not taking into account all possible UTF encoding matching wchar_t
    if constexpr (std::same_as<UTFCharType, wchar_t>)
      return { utfData };
    else
      return StringWrapper<std::basic_string<wchar_t>, wchar_t, true>(utfData).path();
  }

  void clear()
    requires std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  {
    utfData.clear();
  }

  void resize(const size_t newSize, const UTFCharType defaultChar = static_cast<UTFCharType>(0))
    requires std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  {
    utfData.resize(newSize, defaultChar);
  }

  bool IsNullTerminated() const
  {
    return nullTerminated;
  }

private:
  mutable UTFStorageType utfData;
  mutable std::unique_ptr<std::basic_string<UTFCharType>> utfDataSource;
  mutable bool nullTerminated = std::same_as<UTFStorageType, std::basic_string<UTFCharType>>;
};

using String8 = StringWrapper<std::basic_string<char>, char, true>;
using String16 = StringWrapper<std::basic_string<UChar>, UChar, true>;
using String32 = StringWrapper<std::basic_string<UChar32>, UChar32, true>;

using StringW = StringWrapper<std::basic_string<wchar_t>, wchar_t, true>;

using String8CI = StringWrapper<std::basic_string<char>, char, false>;
using String16CI = StringWrapper<std::basic_string<UChar>, UChar, false>;
using String32CI = StringWrapper<std::basic_string<UChar32>, UChar32, false>;

using StringWCI = StringWrapper<std::basic_string<wchar_t>, wchar_t, false>;

using StringView8 = StringWrapper<std::basic_string_view<char>, char, true>;
using StringView16 = StringWrapper<std::basic_string_view<UChar>, UChar, true>;
using StringView32 = StringWrapper<std::basic_string_view<UChar32>, UChar32, true>;

using StringViewW = StringWrapper<std::basic_string_view<wchar_t>, wchar_t, true>;

using StringView8CI = StringWrapper<std::basic_string_view<char>, char, false>;
using StringView16CI = StringWrapper<std::basic_string_view<UChar>, UChar, false>;
using StringView32CI = StringWrapper<std::basic_string_view<UChar32>, UChar32, false>;

using StringViewWCI = StringWrapper<std::basic_string_view<wchar_t>, wchar_t, false>;

template <bool CaseSensitive>
struct StringHasher
{
  template<typename UTFCharType>
  requires IsUTFCharType<UTFCharType>
  [[nodiscard]] size_t operator()(const std::basic_string_view<UTFCharType> utf) const noexcept
  {
    if constexpr (CaseSensitive)
    {
      if constexpr (sizeof(size_t) == sizeof(uint64_t))
        return XXH3_64bits(utf.data(), utf.size() * sizeof(UTFCharType));

      if constexpr (sizeof(size_t) == sizeof(uint32_t))
        return XXH32(utf.data(), utf.size() * sizeof(UTFCharType), 0);
    }
    else
    {
      size_t result = 0;

      if constexpr (IsUTF8CharType<UTFCharType>)
      {
        const auto *reinterpretedInput = reinterpret_cast<const uint8_t *>(utf.data());
        const auto reinterpretedInputSize = utf.size();
        size_t reinterpretedInputOffset = 0;
        auto glyph = 0u;
        while (reinterpretedInputOffset < reinterpretedInputSize)
        {
          U8_NEXT(reinterpretedInput, reinterpretedInputOffset, reinterpretedInputSize, glyph);
          glyph = u_tolower(glyph);

          if constexpr (sizeof(size_t) == sizeof(uint64_t))
            result = XXH3_64bits_withSeed(&glyph, sizeof(glyph), result);
          else if constexpr (sizeof(size_t) == sizeof(uint32_t))
            result = XXH32(utf.data(), utf.size() * sizeof(UTFCharType), result);
        }
      }

      if constexpr (IsUTF16CharType<UTFCharType>)
      {
        const auto *reinterpretedInput = reinterpret_cast<const UChar *>(utf.data());
        const auto reinterpretedInputSize = utf.size();
        size_t reinterpretedInputOffset = 0;
        auto glyph = 0u;
        while (reinterpretedInputOffset < reinterpretedInputSize)
        {
          U16_NEXT(reinterpretedInput, reinterpretedInputOffset, reinterpretedInputSize, glyph);
          glyph = u_tolower(glyph);

          if constexpr (sizeof(size_t) == sizeof(uint64_t))
            result = XXH3_64bits_withSeed(&glyph, sizeof(glyph), result);
          else if constexpr (sizeof(size_t) == sizeof(uint32_t))
            result = XXH32(utf.data(), utf.size() * sizeof(UTFCharType), result);
        }
      }

      if constexpr (IsUTF32CharType<UTFCharType>)
      {
        for (const auto utfChar : utf)
        {
          const auto glyph = u_tolower(utfChar);

          if constexpr (sizeof(size_t) == sizeof(uint64_t))
            result = XXH3_64bits_withSeed(&glyph, sizeof(glyph), result);
          else if constexpr (sizeof(size_t) == sizeof(uint32_t))
            result = XXH32(utf.data(), utf.size() * sizeof(UTFCharType), result);
        }
      }

      if constexpr (sizeof(size_t) == sizeof(uint64_t))
      {
        for (const auto utfChar : utf)
        {
          result = XXH3_64bits_withSeed(&utfChar, 1 * sizeof(UTFCharType), result);
        }
      }

      return result;
    }
  }

  template<typename UTFCharType>
  requires IsUTFCharType<UTFCharType>
  [[nodiscard]] size_t operator()(const std::basic_string<UTFCharType> &utf) const noexcept
  {
    return operator()(std::basic_string_view<UTFCharType>(utf));
  }

  template<typename UTFCharType, size_t UTFSize>
  requires IsUTFCharType<UTFCharType>
  [[nodiscard]] size_t operator()(const UTFCharType (&utf)[UTFSize]) const noexcept
  {
    return operator()(std::basic_string_view<UTFCharType>(utf, UTFSize - 1));
  }

  template<typename UTFCharType>
  requires IsUTFCharType<UTFCharType>
  [[nodiscard]] size_t operator()(const UTFCharType *utf, size_t length) const noexcept
  {
    return operator()(std::basic_string_view<UTFCharType>(utf, length));
  }

  [[nodiscard]] size_t operator()(const std::filesystem::path &utf) const noexcept
  {
    return operator()(std::basic_string_view(utf.native()));
  }

  template <typename UTFStorageType, typename UTFCharType, bool CaseSensitiveInput>
  requires UTF::IsUTFCharType<UTFCharType> && UTF::IsAnyOfTypes<UTFStorageType, std::basic_string<UTFCharType>, std::basic_string_view<UTFCharType>>
  [[nodiscard]] size_t operator()(const StringWrapper<UTFStorageType, UTFCharType, CaseSensitiveInput>& utf) const noexcept
  {
    return operator()(utf.native());
  }
};

}

namespace std
{

template <typename UTFStorageType, typename UTFCharType, bool CaseSensitive>
requires UTF::IsUTFCharType<UTFCharType> && UTF::IsAnyOfTypes<UTFStorageType, std::basic_string<UTFCharType>, std::basic_string_view<UTFCharType>>
struct hash<UTF::StringWrapper<UTFStorageType, UTFCharType, CaseSensitive>>
{
  [[nodiscard]] size_t operator()(const UTF::StringWrapper<UTFStorageType, UTFCharType, CaseSensitive>& utf) const noexcept
  {
    return UTF::StringHasher<CaseSensitive>{}(utf);
  }
};

template <typename UTFStorageType, typename UTFCharType, bool CaseSensitive, typename UTFCharTypeOutput>
requires UTF::IsUTFCharType<UTFCharType> && UTF::IsAnyOfTypes<UTFStorageType, std::basic_string<UTFCharType>, std::basic_string_view<UTFCharType>>
struct formatter<UTF::StringWrapper<UTFStorageType, UTFCharType, CaseSensitive>, UTFCharTypeOutput>
{
  template <typename FormatParseContext>
  [[nodiscard]] auto parse(FormatParseContext& pc) const
  {
    return pc.end();
  }

  template<class FormatContext>
  [[nodiscard]] auto format(const UTF::StringWrapper<UTFStorageType, UTFCharType, CaseSensitive>& utf, FormatContext& ctx) const
  {
    if constexpr (std::same_as<UTFCharType, UTFCharTypeOutput>)
      return std::formatter<UTFStorageType, UTFCharTypeOutput>{}.format(utf.native(), ctx);
    else
      return std::formatter<UTF::StringWrapper<std::basic_string<UTFCharTypeOutput>, UTFCharTypeOutput, CaseSensitive>, UTFCharTypeOutput>{}.format(utf.native(), ctx);
  }
};

}
