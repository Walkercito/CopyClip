#include "runtime/Logging.hpp"

#include "config/Constants.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <string>
#include <string_view>

namespace copyclip::runtime {

namespace {

// spdlog equivalent of the reference's "%(asctime)s %(levelname)-7s %(name)s:
// %(message)s": ms timestamp, level left-padded to 7, name, message. (spdlog
// lower-cases level names by convention; structure otherwise matches.)
constexpr std::string_view kLogPattern{"%Y-%m-%d %H:%M:%S,%e %^%-7l%$ %n: %v"};

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
