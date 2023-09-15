#pragma once

#include "quartz/btree/BTreeTypes.h"

#include <cstddef>
#include <cstdint>

namespace quartz {
namespace btree {

class BTreeNode;

/// Occupancy and utilization metrics for a B-tree node.
struct BTreeStatistics {
    std::uint32_t keyCount = 0;
    std::uint32_t capacity = 0;
    std::uint32_t freeSlots = 0;
    std::uint32_t level = 0;
    double occupancyPercent = 0.0;
    double estimatedUtilization = 0.0;
    std::size_t usedBytes = 0;
    std::size_t freeBytes = 0;
    NodeType nodeType = NodeType::Leaf;
};

BTreeStatistics computeStatistics(const BTreeNode& node);

} // namespace btree
} // namespace quartz
