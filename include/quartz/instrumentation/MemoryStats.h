#pragma once

#include <cstddef>
#include <cstdint>

namespace quartz {
namespace instrumentation {

/// Process-level memory usage estimates (platform-independent placeholders).
struct MemoryStats {
    std::size_t heapAllocatedBytes = 0;
    std::size_t peakHeapBytes = 0;
    std::uint64_t allocationCount = 0;
    std::uint64_t deallocationCount = 0;

    void recordAllocation(std::size_t bytes) noexcept;
    void recordDeallocation(std::size_t bytes) noexcept;
    void reset() noexcept;
};

} // namespace instrumentation
} // namespace quartz
