//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#pragma once

extern "C"
{

#include <Precompiled.h>

}

#include <Feldarian/G1AL/G1AL.hpp>

using namespace Feldarian::Base::PCMS16;
using namespace Feldarian::Base::UTF;
using namespace Feldarian::G1AL;

inline GlyphRangesBuilder g_GlyphRangesBuilder;
inline LocalizationManager g_LocalizationManager{g_GlyphRangesBuilder};

#include <imgui.h>
#include <imgui/backends/sdl2.h>
#include <imgui/backends/sdlrenderer2.h>
#include <scn/scn.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <bitset>
#include <chrono>
#include <compare>
#include <execution>
#include <filesystem>
#include <fstream>
#include <future>
#include <iomanip>
#include <memory>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <variant>
#include <vector>
