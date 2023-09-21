#pragma once

#include "quartz/btree/BTree.h"
#include "quartz/common/Status.h"

namespace quartz {
namespace btree {

/// Validates structural invariants across an entire B-tree.
class TreeValidator {
public:
    static Status validate(const BTree& tree);

private:
    static Status validateLeafRange(const LeafNode& leaf, const Key* minKey, const Key* maxKey);
    static Status validateInternalNode(const BTree& tree, const InternalNode& node,
                                       std::uint32_t depth, std::uint32_t& leafDepth,
                                       const Key* minKey, const Key* maxKey);
    static Status validateSubtree(const BTree& tree, storage::PageId pageId, std::uint32_t depth,
                                  std::uint32_t& leafDepth, const Key* minKey, const Key* maxKey);
};

} // namespace btree
} // namespace quartz
