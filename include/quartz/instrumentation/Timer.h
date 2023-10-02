#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace quartz {
namespace instrumentation {

/// High-resolution elapsed time measurement.
class Timer {
public:
    Timer();

    void reset() noexcept;
    void start() noexcept;
    void stop() noexcept;

    bool running() const noexcept { return running_; }
    double elapsedMicroseconds() const noexcept;
    double elapsedMilliseconds() const noexcept;

    std::string toString() const;

private:
    using Clock = std::chrono::steady_clock;
    Clock::time_point start_;
    Clock::time_point stop_;
    bool running_ = false;
};

} // namespace instrumentation
} // namespace quartz
