#include "quartz/instrumentation/MemoryStats.h"

#include <algorithm>

namespace quartz {
namespace instrumentation {

void MemoryStats::recordAllocation(std::size_t bytes) noexcept {
    heapAllocatedBytes += bytes;
    ++allocationCount;
    peakHeapBytes = std::max(peakHeapBytes, heapAllocatedBytes);
}

void MemoryStats::recordDeallocation(std::size_t bytes) noexcept {
    if (bytes <= heapAllocatedBytes) {
        heapAllocatedBytes -= bytes;
    } else {
        heapAllocatedBytes = 0;
    }
    ++deallocationCount;
}

void MemoryStats::reset() noexcept {
    *this = {};
}

} // namespace instrumentation
} // namespace quartz
