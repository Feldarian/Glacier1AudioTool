//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#include "Precompiled.hpp"

#ifdef HAT_BUILD_TESTS

namespace
{

template <typename TestType, bool ConstructAll>
void TestStringImpl()
{
  const auto testEmpty = [](const auto& test)
  {
    assert(test.empty());
    assert(test.size() == 0);
    assert(test == "");
    assert(test == L"");
    assert(test == U"");
    assert(test == u"");
    assert(test == u8"");
  };

  const auto testConstruction = [](const auto& test)
  {
    assert(!test.empty());
    assert(test.size() == 4);
    assert(test == U"test");
  };

  const auto testComparators = [](const auto& test)
  {
    assert(!test.empty());
    assert(test.size() == 4);
    assert(test != "");
    assert(test != L"");
    assert(test != U"");
    assert(test != u"");
    assert(test != u8"");
    assert(test != std::basic_string_view<char>(""));
    assert(test != std::basic_string_view<wchar_t>(L""));
    assert(test != std::basic_string_view<char32_t>(U""));
    assert(test != std::basic_string_view<char16_t>(u""));
    assert(test != std::basic_string_view<char8_t>(u8""));
    assert(test != std::basic_string<char>(""));
    assert(test != std::basic_string<wchar_t>(L""));
    assert(test != std::basic_string<char32_t>(U""));
    assert(test != std::basic_string<char16_t>(u""));
    assert(test != std::basic_string<char8_t>(u8""));
    assert(test != std::filesystem::path(L""));
    assert(test != String8(""));
    assert(test != String8(L""));
    assert(test != String8(U""));
    assert(test != String8(u""));
    assert(test != String8(u8""));
    assert(test != String8CI(""));
    assert(test != String8CI(L""));
    assert(test != String8CI(U""));
    assert(test != String8CI(u""));
    assert(test != String8CI(u8""));
    assert(test != String16(""));
    assert(test != String16(L""));
    assert(test != String16(U""));
    assert(test != String16(u""));
    assert(test != String16(u8""));
    assert(test != String16CI(""));
    assert(test != String16CI(L""));
    assert(test != String16CI(U""));
    assert(test != String16CI(u""));
    assert(test != String16CI(u8""));
    assert(test != String32(""));
    assert(test != String32(L""));
    assert(test != String32(U""));
    assert(test != String32(u""));
    assert(test != String32(u8""));
    assert(test != String32CI(""));
    assert(test != String32CI(L""));
    assert(test != String32CI(U""));
    assert(test != String32CI(u""));
    assert(test != String32CI(u8""));
    assert(test != StringW(""));
    assert(test != StringW(L""));
    assert(test != StringW(U""));
    assert(test != StringW(u""));
    assert(test != StringW(u8""));
    assert(test != StringWCI(""));
    assert(test != StringWCI(L""));
    assert(test != StringWCI(U""));
    assert(test != StringWCI(u""));
    assert(test != StringWCI(u8""));
    assert(test != StringView8(""));
    assert(test != StringView8(u8""));
    assert(test != StringView8CI(""));
    assert(test != StringView8CI(u8""));
    assert(test != StringView16(L""));
    assert(test != StringView16(u""));
    assert(test != StringView16CI(L""));
    assert(test != StringView16CI(u""));
    assert(test != StringView32(U""));
    assert(test != StringView32CI(U""));
    assert(test != StringViewW(L""));
    assert(test != StringViewW(u""));
    assert(test != StringViewWCI(L""));
    assert(test != StringViewWCI(u""));
    assert(test == "test");
    assert(test == L"test");
    assert(test == U"test");
    assert(test == u"test");
    assert(test == u8"test");
    assert(test == std::basic_string_view<char>("test"));
    assert(test == std::basic_string_view<wchar_t>(L"test"));
    assert(test == std::basic_string_view<char32_t>(U"test"));
    assert(test == std::basic_string_view<char16_t>(u"test"));
    assert(test == std::basic_string_view<char8_t>(u8"test"));
    assert(test == std::basic_string<char>("test"));
    assert(test == std::basic_string<wchar_t>(L"test"));
    assert(test == std::basic_string<char32_t>(U"test"));
    assert(test == std::basic_string<char16_t>(u"test"));
    assert(test == std::basic_string<char8_t>(u8"test"));
    assert(test == std::filesystem::path(L"test"));
    assert(test == String8("test"));
    assert(test == String8(L"test"));
    assert(test == String8(U"test"));
    assert(test == String8(u"test"));
    assert(test == String8(u8"test"));
    assert(test == String8CI("TEST"));
    assert(test == String8CI(L"TEST"));
    assert(test == String8CI(U"TEST"));
    assert(test == String8CI(u"TEST"));
    assert(test == String8CI(u8"TEST"));
    assert(test == String16("test"));
    assert(test == String16(L"test"));
    assert(test == String16(U"test"));
    assert(test == String16(u"test"));
    assert(test == String16(u8"test"));
    assert(test == String16CI("TEST"));
    assert(test == String16CI(L"TEST"));
    assert(test == String16CI(U"TEST"));
    assert(test == String16CI(u"TEST"));
    assert(test == String16CI(u8"TEST"));
    assert(test == String32("test"));
    assert(test == String32(L"test"));
    assert(test == String32(U"test"));
    assert(test == String32(u"test"));
    assert(test == String32(u8"test"));
    assert(test == String32CI("TEST"));
    assert(test == String32CI(L"TEST"));
    assert(test == String32CI(U"TEST"));
    assert(test == String32CI(u"TEST"));
    assert(test == String32CI(u8"TEST"));
    assert(test == StringW("test"));
    assert(test == StringW(L"test"));
    assert(test == StringW(U"test"));
    assert(test == StringW(u"test"));
    assert(test == StringW(u8"test"));
    assert(test == StringWCI("TEST"));
    assert(test == StringWCI(L"TEST"));
    assert(test == StringWCI(U"TEST"));
    assert(test == StringWCI(u"TEST"));
    assert(test == StringWCI(u8"TEST"));
    assert(test == StringView8("test"));
    assert(test == StringView8(u8"test"));
    assert(test == StringView8CI("TEST"));
    assert(test == StringView8CI(u8"TEST"));
    assert(test == StringView16(L"test"));
    assert(test == StringView16(u"test"));
    assert(test == StringView16CI(L"TEST"));
    assert(test == StringView16CI(u"TEST"));
    assert(test == StringView32(U"test"));
    assert(test == StringView32CI(U"TEST"));
    assert(test == StringViewW(L"test"));
    assert(test == StringViewW(u"test"));
    assert(test == StringViewWCI(L"TEST"));
    assert(test == StringViewWCI(u"TEST"));
    assert(test != "nope");
    assert(test != L"nope");
    assert(test != U"nope");
    assert(test != u"nope");
    assert(test != u8"nope");
    assert(test != std::basic_string_view<char>("nope"));
    assert(test != std::basic_string_view<wchar_t>(L"nope"));
    assert(test != std::basic_string_view<char32_t>(U"nope"));
    assert(test != std::basic_string_view<char16_t>(u"nope"));
    assert(test != std::basic_string_view<char8_t>(u8"nope"));
    assert(test != std::basic_string<char>("nope"));
    assert(test != std::basic_string<wchar_t>(L"nope"));
    assert(test != std::basic_string<char32_t>(U"nope"));
    assert(test != std::basic_string<char16_t>(u"nope"));
    assert(test != std::basic_string<char8_t>(u8"nope"));
    assert(test != std::filesystem::path(L"nope"));
    assert(test != String8("nope"));
    assert(test != String8(L"nope"));
    assert(test != String8(U"nope"));
    assert(test != String8(u"nope"));
    assert(test != String8(u8"nope"));
    assert(test != String8CI("NOPE"));
    assert(test != String8CI(L"NOPE"));
    assert(test != String8CI(U"NOPE"));
    assert(test != String8CI(u"NOPE"));
    assert(test != String8CI(u8"NOPE"));
    assert(test != String16("nope"));
    assert(test != String16(L"nope"));
    assert(test != String16(U"nope"));
    assert(test != String16(u"nope"));
    assert(test != String16(u8"nope"));
    assert(test != String16CI("NOPE"));
    assert(test != String16CI(L"NOPE"));
    assert(test != String16CI(U"NOPE"));
    assert(test != String16CI(u"NOPE"));
    assert(test != String16CI(u8"NOPE"));
    assert(test != String32("nope"));
    assert(test != String32(L"nope"));
    assert(test != String32(U"nope"));
    assert(test != String32(u"nope"));
    assert(test != String32(u8"nope"));
    assert(test != String32CI("NOPE"));
    assert(test != String32CI(L"NOPE"));
    assert(test != String32CI(U"NOPE"));
    assert(test != String32CI(u"NOPE"));
    assert(test != String32CI(u8"NOPE"));
    assert(test != StringW("nope"));
    assert(test != StringW(L"nope"));
    assert(test != StringW(U"nope"));
    assert(test != StringW(u"nope"));
    assert(test != StringW(u8"nope"));
    assert(test != StringWCI("NOPE"));
    assert(test != StringWCI(L"NOPE"));
    assert(test != StringWCI(U"NOPE"));
    assert(test != StringWCI(u"NOPE"));
    assert(test != StringWCI(u8"NOPE"));
    assert(test != StringView8("nope"));
    assert(test != StringView8(u8"nope"));
    assert(test != StringView8CI("NOPE"));
    assert(test != StringView8CI(u8"NOPE"));
    assert(test != StringView16(L"nope"));
    assert(test != StringView16(u"nope"));
    assert(test != StringView16CI(L"NOPE"));
    assert(test != StringView16CI(u"NOPE"));
    assert(test != StringView32(U"nope"));
    assert(test != StringView32CI(U"NOPE"));
    assert(test != StringViewW(L"nope"));
    assert(test != StringViewW(u"nope"));
    assert(test != StringViewWCI(L"NOPE"));
    assert(test != StringViewWCI(u"NOPE"));
    assert(test >= "nope");
    assert(test >= L"nope");
    assert(test >= U"nope");
    assert(test >= u"nope");
    assert(test >= u8"nope");
    assert(test >= std::basic_string_view<char>("nope"));
    assert(test >= std::basic_string_view<wchar_t>(L"nope"));
    assert(test >= std::basic_string_view<char32_t>(U"nope"));
    assert(test >= std::basic_string_view<char16_t>(u"nope"));
    assert(test >= std::basic_string_view<char8_t>(u8"nope"));
    assert(test >= std::basic_string<char>("nope"));
    assert(test >= std::basic_string<wchar_t>(L"nope"));
    assert(test >= std::basic_string<char32_t>(U"nope"));
    assert(test >= std::basic_string<char16_t>(u"nope"));
    assert(test >= std::basic_string<char8_t>(u8"nope"));
    assert(test >= std::filesystem::path(L"nope"));
    assert(test >= String8("nope"));
    assert(test >= String8(L"nope"));
    assert(test >= String8(U"nope"));
    assert(test >= String8(u"nope"));
    assert(test >= String8(u8"nope"));
    assert(test >= String8CI("NOPE"));
    assert(test >= String8CI(L"NOPE"));
    assert(test >= String8CI(U"NOPE"));
    assert(test >= String8CI(u"NOPE"));
    assert(test >= String8CI(u8"NOPE"));
    assert(test >= String16("nope"));
    assert(test >= String16(L"nope"));
    assert(test >= String16(U"nope"));
    assert(test >= String16(u"nope"));
    assert(test >= String16(u8"nope"));
    assert(test >= String16CI("NOPE"));
    assert(test >= String16CI(L"NOPE"));
    assert(test >= String16CI(U"NOPE"));
    assert(test >= String16CI(u"NOPE"));
    assert(test >= String16CI(u8"NOPE"));
    assert(test >= String32("nope"));
    assert(test >= String32(L"nope"));
    assert(test >= String32(U"nope"));
    assert(test >= String32(u"nope"));
    assert(test >= String32(u8"nope"));
    assert(test >= String32CI("NOPE"));
    assert(test >= String32CI(L"NOPE"));
    assert(test >= String32CI(U"NOPE"));
    assert(test >= String32CI(u"NOPE"));
    assert(test >= String32CI(u8"NOPE"));
    assert(test >= StringW("nope"));
    assert(test >= StringW(L"nope"));
    assert(test >= StringW(U"nope"));
    assert(test >= StringW(u"nope"));
    assert(test >= StringW(u8"nope"));
    assert(test >= StringWCI("NOPE"));
    assert(test >= StringWCI(L"NOPE"));
    assert(test >= StringWCI(U"NOPE"));
    assert(test >= StringWCI(u"NOPE"));
    assert(test >= StringWCI(u8"NOPE"));
    assert(test >= StringView8("nope"));
    assert(test >= StringView8(u8"nope"));
    assert(test >= StringView8CI("NOPE"));
    assert(test >= StringView8CI(u8"NOPE"));
    assert(test >= StringView16(L"nope"));
    assert(test >= StringView16(u"nope"));
    assert(test >= StringView16CI(L"NOPE"));
    assert(test >= StringView16CI(u"NOPE"));
    assert(test >= StringView32(U"nope"));
    assert(test >= StringView32CI(U"NOPE"));
    assert(test >= StringViewW(L"nope"));
    assert(test >= StringViewW(u"nope"));
    assert(test >= StringViewWCI(L"NOPE"));
    assert(test >= StringViewWCI(u"NOPE"));
    assert(test > "nope");
    assert(test > L"nope");
    assert(test > U"nope");
    assert(test > u"nope");
    assert(test > u8"nope");
    assert(test > std::basic_string_view<char>("nope"));
    assert(test > std::basic_string_view<wchar_t>(L"nope"));
    assert(test > std::basic_string_view<char32_t>(U"nope"));
    assert(test > std::basic_string_view<char16_t>(u"nope"));
    assert(test > std::basic_string_view<char8_t>(u8"nope"));
    assert(test > std::basic_string<char>("nope"));
    assert(test > std::basic_string<wchar_t>(L"nope"));
    assert(test > std::basic_string<char32_t>(U"nope"));
    assert(test > std::basic_string<char16_t>(u"nope"));
    assert(test > std::basic_string<char8_t>(u8"nope"));
    assert(test > std::filesystem::path(L"nope"));
    assert(test > String8("nope"));
    assert(test > String8(L"nope"));
    assert(test > String8(U"nope"));
    assert(test > String8(u"nope"));
    assert(test > String8(u8"nope"));
    assert(test > String8CI("NOPE"));
    assert(test > String8CI(L"NOPE"));
    assert(test > String8CI(U"NOPE"));
    assert(test > String8CI(u"NOPE"));
    assert(test > String8CI(u8"NOPE"));
    assert(test > String16("nope"));
    assert(test > String16(L"nope"));
    assert(test > String16(U"nope"));
    assert(test > String16(u"nope"));
    assert(test > String16(u8"nope"));
    assert(test > String16CI("NOPE"));
    assert(test > String16CI(L"NOPE"));
    assert(test > String16CI(U"NOPE"));
    assert(test > String16CI(u"NOPE"));
    assert(test > String16CI(u8"NOPE"));
    assert(test > String32("nope"));
    assert(test > String32(L"nope"));
    assert(test > String32(U"nope"));
    assert(test > String32(u"nope"));
    assert(test > String32(u8"nope"));
    assert(test > String32CI("NOPE"));
    assert(test > String32CI(L"NOPE"));
    assert(test > String32CI(U"NOPE"));
    assert(test > String32CI(u"NOPE"));
    assert(test > String32CI(u8"NOPE"));
    assert(test > StringW("nope"));
    assert(test > StringW(L"nope"));
    assert(test > StringW(U"nope"));
    assert(test > StringW(u"nope"));
    assert(test > StringW(u8"nope"));
    assert(test > StringWCI("NOPE"));
    assert(test > StringWCI(L"NOPE"));
    assert(test > StringWCI(U"NOPE"));
    assert(test > StringWCI(u"NOPE"));
    assert(test > StringWCI(u8"NOPE"));
    assert(test > StringView8("nope"));
    assert(test > StringView8(u8"nope"));
    assert(test > StringView8CI("NOPE"));
    assert(test > StringView8CI(u8"NOPE"));
    assert(test > StringView16(L"nope"));
    assert(test > StringView16(u"nope"));
    assert(test > StringView16CI(L"NOPE"));
    assert(test > StringView16CI(u"NOPE"));
    assert(test > StringView32(U"nope"));
    assert(test > StringView32CI(U"NOPE"));
    assert(test > StringViewW(L"nope"));
    assert(test > StringViewW(u"nope"));
    assert(test > StringViewWCI(L"NOPE"));
    assert(test > StringViewWCI(u"NOPE"));
    assert(test <= "user");
    assert(test <= L"user");
    assert(test <= U"user");
    assert(test <= u"user");
    assert(test <= u8"user");
    assert(test <= std::basic_string_view<char>("user"));
    assert(test <= std::basic_string_view<wchar_t>(L"user"));
    assert(test <= std::basic_string_view<char32_t>(U"user"));
    assert(test <= std::basic_string_view<char16_t>(u"user"));
    assert(test <= std::basic_string_view<char8_t>(u8"user"));
    assert(test <= std::basic_string<char>("user"));
    assert(test <= std::basic_string<wchar_t>(L"user"));
    assert(test <= std::basic_string<char32_t>(U"user"));
    assert(test <= std::basic_string<char16_t>(u"user"));
    assert(test <= std::basic_string<char8_t>(u8"user"));
    assert(test <= std::filesystem::path(L"user"));
    assert(test <= String8("user"));
    assert(test <= String8(L"user"));
    assert(test <= String8(U"user"));
    assert(test <= String8(u"user"));
    assert(test <= String8(u8"user"));
    assert(test <= String8CI("USER"));
    assert(test <= String8CI(L"USER"));
    assert(test <= String8CI(U"USER"));
    assert(test <= String8CI(u"USER"));
    assert(test <= String8CI(u8"USER"));
    assert(test <= String16("user"));
    assert(test <= String16(L"user"));
    assert(test <= String16(U"user"));
    assert(test <= String16(u"user"));
    assert(test <= String16(u8"user"));
    assert(test <= String16CI("USER"));
    assert(test <= String16CI(L"USER"));
    assert(test <= String16CI(U"USER"));
    assert(test <= String16CI(u"USER"));
    assert(test <= String16CI(u8"USER"));
    assert(test <= String32("user"));
    assert(test <= String32(L"user"));
    assert(test <= String32(U"user"));
    assert(test <= String32(u"user"));
    assert(test <= String32(u8"user"));
    assert(test <= String32CI("USER"));
    assert(test <= String32CI(L"USER"));
    assert(test <= String32CI(U"USER"));
    assert(test <= String32CI(u"USER"));
    assert(test <= String32CI(u8"USER"));
    assert(test <= StringW("user"));
    assert(test <= StringW(L"user"));
    assert(test <= StringW(U"user"));
    assert(test <= StringW(u"user"));
    assert(test <= StringW(u8"user"));
    assert(test <= StringWCI("USER"));
    assert(test <= StringWCI(L"USER"));
    assert(test <= StringWCI(U"USER"));
    assert(test <= StringWCI(u"USER"));
    assert(test <= StringWCI(u8"USER"));
    assert(test <= StringView8("user"));
    assert(test <= StringView8(u8"user"));
    assert(test <= StringView8CI("USER"));
    assert(test <= StringView8CI(u8"USER"));
    assert(test <= StringView16(L"user"));
    assert(test <= StringView16(u"user"));
    assert(test <= StringView16CI(L"USER"));
    assert(test <= StringView16CI(u"USER"));
    assert(test <= StringView32(U"user"));
    assert(test <= StringView32CI(U"USER"));
    assert(test <= StringViewW(L"user"));
    assert(test <= StringViewW(u"user"));
    assert(test <= StringViewWCI(L"USER"));
    assert(test <= StringViewWCI(u"USER"));
    assert(test < "user");
    assert(test < L"user");
    assert(test < U"user");
    assert(test < u"user");
    assert(test < u8"user");
    assert(test < std::basic_string_view<char>("user"));
    assert(test < std::basic_string_view<wchar_t>(L"user"));
    assert(test < std::basic_string_view<char32_t>(U"user"));
    assert(test < std::basic_string_view<char16_t>(u"user"));
    assert(test < std::basic_string_view<char8_t>(u8"user"));
    assert(test < std::basic_string<char>("user"));
    assert(test < std::basic_string<wchar_t>(L"user"));
    assert(test < std::basic_string<char32_t>(U"user"));
    assert(test < std::basic_string<char16_t>(u"user"));
    assert(test < std::basic_string<char8_t>(u8"user"));
    assert(test < std::filesystem::path(L"user"));
    assert(test < String8("user"));
    assert(test < String8(L"user"));
    assert(test < String8(U"user"));
    assert(test < String8(u"user"));
    assert(test < String8(u8"user"));
    assert(test < String8CI("USER"));
    assert(test < String8CI(L"USER"));
    assert(test < String8CI(U"USER"));
    assert(test < String8CI(u"USER"));
    assert(test < String8CI(u8"USER"));
    assert(test < String16("user"));
    assert(test < String16(L"user"));
    assert(test < String16(U"user"));
    assert(test < String16(u"user"));
    assert(test < String16(u8"user"));
    assert(test < String16CI("USER"));
    assert(test < String16CI(L"USER"));
    assert(test < String16CI(U"USER"));
    assert(test < String16CI(u"USER"));
    assert(test < String16CI(u8"USER"));
    assert(test < String32("user"));
    assert(test < String32(L"user"));
    assert(test < String32(U"user"));
    assert(test < String32(u"user"));
    assert(test < String32(u8"user"));
    assert(test < String32CI("USER"));
    assert(test < String32CI(L"USER"));
    assert(test < String32CI(U"USER"));
    assert(test < String32CI(u"USER"));
    assert(test < String32CI(u8"USER"));
    assert(test < StringW("user"));
    assert(test < StringW(L"user"));
    assert(test < StringW(U"user"));
    assert(test < StringW(u"user"));
    assert(test < StringW(u8"user"));
    assert(test < StringWCI("USER"));
    assert(test < StringWCI(L"USER"));
    assert(test < StringWCI(U"USER"));
    assert(test < StringWCI(u"USER"));
    assert(test < StringWCI(u8"USER"));
    assert(test < StringView8("user"));
    assert(test < StringView8(u8"user"));
    assert(test < StringView8CI("USER"));
    assert(test < StringView8CI(u8"USER"));
    assert(test < StringView16(L"user"));
    assert(test < StringView16(u"user"));
    assert(test < StringView16CI(L"USER"));
    assert(test < StringView16CI(u"USER"));
    assert(test < StringView32(U"user"));
    assert(test < StringView32CI(U"USER"));
    assert(test < StringViewW(L"user"));
    assert(test < StringViewW(u"user"));
    assert(test < StringViewWCI(L"USER"));
    assert(test < StringViewWCI(u"USER"));
  };

  {
    TestType test;
    testEmpty(test);
  }

  if constexpr (ConstructAll || StringView32Constructible<TestType>)
  {
    {
      TestType test(U"test");
      testConstruction(test);

      {
        test = U"";
        testEmpty(test);
        test = U"test";
        testConstruction(test);
      }

      {
        test = std::u32string_view{};
        testEmpty(test);
        test = std::u32string_view{ U"test" };
        testConstruction(test);
      }

      {
        std::u32string testStringEmpty;
        test = testStringEmpty;
        testEmpty(test);
        std::u32string testString{ U"test" };
        test = testString;
        testConstruction(test);
      }

      {
        test = StringView32{};
        testEmpty(test);
        test = StringView32{ U"test" };
        testConstruction(test);
      }

      {
        test = StringView32CI{};
        testEmpty(test);
        test = StringView32CI{ U"test" };
        testConstruction(test);
      }

      {
        String32 testStringEmpty;
        test = testStringEmpty;
        testEmpty(test);
        String32 testString{ U"test" };
        test = testString;
        testConstruction(test);
      }

      {
        String32CI testStringEmpty;
        test = testStringEmpty;
        testEmpty(test);
        String32CI testString{ U"test" };
        test = testString;
        testConstruction(test);
      }

      {
        String32 testAppend("appendTest:");
        testAppend += StringView8(u8"UTF8:");
        testAppend += StringView16(u"UTF16:");
        testAppend += StringView32(U"UTF32;");
        assert(!testAppend.empty());
        assert(testAppend.size() == 28);
        assert(testAppend == U"appendTest:UTF8:UTF16:UTF32;");
      }

      {
        String32CI testAppend("appendTest:");
        testAppend += StringView8(u8"UTF8:");
        testAppend += StringView16(u"UTF16:");
        testAppend += StringView32(U"UTF32;");
        assert(!testAppend.empty());
        assert(testAppend.size() == 28);
        assert(testAppend == U"appendTest:UTF8:UTF16:UTF32;");
      }
    }

    {
      TestType test(U"test", 4);
      testConstruction(test);
    }

    {
      TestType test(std::u32string_view(U"test"));
      testConstruction(test);
    }

    {
      std::u32string testString{ U"test" };
      TestType test(testString);
      testConstruction(test);
    }

    {
      TestType test(StringView32(U"test"));
      testConstruction(test);
      testComparators(test);
    }

    {
      TestType test(StringView32CI(U"test"));
      testConstruction(test);
    }

    {
      String32 testString{ U"test" };
      TestType test(testString);
      testConstruction(test);
    }

    {
      String32CI testString{ U"test" };
      TestType test(testString);
      testConstruction(test);
    }
  }

  if constexpr (ConstructAll || StringView16Constructible<TestType>)
  {
    {
      TestType test(u"test");
      testConstruction(test);

      {
        test = u"";
        testEmpty(test);
        test = u"test";
        testConstruction(test);
      }

      {
        test = std::u16string_view{};
        testEmpty(test);
        test = std::u16string_view{ u"test" };
        testConstruction(test);
      }

      {
        std::u16string testStringEmpty;
        test = testStringEmpty;
        testEmpty(test);
        std::u16string testString{ u"test" };
        test = testString;
        testConstruction(test);
      }

      {
        test = StringView16{};
        testEmpty(test);
        test = StringView16{ u"test" };
        testConstruction(test);
      }

      {
        test = StringView16CI{};
        testEmpty(test);
        test = StringView16CI{ u"test" };
        testConstruction(test);
      }

      {
        String16 testStringEmpty;
        test = testStringEmpty;
        testEmpty(test);
        String16 testString{ u"test" };
        test = testString;
        testConstruction(test);
      }

      {
        String16CI testStringEmpty;
        test = testStringEmpty;
        testEmpty(test);
        String16CI testString{ u"test" };
        test = testString;
        testConstruction(test);
      }

      {
        std::filesystem::path testStringEmpty;
        test = testStringEmpty;
        testEmpty(test);
        std::filesystem::path testString{ L"test" };
        test = testString;
        testConstruction(test);
      }

      {
        String16 testAppend("appendTest:");
        testAppend += StringView8(u8"UTF8:");
        testAppend += StringView16(u"UTF16:");
        testAppend += StringView32(U"UTF32;");
        assert(!testAppend.empty());
        assert(testAppend.size() == 28);
        assert(testAppend == U"appendTest:UTF8:UTF16:UTF32;");
      }

      {
        String16CI testAppend("appendTest:");
        testAppend += StringView8(u8"UTF8:");
        testAppend += StringView16(u"UTF16:");
        testAppend += StringView32(U"UTF32;");
        assert(!testAppend.empty());
        assert(testAppend.size() == 28);
        assert(testAppend == U"appendTest:UTF8:UTF16:UTF32;");
      }
    }

    {
      TestType test(u"test", 4);
      testConstruction(test);
    }

    {
      TestType test(std::u16string_view(u"test"));
      testConstruction(test);
    }

    {
      std::u16string testString{ u"test" };
      TestType test(testString);
      testConstruction(test);
    }

    {
      TestType test(StringView16(u"test"));
      testConstruction(test);
      testComparators(test);
    }

    {
      TestType test(StringView16CI(u"test"));
      testConstruction(test);
    }

    {
      String16 testString{ u"test" };
      TestType test(testString);
      testConstruction(test);
    }

    {
      String16CI testString{ u"test" };
      TestType test(testString);
      testConstruction(test);
    }

    {
      TestType test(L"test");
      testConstruction(test);
    }

    {
      TestType test(L"test", 4);
      testConstruction(test);
    }

    {
      TestType test(std::wstring_view(L"test"));
      testConstruction(test);
    }

    {
      std::wstring testString{ L"test" };
      TestType test(testString);
      testConstruction(test);
    }

    {
      TestType test(StringViewW(L"test"));
      testConstruction(test);
    }

    {
      TestType test(StringViewWCI(L"test"));
      testConstruction(test);
    }

    {
      StringW testString{ L"test" };
      TestType test(testString);
      testConstruction(test);
    }

    {
      StringWCI testString{ L"test" };
      TestType test(testString);
      testConstruction(test);
    }

    {
      std::filesystem::path testString{ L"test" };
      TestType test(testString);
      testConstruction(test);
    }
  }

  if constexpr (ConstructAll || StringView8Constructible<TestType>)
  {
    {
      TestType test(u8"test");
      testConstruction(test);

      {
        test = u8"";
        testEmpty(test);
        test = u8"test";
        testConstruction(test);
      }

      {
        test = std::u8string_view{};
        testEmpty(test);
        test = std::u8string_view{ u8"test" };
        testConstruction(test);
      }

      {
        std::u8string testStringEmpty;
        test = testStringEmpty;
        testEmpty(test);
        std::u8string testString{ u8"test" };
        test = testString;
        testConstruction(test);
      }

      {
        test = StringView8{};
        testEmpty(test);
        test = StringView8{ u8"test" };
        testConstruction(test);
      }

      {
        test = StringView8CI{};
        testEmpty(test);
        test = StringView8CI{ u8"test" };
        testConstruction(test);
      }

      {
        String8 testStringEmpty;
        test = testStringEmpty;
        testEmpty(test);
        String8 testString{ u8"test" };
        test = testString;
        testConstruction(test);
      }

      {
        String8CI testStringEmpty;
        test = testStringEmpty;
        testEmpty(test);
        String8CI testString{ u8"test" };
        test = testString;
        testConstruction(test);
      }

      {
        String8 testAppend("appendTest:");
        testAppend += StringView8(u8"UTF8:");
        testAppend += StringView16(u"UTF16:");
        testAppend += StringView32(U"UTF32;");
        assert(!testAppend.empty());
        assert(testAppend.size() == 28);
        assert(testAppend == U"appendTest:UTF8:UTF16:UTF32;");
      }

      {
        String8CI testAppend("appendTest:");
        testAppend += StringView8(u8"UTF8:");
        testAppend += StringView16(u"UTF16:");
        testAppend += StringView32(U"UTF32;");
        assert(!testAppend.empty());
        assert(testAppend.size() == 28);
        assert(testAppend == U"appendTest:UTF8:UTF16:UTF32;");
      }
    }

    {
      TestType test(u8"test", 4);
      testConstruction(test);
    }

    {
      TestType test(std::u8string_view(u8"test"));
      testConstruction(test);
    }

    {
      std::u8string testString{ u8"test" };
      TestType test(testString);
      testConstruction(test);
    }

    {
      TestType test(StringView8(u8"test"));
      testConstruction(test);
      testComparators(test);
    }

    {
      TestType test(StringView8CI(u8"test"));
      testConstruction(test);
    }

    {
      String8 testString{ u8"test" };
      TestType test(testString);
      testConstruction(test);
    }

    {
      String8CI testString{ u8"test" };
      TestType test(testString);
      testConstruction(test);
    }

    {
      TestType test("test");
      testConstruction(test);
    }

    {
      TestType test("test", 4);
      testConstruction(test);
    }

    {
      TestType test(std::string_view("test"));
      testConstruction(test);
    }

    {
      std::string testString{ "test" };
      TestType test(testString);
      testConstruction(test);
    }
  }
}

void TestStringImpls()
{
  TestStringImpl<String8, true>();
  TestStringImpl<String8CI, true>();
  TestStringImpl<String16, true>();
  TestStringImpl<String16CI, true>();
  TestStringImpl<String32, true>();
  TestStringImpl<String32CI, true>();
  TestStringImpl<StringW, true>();
  TestStringImpl<StringWCI, true>();
  TestStringImpl<StringView8, false>();
  TestStringImpl<StringView8CI, false>();
  TestStringImpl<StringView16, false>();
  TestStringImpl<StringView16CI, false>();
  TestStringImpl<StringView32, false>();
  TestStringImpl<StringView32CI, false>();
  TestStringImpl<StringViewW, false>();
  TestStringImpl<StringViewWCI, false>();
}

}

void RunTests()
{
  TestStringImpls();
}

#else

void RunTests()
{}

#endif
