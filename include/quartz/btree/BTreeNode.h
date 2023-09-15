#pragma once

#include "quartz/btree/BTreeTypes.h"
#include "quartz/btree/Key.h"
#include "quartz/common/Status.h"
#include "quartz/format/PageReference.h"
#include "quartz/pages/IndexPage.h"
#include "quartz/serialization/BinaryReader.h"
#include "quartz/serialization/BinaryWriter.h"
#include "quartz/storage/StorageConstants.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace quartz {
namespace btree {

struct BTreeStatistics;
struct SplitPlan;
struct MergePlan;

/// Common B-tree node abstraction backed by an IndexPage.
class BTreeNode {
public:
    virtual ~BTreeNode() = default;

    BTreeNode();
    explicit BTreeNode(pages::IndexPage page);

    BTreeNode(BTreeNode&&) noexcept = default;
    BTreeNode& operator=(BTreeNode&&) noexcept = default;
    BTreeNode(const BTreeNode&) = delete;
    BTreeNode& operator=(const BTreeNode&) = delete;

    pages::IndexPage& indexPage() noexcept { return page_; }
    const pages::IndexPage& indexPage() const noexcept { return page_; }

    storage::PageId pageId() const noexcept;
    NodeType nodeType() const noexcept;
    std::uint32_t keyCount() const noexcept;
    std::uint32_t capacity() const noexcept;
    std::uint32_t level() const noexcept;
    NodeFlags flags() const noexcept;
    const BTreeNodeConfig& config() const noexcept { return config_; }

    format::PageReference parent() const noexcept;
    void setParent(format::PageReference parent) noexcept;
    void setLevel(std::uint32_t level) noexcept;

    std::size_t freeSlots() const noexcept;
    double occupancyPercent() const noexcept;
    std::size_t freeSpaceBytes() const noexcept;

    virtual Status validate() const;
    Status serialize(serialization::BinaryWriter& writer) const;
    Status deserialize(serialization::BinaryReader& reader);

    BTreeStatistics statistics() const;

    /// Compute maximum keys that fit in the node body for the given configuration.
    static std::uint32_t computeCapacity(const BTreeNodeConfig& config, NodeType type);

protected:
    pages::IndexPage page_;
    BTreeNodeConfig config_;
    format::PageReference parent_;

    void initializeHeader(NodeType type, const BTreeNodeConfig& config);
    Status syncHeader() noexcept;
    Status loadHeader() noexcept;

    static std::size_t maxBodyBytes() noexcept;
    static std::size_t entrySize(const BTreeNodeConfig& config, NodeType type) noexcept;
};

} // namespace btree
} // namespace quartz
