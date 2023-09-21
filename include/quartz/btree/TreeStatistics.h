#pragma once

#include "quartz/btree/BTreeTypes.h"

#include <cstddef>
#include <cstdint>

namespace quartz {
namespace btree {

/// Counters for structural tree operations.
struct TreeOperationStats {
    std::uint64_t splitCount = 0;
    std::uint64_t mergeCount = 0;
    std::uint64_t rotationCount = 0;
    std::uint64_t allocationCount = 0;
};

/// Aggregate metrics for an entire B-tree.
struct TreeStatistics {
    std::uint32_t height = 0;
    std::uint32_t nodeCount = 0;
    std::uint32_t leafCount = 0;
    std::uint32_t internalCount = 0;
    std::uint32_t maxDepth = 0;
    std::size_t keyCount = 0;
    double averageOccupancy = 0.0;
    TreeOperationStats operations;
};

/// Tracks structural operations performed by a B-tree instance.
class TreeStatisticsCollector {
public:
    void recordSplit() noexcept { ++stats_.splitCount; }
    void recordMerge() noexcept { ++stats_.mergeCount; }
    void recordRotation() noexcept { ++stats_.rotationCount; }
    void recordAllocation() noexcept { ++stats_.allocationCount; }

    const TreeOperationStats& operations() const noexcept { return stats_; }
    void reset() noexcept { stats_ = {}; }

private:
    TreeOperationStats stats_;
};

} // namespace btree
} // namespace quartz
