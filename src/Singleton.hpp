//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#pragma once

template <typename InstanceType>
class Singleton
{
public:
  static InstanceType& Get()
  {
    static InstanceType instance;
    return instance;
  }
};
