#pragma once

// Centralized logging configuration for the application.
//
// Mirrors the reference module copyclip/runtime/logging.py: it configures a
// single application logger and is safe to call repeatedly (idempotent — the
// sink is created once, only the level is updated on later calls). Runtime layer
// — Qt-free; uses spdlog only.

#include <spdlog/common.h>

namespace copyclip::runtime {

// Configure the application logger at `level`. The first call installs a single
// stderr sink with the application log pattern and makes it the default logger;
// subsequent calls only adjust the level (no duplicate sinks).
void configure_logging(spdlog::level::level_enum level = spdlog::level::info);

} // namespace copyclip::runtime
