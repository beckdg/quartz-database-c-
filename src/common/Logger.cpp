#include "quartz/common/Logger.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace quartz {

Logger::Logger(std::ostream& stream)
    : stream_(&stream) {}

Logger::~Logger() = default;

void Logger::setLevel(Level level) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

Logger::Level Logger::level() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return level_;
}

void Logger::setStream(std::ostream& stream) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    stream_ = &stream;
}

void Logger::log(Level level, std::string_view message) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (level < level_) {
        return;
    }

    *stream_ << "[" << currentTimestamp() << "] "
             << "[" << levelToString(level) << "] "
             << message << std::endl;
}

void Logger::debug(std::string_view message) {
    log(Level::Debug, message);
}

void Logger::info(std::string_view message) {
    log(Level::Info, message);
}

void Logger::warning(std::string_view message) {
    log(Level::Warning, message);
}

void Logger::error(std::string_view message) {
    log(Level::Error, message);
}

const char* Logger::levelToString(Level level) noexcept {
    switch (level) {
        case Level::Debug:   return "DEBUG";
        case Level::Info:    return "INFO";
        case Level::Warning: return "WARN";
        case Level::Error:   return "ERROR";
        default:             return "UNKNOWN";
    }
}

std::string Logger::currentTimestamp() {
    using namespace std::chrono;

    auto now = system_clock::now();
    auto tt = system_clock::to_time_t(now);
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000;

    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
        << "." << std::setfill('0') << std::setw(3) << ms;
    return oss.str();
}

} // namespace quartz
