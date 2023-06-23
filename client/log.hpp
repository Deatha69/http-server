#pragma once
#ifndef GUARD_LOG_HPP
#define GUARD_LOG_HPP

#include "spdlog/spdlog.h"

// clang-format off

#define LOG_INIT() { spdlog::set_level(spdlog::level::debug); }
#define LOG_DEBUG(message, ...)    spdlog::debug(message, ##__VA_ARGS__)
#define LOG_INFO(message, ...)     spdlog::info(message, ##__VA_ARGS__)
#define LOG_ERROR(message, ...)    spdlog::error(message, ##__VA_ARGS__)
#define LOG_CRITICAL(message, ...) spdlog::critical(message, ##__VA_ARGS__)

// clang-format on

#endif