#include "quartz/btree/Cursor.h"

#include "quartz/btree/BTree.h"
#include "quartz/btree/InternalNode.h"
#include "quartz/btree/KeyComparator.h"
#include "quartz/btree/LeafNode.h"
#include "quartz/btree/SearchPath.h"

namespace quartz {
namespace btree {

void Cursor::reset() noexcept {
    mode_ = Mode::None;
    leaf_ = nullptr;
    internal_ = nullptr;
    tree_ = nullptr;
    leafId_ = storage::kInvalidPageId;
    index_ = 0;
    atEnd_ = false;
}

bool Cursor::valid() const noexcept {
    if (mode_ == Mode::Tree && tree_ != nullptr && !atEnd_) {
        return tree_->isLeaf(leafId_) && index_ < tree_->leaf(leafId_).keyCount();
    }
    if (mode_ == Mode::Leaf && leaf_ != nullptr) {
        return index_ < leaf_->keys().size();
    }
    if (mode_ == Mode::Internal && internal_ != nullptr) {
        return index_ < internal_->keys().size();
    }
    return false;
}

bool Cursor::isEnd() const noexcept {
    if (mode_ == Mode::Tree) {
        return atEnd_;
    }
    return !valid();
}

void Cursor::bindLeaf(const LeafNode* node) noexcept {
    reset();
    mode_ = Mode::Leaf;
    leaf_ = node;
}

void Cursor::bindInternal(const InternalNode* node) noexcept {
    reset();
    mode_ = Mode::Internal;
    internal_ = node;
}

void Cursor::bindTree(const BTree* tree, bool atEnd) noexcept {
    reset();
    mode_ = Mode::Tree;
    tree_ = tree;
    atEnd_ = atEnd;
}

Status Cursor::seek(std::size_t index) {
    if (mode_ == Mode::Leaf && leaf_ != nullptr) {
        if (index >= leaf_->keys().size()) {
            return Status::invalidArgument("Cursor: leaf index out of range");
        }
        index_ = index;
        return Status::success();
    }
    if (mode_ == Mode::Internal && internal_ != nullptr) {
        if (index >= internal_->keys().size()) {
            return Status::invalidArgument("Cursor: internal index out of range");
        }
        index_ = index;
        return Status::success();
    }
    if (mode_ == Mode::Tree && tree_ != nullptr && tree_->isLeaf(leafId_)) {
        if (index >= tree_->leaf(leafId_).keyCount()) {
            return Status::invalidArgument("Cursor: tree leaf index out of range");
        }
        index_ = index;
        atEnd_ = false;
        return Status::success();
    }
    return Status::invalidArgument("Cursor: not bound to a node");
}

Status Cursor::seekKey(const Key& key) {
    if (mode_ == Mode::Leaf && leaf_ != nullptr) {
        index_ = leaf_->lowerBound(key);
        if (index_ >= leaf_->keys().size() || !KeyComparator::equal(leaf_->keys()[index_], key)) {
            return Status::invalidArgument("Cursor: key not found");
        }
        return Status::success();
    }
    if (mode_ == Mode::Internal && internal_ != nullptr) {
        index_ = internal_->lowerBound(key);
        if (index_ >= internal_->keys().size() ||
            !KeyComparator::equal(internal_->keys()[index_], key)) {
            return Status::invalidArgument("Cursor: key not found");
        }
        return Status::success();
    }
    if (mode_ == Mode::Tree && tree_ != nullptr) {
        return tree_->lowerBound(key, *this);
    }
    return Status::invalidArgument("Cursor: not bound to a node");
}

Status Cursor::seekBegin() {
    if (mode_ != Mode::Tree || tree_ == nullptr) {
        return Status::invalidArgument("Cursor: tree not bound");
    }
    if (tree_->empty()) {
        atEnd_ = true;
        return Status::success();
    }

    storage::PageId current = tree_->rootId();
    while (tree_->isInternal(current)) {
        current = tree_->internal(current).childAt(0).pageId;
    }
    leafId_ = current;
    index_ = 0;
    atEnd_ = false;
    return Status::success();
}

Status Cursor::seekEnd() {
    if (mode_ != Mode::Tree || tree_ == nullptr) {
        return Status::invalidArgument("Cursor: tree not bound");
    }
    atEnd_ = true;
    return Status::success();
}

Status Cursor::advanceToNextLeaf() {
    if (mode_ != Mode::Tree || tree_ == nullptr || tree_->empty()) {
        atEnd_ = true;
        return Status::invalidArgument("Cursor: cannot advance");
    }

    SearchPath path;
    storage::PageId current = tree_->rootId();
    while (current != leafId_) {
        if (!tree_->isInternal(current)) {
            atEnd_ = true;
            return Status::invalidArgument("Cursor: leaf not reachable");
        }
        const auto& node = tree_->internal(current);
        std::size_t childIndex = 0;
        for (std::size_t i = 0; i <= node.keyCount(); ++i) {
            if (node.childAt(i).pageId == leafId_) {
                childIndex = i;
                break;
            }
        }
        SearchPathEntry entry;
        entry.pageId = current;
        entry.childIndex = static_cast<std::uint32_t>(childIndex);
        entry.pageRef = tree_->makeIndexRef(current);
        path.push(entry);
        current = leafId_;
    }

    while (!path.empty()) {
        const auto& frame = path.top();
        auto& parent = tree_->internal(frame.pageId);
        const auto nextChild = frame.childIndex + 1;
        if (nextChild <= parent.keyCount()) {
            storage::PageId nodeId = parent.childAt(nextChild).pageId;
            while (tree_->isInternal(nodeId)) {
                nodeId = tree_->internal(nodeId).childAt(0).pageId;
            }
            leafId_ = nodeId;
            index_ = 0;
            atEnd_ = false;
            return Status::success();
        }
        path.pop();
    }

    atEnd_ = true;
    return Status::invalidArgument("Cursor: end of tree");
}

Status Cursor::retreatToPreviousLeaf() {
    if (mode_ != Mode::Tree || tree_ == nullptr || tree_->empty()) {
        return Status::invalidArgument("Cursor: cannot retreat");
    }

    SearchPath path;
    storage::PageId current = tree_->rootId();
    while (current != leafId_) {
        if (!tree_->isInternal(current)) {
            return Status::invalidArgument("Cursor: leaf not reachable");
        }
        const auto& node = tree_->internal(current);
        std::size_t childIndex = 0;
        for (std::size_t i = 0; i <= node.keyCount(); ++i) {
            if (node.childAt(i).pageId == leafId_) {
                childIndex = i;
                break;
            }
        }
        SearchPathEntry entry;
        entry.pageId = current;
        entry.childIndex = static_cast<std::uint32_t>(childIndex);
        entry.pageRef = tree_->makeIndexRef(current);
        path.push(entry);
        current = leafId_;
    }

    while (!path.empty()) {
        const auto& frame = path.top();
        if (frame.childIndex > 0) {
            auto& parent = tree_->internal(frame.pageId);
            storage::PageId nodeId = parent.childAt(frame.childIndex - 1).pageId;
            while (tree_->isInternal(nodeId)) {
                const auto& internal = tree_->internal(nodeId);
                nodeId = internal.childAt(internal.keyCount()).pageId;
            }
            leafId_ = nodeId;
            index_ = tree_->leaf(leafId_).keyCount();
            if (index_ > 0) {
                --index_;
            }
            atEnd_ = false;
            return Status::success();
        }
        path.pop();
    }

    return Status::invalidArgument("Cursor: beginning of tree");
}

Status Cursor::next() {
    if (mode_ == Mode::Tree && tree_ != nullptr) {
        if (atEnd_) {
            return Status::invalidArgument("Cursor: end of tree");
        }
        const auto& leaf = tree_->leaf(leafId_);
        if (index_ + 1 < leaf.keyCount()) {
            ++index_;
            return Status::success();
        }
        return advanceToNextLeaf();
    }

    if (!valid()) {
        return Status::invalidArgument("Cursor: invalid position");
    }
    ++index_;
    if (!valid()) {
        return Status::invalidArgument("Cursor: end of node");
    }
    return Status::success();
}

Status Cursor::previous() {
    if (mode_ == Mode::Tree && tree_ != nullptr) {
        if (atEnd_) {
            if (tree_->empty()) {
                return Status::invalidArgument("Cursor: empty tree");
            }
            storage::PageId nodeId = tree_->rootId();
            while (tree_->isInternal(nodeId)) {
                const auto& internal = tree_->internal(nodeId);
                nodeId = internal.childAt(internal.keyCount()).pageId;
            }
            leafId_ = nodeId;
            index_ = tree_->leaf(leafId_).keyCount();
            if (index_ > 0) {
                --index_;
            }
            atEnd_ = false;
            return Status::success();
        }
        if (index_ > 0) {
            --index_;
            return Status::success();
        }
        return retreatToPreviousLeaf();
    }

    if (mode_ == Mode::None) {
        return Status::invalidArgument("Cursor: not bound to a node");
    }
    if (index_ == 0) {
        return Status::invalidArgument("Cursor: beginning of node");
    }
    --index_;
    return Status::success();
}

const Key& Cursor::currentKey() const {
    if (mode_ == Mode::Tree && tree_ != nullptr && !atEnd_ && tree_->isLeaf(leafId_)) {
        return tree_->leaf(leafId_).keys()[index_];
    }
    if (mode_ == Mode::Leaf && leaf_ != nullptr && index_ < leaf_->keys().size()) {
        return leaf_->keys()[index_];
    }
    if (mode_ == Mode::Internal && internal_ != nullptr && index_ < internal_->keys().size()) {
        return internal_->keys()[index_];
    }
    return scratchKey_;
}

format::PageReference Cursor::currentReference() const {
    if (mode_ == Mode::Tree && tree_ != nullptr && !atEnd_ && tree_->isLeaf(leafId_)) {
        return tree_->leaf(leafId_).referenceAt(index_);
    }
    if (mode_ == Mode::Leaf && leaf_ != nullptr && index_ < leaf_->references().size()) {
        return leaf_->references()[index_];
    }
    if (mode_ == Mode::Internal && internal_ != nullptr) {
        return internal_->childAt(index_ + 1);
    }
    return format::PageReference::invalid();
}

} // namespace btree
} // namespace quartz
