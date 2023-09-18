#pragma once

#include "quartz/btree/BTree.h"
#include "quartz/btree/Key.h"
#include "quartz/btree/SearchPath.h"
#include "quartz/common/Status.h"
#include "quartz/format/PageReference.h"

namespace quartz {
namespace btree {

class LeafNode;
class InternalNode;

/// Split, merge, borrow, and rotation operations for B-tree rebalancing.
class Rebalance {
public:
    static Status splitAndInsertLeaf(BTree& tree, LeafNode& leaf, SearchPath& path,
                                   const Key& key, format::PageReference value);

    static Status splitAndInsertInternal(BTree& tree, InternalNode& node, SearchPath& path,
                                         std::size_t insertIndex, const Key& key,
                                         format::PageReference rightChild);

    static Status insertIntoParent(BTree& tree, SearchPath& path, const Key& promoted,
                                   storage::PageId leftId, storage::PageId rightId);

    static Status fixLeafUnderflow(BTree& tree, LeafNode& leaf, SearchPath& path);
    static Status fixInternalUnderflow(BTree& tree, InternalNode& node, SearchPath& path);

private:
    static Status borrowFromLeftLeaf(BTree& tree, InternalNode& parent, std::size_t childIndex,
                                     LeafNode& left, LeafNode& node);
    static Status borrowFromRightLeaf(BTree& tree, InternalNode& parent, std::size_t childIndex,
                                      LeafNode& node, LeafNode& right);
    static Status mergeLeafLeft(BTree& tree, SearchPath& path, InternalNode& parent,
                                std::size_t childIndex);
    static Status mergeLeafRight(BTree& tree, SearchPath& path, InternalNode& parent,
                                 std::size_t childIndex);

    static Status borrowFromLeftInternal(BTree& tree, InternalNode& parent, std::size_t childIndex,
                                         InternalNode& left, InternalNode& node);
    static Status borrowFromRightInternal(BTree& tree, InternalNode& parent, std::size_t childIndex,
                                          InternalNode& node, InternalNode& right);
    static Status mergeInternalLeft(BTree& tree, SearchPath& path, InternalNode& parent,
                                    std::size_t childIndex);
    static Status mergeInternalRight(BTree& tree, SearchPath& path, InternalNode& parent,
                                     std::size_t childIndex);

    static Status setParentKey(InternalNode& parent, std::size_t index, const Key& key);
};

} // namespace btree
} // namespace quartz
