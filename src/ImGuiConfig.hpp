//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#pragma once

#ifdef _DEBUG
#include <cassert>
#define IM_ASSERT(x) assert(x)
#else
#define IM_ASSERT(x)
#endif
