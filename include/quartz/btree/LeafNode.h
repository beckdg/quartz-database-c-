#pragma once

#include "quartz/btree/BTreeNode.h"
#include "quartz/btree/KeyComparator.h"
#include "quartz/format/PageReference.h"

#include <cstddef>
#include <vector>

namespace quartz {
namespace btree {

/// Leaf B-tree node storing ordered keys and page references (no record payloads).
class LeafNode : public BTreeNode {
public:
    LeafNode();
    explicit LeafNode(pages::IndexPage page);

    LeafNode(LeafNode&&) noexcept = default;
    LeafNode& operator=(LeafNode&&) noexcept = default;
    LeafNode(const LeafNode&) = delete;
    LeafNode& operator=(const LeafNode&) = delete;

    static LeafNode create(storage::PageId pageId, const BTreeNodeConfig& config);
    static LeafNode fromPage(pages::IndexPage page);

    const std::vector<Key>& keys() const noexcept { return keys_; }
    const std::vector<format::PageReference>& references() const noexcept { return refs_; }

    bool contains(const Key& key) const;
    std::size_t lowerBound(const Key& key) const;
    std::size_t upperBound(const Key& key) const;
    std::size_t find(const Key& key) const;

    Status insert(const Key& key, format::PageReference ref);
    Status erase(const Key& key);
    Status eraseAt(std::size_t index);

    Key keyAt(std::size_t index) const;
    format::PageReference referenceAt(std::size_t index) const;

    Status assignEntries(std::vector<Key> keys, std::vector<format::PageReference> refs);

    Status loadEntries();
    Status syncEntries();

    Status validate() const override;
    Status serializeBody(serialization::BinaryWriter& writer) const;
    Status deserializeBody(serialization::BinaryReader& reader);

private:
    std::vector<Key> keys_;
    std::vector<format::PageReference> refs_;
};

} // namespace btree
} // namespace quartz
