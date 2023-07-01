//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#pragma once

namespace UTF
{

class GlyphRangesBuilder : public Singleton<GlyphRangesBuilder>
{
public:

  GlyphRangesBuilder()
  {
    Initialize();
  }

  void Clear()
  {
    glyphsInUse.reset();

    Initialize();
  }

  bool NeedsBuild() const
  {
    return newGlyphsAdded;
  }

  std::vector<std::pair<uint32_t, uint32_t>> Build()
  {
    std::vector<std::pair<uint32_t, uint32_t>> glyphRanges;

    std::lock_guard lock(dataMutex);

    std::pair glyphRange{CodepointInvalid, CodepointInvalid};
    for (uint32_t glyph = 1; glyph <= CodepointMax; ++glyph)
    {
      if (glyphsInUse.test(glyph))
      {
        if (glyphRange.first == CodepointInvalid)
          glyphRange.first = glyph;

        glyphRange.second = glyph;
      }
      else
      {
        if (glyphRange.first != CodepointInvalid)
        {
          glyphRanges.emplace_back(glyphRange);
          glyphRange = {CodepointInvalid, CodepointInvalid};
        }
      }
    }

    if (glyphRange.first != CodepointInvalid)
      glyphRanges.emplace_back(glyphRange);

    newGlyphsAdded = false;
    return glyphRanges;
  }

  void AddRange(const std::pair<uint32_t, uint32_t> glyphRange)
  {
    for (auto glyph = glyphRange.first; glyph <= glyphRange.second; ++glyph)
      Add(glyph);
  }

  void Add(const uint32_t glyph)
  {
    if (glyph == CodepointInvalid || glyph > CodepointMax)
      return;

    std::lock_guard lock(dataMutex);
    newGlyphsAdded |= !glyphsInUse.test(glyph);
    glyphsInUse.set(glyph);
  }

  template <typename UTFCharType, bool CaseSensitive = true, typename UTFCharTraits = std::char_traits<UTFCharType>>
  requires IsUTF8CharType<UTFCharType>
  void AddText(const StringViewWrapper<UTFCharType, CaseSensitive, UTFCharTraits> &utf)
  {
    const auto *utfData = utf.data();
    const auto utfSize = utf.size();
    for (size_t utfOffset = 0; utfOffset < utfSize;)
    {
      uint32_t glyph = CodepointInvalid;
      U8_NEXT(utfData, utfOffset, utfSize, glyph);

      Add(glyph);
    }
  }

  template <typename UTFCharType, bool CaseSensitive = true, typename UTFCharTraits = std::char_traits<UTFCharType>>
  requires IsUTF16CharType<UTFCharType>
  void AddText(const StringViewWrapper<UTFCharType, CaseSensitive, UTFCharTraits> &utf)
  {
    const auto *utfData = utf.data();
    const auto utfSize = utf.size();
    for (size_t utfOffset = 0; utfOffset < utfSize;)
    {
      uint32_t glyph = CodepointInvalid;
      U16_NEXT(utfData, utfOffset, utfSize, glyph);

      Add(glyph);
    }
  }

  template <typename UTFCharType, bool CaseSensitive = true, typename UTFCharTraits = std::char_traits<UTFCharType>>
  requires IsUTF32CharType<UTFCharType>
  void AddText(const StringViewWrapper<UTFCharType, CaseSensitive, UTFCharTraits> &utf)
  {
    for (const auto glyph : utf.native())
      Add(static_cast<uint32_t>(glyph));
  }

  template <typename ...Args>
  requires StringViewConstructible<Args...>
  void AddText(const Args&... args)
  {
    if constexpr (StringView8Constructible<Args...>)
      return AddText(StringView8(args...));
    else if constexpr (StringView16Constructible<Args...>)
      return AddText(StringView16(args...));
    else if constexpr (StringView32Constructible<Args...>)
      return AddText(StringView32(args...));
  }

private:
  void Initialize()
  {
    // Basic Latin (ASCII) + Latin-1 Supplement
    AddRange({0x0020, 0x00FF});

    // Invalid Unicode Character
    Add(0xFFFD);
  }

  std::bitset<CodepointMax + 1> glyphsInUse;
  bool newGlyphsAdded = false;

  mutable std::mutex dataMutex;
};

}
