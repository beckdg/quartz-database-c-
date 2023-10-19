#pragma once

#include "quartz/common/NonCopyable.h"
#include "quartz/space/Extent.h"
#include "quartz/storage/StorageConstants.h"

#include <cstddef>
#include <cstdint>

namespace quartz {
namespace space {

class FreeSpaceMap;

struct SpaceStats {
    std::uint64_t allocationsRequested = 0;
    std::uint64_t allocationsSucceeded = 0;
    std::uint64_t allocationsFailed = 0;
    std::uint64_t freesRequested = 0;
    std::uint64_t freesSucceeded = 0;
    std::uint64_t freesFailed = 0;
    std::uint32_t currentAllocatedPages = 0;
    std::uint32_t currentFreePages = 0;
    std::uint32_t extentCount = 0;
    double fragmentationPercent = 0.0;

    void recordAllocation(bool success) noexcept;
    void recordFree(bool success) noexcept;
};

class SpaceStatisticsCollector : private NonCopyable {
public:
    void recordAllocation(bool success) noexcept;
    void recordFree(bool success) noexcept;
    void update(const FreeSpaceMap& fsm, std::uint32_t totalPages) noexcept;

    const SpaceStats& stats() const noexcept { return stats_; }
    void reset() noexcept;

private:
    SpaceStats stats_;
};

} // namespace space
} // namespace quartz
