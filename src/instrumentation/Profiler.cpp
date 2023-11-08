#include "quartz/instrumentation/Profiler.h"

#include <sstream>

namespace quartz {
namespace instrumentation {

Timer& Profiler::timer(const std::string& name) {
    return timers_[name];
}

Counter& Profiler::counter(const std::string& name) {
    return counters_[name];
}

void Profiler::reset() noexcept {
    timers_.clear();
    counters_.clear();
}

std::string Profiler::summary() const {
    std::ostringstream oss;
    for (const auto& entry : timers_) {
        oss << entry.first << ": " << entry.second.toString() << "\n";
    }
    for (const auto& entry : counters_) {
        oss << entry.first << ": " << entry.second.value() << "\n";
    }
    return oss.str();
}

} // namespace instrumentation
} // namespace quartz
