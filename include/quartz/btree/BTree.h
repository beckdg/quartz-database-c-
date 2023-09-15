#pragma once

#include "quartz/btree/BTreeTypes.h"
#include "quartz/btree/BTreeWalSink.h"
#include "quartz/btree/Key.h"
#include "quartz/btree/SearchPath.h"
#include "quartz/btree/TreeStatistics.h"
#include "quartz/common/Status.h"
#include "quartz/format/PageReference.h"
#include "quartz/serialization/BinaryReader.h"
#include "quartz/serialization/BinaryWriter.h"
#include "quartz/storage/StorageConstants.h"

#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace quartz {
namespace space {
class SpaceManager;
}
namespace btree {

class Cursor;
class LeafNode;
class InternalNode;
class Rebalance;
class TreeValidator;

/// Complete B-tree built on LeafNode and InternalNode with SpaceManager integration.
class BTree {
public:
    explicit BTree(space::SpaceManager& space, BTreeNodeConfig config = {});

    ~BTree();

    static BTree create(space::SpaceManager& space, BTreeNodeConfig config = {});

    void clear();
    bool empty() const noexcept;
    std::size_t size() const noexcept;
    std::uint32_t height() const noexcept;
    storage::PageId rootId() const noexcept;
    const BTreeNodeConfig& config() const noexcept { return config_; }

    bool contains(const Key& key) const;
    Status find(const Key& key, format::PageReference& out) const;
    Status lowerBound(const Key& key, Cursor& cursor) const;
    Status upperBound(const Key& key, Cursor& cursor) const;

    Status insert(const Key& key, format::PageReference value);
    Status erase(const Key& key);

    Status validate() const;
    TreeStatistics statistics() const;

    Cursor begin() const;
    Cursor end() const;

    Status serialize(serialization::BinaryWriter& writer) const;
    Status deserialize(serialization::BinaryReader& reader);

    /// Optional WAL sink; tree operations succeed even when unset.
    void setWalSink(BTreeWalSink* sink) noexcept;
    BTreeWalSink* walSink() const noexcept { return walSink_; }

private:
    friend class Rebalance;
    friend class TreeValidator;
    friend class Cursor;

    space::SpaceManager& space_;
    BTreeNodeConfig config_;
    storage::PageId rootId_ = storage::kInvalidPageId;
    std::size_t size_ = 0;
    std::uint32_t height_ = 0;
    TreeStatisticsCollector opStats_;

    BTreeWalSink* walSink_ = nullptr;

    std::unordered_map<storage::PageId, LeafNode> leaves_;
    std::unordered_map<storage::PageId, InternalNode> internals_;

    bool isLeaf(storage::PageId id) const;
    bool isInternal(storage::PageId id) const;

    LeafNode& leaf(storage::PageId id);
    InternalNode& internal(storage::PageId id);
    const LeafNode& leaf(storage::PageId id) const;
    const InternalNode& internal(storage::PageId id) const;

    Status allocateLeaf(LeafNode*& out, std::uint32_t level);
    Status allocateInternal(InternalNode*& out, std::uint32_t level);
    void releaseNode(storage::PageId id);

    format::PageReference makeIndexRef(storage::PageId id) const;

    void setRoot(storage::PageId id) noexcept;
    void setHeight(std::uint32_t h) noexcept;
    void incrementSize() noexcept { ++size_; }
    void decrementSize() noexcept { if (size_ > 0) --size_; }

    TreeStatisticsCollector& statsCollector() noexcept { return opStats_; }

    Status descendToLeaf(const Key& key, SearchPath& path, LeafNode*& leaf,
                         std::size_t& index, bool exactMatch) const;
    Status descendForInsert(const Key& key, SearchPath& path, LeafNode*& leaf,
                            std::size_t& index) const;

    Status collapseRootIfNeeded();

    Status walOnInsert(const Key& key, format::PageReference value);
    Status walOnErase(const Key& key);
    Status walOnPageAllocate(storage::PageId pageId);
    Status walOnPageDeallocate(storage::PageId pageId);
    Status walOnNodeSplit(storage::PageId leftId, storage::PageId rightId, const Key& promoted);
    Status walOnNodeMerge(storage::PageId survivorId, storage::PageId removedId);
};

} // namespace btree
} // namespace quartz
