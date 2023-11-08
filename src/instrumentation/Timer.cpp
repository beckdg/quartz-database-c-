#include "quartz/instrumentation/Timer.h"

#include <sstream>

namespace quartz {
namespace instrumentation {

Timer::Timer() {
    reset();
}

void Timer::reset() noexcept {
    start_ = Clock::now();
    stop_ = start_;
    running_ = false;
}

void Timer::start() noexcept {
    start_ = Clock::now();
    running_ = true;
}

void Timer::stop() noexcept {
    stop_ = Clock::now();
    running_ = false;
}

double Timer::elapsedMicroseconds() const noexcept {
    const auto end = running_ ? Clock::now() : stop_;
    return static_cast<double>(
        std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count());
}

double Timer::elapsedMilliseconds() const noexcept {
    return elapsedMicroseconds() / 1000.0;
}

std::string Timer::toString() const {
    std::ostringstream oss;
    oss << elapsedMilliseconds() << " ms";
    return oss.str();
}

} // namespace instrumentation
} // namespace quartz
