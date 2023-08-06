//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#pragma once

#include "Common.hpp"

#include "Strings-impl/StringViewWrapper-declarations.hpp"
#include "Strings-impl/StringWrapper-declarations.hpp"

#include "Strings-impl/StringViewWrapper-definitions.hpp"
#include "Strings-impl/StringWrapper-definitions.hpp"

// TODO:
//  - operator+
//  - clean the header even more (split into more files)

namespace UTF
{

template <typename UTFCharTypeLeft, typename UTFCharTypeRight, bool CaseSensitiveLeft = true, bool CaseSensitiveRight = true, typename UTFCharTypeTraitsLeft = std::char_traits<UTFCharTypeLeft>, typename UTFCharTypeTraitsRight = std::char_traits<UTFCharTypeRight>>
requires IsUTF8CharType<UTFCharTypeLeft> && IsUTF8CharType<UTFCharTypeRight>
[[nodiscard]] auto operator<=>(const StringViewWrapper<UTFCharTypeLeft, CaseSensitiveLeft, UTFCharTypeTraitsLeft> &left, const StringViewWrapper<UTFCharTypeRight, CaseSensitiveRight, UTFCharTypeTraitsRight> &right)
{
  auto *reinterpretedData = reinterpret_cast<const uint8_t *>(left.data());
  const auto reinterpretedDataSize = left.size();
  size_t reinterpretedDataOffset = 0;
  auto *reinterpretedInput = reinterpret_cast<const uint8_t *>(right.data());
  const auto reinterpretedInputSize = right.size();
  size_t reinterpretedInputOffset = 0;
  auto glyph = CodepointInvalid;
  auto glyphInput = CodepointInvalid;
  while (reinterpretedDataOffset < reinterpretedDataSize && reinterpretedInputOffset < reinterpretedInputSize)
  {
    U8_NEXT(reinterpretedData, reinterpretedDataOffset, reinterpretedDataSize, glyph);
    U8_NEXT(reinterpretedInput, reinterpretedInputOffset, reinterpretedInputSize, glyphInput);

    if constexpr (!CaseSensitiveLeft || !CaseSensitiveRight)
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

  if constexpr (!CaseSensitiveLeft || !CaseSensitiveRight)
  {
    glyph = u_tolower(glyph);
    glyphInput = u_tolower(glyphInput);
  }

  return glyph <=> glyphInput;
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight, bool CaseSensitiveLeft = true, bool CaseSensitiveRight = true, typename UTFCharTypeTraitsLeft = std::char_traits<UTFCharTypeLeft>, typename UTFCharTypeTraitsRight = std::char_traits<UTFCharTypeRight>>
requires IsUTF8CharType<UTFCharTypeLeft> && IsUTF16CharType<UTFCharTypeRight>
[[nodiscard]] auto operator<=>(const StringViewWrapper<UTFCharTypeLeft, CaseSensitiveLeft, UTFCharTypeTraitsLeft> &left, const StringViewWrapper<UTFCharTypeRight, CaseSensitiveRight, UTFCharTypeTraitsRight> &right)
{
  auto *reinterpretedData = reinterpret_cast<const uint8_t *>(left.data());
  const auto reinterpretedDataSize = left.size();
  size_t reinterpretedDataOffset = 0;
  auto *reinterpretedInput = reinterpret_cast<const UChar *>(right.data());
  const auto reinterpretedInputSize = right.size();
  size_t reinterpretedInputOffset = 0;
  auto glyph = CodepointInvalid;
  auto glyphInput = CodepointInvalid;
  while (reinterpretedDataOffset < reinterpretedDataSize && reinterpretedInputOffset < reinterpretedInputSize)
  {
    U8_NEXT(reinterpretedData, reinterpretedDataOffset, reinterpretedDataSize, glyph);
    U16_NEXT(reinterpretedInput, reinterpretedInputOffset, reinterpretedInputSize, glyphInput);

    if constexpr (!CaseSensitiveLeft || !CaseSensitiveRight)
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

  if constexpr (!CaseSensitiveLeft || !CaseSensitiveRight)
  {
    glyph = u_tolower(glyph);
    glyphInput = u_tolower(glyphInput);
  }

  return glyph <=> glyphInput;
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight, bool CaseSensitiveLeft = true, bool CaseSensitiveRight = true, typename UTFCharTypeTraitsLeft = std::char_traits<UTFCharTypeLeft>, typename UTFCharTypeTraitsRight = std::char_traits<UTFCharTypeRight>>
requires IsUTF8CharType<UTFCharTypeLeft> && IsUTF32CharType<UTFCharTypeRight>
[[nodiscard]] auto operator<=>(const StringViewWrapper<UTFCharTypeLeft, CaseSensitiveLeft, UTFCharTypeTraitsLeft> &left, const StringViewWrapper<UTFCharTypeRight, CaseSensitiveRight, UTFCharTypeTraitsRight> &right)
{
  auto *reinterpretedData = reinterpret_cast<const uint8_t *>(left.data());
  const auto reinterpretedDataSize = left.size();
  size_t reinterpretedDataOffset = 0;
  size_t inputOffset = 0;
  auto glyph = CodepointInvalid;
  auto glyphInput = CodepointInvalid;
  while (reinterpretedDataOffset < reinterpretedDataSize && inputOffset < right.size())
  {
    U8_NEXT(reinterpretedData, reinterpretedDataOffset, reinterpretedDataSize, glyph);
    glyphInput = static_cast<uint32_t>(right[inputOffset++]);

    if constexpr (!CaseSensitiveLeft || !CaseSensitiveRight)
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

  if (inputOffset != right.size())
    glyphInput = static_cast<uint32_t>(right[inputOffset]);
  else
    glyphInput =  CodepointInvalid;

  if constexpr (!CaseSensitiveLeft || !CaseSensitiveRight)
  {
    glyph = u_tolower(glyph);
    glyphInput = u_tolower(glyphInput);
  }

  return glyph <=> glyphInput;
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight, bool CaseSensitiveLeft = true, bool CaseSensitiveRight = true, typename UTFCharTypeTraitsLeft = std::char_traits<UTFCharTypeLeft>, typename UTFCharTypeTraitsRight = std::char_traits<UTFCharTypeRight>>
requires IsUTF16CharType<UTFCharTypeLeft> && IsUTF8CharType<UTFCharTypeRight>
[[nodiscard]] auto operator<=>(const StringViewWrapper<UTFCharTypeLeft, CaseSensitiveLeft, UTFCharTypeTraitsLeft> &left, const StringViewWrapper<UTFCharTypeRight, CaseSensitiveRight, UTFCharTypeTraitsRight> &right)
{
   const auto result = right <=> left;
   if (result == std::strong_ordering::equal)
     return result;

   return result == std::strong_ordering::less ? std::strong_ordering::greater : std::strong_ordering::less;
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight, bool CaseSensitiveLeft = true, bool CaseSensitiveRight = true, typename UTFCharTypeTraitsLeft = std::char_traits<UTFCharTypeLeft>, typename UTFCharTypeTraitsRight = std::char_traits<UTFCharTypeRight>>
requires IsUTF16CharType<UTFCharTypeLeft> && IsUTF16CharType<UTFCharTypeRight>
[[nodiscard]] auto operator<=>(const StringViewWrapper<UTFCharTypeLeft, CaseSensitiveLeft, UTFCharTypeTraitsLeft> &left, const StringViewWrapper<UTFCharTypeRight, CaseSensitiveRight, UTFCharTypeTraitsRight> &right)
{
  auto *reinterpretedData = reinterpret_cast<const UChar *>(left.data());
  const auto reinterpretedDataSize = left.size();
  size_t reinterpretedDataOffset = 0;
  auto *reinterpretedInput = reinterpret_cast<const UChar *>(right.data());
  const auto reinterpretedInputSize = right.size();
  size_t reinterpretedInputOffset = 0;
  auto glyph = CodepointInvalid;
  auto glyphInput = CodepointInvalid;
  while (reinterpretedDataOffset < reinterpretedDataSize && reinterpretedInputOffset < reinterpretedInputSize)
  {
    U16_NEXT(reinterpretedData, reinterpretedDataOffset, reinterpretedDataSize, glyph);
    U16_NEXT(reinterpretedInput, reinterpretedInputOffset, reinterpretedInputSize, glyphInput);

    if constexpr (!CaseSensitiveLeft || !CaseSensitiveRight)
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

  if constexpr (!CaseSensitiveLeft || !CaseSensitiveRight)
  {
    glyph = u_tolower(glyph);
    glyphInput = u_tolower(glyphInput);
  }

  return glyph <=> glyphInput;
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight, bool CaseSensitiveLeft = true, bool CaseSensitiveRight = true, typename UTFCharTypeTraitsLeft = std::char_traits<UTFCharTypeLeft>, typename UTFCharTypeTraitsRight = std::char_traits<UTFCharTypeRight>>
requires IsUTF16CharType<UTFCharTypeLeft> && IsUTF32CharType<UTFCharTypeRight>
[[nodiscard]] auto operator<=>(const StringViewWrapper<UTFCharTypeLeft, CaseSensitiveLeft, UTFCharTypeTraitsLeft> &left, const StringViewWrapper<UTFCharTypeRight, CaseSensitiveRight, UTFCharTypeTraitsRight> &right)
{
  auto *reinterpretedData = reinterpret_cast<const UChar *>(left.data());
  const auto reinterpretedDataSize = left.size();
  size_t reinterpretedDataOffset = 0;
  size_t inputOffset = 0;
  auto glyph = CodepointInvalid;
  auto glyphInput = CodepointInvalid;
  while (reinterpretedDataOffset < reinterpretedDataSize && inputOffset < right.size())
  {
    U16_NEXT(reinterpretedData, reinterpretedDataOffset, reinterpretedDataSize, glyph);
    glyphInput = static_cast<uint32_t>(right[inputOffset++]);

    if constexpr (!CaseSensitiveLeft || !CaseSensitiveRight)
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

  if (inputOffset != right.size())
    glyphInput = static_cast<uint32_t>(right[inputOffset]);
  else
    glyphInput =  CodepointInvalid;

  if constexpr (!CaseSensitiveLeft || !CaseSensitiveRight)
  {
    glyph = u_tolower(glyph);
    glyphInput = u_tolower(glyphInput);
  }

  return glyph <=> glyphInput;
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight, bool CaseSensitiveLeft = true, bool CaseSensitiveRight = true, typename UTFCharTypeTraitsLeft = std::char_traits<UTFCharTypeLeft>, typename UTFCharTypeTraitsRight = std::char_traits<UTFCharTypeRight>>
requires IsUTF32CharType<UTFCharTypeLeft> && IsUTF8CharType<UTFCharTypeRight>
[[nodiscard]] auto operator<=>(const StringViewWrapper<UTFCharTypeLeft, CaseSensitiveLeft, UTFCharTypeTraitsLeft> &left, const StringViewWrapper<UTFCharTypeRight, CaseSensitiveRight, UTFCharTypeTraitsRight> &right)
{
   const auto result = right <=> left;
   if (result == std::strong_ordering::equal)
     return result;

   return result == std::strong_ordering::less ? std::strong_ordering::greater : std::strong_ordering::less;
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight, bool CaseSensitiveLeft = true, bool CaseSensitiveRight = true, typename UTFCharTypeTraitsLeft = std::char_traits<UTFCharTypeLeft>, typename UTFCharTypeTraitsRight = std::char_traits<UTFCharTypeRight>>
requires IsUTF32CharType<UTFCharTypeLeft> && IsUTF16CharType<UTFCharTypeRight>
[[nodiscard]] auto operator<=>(const StringViewWrapper<UTFCharTypeLeft, CaseSensitiveLeft, UTFCharTypeTraitsLeft> &left, const StringViewWrapper<UTFCharTypeRight, CaseSensitiveRight, UTFCharTypeTraitsRight> &right)
{
   const auto result = right <=> left;
   if (result == std::strong_ordering::equal)
     return result;

   return result == std::strong_ordering::less ? std::strong_ordering::greater : std::strong_ordering::less;
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight, bool CaseSensitiveLeft = true, bool CaseSensitiveRight = true, typename UTFCharTypeTraitsLeft = std::char_traits<UTFCharTypeLeft>, typename UTFCharTypeTraitsRight = std::char_traits<UTFCharTypeRight>>
requires IsUTF32CharType<UTFCharTypeLeft> && IsUTF32CharType<UTFCharTypeRight>
[[nodiscard]] auto operator<=>(const StringViewWrapper<UTFCharTypeLeft, CaseSensitiveLeft, UTFCharTypeTraitsLeft> &left, const StringViewWrapper<UTFCharTypeRight, CaseSensitiveRight, UTFCharTypeTraitsRight> &right)
{
  size_t inputOffset = 0;
  size_t dataOffset = 0;
  auto glyph = CodepointInvalid;
  auto glyphInput = CodepointInvalid;
  while (dataOffset < left.size() && inputOffset < right.size())
  {
    glyph = static_cast<uint32_t>(left[dataOffset++]);
    glyphInput = static_cast<uint32_t>(right[inputOffset++]);

    if constexpr (!CaseSensitiveLeft || !CaseSensitiveRight)
    {
      glyph = u_tolower(glyph);
      glyphInput = u_tolower(glyphInput);
    }

    if (glyphInput != glyph)
      return glyph <=> glyphInput;
  }

  if (dataOffset != left.size())
    glyph = static_cast<uint32_t>(left[dataOffset]);
  else
    glyph = CodepointInvalid;

  if (inputOffset != right.size())
    glyphInput = static_cast<uint32_t>(right[inputOffset]);
  else
    glyphInput = CodepointInvalid;

  if constexpr (!CaseSensitiveLeft || !CaseSensitiveRight)
  {
    glyph = u_tolower(glyph);
    glyphInput = u_tolower(glyphInput);
  }

  return glyph <=> glyphInput;
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight, bool CaseSensitiveLeft = true, bool CaseSensitiveRight = true, typename UTFCharTypeTraitsLeft = std::char_traits<UTFCharTypeLeft>, typename UTFCharTypeTraitsRight = std::char_traits<UTFCharTypeRight>, typename UTFAllocatorLeft = std::allocator<UTFCharTypeLeft>, typename UTFAllocatorRight = std::allocator<UTFCharTypeRight>>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
[[nodiscard]] auto operator<=>(const StringWrapper<UTFCharTypeLeft, CaseSensitiveLeft, UTFCharTypeTraitsLeft, UTFAllocatorLeft> &left, const StringWrapper<UTFCharTypeRight, CaseSensitiveRight, UTFCharTypeTraitsRight, UTFAllocatorRight> &right)
{
  return StringViewWrapper<UTFCharTypeLeft, CaseSensitiveLeft, UTFCharTypeTraitsLeft>{ left } <=> StringViewWrapper<UTFCharTypeRight, CaseSensitiveRight, UTFCharTypeTraitsRight>{ right };
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight, bool CaseSensitiveLeft = true, bool CaseSensitiveRight = true, typename UTFCharTypeTraitsLeft = std::char_traits<UTFCharTypeLeft>, typename UTFCharTypeTraitsRight = std::char_traits<UTFCharTypeRight>, typename UTFAllocatorLeft = std::allocator<UTFCharTypeLeft>>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
[[nodiscard]] auto operator<=>(const StringWrapper<UTFCharTypeLeft, CaseSensitiveLeft, UTFCharTypeTraitsLeft, UTFAllocatorLeft> &left, const StringViewWrapper<UTFCharTypeRight, CaseSensitiveRight, UTFCharTypeTraitsRight> &right)
{
  return StringViewWrapper<UTFCharTypeLeft, CaseSensitiveLeft, UTFCharTypeTraitsLeft>{ left } <=> right;
}

template <typename UTFCharTypeLeft, typename UTFCharTypeRight, bool CaseSensitiveLeft = true, bool CaseSensitiveRight = true, typename UTFCharTypeTraitsLeft = std::char_traits<UTFCharTypeLeft>, typename UTFCharTypeTraitsRight = std::char_traits<UTFCharTypeRight>, typename UTFAllocatorRight = std::allocator<UTFCharTypeRight>>
requires IsUTFCharType<UTFCharTypeLeft> && IsUTFCharType<UTFCharTypeRight>
[[nodiscard]] auto operator<=>(const StringViewWrapper<UTFCharTypeLeft, CaseSensitiveLeft, UTFCharTypeTraitsLeft> &left, const StringWrapper<UTFCharTypeRight, CaseSensitiveRight, UTFCharTypeTraitsRight, UTFAllocatorRight> &right)
{
  return left <=> StringViewWrapper<UTFCharTypeRight, CaseSensitiveRight, UTFCharTypeTraitsRight>{ right };
}

template <typename TypeLeft, typename UTFCharTypeRight, bool CaseSensitiveRight = true, typename UTFCharTypeTraitsRight = std::char_traits<UTFCharTypeRight>>
requires StringViewConstructible<TypeLeft> && IsUTFCharType<UTFCharTypeRight>
[[nodiscard]] auto operator<=>(const TypeLeft &left, const StringViewWrapper<UTFCharTypeRight, CaseSensitiveRight, UTFCharTypeTraitsRight> &right)
{
  if constexpr (StringView8Constructible<TypeLeft>)
    return StringView8{ left } <=> right;
  else if constexpr (StringView16Constructible<TypeLeft>)
    return StringView16{ left } <=> right;
  else if constexpr (StringView32Constructible<TypeLeft>)
    return StringView32{ left } <=> right;
}

template <typename UTFCharTypeLeft, typename TypeRight, bool CaseSensitiveLeft = true, typename UTFCharTypeTraitsLeft = std::char_traits<UTFCharTypeLeft>>
requires IsUTFCharType<UTFCharTypeLeft> && StringViewConstructible<TypeRight>
[[nodiscard]] auto operator<=>(const StringViewWrapper<UTFCharTypeLeft, CaseSensitiveLeft, UTFCharTypeTraitsLeft> &left, const TypeRight &right)
{
  if constexpr (StringView8Constructible<TypeRight>)
    return left <=> StringView8{ right };
  else if constexpr (StringView16Constructible<TypeRight>)
    return left <=> StringView16{ right };
  else if constexpr (StringView32Constructible<TypeRight>)
    return left <=> StringView32{ right };
}

template <typename TypeLeft, typename UTFCharTypeRight, bool CaseSensitiveRight = true, typename UTFCharTypeTraitsRight = std::char_traits<UTFCharTypeRight>, typename UTFAllocatorRight = std::allocator<UTFCharTypeRight>>
requires StringViewConstructible<TypeLeft> && IsUTFCharType<UTFCharTypeRight>
[[nodiscard]] auto operator<=>(const TypeLeft &left, const StringWrapper<UTFCharTypeRight, CaseSensitiveRight, UTFCharTypeTraitsRight, UTFAllocatorRight> &right)
{
  return left <=> StringViewWrapper<UTFCharTypeRight, CaseSensitiveRight, UTFCharTypeTraitsRight>{ right };
}

template <typename UTFCharTypeLeft, typename TypeRight, bool CaseSensitiveLeft = true, typename UTFCharTypeTraitsLeft = std::char_traits<UTFCharTypeLeft>, typename UTFAllocatorLeft = std::allocator<UTFCharTypeLeft>>
requires IsUTFCharType<UTFCharTypeLeft> && StringViewConstructible<TypeRight>
[[nodiscard]] auto operator<=>(const StringWrapper<UTFCharTypeLeft, CaseSensitiveLeft, UTFCharTypeTraitsLeft, UTFAllocatorLeft> &left, const TypeRight &right)
{
  return StringViewWrapper<UTFCharTypeLeft, CaseSensitiveLeft, UTFCharTypeTraitsLeft>{ left } <=> right;
}

template <typename TypeLeft, typename TypeRight>
requires StringViewConstructible<TypeLeft> && StringViewConstructible<TypeRight>
[[nodiscard]] auto operator<=>(const TypeLeft &left, const TypeRight &right)
{
  if constexpr (StringView8Constructible<TypeLeft>)
  {
    if constexpr (StringView8Constructible<TypeRight>)
      return StringView8{ left } <=> StringView8{ right };
    else if constexpr (StringView16Constructible<TypeRight>)
      return StringView8{ left } <=> StringView16{ right };
    else if constexpr (StringView32Constructible<TypeRight>)
      return StringView8{ left } <=> StringView32{ right };
  }
  else if constexpr (StringView16Constructible<TypeLeft>)
  {
    if constexpr (StringView8Constructible<TypeRight>)
      return StringView16{ left } <=> StringView8{ right };
    else if constexpr (StringView16Constructible<TypeRight>)
      return StringView16{ left } <=> StringView16{ right };
    else if constexpr (StringView32Constructible<TypeRight>)
      return StringView16{ left } <=> StringView32{ right };
  }
  else if constexpr (StringView32Constructible<TypeLeft>)
  {
    if constexpr (StringView8Constructible<TypeRight>)
      return StringView32{ left } <=> StringView8{ right };
    else if constexpr (StringView16Constructible<TypeRight>)
      return StringView32{ left } <=> StringView16{ right };
    else if constexpr (StringView32Constructible<TypeRight>)
      return StringView32{ left } <=> StringView32{ right };
  }
}

template <typename TypeLeft, typename TypeRight>
requires StringViewConstructible<TypeLeft> && StringViewConstructible<TypeRight>
[[nodiscard]] bool operator==(const TypeLeft &left, const TypeRight &right)
{
  return (left <=> right) == std::strong_ordering::equal;
}

template <bool CaseSensitive>
struct StringViewHasher
{
  template <typename UTFCharTypeInput, bool CaseSensitiveInput = true, typename UTFCharTypeTraitsInput = std::char_traits<UTFCharTypeInput>>
  requires IsUTFCharType<UTFCharTypeInput>
  [[nodiscard]] size_t operator()(const StringViewWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput> &utf) const noexcept
  {
    if constexpr (CaseSensitive)
    {
      if constexpr (sizeof(size_t) == sizeof(uint64_t))
        return XXH3_64bits(utf.data(), utf.size() * sizeof(UTFCharTypeInput));

      if constexpr (sizeof(size_t) == sizeof(uint32_t))
        return XXH32(utf.data(), utf.size() * sizeof(UTFCharTypeInput), 0);
    }
    else
    {
      size_t result = 0;

      if constexpr (IsUTF8CharType<UTFCharTypeInput>)
      {
        const auto *reinterpretedInput = reinterpret_cast<const uint8_t *>(utf.data());
        const auto reinterpretedInputSize = utf.size();
        size_t reinterpretedInputOffset = 0;
        auto glyph = 0ul;
        while (reinterpretedInputOffset < reinterpretedInputSize)
        {
          U8_NEXT(reinterpretedInput, reinterpretedInputOffset, reinterpretedInputSize, glyph);
          glyph = u_tolower(glyph);

          if constexpr (sizeof(size_t) == sizeof(uint64_t))
            result = XXH3_64bits_withSeed(&glyph, sizeof(glyph), result);
          else if constexpr (sizeof(size_t) == sizeof(uint32_t))
            result = XXH32(utf.data(), utf.size() * sizeof(UTFCharTypeInput), result);
        }
      }

      if constexpr (IsUTF16CharType<UTFCharTypeInput>)
      {
        const auto *reinterpretedInput = reinterpret_cast<const UChar *>(utf.data());
        const auto reinterpretedInputSize = utf.size();
        size_t reinterpretedInputOffset = 0;
        auto glyph = 0ul;
        while (reinterpretedInputOffset < reinterpretedInputSize)
        {
          U16_NEXT(reinterpretedInput, reinterpretedInputOffset, reinterpretedInputSize, glyph);
          glyph = u_tolower(glyph);

          if constexpr (sizeof(size_t) == sizeof(uint64_t))
            result = XXH3_64bits_withSeed(&glyph, sizeof(glyph), result);
          else if constexpr (sizeof(size_t) == sizeof(uint32_t))
            result = XXH32(utf.data(), utf.size() * sizeof(UTFCharTypeInput), result);
        }
      }

      if constexpr (IsUTF32CharType<UTFCharTypeInput>)
      {
        for (const auto utfChar : utf)
        {
          const auto glyph = u_tolower(utfChar);

          if constexpr (sizeof(size_t) == sizeof(uint64_t))
            result = XXH3_64bits_withSeed(&glyph, sizeof(glyph), result);
          else if constexpr (sizeof(size_t) == sizeof(uint32_t))
            result = XXH32(utf.data(), utf.size() * sizeof(UTFCharTypeInput), result);
        }
      }

      if constexpr (sizeof(size_t) == sizeof(uint64_t))
      {
        for (const auto utfChar : utf)
        {
          result = XXH3_64bits_withSeed(&utfChar, 1 * sizeof(UTFCharTypeInput), result);
        }
      }

      return result;
    }
  }
};

template <bool CaseSensitive>
using StringHasher = StringViewHasher<CaseSensitive>;

}

namespace std
{

template <typename UTFCharTypeInput, bool CaseSensitiveInput, typename UTFCharTypeTraitsInput, typename UTFAllocatorInput>
requires UTF::IsUTFCharType<UTFCharTypeInput>
struct hash<UTF::StringWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput, UTFAllocatorInput>>
{
  [[nodiscard]] size_t operator()(const UTF::StringWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput, UTFAllocatorInput>& utf) const noexcept
  {
    return UTF::StringHasher<CaseSensitiveInput>{}(utf);
  }
};

template <typename UTFCharTypeInput, bool CaseSensitiveInput, typename UTFCharTypeTraitsInput>
requires UTF::IsUTFCharType<UTFCharTypeInput>
struct hash<UTF::StringViewWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput>>
{
  [[nodiscard]] size_t operator()(const UTF::StringViewWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput>& utf) const noexcept
  {
    return UTF::StringViewHasher<CaseSensitiveInput>{}(utf);
  }
};

}

#if FBL_VENDOR_USE_STD_FORMAT
namespace std
{

template <typename UTFCharTypeOutput, typename UTFCharTypeInput, bool CaseSensitiveInput, typename UTFCharTypeTraitsInput, typename UTFAllocatorInput>
requires UTF::IsUTFCharType<UTFCharTypeInput>
struct formatter<UTF::StringWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput, UTFAllocatorInput>, UTFCharTypeOutput>
{
  template <typename FormatParseContext>
  [[nodiscard]] auto parse(FormatParseContext& parseContext) const
  {
    return parseContext.end();
  }

  template<class FormatContext>
  [[nodiscard]] auto format(const UTF::StringWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput, UTFAllocatorInput>& utf, FormatContext& context) const
  {
    if constexpr (std::same_as<UTFCharTypeInput, UTFCharTypeOutput>)
      return std::formatter<std::basic_string_view<UTFCharTypeInput, UTFCharTypeTraitsInput>, UTFCharTypeOutput>{}.format(utf.native(), context);
    else if constexpr (UTF::IsSameUTFCharType<UTFCharTypeInput, UTFCharTypeOutput>)
      return std::formatter<UTF::StringViewWrapper<UTFCharTypeOutput>, UTFCharTypeOutput>{}.format(utf.native(), context);
    else
      return std::formatter<UTF::StringWrapper<UTFCharTypeOutput, CaseSensitiveInput, UTFCharTypeTraitsInput, UTFAllocatorInput>, UTFCharTypeOutput>{}.format(utf.native(), context);
  }
};

template <typename UTFCharTypeOutput, typename UTFCharTypeInput, bool CaseSensitiveInput, typename UTFCharTypeTraitsInput>
requires UTF::IsUTFCharType<UTFCharTypeInput>
struct formatter<UTF::StringViewWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput>, UTFCharTypeOutput>
{
  template <typename FormatParseContext>
  [[nodiscard]] auto parse(FormatParseContext& parseContext) const
  {
    return parseContext.end();
  }

  template<class FormatContext>
  [[nodiscard]] auto format(const UTF::StringViewWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput>& utf, FormatContext& context) const
  {
    if constexpr (std::same_as<UTFCharTypeInput, UTFCharTypeOutput>)
      return std::formatter<std::basic_string_view<UTFCharTypeInput, UTFCharTypeTraitsInput>, UTFCharTypeOutput>{}.format(utf.native(), context);
    else if constexpr (UTF::IsSameUTFCharType<UTFCharTypeInput, UTFCharTypeOutput>)
      return std::formatter<UTF::StringViewWrapper<UTFCharTypeOutput>, UTFCharTypeOutput>{}.format(utf.native(), context);
    else
      return std::formatter<UTF::StringWrapper<UTFCharTypeOutput, CaseSensitiveInput, UTFCharTypeTraitsInput>, UTFCharTypeOutput>{}.format(utf.native(), context);
  }
};

}
#endif

#if FBL_VENDOR_USE_FMT
namespace fmt
{

template <typename UTFCharTypeOutput, typename UTFCharTypeInput, bool CaseSensitiveInput, typename UTFCharTypeTraitsInput, typename UTFAllocatorInput>
requires UTF::IsUTFCharType<UTFCharTypeInput>
struct formatter<UTF::StringWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput, UTFAllocatorInput>, UTFCharTypeOutput>
{
  template <typename FormatParseContext>
  [[nodiscard]] auto parse(FormatParseContext& parseContext) const
  {
    return parseContext.end();
  }

  template<class FormatContext>
  [[nodiscard]] auto format(const UTF::StringWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput, UTFAllocatorInput>& utf, FormatContext& context) const
  {
    if constexpr (std::same_as<UTFCharTypeInput, UTFCharTypeOutput>)
      return fmt::formatter<std::basic_string_view<UTFCharTypeInput, UTFCharTypeTraitsInput>, UTFCharTypeOutput>{}.format(utf.native(), context);
    else if constexpr (UTF::IsSameUTFCharType<UTFCharTypeInput, UTFCharTypeOutput>)
      return fmt::formatter<UTF::StringViewWrapper<UTFCharTypeOutput>, UTFCharTypeOutput>{}.format(utf.native(), context);
    else
      return fmt::formatter<UTF::StringWrapper<UTFCharTypeOutput, CaseSensitiveInput, UTFCharTypeTraitsInput, UTFAllocatorInput>, UTFCharTypeOutput>{}.format(utf.native(), context);
  }
};

template <typename UTFCharTypeOutput, typename UTFCharTypeInput, bool CaseSensitiveInput, typename UTFCharTypeTraitsInput>
requires UTF::IsUTFCharType<UTFCharTypeInput>
struct formatter<UTF::StringViewWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput>, UTFCharTypeOutput>
{
  template <typename FormatParseContext>
  [[nodiscard]] auto parse(FormatParseContext& parseContext) const
  {
    return parseContext.end();
  }

  template<class FormatContext>
  [[nodiscard]] auto format(const UTF::StringViewWrapper<UTFCharTypeInput, CaseSensitiveInput, UTFCharTypeTraitsInput>& utf, FormatContext& context) const
  {
    if constexpr (std::same_as<UTFCharTypeInput, UTFCharTypeOutput>)
      return fmt::formatter<std::basic_string_view<UTFCharTypeInput, UTFCharTypeTraitsInput>, UTFCharTypeOutput>{}.format(utf.native(), context);
    else if constexpr (UTF::IsSameUTFCharType<UTFCharTypeInput, UTFCharTypeOutput>)
      return fmt::formatter<UTF::StringViewWrapper<UTFCharTypeOutput>, UTFCharTypeOutput>{}.format(utf.native(), context);
    else
      return fmt::formatter<UTF::StringWrapper<UTFCharTypeOutput, CaseSensitiveInput, UTFCharTypeTraitsInput>, UTFCharTypeOutput>{}.format(utf.native(), context);
  }
};

}
#endif

#if FBL_VENDOR_USE_STD_FORMAT || FBL_VENDOR_USE_FMT
namespace UTF
{

	template <StringViewConstructible Type, typename... FormatArgs>
  String8 Format(const Type &key, FormatArgs &&...args)
  {
    return Format(String8(key), std::forward<FormatArgs>(args)...);
  }

	template <StringViewConstructible Type, typename... FormatArgs>
  String8 &FormatTo(String8 &buffer, const Type &key, FormatArgs &&...args)
  {
    return FormatTo(buffer, String8(key), std::forward<FormatArgs>(args)...);
  }

	template <StringView8Constructible Type, typename... FormatArgs>
  String8 Format(const Type &key, FormatArgs &&...args)
  {
    try
    {
#if FBL_VENDOR_USE_FMT
      return String8(fmt::vformat(StringView8(key).native(), fmt::make_format_args(std::forward<FormatArgs>(args)...)));
#elif FBL_VENDOR_USE_STD_FORMAT
      return String8(std::vformat(StringView8(key).native(), std::make_format_args(std::forward<FormatArgs>(args)...)));
#endif
    }
    //catch (std::format_error &)
    //{
    //  return buffer = key;
    //}
    catch (...)
    {
      return key;
    }
  }

	template <StringView8Constructible Type, typename... FormatArgs>
  String8 &FormatTo(String8 &buffer, const Type &key, FormatArgs &&...args)
  {
    buffer.clear();

    try
    {
#if FBL_VENDOR_USE_FMT
      fmt::vformat_to(std::back_inserter(buffer.native()), StringView8(key).native(),
                      fmt::make_format_args(std::forward<FormatArgs>(args)...));
#elif FBL_VENDOR_USE_STD_FORMAT
      std::vformat_to(std::back_inserter(buffer.native()), StringView8(key).native(),
                      std::make_format_args(std::forward<FormatArgs>(args)...));
#endif
    }
    //catch (std::format_error &)
    //{
    //  return buffer = key;
    //}
    catch (...)
    {
      return buffer = key;
    }

    return buffer;
  }

}

namespace UTF
{

}
#endif
