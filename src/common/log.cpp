#include "remu/common/log.hpp"

#include <iostream>
#include <mutex>

namespace remu::common {

namespace {
    LogLevel g_log_level = LogLevel::Info;
    std::mutex g_log_mutex;

    const char* level_to_string(LogLevel level) {
        switch (level) {
            case LogLevel::Error: return "ERROR";
            case LogLevel::Warn:  return "WARN ";
            case LogLevel::Info:  return "INFO ";
            case LogLevel::Debug: return "DEBUG";
            default:              return "UNKWN";
        }
    }
}

void set_log_level(LogLevel level) {
    g_log_level = level;
}

LogLevel get_log_level() {
    return g_log_level;
}

void log(LogLevel level, std::string_view message) {
#ifdef REMU_ENABLE_LOG
    if (static_cast<int>(level) > static_cast<int>(g_log_level)) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_log_mutex);

    std::ostream& os =
        (level == LogLevel::Error) ? std::cerr : std::cout;

    os << "[" << level_to_string(level) << "] "
       << message << std::endl;
#else
    (void)level;
    (void)message;
#endif
}

void log_error(std::string_view message) {
    log(LogLevel::Error, message);
}

void log_warn(std::string_view message) {
    log(LogLevel::Warn, message);
}

void log_info(std::string_view message) {
    log(LogLevel::Info, message);
}

void log_debug(std::string_view message) {
    log(LogLevel::Debug, message);
}

} // namespace remu::common
