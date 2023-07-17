//
// Created by Andrej Redeky.
// Copyright © 2015-2023 Feldarian Softworks. All rights reserved.
// SPDX-License-Identifier: EUPL-1.2
//

#include <Precompiled.hpp>

#include "Log.hpp"

namespace App {

Log::Log() {
  std::vector<spdlog::sink_ptr> log_sinks;

  const spdlog::level::level_enum level{spdlog::level::debug};

  log_sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
  log_sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("app.log", true));

  log_sinks[0]->set_pattern("%^[%T] %n(%l): %v%$");
  log_sinks[1]->set_pattern("[%T] [%l] %n(%l): %v");

  m_logger = std::make_shared<spdlog::logger>("APP", begin(log_sinks), end(log_sinks));
  spdlog::register_logger(m_logger);
  spdlog::set_default_logger(m_logger);
  m_logger->set_level(level);
  m_logger->flush_on(level);
}

}  // namespace App
