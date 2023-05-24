//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#pragma once

extern "C"
{
#include "Precompiled.h"
}

#include <imgui.h>
#include <scn/scn.h>
#include <sndfile.hh>
#include <toml++/toml.h>

#include <array>
#include <bitset>
#include <compare>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "Singleton.hpp"
#include "UTF/UTF.hpp"

using namespace UTF;

#include "Options.hpp"
#include "Utils.hpp"
