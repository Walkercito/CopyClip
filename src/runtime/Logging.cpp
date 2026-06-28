#include "runtime/Logging.hpp"

#include "config/Constants.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <string>
#include <string_view>

namespace copyclip::runtime {

namespace {

// spdlog rendering of the reference's "<time> <LEVEL> <name>: <message>" format.
constexpr std::string_view kLogPattern{"%Y-%m-%d %H:%M:%S %^%-8l%$ %n: %v"};

} // namespace

void configure_logging(spdlog::level::level_enum level) {
    const std::string name{config::kAppId};
    auto logger = spdlog::get(name);
    if (!logger) {
        logger = spdlog::stderr_color_mt(name);
        logger->set_pattern(std::string{kLogPattern});
        spdlog::set_default_logger(logger);
    }
    logger->set_level(level);
}

} // namespace copyclip::runtime
