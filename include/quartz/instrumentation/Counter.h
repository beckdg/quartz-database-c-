#pragma once

#include <atomic>
#include <cstdint>
#include <string>

namespace quartz {
namespace instrumentation {

/// Thread-safe monotonic counter.
class Counter {
public:
    explicit Counter(std::string name = {});

    void increment(std::uint64_t delta = 1) noexcept;
    std::uint64_t value() const noexcept;
    void reset() noexcept;

    const std::string& name() const noexcept { return name_; }

private:
    std::string name_;
    std::atomic<std::uint64_t> value_{0};
};

} // namespace instrumentation
} // namespace quartz
