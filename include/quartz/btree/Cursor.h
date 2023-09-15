#pragma once

#include "quartz/btree/Key.h"
#include "quartz/common/Status.h"
#include "quartz/format/PageReference.h"
#include "quartz/storage/StorageConstants.h"

#include <cstddef>

namespace quartz {
namespace btree {

class BTree;
class LeafNode;
class InternalNode;

/// Iterator over keys in a complete B-tree or within a single node.
class Cursor {
public:
    Cursor() = default;

    void reset() noexcept;
    bool valid() const noexcept;
    bool isEnd() const noexcept;

    void bindLeaf(const LeafNode* node) noexcept;
    void bindInternal(const InternalNode* node) noexcept;
    void bindTree(const BTree* tree, bool atEnd = false) noexcept;

    Status seek(std::size_t index);
    Status seekKey(const Key& key);
    Status seekBegin();
    Status seekEnd();

    Status next();
    Status previous();

    const Key& currentKey() const;
    format::PageReference currentReference() const;

    std::size_t position() const noexcept { return index_; }

private:
    friend class BTree;

    enum class Mode { None, Leaf, Internal, Tree };

    Mode mode_ = Mode::None;
    const LeafNode* leaf_ = nullptr;
    const InternalNode* internal_ = nullptr;
    const BTree* tree_ = nullptr;
    storage::PageId leafId_ = storage::kInvalidPageId;
    std::size_t index_ = 0;
    bool atEnd_ = false;
    Key scratchKey_;

    Status advanceToNextLeaf();
    Status retreatToPreviousLeaf();
};

} // namespace btree
} // namespace quartz
