#pragma once

#include "quartz/btree/BTreeStatistics.h"
#include "quartz/btree/Key.h"

#include <cstdint>

namespace quartz {
namespace btree {

/// Result of split planning. Does not modify any nodes.
struct SplitPlan {
    bool feasible = false;
    std::uint32_t splitPosition = 0;
    Key promotedKey;
    double leftOccupancy = 0.0;
    double rightOccupancy = 0.0;
};

/// Result of merge feasibility analysis. Does not modify any nodes.
struct MergePlan {
    bool feasible = false;
    double combinedOccupancy = 0.0;
    std::uint32_t resultingKeyCount = 0;
};

/// Static helpers for split and merge planning (no tree mutation).
class SplitMergePlanner {
public:
  static SplitPlan planLeafSplit(const class LeafNode& node);
  static SplitPlan planInternalSplit(const class InternalNode& node);
  static MergePlan planLeafMerge(const class LeafNode& left, const class LeafNode& right);
  static MergePlan planInternalMerge(const class InternalNode& left,
                                     const class InternalNode& right);
};

} // namespace btree
} // namespace quartz
