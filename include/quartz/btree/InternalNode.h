#pragma once

#include "quartz/btree/BTreeNode.h"
#include "quartz/btree/KeyComparator.h"
#include "quartz/format/PageReference.h"

#include <cstddef>
#include <vector>

namespace quartz {
namespace btree {

/// Internal B-tree node storing separator keys and child page references.
class InternalNode : public BTreeNode {
public:
    InternalNode();
    explicit InternalNode(pages::IndexPage page);

    InternalNode(InternalNode&&) noexcept = default;
    InternalNode& operator=(InternalNode&&) noexcept = default;
    InternalNode(const InternalNode&) = delete;
    InternalNode& operator=(const InternalNode&) = delete;

    static InternalNode create(storage::PageId pageId, const BTreeNodeConfig& config);
    static InternalNode fromPage(pages::IndexPage page);

    const std::vector<Key>& keys() const noexcept { return keys_; }
    const std::vector<format::PageReference>& children() const noexcept { return children_; }

    std::size_t lowerBound(const Key& key) const;
    std::size_t upperBound(const Key& key) const;
    std::size_t find(const Key& key) const;

    /// Insert separator key and associated right child at index.
    Status insert(std::size_t index, const Key& key, format::PageReference rightChild);
    Status eraseAt(std::size_t index);

    Key keyAt(std::size_t index) const;
    format::PageReference childAt(std::size_t index) const;

    Status setKeyAt(std::size_t index, const Key& key);

    Status loadEntries();
    Status syncEntries();

    Status assignEntries(std::vector<Key> keys, std::vector<format::PageReference> children);

    Status validate() const override;
    Status serializeBody(serialization::BinaryWriter& writer) const;
    Status deserializeBody(serialization::BinaryReader& reader);

private:
    std::vector<Key> keys_;
    std::vector<format::PageReference> children_;
};

} // namespace btree
} // namespace quartz
