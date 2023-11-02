#include "quartz/btree/TreeValidator.h"

#include "quartz/btree/InternalNode.h"
#include "quartz/btree/KeyComparator.h"
#include "quartz/btree/LeafNode.h"
#include "quartz/btree/NodeValidator.h"

namespace quartz {
namespace btree {

Status TreeValidator::validateLeafRange(const LeafNode& leaf, const Key* minKey, const Key* maxKey) {
    auto st = NodeValidator::validateLeaf(leaf);
    if (!st.ok()) {
        return st;
    }
    for (std::size_t i = 0; i < leaf.keyCount(); ++i) {
        const auto& key = leaf.keyAt(i);
        if (minKey != nullptr && KeyComparator::less(key, *minKey)) {
            return Status::corruption("TreeValidator: leaf key below parent separator");
        }
        if (maxKey != nullptr && !KeyComparator::less(key, *maxKey)) {
            return Status::corruption("TreeValidator: leaf key at or above upper separator");
        }
        if (i > 0 && KeyComparator::less(key, leaf.keyAt(i - 1))) {
            return Status::corruption("TreeValidator: leaf keys not sorted");
        }
    }
    return Status::success();
}

Status TreeValidator::validateSubtree(const BTree& tree, storage::PageId pageId, std::uint32_t depth,
                                      std::uint32_t& leafDepth, const Key* minKey, const Key* maxKey) {
    if (tree.isLeaf(pageId)) {
        if (leafDepth == 0) {
            leafDepth = depth;
        } else if (leafDepth != depth) {
            return Status::corruption("TreeValidator: leaves at different depths");
        }
        return validateLeafRange(tree.leaf(pageId), minKey, maxKey);
    }

    if (!tree.isInternal(pageId)) {
        return Status::corruption("TreeValidator: unknown page in tree");
    }

    const auto& node = tree.internal(pageId);
    return validateInternalNode(tree, node, depth, leafDepth, minKey, maxKey);
}

Status TreeValidator::validateInternalNode(const BTree& tree, const InternalNode& node,
                                           std::uint32_t depth, std::uint32_t& leafDepth,
                                           const Key* minKey, const Key* maxKey) {
    auto st = NodeValidator::validateInternal(node);
    if (!st.ok()) {
        return st;
    }

    for (std::size_t i = 0; i < node.keyCount(); ++i) {
        const auto& key = node.keyAt(i);
        if (minKey != nullptr && KeyComparator::less(key, *minKey)) {
            return Status::corruption("TreeValidator: internal key below bound");
        }
        if (maxKey != nullptr && KeyComparator::greater(key, *maxKey)) {
            return Status::corruption("TreeValidator: internal key above bound");
        }
        if (i > 0 && KeyComparator::less(key, node.keyAt(i - 1))) {
            return Status::corruption("TreeValidator: internal keys not sorted");
        }
    }

    for (std::size_t i = 0; i <= node.keyCount(); ++i) {
        const auto childRef = node.childAt(i);
        if (!childRef.isValid()) {
            return Status::corruption("TreeValidator: invalid child reference");
        }
        const Key* childMin = minKey;
        const Key* childMax = maxKey;
        Key leftBound;
        Key rightBound;
        if (i > 0) {
            leftBound = node.keyAt(i - 1);
            childMin = &leftBound;
        }
        if (i < node.keyCount()) {
            rightBound = node.keyAt(i);
            childMax = &rightBound;
        }
        st = validateSubtree(tree, childRef.pageId, depth + 1, leafDepth, childMin, childMax);
        if (!st.ok()) {
            return st;
        }
    }
    return Status::success();
}

Status TreeValidator::validate(const BTree& tree) {
    if (tree.empty()) {
        return Status::success();
    }

    if (!tree.isLeaf(tree.rootId()) && !tree.isInternal(tree.rootId())) {
        return Status::corruption("TreeValidator: invalid root page");
    }

    std::uint32_t leafDepth = 0;
    return validateSubtree(tree, tree.rootId(), 1, leafDepth, nullptr, nullptr);
}

} // namespace btree
} // namespace quartz
