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

    std::pair<uint32_t, uint32_t> glyphRange{CodepointInvalid, CodepointInvalid};
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

  void AddRange(std::pair<uint32_t, uint32_t> glyphRange)
  {
    for (auto glyph = glyphRange.first; glyph <= glyphRange.second; ++glyph)
      Add(glyph);
  }

  void Add(uint32_t glyph)
  {
    if (glyph == CodepointInvalid || glyph > CodepointMax)
      return;

    std::lock_guard lock(dataMutex);
    newGlyphsAdded |= !glyphsInUse.test(glyph);
    glyphsInUse.set(glyph);
  }

  template <typename UTFCharType>
  requires IsUTFCharType<UTFCharType>
  void AddText(const std::basic_string_view<UTFCharType> utf)
  {
    if constexpr (IsUTF8CharType<UTFCharType>)
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

    if constexpr (IsUTF16CharType<UTFCharType>)
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

    if constexpr (IsUTF32CharType<UTFCharType>)
    {
      for (const auto glyph : utf)
        Add(static_cast<uint32_t>(glyph));
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

  void AddText(const std::filesystem::path& path)
  {
    AddText(path.native());
  }

  template <typename UTFStorageType, typename UTFCharType, bool CaseSensitive>
  requires IsUTFCharType<UTFCharType> && IsAnyOfTypes<UTFStorageType, std::basic_string<UTFCharType>, std::basic_string_view<UTFCharType>>
  void AddText(const StringWrapper<UTFStorageType, UTFCharType, CaseSensitive>& utf)
  {
    AddText(utf.native());
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
