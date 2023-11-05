#include "quartz/instrumentation/Counter.h"

namespace quartz {
namespace instrumentation {

Counter::Counter(std::string name)
    : name_(std::move(name)) {}

void Counter::increment(std::uint64_t delta) noexcept {
    value_.fetch_add(delta, std::memory_order_relaxed);
}

std::uint64_t Counter::value() const noexcept {
    return value_.load(std::memory_order_relaxed);
}

void Counter::reset() noexcept {
    value_.store(0, std::memory_order_relaxed);
}

} // namespace instrumentation
} // namespace quartz
