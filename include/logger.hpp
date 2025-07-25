#ifndef SHM_LOGGER_H
#define SHM_LOGGER_H

#include <spdlog/fmt/bundled/format.h>  // Make sure this is included for fmt::runtime
#include <spdlog/fmt/fmt.h>             // Include this if not already there, might help fmt know its types

#include "spdlog/spdlog.h"

namespace Core::Logger {
/*template <typename... Args>*/
/*inline void info(const char *fmt, Args &&...dpargs) {*/
/*    spdlog::info(fmt, dpargs...);*/
/*}*/

template <typename... Args>
static void info(const char *fmt_str, Args &&...dpargs) {  // Use const char* here
    // Use fmt::runtime to explicitly tell spdlog/fmt to parse this format string at runtime
    spdlog::info(fmt::runtime(fmt_str), std::forward<Args>(dpargs)...);
}

template <typename... Args>
static void error(const char *fmt_str, Args &&...dpargs) {  // Use const char* here
    spdlog::error(fmt::runtime(fmt_str), std::forward<Args>(dpargs)...);
}

/*template <typename... Args>*/
/*inline void error(const char *fmt, Args &&...dpargs) {*/
/*    spdlog::error(fmt, dpargs...);*/
/*}*/
template <typename... Args>
inline void warn(const char *fmt, Args &&...dpargs) {
    spdlog::warn(fmt, dpargs...);
}
template <typename... Args>
inline void debug(const char *fmt, Args &&...dpargs) {
    spdlog::debug(fmt, dpargs...);
}

}  // namespace Core::Logger

#endif  // SHM_LOGGER_H
