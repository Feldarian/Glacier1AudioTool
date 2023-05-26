//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

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
  requires IsUTFCharType<UTFCharTypeInput> && (std::same_as<UTFStorageType, std::basic_string<UTFCharType>> || std::same_as<UTFCharType, UTFCharTypeInput>)
  StringWrapper(const std::basic_string_view<UTFCharTypeInput> other)
  {
    *this = other;
  }

  template <typename UTFCharTypeInput>
  requires IsUTFCharType<UTFCharTypeInput> && (std::same_as<UTFStorageType, std::basic_string<UTFCharType>> || std::same_as<UTFCharType, UTFCharTypeInput>)
  StringWrapper(const std::basic_string<UTFCharTypeInput> &other)
    : StringWrapper{ std::basic_string_view<UTFCharTypeInput>(other) }
  {}

  template<typename = std::enable_if_t<std::same_as<UTFStorageType, std::basic_string<UTFCharType>>>>
  requires std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  StringWrapper(std::basic_string<UTFCharType> &&other) noexcept
    : utfData(std::move(other))
  {}

  template <typename UTFCharTypeInput, size_t UTFInputSize>
  requires IsUTFCharType<UTFCharTypeInput> && (std::same_as<UTFStorageType, std::basic_string<UTFCharType>> || std::same_as<UTFCharType, UTFCharTypeInput>)
  StringWrapper(const UTFCharTypeInput (&other)[UTFInputSize])
    : StringWrapper{ std::basic_string_view<UTFCharTypeInput>(other, UTFInputSize - 1) }
  {}

  template <typename UTFCharTypeInput>
  requires IsUTFCharType<UTFCharTypeInput> && (std::same_as<UTFStorageType, std::basic_string<UTFCharType>> || std::same_as<UTFCharType, UTFCharTypeInput>)
  StringWrapper(const UTFCharTypeInput *other, const size_t length)
    : StringWrapper{ std::basic_string_view<UTFCharTypeInput>(other, length) }
  {}

  template<typename = std::enable_if_t<std::same_as<UTFStorageType, std::basic_string<UTFCharType>> || std::same_as<UTFCharType, wchar_t>>>
  StringWrapper(const std::filesystem::path &other)
    : StringWrapper{ std::basic_string_view<wchar_t>(other.native()) }
  {}

  template<typename = std::enable_if_t<std::same_as<UTFStorageType, std::basic_string<wchar_t>>>>
  StringWrapper(std::filesystem::path &&other) noexcept
    : StringWrapper{ std::move(other.native()) }
  {}

  template <typename UTFStorageTypeInput, typename UTFCharTypeInput, bool CaseSensitiveInput>
  requires IsUTFCharType<UTFCharTypeInput> && IsAnyOfTypes<UTFStorageTypeInput, std::basic_string<UTFCharTypeInput>, std::basic_string_view<UTFCharTypeInput>> && (std::same_as<UTFStorageType, std::basic_string<UTFCharType>> || std::same_as<UTFCharType, UTFCharTypeInput>)
  StringWrapper(const StringWrapper<UTFStorageTypeInput, UTFCharTypeInput, CaseSensitiveInput>& other)
    : StringWrapper{ other.native() }
  {}

  StringWrapper(const StringWrapper& other)
    : StringWrapper{ other.utfData }
  {}

  StringWrapper(StringWrapper&& other) noexcept
    : StringWrapper{ std::move(other.utfData) }
  {}

  template <typename UTFCharTypeInput>
  requires (IsUTF8CharType<UTFCharType> && IsUTF8CharType<UTFCharTypeInput>) || (IsUTF16CharType<UTFCharType> && IsUTF16CharType<UTFCharTypeInput>) || (IsUTF32CharType<UTFCharType> && IsUTF32CharType<UTFCharTypeInput>)
  StringWrapper& operator=(const std::basic_string_view<UTFCharTypeInput> other)
  {
    if (other.empty())
      utfData = {};
    else
      utfData = {reinterpret_cast<const UTFCharType *>(&other.front()), reinterpret_cast<const UTFCharType *>(&other.back()) + 1};

    return *this;
  }

  template <typename UTFCharTypeInput>
  requires IsUTF8CharType<UTFCharType> && IsUTF16CharType<UTFCharTypeInput> && std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  StringWrapper& operator=(const std::basic_string_view<UTFCharTypeInput> other)
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
  requires IsUTF8CharType<UTFCharType> && IsUTF32CharType<UTFCharTypeInput> && std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  StringWrapper& operator=(const std::basic_string_view<UTFCharTypeInput> other)
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
  requires IsUTF16CharType<UTFCharType> && IsUTF8CharType<UTFCharTypeInput> && std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  StringWrapper& operator=(const std::basic_string_view<UTFCharTypeInput> other)
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
  requires IsUTF16CharType<UTFCharType> && IsUTF32CharType<UTFCharTypeInput> && std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  StringWrapper& operator=(const std::basic_string_view<UTFCharTypeInput> other)
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
  requires IsUTF32CharType<UTFCharType> && IsUTF8CharType<UTFCharTypeInput> && std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  StringWrapper& operator=(const std::basic_string_view<UTFCharTypeInput> other)
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
  requires IsUTF32CharType<UTFCharType> && IsUTF16CharType<UTFCharTypeInput> && std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  StringWrapper& operator=(const std::basic_string_view<UTFCharTypeInput> other)
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
  requires IsUTFCharType<UTFCharTypeInput> && (std::same_as<UTFStorageType, std::basic_string<UTFCharType>> || std::same_as<UTFCharType, UTFCharTypeInput>)
  StringWrapper& operator=(const std::basic_string<UTFCharTypeInput> &other)
  {
    return *this = std::basic_string_view<UTFCharTypeInput>(other);
  }

  template <typename UTFCharTypeInput, size_t UTFInputSize>
  requires IsUTFCharType<UTFCharTypeInput> && (std::same_as<UTFStorageType, std::basic_string<UTFCharType>> || std::same_as<UTFCharType, UTFCharTypeInput>)
  StringWrapper& operator=(const UTFCharTypeInput (&other)[UTFInputSize])
  {
    return *this = std::basic_string_view<UTFCharTypeInput>(other, UTFInputSize - 1);
  }

  template<typename = std::enable_if_t<std::same_as<UTFStorageType, std::basic_string<UTFCharType>> || std::same_as<UTFCharType, wchar_t>>>
  StringWrapper& operator=(const std::filesystem::path& other)
  {
    return *this = other.native();
  }

  template<typename = std::enable_if_t<std::same_as<UTFStorageType, std::basic_string<wchar_t>>>>
  StringWrapper& operator=(std::filesystem::path &&other) noexcept
  {
    return *this = std::move(other.native());
  }

  template <typename UTFStorageTypeInput, typename UTFCharTypeInput, bool CaseSensitiveInput>
  requires IsUTFCharType<UTFCharTypeInput> && IsAnyOfTypes<UTFStorageTypeInput, std::basic_string<UTFCharTypeInput>, std::basic_string_view<UTFCharTypeInput>> && (std::same_as<UTFStorageType, std::basic_string<UTFCharType>> || std::same_as<UTFCharType, UTFCharTypeInput>)
  StringWrapper& operator=(const StringWrapper<UTFStorageTypeInput, UTFCharTypeInput, CaseSensitiveInput>& other)
  {
    return *this = other.native();
  }

  StringWrapper& operator=(const StringWrapper& other)
  {
    if (this == &other)
      return *this;

    utfData = other.utfData;
    return *this;
  }

  StringWrapper& operator=(StringWrapper&& other) noexcept
  {
    if (this == &other)
      return *this;

    utfData = std::move(other.utfData);
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
  requires IsUTF8CharType<UTFCharType> && IsUTF8CharType<UTFCharTypeInput>
  [[nodiscard]] auto operator<=>(const std::basic_string_view<UTFCharTypeInput> other) const
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
  requires IsUTF8CharType<UTFCharType> && IsUTF16CharType<UTFCharTypeInput>
  [[nodiscard]] auto operator<=>(const std::basic_string_view<UTFCharTypeInput> other) const
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
  requires IsUTF8CharType<UTFCharType> && IsUTF32CharType<UTFCharTypeInput>
  [[nodiscard]] auto operator<=>(const std::basic_string_view<UTFCharTypeInput> other) const
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
  requires IsUTF16CharType<UTFCharType> && IsUTF8CharType<UTFCharTypeInput>
  [[nodiscard]] auto operator<=>(const std::basic_string_view<UTFCharTypeInput> other) const
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
  requires IsUTF16CharType<UTFCharType> && IsUTF16CharType<UTFCharTypeInput>
  [[nodiscard]] auto operator<=>(const std::basic_string_view<UTFCharTypeInput> other) const
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
  requires IsUTF16CharType<UTFCharType> && IsUTF32CharType<UTFCharTypeInput>
  [[nodiscard]] auto operator<=>(const std::basic_string_view<UTFCharTypeInput> other) const
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
  requires IsUTF32CharType<UTFCharType> && IsUTF8CharType<UTFCharTypeInput>
  [[nodiscard]] auto operator<=>(const std::basic_string_view<UTFCharTypeInput> other) const
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
  requires IsUTF32CharType<UTFCharType> && IsUTF16CharType<UTFCharTypeInput>
  [[nodiscard]] auto operator<=>(const std::basic_string_view<UTFCharTypeInput> other) const
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
  requires IsUTF32CharType<UTFCharType> && IsUTF32CharType<UTFCharTypeInput>
  [[nodiscard]] auto operator<=>(const std::basic_string_view<UTFCharTypeInput> other) const
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
  requires IsUTFCharType<UTFCharTypeInput>
  [[nodiscard]] auto operator<=>(const std::basic_string<UTFCharTypeInput>& other) const
  {
    return *this <=> std::basic_string_view<UTFCharTypeInput>(other);
  }

  template <typename UTFCharTypeInput, size_t UTFInputSize>
  requires IsUTFCharType<UTFCharTypeInput>
  [[nodiscard]] auto operator<=>(const UTFCharTypeInput (&other)[UTFInputSize]) const
  {
    return *this <=> std::basic_string_view<UTFCharTypeInput>(other, UTFInputSize - 1);
  }

  [[nodiscard]] auto operator<=>(const std::filesystem::path& other) const
  {
    return *this <=> other.native();
  }

  template <typename UTFStorageTypeInput, typename UTFCharTypeInput>
  requires IsUTFCharType<UTFCharTypeInput> && IsAnyOfTypes<UTFStorageTypeInput, std::basic_string<UTFCharTypeInput>, std::basic_string_view<UTFCharTypeInput>>
  [[nodiscard]] auto operator<=>(const StringWrapper<UTFStorageTypeInput, UTFCharTypeInput, true>& other) const
  {
    return *this <=> other.native();
  }

  template <typename UTFStorageTypeInput, typename UTFCharTypeInput>
  requires IsUTFCharType<UTFCharTypeInput> && IsAnyOfTypes<UTFStorageTypeInput, std::basic_string<UTFCharTypeInput>, std::basic_string_view<UTFCharTypeInput>>
  [[nodiscard]] auto operator<=>(const StringWrapper<UTFStorageTypeInput, UTFCharTypeInput, false>& other) const
  {
    return other <=> native();
  }

  [[nodiscard]] auto operator<=>(const StringWrapper& other) const
  {
    return *this <=> other.utfData;
  }

  template <typename UTFCharTypeInput>
  requires IsUTFCharType<UTFCharTypeInput>
  [[nodiscard]] bool operator==(const std::basic_string_view<UTFCharTypeInput> other) const
  {
    return (*this <=> other) == std::weak_ordering::equivalent;
  }

  template <typename UTFCharTypeInput>
  requires IsUTFCharType<UTFCharTypeInput>
  [[nodiscard]] bool operator==(const std::basic_string<UTFCharTypeInput>& other) const
  {
    return (*this <=> other) == std::weak_ordering::equivalent;
  }

  template <typename UTFCharTypeInput, size_t UTFInputSize>
  requires IsUTFCharType<UTFCharTypeInput>
  [[nodiscard]] bool operator==(const UTFCharTypeInput (&other)[UTFInputSize]) const
  {
    return (*this <=> other) == std::weak_ordering::equivalent;
  }

  [[nodiscard]] bool operator==(const std::filesystem::path& other) const
  {
    return (*this <=> other.native()) == std::weak_ordering::equivalent;
  }

  template <typename UTFStorageTypeInput, typename UTFCharTypeInput, bool CaseSensitiveInput>
  requires IsUTFCharType<UTFCharTypeInput> && IsAnyOfTypes<UTFStorageTypeInput, std::basic_string<UTFCharTypeInput>, std::basic_string_view<UTFCharTypeInput>>
  [[nodiscard]] bool operator==(const StringWrapper<UTFStorageTypeInput, UTFCharTypeInput, CaseSensitiveInput>& other) const
  {
    return (*this <=> other.native()) == std::weak_ordering::equivalent;
  }

  [[nodiscard]] bool operator==(const StringWrapper& other) const
  {
    return (*this <=> other.utfData) == std::weak_ordering::equivalent;
  }

  template <typename UTFCharTypeInput>
  requires IsUTFCharType<UTFCharTypeInput>
  [[nodiscard]] bool operator!=(const std::basic_string_view<UTFCharTypeInput> other) const
  {
    return (*this <=> other) == std::weak_ordering::equivalent;
  }

  template <typename UTFCharTypeInput>
  requires IsUTFCharType<UTFCharTypeInput>
  [[nodiscard]] bool operator!=(const std::basic_string<UTFCharTypeInput>& other) const
  {
    return (*this <=> other) != std::weak_ordering::equivalent;
  }

  template <typename UTFCharTypeInput, size_t UTFInputSize>
  requires IsUTFCharType<UTFCharTypeInput>
  [[nodiscard]] bool operator!=(const UTFCharTypeInput (&other)[UTFInputSize]) const
  {
    return (*this <=> other) != std::weak_ordering::equivalent;
  }

  [[nodiscard]] bool operator!=(const std::filesystem::path& other) const
  {
    return (*this <=> other.native()) != std::weak_ordering::equivalent;
  }

  template <typename UTFStorageTypeInput, typename UTFCharTypeInput, bool CaseSensitiveInput>
  requires IsUTFCharType<UTFCharTypeInput> && IsAnyOfTypes<UTFStorageTypeInput, std::basic_string<UTFCharTypeInput>, std::basic_string_view<UTFCharTypeInput>>
  [[nodiscard]] bool operator!=(const StringWrapper<UTFStorageTypeInput, UTFCharTypeInput, CaseSensitiveInput>& other) const
  {
    return (*this <=> other.native()) != std::weak_ordering::equivalent;
  }

  [[nodiscard]] bool operator!=(const StringWrapper& other) const
  {
    return (*this <=> other.utfData) != std::weak_ordering::equivalent;
  }

  template <typename UTFCharTypeInput>
  requires IsUTFCharType<UTFCharTypeInput>
  [[nodiscard]] bool operator<=(const std::basic_string_view<UTFCharTypeInput> other) const
  {
    const auto cmp = *this <=> other;
    return cmp == std::weak_ordering::less || cmp == std::weak_ordering::equivalent;
  }

  template <typename UTFCharTypeInput>
  requires IsUTFCharType<UTFCharTypeInput>
  [[nodiscard]] bool operator<=(const std::basic_string<UTFCharTypeInput>& other) const
  {
    const auto cmp = *this <=> other;
    return cmp == std::weak_ordering::less || cmp == std::weak_ordering::equivalent;
  }

  template <typename UTFCharTypeInput, size_t UTFInputSize>
  requires IsUTFCharType<UTFCharTypeInput>
  [[nodiscard]] bool operator<=(const UTFCharTypeInput (&other)[UTFInputSize]) const
  {
    const auto cmp = *this <=> other;
    return cmp == std::weak_ordering::less || cmp == std::weak_ordering::equivalent;
  }

  [[nodiscard]] bool operator<=(const std::filesystem::path& other) const
  {
    const auto cmp = *this <=> other.native();
    return cmp == std::weak_ordering::less || cmp == std::weak_ordering::equivalent;
  }

  template <typename UTFStorageTypeInput, typename UTFCharTypeInput, bool CaseSensitiveInput>
  requires IsUTFCharType<UTFCharTypeInput> && IsAnyOfTypes<UTFStorageTypeInput, std::basic_string<UTFCharTypeInput>, std::basic_string_view<UTFCharTypeInput>>
  [[nodiscard]] bool operator<=(const StringWrapper<UTFStorageTypeInput, UTFCharTypeInput, CaseSensitiveInput>& other) const
  {
    const auto cmp = *this <=> other.native();
    return cmp == std::weak_ordering::less || cmp == std::weak_ordering::equivalent;
  }

  [[nodiscard]] bool operator<=(const StringWrapper& other) const
  {
    const auto cmp = *this <=> other.utfData;
    return cmp == std::weak_ordering::less || cmp == std::weak_ordering::equivalent;
  }

  template <typename UTFCharTypeInput>
  requires IsUTFCharType<UTFCharTypeInput>
  [[nodiscard]] bool operator>=(const std::basic_string_view<UTFCharTypeInput> other) const
  {
    const auto cmp = *this <=> other;
    return cmp == std::weak_ordering::greater || cmp == std::weak_ordering::equivalent;
  }

  template <typename UTFCharTypeInput>
  requires IsUTFCharType<UTFCharTypeInput>
  [[nodiscard]] bool operator>=(const std::basic_string<UTFCharTypeInput>& other) const
  {
    const auto cmp = *this <=> other;
    return cmp == std::weak_ordering::greater || cmp == std::weak_ordering::equivalent;
  }

  template <typename UTFCharTypeInput, size_t UTFInputSize>
  requires IsUTFCharType<UTFCharTypeInput>
  [[nodiscard]] bool operator>=(const UTFCharTypeInput (&other)[UTFInputSize]) const
  {
    const auto cmp = *this <=> other;
    return cmp == std::weak_ordering::greater || cmp == std::weak_ordering::equivalent;
  }

  [[nodiscard]] bool operator>=(const std::filesystem::path& other) const
  {
    const auto cmp = *this <=> other.native();
    return cmp == std::weak_ordering::greater || cmp == std::weak_ordering::equivalent;
  }

  template <typename UTFStorageTypeInput, typename UTFCharTypeInput, bool CaseSensitiveInput>
  requires IsUTFCharType<UTFCharTypeInput> && IsAnyOfTypes<UTFStorageTypeInput, std::basic_string<UTFCharTypeInput>, std::basic_string_view<UTFCharTypeInput>>
  [[nodiscard]] bool operator>=(const StringWrapper<UTFStorageTypeInput, UTFCharTypeInput, CaseSensitiveInput>& other) const
  {
    const auto cmp = *this <=> other.native();
    return cmp == std::weak_ordering::greater || cmp == std::weak_ordering::equivalent;
  }

  [[nodiscard]] bool operator>=(const StringWrapper& other) const
  {
    const auto cmp = *this <=> other.utfData;
    return cmp == std::weak_ordering::greater || cmp == std::weak_ordering::equivalent;
  }

  template <typename UTFCharTypeInput>
  requires IsUTFCharType<UTFCharTypeInput>
  [[nodiscard]] bool operator<(const std::basic_string_view<UTFCharTypeInput> other) const
  {
    return (*this <=> other) == std::weak_ordering::less;
  }

  template <typename UTFCharTypeInput>
  requires IsUTFCharType<UTFCharTypeInput>
  [[nodiscard]] bool operator<(const std::basic_string<UTFCharTypeInput>& other) const
  {
    return (*this <=> other) == std::weak_ordering::less;
  }

  template <typename UTFCharTypeInput, size_t UTFInputSize>
  requires IsUTFCharType<UTFCharTypeInput>
  [[nodiscard]] bool operator<(const UTFCharTypeInput (&other)[UTFInputSize]) const
  {
    return (*this <=> other) == std::weak_ordering::less;
  }

  [[nodiscard]] bool operator<(const std::filesystem::path& other) const
  {
    return (*this <=> other.native()) == std::weak_ordering::less;
  }

  template <typename UTFStorageTypeInput, typename UTFCharTypeInput, bool CaseSensitiveInput>
  requires IsUTFCharType<UTFCharTypeInput> && IsAnyOfTypes<UTFStorageTypeInput, std::basic_string<UTFCharTypeInput>, std::basic_string_view<UTFCharTypeInput>>
  [[nodiscard]] bool operator<(const StringWrapper<UTFStorageTypeInput, UTFCharTypeInput, CaseSensitiveInput>& other) const
  {
    return (*this <=> other.native()) == std::weak_ordering::less;
  }

  [[nodiscard]] bool operator<(const StringWrapper& other) const
  {
    return (*this <=> other.utfData) == std::weak_ordering::less;
  }

  template <typename UTFCharTypeInput>
  requires IsUTFCharType<UTFCharTypeInput>
  [[nodiscard]] bool operator>(const std::basic_string_view<UTFCharTypeInput> other) const
  {
    return (*this <=> other) == std::weak_ordering::greater;
  }

  template <typename UTFCharTypeInput>
  requires IsUTFCharType<UTFCharTypeInput>
  [[nodiscard]] bool operator>(const std::basic_string<UTFCharTypeInput>& other) const
  {
    return (*this <=> other) == std::weak_ordering::greater;
  }

  template <typename UTFCharTypeInput, size_t UTFInputSize>
  requires IsUTFCharType<UTFCharTypeInput>
  [[nodiscard]] bool operator>(const UTFCharTypeInput (&other)[UTFInputSize]) const
  {
    return (*this <=> other) == std::weak_ordering::greater;
  }

  [[nodiscard]] bool operator>(const std::filesystem::path& other) const
  {
    return (*this <=> other.native()) == std::weak_ordering::greater;
  }

  template <typename UTFStorageTypeInput, typename UTFCharTypeInput, bool CaseSensitiveInput>
  requires IsUTFCharType<UTFCharTypeInput> && IsAnyOfTypes<UTFStorageTypeInput, std::basic_string<UTFCharTypeInput>, std::basic_string_view<UTFCharTypeInput>>
  [[nodiscard]] bool operator>(const StringWrapper<UTFStorageTypeInput, UTFCharTypeInput, CaseSensitiveInput>& other) const
  {
    return (*this <=> other.native()) == std::weak_ordering::greater;
  }

  [[nodiscard]] bool operator>(const StringWrapper& other) const
  {
    return (*this <=> other.utfData) == std::weak_ordering::greater;
  }

  template <typename UTFCharTypeInput>
  requires IsUTFCharType<UTFCharTypeInput> && std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  StringWrapper& operator+=(const std::basic_string_view<UTFCharTypeInput> other)
  {
    return *this += StringWrapper<std::basic_string_view<UTFCharTypeInput>, UTFCharTypeInput, CaseSensitive>{other};
  }

  template <typename UTFCharTypeInput>
  requires IsUTFCharType<UTFCharTypeInput> && std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  StringWrapper& operator+=(const std::basic_string<UTFCharTypeInput>& other)
  {
    return *this += StringWrapper<std::basic_string_view<UTFCharTypeInput>, UTFCharTypeInput, CaseSensitive>{other};
  }

  template <typename UTFCharTypeInput, size_t UTFInputSize>
  requires IsUTFCharType<UTFCharTypeInput> && std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  StringWrapper& operator+=(const UTFCharTypeInput (&other)[UTFInputSize])
  {
    return *this += StringWrapper<std::basic_string_view<UTFCharTypeInput>, UTFCharTypeInput, CaseSensitive>{other};
  }

  template<typename = std::enable_if_t<std::same_as<UTFStorageType, std::basic_string<UTFCharType>>>>
  StringWrapper& operator+=(const std::filesystem::path& other)
  {
    return *this += StringWrapper<std::basic_string_view<wchar_t>, wchar_t, CaseSensitive>{other};
  }

  template <typename UTFStorageTypeInput, typename UTFCharTypeInput, bool CaseSensitiveInput>
  requires ((IsUTF8CharType<UTFCharType> && IsUTF8CharType<UTFCharTypeInput>) || (IsUTF16CharType<UTFCharType> && IsUTF16CharType<UTFCharTypeInput>) || (IsUTF32CharType<UTFCharType> && IsUTF32CharType<UTFCharTypeInput>)) && IsAnyOfTypes<UTFStorageTypeInput, std::basic_string<UTFCharTypeInput>, std::basic_string_view<UTFCharTypeInput>> && std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  StringWrapper& operator+=(const StringWrapper<UTFStorageTypeInput, UTFCharTypeInput, CaseSensitiveInput>& other)
  {
    if (!other.empty())
      utfData.append(reinterpret_cast<const UTFCharType *>(&other.front()), reinterpret_cast<const UTFCharType *>(&other.back()) + 1);

    return *this;
  }

  template <typename UTFStorageTypeInput, typename UTFCharTypeInput, bool CaseSensitiveInput>
  requires ((!IsUTF8CharType<UTFCharType> && IsUTF8CharType<UTFCharTypeInput>) || (!IsUTF16CharType<UTFCharType> && IsUTF16CharType<UTFCharTypeInput>) || (!IsUTF32CharType<UTFCharType> && IsUTF32CharType<UTFCharTypeInput>)) && IsAnyOfTypes<UTFStorageTypeInput, std::basic_string<UTFCharTypeInput>, std::basic_string_view<UTFCharTypeInput>> && std::same_as<UTFStorageType, std::basic_string<UTFCharType>>
  StringWrapper& operator+=(const StringWrapper<UTFStorageTypeInput, UTFCharTypeInput, CaseSensitiveInput>& other)
  {
    return *this += StringWrapper{other};
  }

  template<typename = std::enable_if_t<std::same_as<UTFStorageType, std::basic_string<UTFCharType>>>>
  StringWrapper& operator+=(const StringWrapper& other)
  {
    if (!other.empty())
      utfData.append(other.cbegin(), other.cend());

    return *this;
  }

  template <typename UTFCharTypeInput>
  requires IsUTFCharType<UTFCharTypeInput>
  [[nodiscard]] StringWrapper<std::basic_string<UTFCharType>, UTFCharType, CaseSensitive> operator+(const std::basic_string_view<UTFCharTypeInput> other)
  {
    return *this + StringWrapper<std::basic_string_view<UTFCharTypeInput>, UTFCharTypeInput, CaseSensitive>{other};
  }

  template <typename UTFCharTypeInput>
  requires IsUTFCharType<UTFCharTypeInput>
  [[nodiscard]] StringWrapper<std::basic_string<UTFCharType>, UTFCharType, CaseSensitive> operator+(const std::basic_string<UTFCharTypeInput>& other)
  {
    return *this + StringWrapper<std::basic_string_view<UTFCharTypeInput>, UTFCharTypeInput, CaseSensitive>{other};
  }

  template <typename UTFCharTypeInput, size_t UTFInputSize>
  requires IsUTFCharType<UTFCharTypeInput>
  [[nodiscard]] StringWrapper<std::basic_string<UTFCharType>, UTFCharType, CaseSensitive> operator+(const UTFCharTypeInput (&other)[UTFInputSize])
  {
    return *this + StringWrapper<std::basic_string_view<UTFCharTypeInput>, UTFCharTypeInput, CaseSensitive>{other};
  }

  [[nodiscard]] StringWrapper<std::basic_string<UTFCharType>, UTFCharType, CaseSensitive> operator+(const std::filesystem::path& other)
  {
    return *this + StringWrapper<std::basic_string_view<wchar_t>, wchar_t, CaseSensitive>{other};
  }

  template <typename UTFStorageTypeInput, typename UTFCharTypeInput, bool CaseSensitiveInput>
  requires IsUTFCharType<UTFCharTypeInput> && IsAnyOfTypes<UTFStorageTypeInput, std::basic_string<UTFCharTypeInput>, std::basic_string_view<UTFCharTypeInput>>
  [[nodiscard]] StringWrapper<std::basic_string<UTFCharType>, UTFCharType, CaseSensitive> operator+(const StringWrapper<UTFStorageTypeInput, UTFCharTypeInput, CaseSensitiveInput>& other)
  {
    StringWrapper<std::basic_string<UTFCharType>, UTFCharType, CaseSensitive> result{*this};
    return result += other;
  }

  [[nodiscard]] StringWrapper<std::basic_string<UTFCharType>, UTFCharType, CaseSensitive> operator+(const StringWrapper& other)
  {
    StringWrapper<std::basic_string<UTFCharType>, UTFCharType, CaseSensitive> result{*this};
    return result += other;
  }

  template<typename = std::enable_if_t<std::same_as<UTFStorageType, std::basic_string<UTFCharType>>>>
  [[nodiscard]] auto begin()
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

  template<typename = std::enable_if_t<std::same_as<UTFStorageType, std::basic_string<UTFCharType>>>>
  [[nodiscard]] auto end()
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

  template<typename = std::enable_if_t<std::same_as<UTFStorageType, std::basic_string<UTFCharType>>>>
  [[nodiscard]] UTFCharType* data()
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
      // TODO - think of a better way to make sure view is null-terminated, as this could crash on char[1] = {'a'} for example
      if (*(&back() + 1) == UTFCharType(0))
        return utfData.data();

      // TODO - dont throw but handle the case
      //        if we had string cache and all string were just referencing it,
      //        we could create new string in cache here and return it
      throw std::exception("Encounter not null-terminated string view!");
    }
  }

  template<typename = std::enable_if_t<std::same_as<UTFStorageType, std::basic_string<UTFCharType>>>>
  [[nodiscard]] UTFCharType& front()
  {
    return utfData.front();
  }

  [[nodiscard]] const UTFCharType& front() const
  {
    return utfData.front();
  }

  template<typename = std::enable_if_t<std::same_as<UTFStorageType, std::basic_string<UTFCharType>>>>
  [[nodiscard]] UTFCharType& back()
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
    if constexpr (std::same_as<UTFCharType, wchar_t>)
      return { utfData };
    else
      return StringWrapper<std::basic_string<wchar_t>, wchar_t, true>(utfData).path();
  }

  template<typename = std::enable_if_t<std::same_as<UTFStorageType, std::basic_string<UTFCharType>>>>
  void clear()
  {
    utfData.clear();
  }

  template<typename = std::enable_if_t<std::same_as<UTFStorageType, std::basic_string<UTFCharType>>>>
  void resize(const size_t newSize, const UTFCharType defaultChar = static_cast<UTFCharType>(0))
  {
    utfData.resize(newSize, defaultChar);
  }

private:
  UTFStorageType utfData;
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
            result = XXH3_64bits_withSeed(&glyph, sizeof glyph, result);
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
            result = XXH3_64bits_withSeed(&glyph, sizeof glyph, result);
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
            result = XXH3_64bits_withSeed(&glyph, sizeof glyph, result);
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
