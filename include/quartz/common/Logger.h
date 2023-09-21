#pragma once

#include <iostream>
#include <mutex>
#include <ostream>
#include <string>
#include <string_view>

namespace quartz {

class Logger {
public:
    enum class Level : int {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Error = 3
    };

    explicit Logger(std::ostream& stream = std::cout);
    ~Logger();

    void setLevel(Level level) noexcept;
    Level level() const noexcept;

    void setStream(std::ostream& stream) noexcept;

    void log(Level level, std::string_view message);
    void debug(std::string_view message);
    void info(std::string_view message);
    void warning(std::string_view message);
    void error(std::string_view message);

private:
    static const char* levelToString(Level level) noexcept;
    static std::string currentTimestamp();

    mutable std::mutex mutex_;
    Level level_ = Level::Info;
    std::ostream* stream_;
};

} // namespace quartz
