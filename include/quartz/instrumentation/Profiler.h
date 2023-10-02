#pragma once

#include "quartz/instrumentation/Counter.h"
#include "quartz/instrumentation/Timer.h"

#include <string>
#include <unordered_map>

namespace quartz {
namespace instrumentation {

/// Lightweight scoped profiler collecting named timers and counters.
class Profiler {
public:
    Timer& timer(const std::string& name);
    Counter& counter(const std::string& name);

    void reset() noexcept;
    std::string summary() const;

private:
    std::unordered_map<std::string, Timer> timers_;
    std::unordered_map<std::string, Counter> counters_;
};

} // namespace instrumentation
} // namespace quartz
