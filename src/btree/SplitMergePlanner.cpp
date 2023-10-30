#include "quartz/btree/SplitMergePlanner.h"

#include "quartz/btree/InternalNode.h"
#include "quartz/btree/LeafNode.h"

namespace quartz {
namespace btree {

namespace {

constexpr double kDefaultSplitRatio = 0.5;

SplitPlan makeSplitPlan(std::uint32_t keyCount, std::uint32_t capacity, const Key& promoted,
                        std::uint32_t splitPos) {
    SplitPlan plan;
    if (keyCount == 0 || capacity == 0) {
        return plan;
    }
    plan.splitPosition = splitPos;
    plan.promotedKey = promoted;
    plan.leftOccupancy =
        (static_cast<double>(splitPos) / static_cast<double>(capacity)) * 100.0;
    plan.rightOccupancy =
        (static_cast<double>(keyCount - splitPos) / static_cast<double>(capacity)) * 100.0;
    plan.feasible = splitPos > 0 && splitPos < keyCount;
    return plan;
}

} // namespace

SplitPlan SplitMergePlanner::planLeafSplit(const LeafNode& node) {
    const auto count = node.keyCount();
    const auto cap = node.capacity();
    if (count == 0 || cap == 0) {
        return {};
    }
    const std::uint32_t splitPos =
        static_cast<std::uint32_t>(static_cast<double>(count) * kDefaultSplitRatio);
    const auto pos = splitPos > 0 ? splitPos : 1;
    const Key promoted = node.keyAt(pos);
    return makeSplitPlan(count, cap, promoted, pos);
}

SplitPlan SplitMergePlanner::planInternalSplit(const InternalNode& node) {
    const auto count = node.keyCount();
    const auto cap = node.capacity();
    if (count == 0 || cap == 0) {
        return {};
    }
    const std::uint32_t splitPos =
        static_cast<std::uint32_t>(static_cast<double>(count) * kDefaultSplitRatio);
    const auto pos = splitPos > 0 ? splitPos : 1;
    const Key promoted = node.keyAt(pos);
    return makeSplitPlan(count, cap, promoted, pos);
}

MergePlan SplitMergePlanner::planLeafMerge(const LeafNode& left, const LeafNode& right) {
    MergePlan plan;
    const auto combined = left.keyCount() + right.keyCount();
    const auto cap = left.capacity();
    plan.resultingKeyCount = combined;
    plan.combinedOccupancy =
        cap > 0 ? (static_cast<double>(combined) / static_cast<double>(cap)) * 100.0 : 0.0;
    plan.feasible = combined <= cap;
    return plan;
}

MergePlan SplitMergePlanner::planInternalMerge(const InternalNode& left,
                                               const InternalNode& right) {
    MergePlan plan;
    const auto combined = left.keyCount() + right.keyCount();
    const auto cap = left.capacity();
    plan.resultingKeyCount = combined;
    plan.combinedOccupancy =
        cap > 0 ? (static_cast<double>(combined) / static_cast<double>(cap)) * 100.0 : 0.0;
    plan.feasible = combined <= cap;
    return plan;
}

} // namespace btree
} // namespace quartz
