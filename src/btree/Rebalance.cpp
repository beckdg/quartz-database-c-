#include "quartz/btree/Rebalance.h"

#include "quartz/btree/InternalNode.h"
#include "quartz/btree/KeyComparator.h"
#include "quartz/btree/LeafNode.h"

namespace quartz {
namespace btree {

Status Rebalance::setParentKey(InternalNode& parent, std::size_t index, const Key& key) {
    return parent.setKeyAt(index, key);
}

Status Rebalance::insertIntoParent(BTree& tree, SearchPath& path, const Key& promoted,
                                 storage::PageId leftId, storage::PageId rightId) {
    if (path.empty()) {
        InternalNode* root = nullptr;
        auto st = tree.allocateInternal(root, tree.height());
        if (!st.ok()) {
            return st;
        }
        st = root->assignEntries(
            std::vector<Key>{promoted},
            std::vector<format::PageReference>{tree.makeIndexRef(leftId), tree.makeIndexRef(rightId)});
        if (!st.ok()) {
            return st;
        }
        tree.setRoot(root->pageId());
        tree.setHeight(tree.height() + 1);
        return Status::success();
    }

    const auto& frame = path.top();
    auto& parent = tree.internal(frame.pageId);
    if (parent.keyCount() < parent.capacity()) {
        return parent.insert(frame.childIndex, promoted, tree.makeIndexRef(rightId));
    }

    return splitAndInsertInternal(tree, parent, path, frame.childIndex, promoted,
                                  tree.makeIndexRef(rightId));
}

Status Rebalance::splitAndInsertLeaf(BTree& tree, LeafNode& leaf, SearchPath& path,
                                   const Key& key, format::PageReference value) {
    std::vector<Key> keys = leaf.keys();
    std::vector<format::PageReference> refs = leaf.references();

    const auto pos = KeyComparator::lowerBound(keys, key);
    keys.insert(keys.begin() + static_cast<std::ptrdiff_t>(pos), key);
    refs.insert(refs.begin() + static_cast<std::ptrdiff_t>(pos), value);

    const auto splitIdx = keys.size() / 2;
    const Key promoted = keys[splitIdx];

    std::vector<Key> leftKeys(keys.begin(), keys.begin() + static_cast<std::ptrdiff_t>(splitIdx));
    std::vector<format::PageReference> leftRefs(refs.begin(),
                                                refs.begin() + static_cast<std::ptrdiff_t>(splitIdx));
    std::vector<Key> rightKeys(keys.begin() + static_cast<std::ptrdiff_t>(splitIdx + 1), keys.end());
    std::vector<format::PageReference> rightRefs(refs.begin() + static_cast<std::ptrdiff_t>(splitIdx + 1),
                                                 refs.end());

    auto st = leaf.assignEntries(std::move(leftKeys), std::move(leftRefs));
    if (!st.ok()) {
        return st;
    }

    LeafNode* sibling = nullptr;
    st = tree.allocateLeaf(sibling, leaf.level());
    if (!st.ok()) {
        return st;
    }
    st = sibling->assignEntries(std::move(rightKeys), std::move(rightRefs));
    if (!st.ok()) {
        return st;
    }

    tree.statsCollector().recordSplit();
    st = tree.walOnNodeSplit(leaf.pageId(), sibling->pageId(), promoted);
    if (!st.ok()) {
        return st;
    }
    return insertIntoParent(tree, path, promoted, leaf.pageId(), sibling->pageId());
}

Status Rebalance::splitAndInsertInternal(BTree& tree, InternalNode& node, SearchPath& path,
                                         std::size_t insertIndex, const Key& key,
                                         format::PageReference rightChild) {
    std::vector<Key> keys = node.keys();
    std::vector<format::PageReference> children = node.children();

    keys.insert(keys.begin() + static_cast<std::ptrdiff_t>(insertIndex), key);
    children.insert(children.begin() + static_cast<std::ptrdiff_t>(insertIndex + 1), rightChild);

    const auto splitIdx = keys.size() / 2;
    const Key promoted = keys[splitIdx];

    std::vector<Key> leftKeys(keys.begin(), keys.begin() + static_cast<std::ptrdiff_t>(splitIdx));
    std::vector<format::PageReference> leftChildren(
        children.begin(), children.begin() + static_cast<std::ptrdiff_t>(splitIdx + 1));
    std::vector<Key> rightKeys(keys.begin() + static_cast<std::ptrdiff_t>(splitIdx + 1), keys.end());
    std::vector<format::PageReference> rightChildren(
        children.begin() + static_cast<std::ptrdiff_t>(splitIdx + 1), children.end());

    auto st = node.assignEntries(std::move(leftKeys), std::move(leftChildren));
    if (!st.ok()) {
        return st;
    }

    InternalNode* sibling = nullptr;
    st = tree.allocateInternal(sibling, node.level());
    if (!st.ok()) {
        return st;
    }
    st = sibling->assignEntries(std::move(rightKeys), std::move(rightChildren));
    if (!st.ok()) {
        return st;
    }

    tree.statsCollector().recordSplit();

    SearchPath ancestorPath = path;
    ancestorPath.pop();
    st = tree.walOnNodeSplit(node.pageId(), sibling->pageId(), promoted);
    if (!st.ok()) {
        return st;
    }
    return insertIntoParent(tree, ancestorPath, promoted, node.pageId(), sibling->pageId());
}

Status Rebalance::borrowFromLeftLeaf(BTree& tree, InternalNode& parent, std::size_t childIndex,
                                     LeafNode& left, LeafNode& node) {
    const Key separator = parent.keyAt(childIndex - 1);
    const Key leftKey = left.keyAt(left.keyCount() - 1);
    const format::PageReference leftRef = left.referenceAt(left.keyCount() - 1);

    auto st = left.eraseAt(left.keyCount() - 1);
    if (!st.ok()) {
        return st;
    }
    st = setParentKey(parent, childIndex - 1, leftKey);
    if (!st.ok()) {
        return st;
    }

    std::vector<Key> keys = node.keys();
    std::vector<format::PageReference> refs = node.references();
    keys.insert(keys.begin(), separator);
    refs.insert(refs.begin(), leftRef);
    st = node.assignEntries(std::move(keys), std::move(refs));
    if (st.ok()) {
        tree.statsCollector().recordRotation();
    }
    return st;
}

Status Rebalance::borrowFromRightLeaf(BTree& tree, InternalNode& parent, std::size_t childIndex,
                                      LeafNode& node, LeafNode& right) {
    const Key separator = parent.keyAt(childIndex);
    const Key rightKey = right.keyAt(0);
    const format::PageReference rightRef = right.referenceAt(0);

    auto st = right.eraseAt(0);
    if (!st.ok()) {
        return st;
    }
    st = setParentKey(parent, childIndex, rightKey);
    if (!st.ok()) {
        return st;
    }

    std::vector<Key> keys = node.keys();
    std::vector<format::PageReference> refs = node.references();
    keys.push_back(separator);
    refs.push_back(rightRef);
    st = node.assignEntries(std::move(keys), std::move(refs));
    if (st.ok()) {
        tree.statsCollector().recordRotation();
    }
    return st;
}

Status Rebalance::mergeLeafLeft(BTree& tree, SearchPath& path, InternalNode& parent,
                                std::size_t childIndex) {
    auto& left = tree.leaf(parent.childAt(childIndex - 1).pageId);
    auto& current = tree.leaf(parent.childAt(childIndex).pageId);

    std::vector<Key> keys = left.keys();
    std::vector<format::PageReference> refs = left.references();
    for (std::size_t i = 0; i < current.keyCount(); ++i) {
        keys.push_back(current.keyAt(i));
        refs.push_back(current.referenceAt(i));
    }

    auto st = left.assignEntries(std::move(keys), std::move(refs));
    if (!st.ok()) {
        return st;
    }

    const auto removedId = current.pageId();
    st = parent.eraseAt(childIndex - 1);
    if (!st.ok()) {
        return st;
    }
    st = tree.walOnNodeMerge(left.pageId(), removedId);
    if (!st.ok()) {
        return st;
    }
    tree.releaseNode(removedId);
    tree.statsCollector().recordMerge();

    if (parent.pageId() == tree.rootId() && parent.keyCount() == 0) {
        const auto newRoot = parent.childAt(0).pageId;
        tree.releaseNode(parent.pageId());
        tree.setRoot(newRoot);
        if (tree.height() > 0) {
            tree.setHeight(tree.height() - 1);
        }
        return Status::success();
    }

    if (isUnderfull(parent.keyCount(), parent.capacity(), parent.pageId() == tree.rootId())) {
        SearchPath parentPath = path;
        parentPath.pop();
        return fixInternalUnderflow(tree, parent, parentPath);
    }
    return Status::success();
}

Status Rebalance::mergeLeafRight(BTree& tree, SearchPath& path, InternalNode& parent,
                                 std::size_t childIndex) {
    auto& current = tree.leaf(parent.childAt(childIndex).pageId);
    auto& right = tree.leaf(parent.childAt(childIndex + 1).pageId);

    std::vector<Key> keys = current.keys();
    std::vector<format::PageReference> refs = current.references();
    for (std::size_t i = 0; i < right.keyCount(); ++i) {
        keys.push_back(right.keyAt(i));
        refs.push_back(right.referenceAt(i));
    }

    auto st = current.assignEntries(std::move(keys), std::move(refs));
    if (!st.ok()) {
        return st;
    }

    const auto removedId = right.pageId();
    st = parent.eraseAt(childIndex);
    if (!st.ok()) {
        return st;
    }
    st = tree.walOnNodeMerge(current.pageId(), removedId);
    if (!st.ok()) {
        return st;
    }
    tree.releaseNode(removedId);
    tree.statsCollector().recordMerge();

    if (parent.pageId() == tree.rootId() && parent.keyCount() == 0) {
        const auto newRoot = parent.childAt(0).pageId;
        tree.releaseNode(parent.pageId());
        tree.setRoot(newRoot);
        if (tree.height() > 0) {
            tree.setHeight(tree.height() - 1);
        }
        return Status::success();
    }

    if (isUnderfull(parent.keyCount(), parent.capacity(), parent.pageId() == tree.rootId())) {
        SearchPath parentPath = path;
        parentPath.pop();
        return fixInternalUnderflow(tree, parent, parentPath);
    }
    return Status::success();
}

Status Rebalance::fixLeafUnderflow(BTree& tree, LeafNode& leaf, SearchPath& path) {
    if (path.empty()) {
        return Status::success();
    }

    const auto& frame = path.top();
    auto& parent = tree.internal(frame.pageId);
    const auto childIndex = frame.childIndex;

    if (childIndex > 0) {
        auto& left = tree.leaf(parent.childAt(childIndex - 1).pageId);
        if (left.keyCount() > minKeyCount(left.capacity(), false)) {
            return borrowFromLeftLeaf(tree, parent, childIndex, left, leaf);
        }
    }

    if (childIndex + 1 <= parent.keyCount()) {
        auto& right = tree.leaf(parent.childAt(childIndex + 1).pageId);
        if (right.keyCount() > minKeyCount(right.capacity(), false)) {
            return borrowFromRightLeaf(tree, parent, childIndex, leaf, right);
        }
    }

    if (childIndex > 0) {
        return mergeLeafLeft(tree, path, parent, childIndex);
    }
    return mergeLeafRight(tree, path, parent, childIndex);
}

Status Rebalance::borrowFromLeftInternal(BTree& tree, InternalNode& parent, std::size_t childIndex,
                                         InternalNode& left, InternalNode& node) {
    const Key separator = parent.keyAt(childIndex - 1);
    const Key leftKey = left.keyAt(left.keyCount() - 1);
    const format::PageReference leftChild = left.childAt(left.keyCount());

    std::vector<Key> leftKeys = left.keys();
    std::vector<format::PageReference> leftChildren = left.children();
    leftKeys.pop_back();
    leftChildren.pop_back();
    auto st = left.assignEntries(std::move(leftKeys), std::move(leftChildren));
    if (!st.ok()) {
        return st;
    }

    st = setParentKey(parent, childIndex - 1, leftKey);
    if (!st.ok()) {
        return st;
    }

    std::vector<Key> keys = node.keys();
    std::vector<format::PageReference> children = node.children();
    keys.insert(keys.begin(), separator);
    children.insert(children.begin(), leftChild);
    st = node.assignEntries(std::move(keys), std::move(children));
    if (st.ok()) {
        tree.statsCollector().recordRotation();
    }
    return st;
}

Status Rebalance::borrowFromRightInternal(BTree& tree, InternalNode& parent, std::size_t childIndex,
                                          InternalNode& node, InternalNode& right) {
    const Key separator = parent.keyAt(childIndex);
    const Key rightKey = right.keyAt(0);
    const format::PageReference rightChild = right.childAt(0);

    std::vector<Key> rightKeys = right.keys();
    std::vector<format::PageReference> rightChildren = right.children();
    rightKeys.erase(rightKeys.begin());
    rightChildren.erase(rightChildren.begin());
    auto st = right.assignEntries(std::move(rightKeys), std::move(rightChildren));
    if (!st.ok()) {
        return st;
    }

    st = setParentKey(parent, childIndex, rightKey);
    if (!st.ok()) {
        return st;
    }

    std::vector<Key> keys = node.keys();
    std::vector<format::PageReference> children = node.children();
    keys.push_back(separator);
    children.push_back(rightChild);
    st = node.assignEntries(std::move(keys), std::move(children));
    if (st.ok()) {
        tree.statsCollector().recordRotation();
    }
    return st;
}

Status Rebalance::mergeInternalLeft(BTree& tree, SearchPath& path, InternalNode& parent,
                                    std::size_t childIndex) {
    auto& left = tree.internal(parent.childAt(childIndex - 1).pageId);
    auto& current = tree.internal(parent.childAt(childIndex).pageId);

    std::vector<Key> keys = left.keys();
    keys.push_back(parent.keyAt(childIndex - 1));
    keys.insert(keys.end(), current.keys().begin(), current.keys().end());

    std::vector<format::PageReference> children = left.children();
    children.insert(children.end(), current.children().begin() + 1, current.children().end());

    auto st = left.assignEntries(std::move(keys), std::move(children));
    if (!st.ok()) {
        return st;
    }

    const auto removedId = current.pageId();
    st = parent.eraseAt(childIndex - 1);
    if (!st.ok()) {
        return st;
    }
    st = tree.walOnNodeMerge(left.pageId(), removedId);
    if (!st.ok()) {
        return st;
    }
    tree.releaseNode(removedId);
    tree.statsCollector().recordMerge();

    if (parent.pageId() == tree.rootId() && parent.keyCount() == 0) {
        const auto newRoot = parent.childAt(0).pageId;
        tree.releaseNode(parent.pageId());
        tree.setRoot(newRoot);
        if (tree.height() > 0) {
            tree.setHeight(tree.height() - 1);
        }
        return Status::success();
    }

    if (isUnderfull(parent.keyCount(), parent.capacity(), parent.pageId() == tree.rootId())) {
        SearchPath parentPath = path;
        parentPath.pop();
        return fixInternalUnderflow(tree, parent, parentPath);
    }
    return Status::success();
}

Status Rebalance::mergeInternalRight(BTree& tree, SearchPath& path, InternalNode& parent,
                                     std::size_t childIndex) {
    auto& current = tree.internal(parent.childAt(childIndex).pageId);
    auto& right = tree.internal(parent.childAt(childIndex + 1).pageId);

    std::vector<Key> keys = current.keys();
    keys.push_back(parent.keyAt(childIndex));
    keys.insert(keys.end(), right.keys().begin(), right.keys().end());

    std::vector<format::PageReference> children = current.children();
    children.insert(children.end(), right.children().begin() + 1, right.children().end());

    auto st = current.assignEntries(std::move(keys), std::move(children));
    if (!st.ok()) {
        return st;
    }

    const auto removedId = right.pageId();
    st = parent.eraseAt(childIndex);
    if (!st.ok()) {
        return st;
    }
    st = tree.walOnNodeMerge(current.pageId(), removedId);
    if (!st.ok()) {
        return st;
    }
    tree.releaseNode(removedId);
    tree.statsCollector().recordMerge();

    if (parent.pageId() == tree.rootId() && parent.keyCount() == 0) {
        const auto newRoot = parent.childAt(0).pageId;
        tree.releaseNode(parent.pageId());
        tree.setRoot(newRoot);
        if (tree.height() > 0) {
            tree.setHeight(tree.height() - 1);
        }
        return Status::success();
    }

    if (isUnderfull(parent.keyCount(), parent.capacity(), parent.pageId() == tree.rootId())) {
        SearchPath parentPath = path;
        parentPath.pop();
        return fixInternalUnderflow(tree, parent, parentPath);
    }
    return Status::success();
}

Status Rebalance::fixInternalUnderflow(BTree& tree, InternalNode& node, SearchPath& path) {
    if (path.empty()) {
        return Status::success();
    }

    const auto& frame = path.top();
    auto& parent = tree.internal(frame.pageId);
    const auto childIndex = frame.childIndex;

    if (childIndex > 0) {
        auto& left = tree.internal(parent.childAt(childIndex - 1).pageId);
        if (left.keyCount() > minKeyCount(left.capacity(), false)) {
            return borrowFromLeftInternal(tree, parent, childIndex, left, node);
        }
    }

    if (childIndex + 1 <= parent.keyCount()) {
        auto& right = tree.internal(parent.childAt(childIndex + 1).pageId);
        if (right.keyCount() > minKeyCount(right.capacity(), false)) {
            return borrowFromRightInternal(tree, parent, childIndex, node, right);
        }
    }

    if (childIndex > 0) {
        return mergeInternalLeft(tree, path, parent, childIndex);
    }
    return mergeInternalRight(tree, path, parent, childIndex);
}

} // namespace btree
} // namespace quartz
