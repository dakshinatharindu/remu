#pragma once

#include <string>
#include <string_view>

namespace remu::common {

enum class LogLevel {
    Error = 0,
    Warn,
    Info,
    Debug
};

/// Set global log level (default = Info)
void set_log_level(LogLevel level);

/// Get current log level
LogLevel get_log_level();

/// Core logging function
void log(LogLevel level, std::string_view message);

/// Convenience helpers
void log_error(std::string_view message);
void log_warn(std::string_view message);
void log_info(std::string_view message);
void log_debug(std::string_view message);

} // namespace rvemu::common
