#include "quartz/space/SpaceStatistics.h"
#include "quartz/space/FreeSpaceMap.h"

namespace quartz {
namespace space {

void SpaceStats::recordAllocation(bool success) noexcept {
    allocationsRequested++;
    if (success) {
        allocationsSucceeded++;
        currentAllocatedPages++;
    } else {
        allocationsFailed++;
    }
}

void SpaceStats::recordFree(bool success) noexcept {
    freesRequested++;
    if (success) {
        freesSucceeded++;
        if (currentAllocatedPages > 0) {
            currentAllocatedPages--;
        }
    } else {
        freesFailed++;
    }
}

void SpaceStatisticsCollector::recordAllocation(bool success) noexcept {
    stats_.recordAllocation(success);
}

void SpaceStatisticsCollector::recordFree(bool success) noexcept {
    stats_.recordFree(success);
}

void SpaceStatisticsCollector::update(const FreeSpaceMap& fsm,
                                      std::uint32_t totalPages) noexcept {
    stats_.currentFreePages = fsm.countFree();
    stats_.currentAllocatedPages = totalPages - stats_.currentFreePages;
    stats_.extentCount = fsm.countFreeExtents();

    if (stats_.currentFreePages > 0) {
        auto largest = fsm.extents().empty() ? 0 : fsm.extents().front().length;
        for (const auto& ext : fsm.extents()) {
            if (ext.length > largest) largest = ext.length;
        }
        stats_.fragmentationPercent =
            100.0 * (1.0 - static_cast<double>(largest) / static_cast<double>(stats_.currentFreePages));
    } else {
        stats_.fragmentationPercent = 0.0;
    }
}

void SpaceStatisticsCollector::reset() noexcept {
    stats_ = SpaceStats{};
}

} // namespace space
} // namespace quartz
