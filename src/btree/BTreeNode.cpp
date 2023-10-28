#include "quartz/btree/BTreeNode.h"

#include "quartz/btree/BTreeStatistics.h"
#include "quartz/pages/PageLayouts.h"

#include <cstring>

namespace quartz {
namespace btree {

namespace {

constexpr std::size_t kBodyHeaderSize = sizeof(std::uint32_t) + format::PageReference::kSize +
                                        sizeof(std::uint32_t) + sizeof(std::uint8_t) +
                                        sizeof(std::uint8_t) + sizeof(std::uint16_t);

std::size_t keyWireSize(const BTreeNodeConfig& config) {
    switch (config.keyType) {
    case KeyType::UInt32:
        return 1 + sizeof(std::uint32_t);
    case KeyType::UInt64:
        return 1 + sizeof(std::uint64_t);
    case KeyType::Binary:
        return 1 + sizeof(std::uint16_t) + config.binaryKeySize;
    default:
        return 0;
    }
}

} // namespace

BTreeNode::BTreeNode() = default;

BTreeNode::BTreeNode(pages::IndexPage page)
    : page_(std::move(page)) {
    (void)loadHeader();
}

storage::PageId BTreeNode::pageId() const noexcept {
    return page_.id();
}

NodeType BTreeNode::nodeType() const noexcept {
    return static_cast<NodeType>(page_.nodeType());
}

std::uint32_t BTreeNode::keyCount() const noexcept {
    return page_.keyCount();
}

std::uint32_t BTreeNode::capacity() const noexcept {
    return page_.capacity();
}

std::uint32_t BTreeNode::level() const noexcept {
    return config_.level;
}

NodeFlags BTreeNode::flags() const noexcept {
    return static_cast<NodeFlags>(page_.flags());
}

format::PageReference BTreeNode::parent() const noexcept {
    return parent_;
}

void BTreeNode::setParent(format::PageReference parent) noexcept {
    parent_ = parent;
}

void BTreeNode::setLevel(std::uint32_t level) noexcept {
    config_.level = level;
    auto* layout = page_.layout();
    if (layout != nullptr) {
        layout->reserved[0] = level;
    }
}

std::size_t BTreeNode::freeSlots() const noexcept {
    const auto cap = capacity();
    const auto count = keyCount();
    return cap > count ? static_cast<std::size_t>(cap - count) : 0;
}

double BTreeNode::occupancyPercent() const noexcept {
    const auto cap = capacity();
    if (cap == 0) return 0.0;
    return (static_cast<double>(keyCount()) / static_cast<double>(cap)) * 100.0;
}

std::size_t BTreeNode::freeSpaceBytes() const noexcept {
    const auto used = keyCount() * entrySize(config_, nodeType());
    const auto total = maxBodyBytes() > kBodyHeaderSize ? maxBodyBytes() - kBodyHeaderSize : 0;
    return total > used ? total - used : 0;
}

Status BTreeNode::validate() const {
    return page_.validate();
}

Status BTreeNode::serialize(serialization::BinaryWriter& writer) const {
    return page_.serialize(writer);
}

Status BTreeNode::deserialize(serialization::BinaryReader& reader) {
    auto st = page_.deserialize(reader);
    if (!st.ok()) return st;
    return loadHeader();
}

BTreeStatistics BTreeNode::statistics() const {
    BTreeStatistics stats;
    stats.keyCount = keyCount();
    stats.capacity = capacity();
    stats.freeSlots = static_cast<std::uint32_t>(freeSlots());
    stats.level = level();
    stats.occupancyPercent = occupancyPercent();
    stats.nodeType = nodeType();
    stats.usedBytes = keyCount() * entrySize(config_, nodeType()) + kBodyHeaderSize;
    stats.freeBytes = freeSpaceBytes();
    const auto body = maxBodyBytes();
    stats.estimatedUtilization =
        body > 0 ? (static_cast<double>(stats.usedBytes) / static_cast<double>(body)) * 100.0 : 0.0;
    return stats;
}

std::uint32_t BTreeNode::computeCapacity(const BTreeNodeConfig& config, NodeType type) {
    const auto entry = entrySize(config, type);
    if (entry == 0) return 0;
    const auto available = maxBodyBytes() > kBodyHeaderSize ? maxBodyBytes() - kBodyHeaderSize : 0;
    if (type == NodeType::Internal) {
        // n keys and n+1 children stored as n (key, right-child) pairs plus one left child.
        const auto pairSize = entry;
        const auto leftChildSize = format::PageReference::kSize;
        if (available <= leftChildSize) return 0;
        return static_cast<std::uint32_t>((available - leftChildSize) / pairSize);
    }
    return static_cast<std::uint32_t>(available / entry);
}

void BTreeNode::initializeHeader(NodeType type, const BTreeNodeConfig& config) {
    config_ = config;
    page_.setNodeType(static_cast<std::uint32_t>(type));
    page_.setKeyCount(0);
    const auto cap = computeCapacity(config, type);
    page_.setCapacity(cap);
    page_.setFlags(config.allowDuplicates ? static_cast<std::uint32_t>(NodeFlags::AllowDuplicates)
                                        : static_cast<std::uint32_t>(NodeFlags::None));
    parent_ = format::PageReference::invalid();
    auto* layout = page_.layout();
    if (layout != nullptr) {
        layout->reserved[0] = config.level;
        layout->reserved[1] = 0;
        layout->reserved[2] = 0;
        std::memset(layout->data, 0, sizeof(layout->data));
    }
    (void)syncHeader();
}

Status BTreeNode::syncHeader() noexcept {
    auto* layout = page_.layout();
    if (layout == nullptr) {
        return Status::corruption("BTreeNode: missing page layout");
    }

    serialization::Buffer buf;
    serialization::BinaryWriter writer(buf);

    auto st = writer.write(kNodeBodyMagic);
    if (!st.ok()) return st;
    st = writer.write(parent_);
    if (!st.ok()) return st;
    st = writer.write(config_.level);
    if (!st.ok()) return st;
    const auto keyTypeByte = static_cast<std::uint8_t>(config_.keyType);
    st = writer.write(keyTypeByte);
    if (!st.ok()) return st;
    const auto allowDup = static_cast<std::uint8_t>(config_.allowDuplicates ? 1 : 0);
    st = writer.write(allowDup);
    if (!st.ok()) return st;
    st = writer.write(config_.binaryKeySize);
    if (!st.ok()) return st;

    if (writer.tell() > sizeof(layout->data)) {
        return Status::corruption("BTreeNode: body header exceeds data area");
    }
    std::memcpy(layout->data, buf.data(), writer.tell());
    layout->reserved[0] = config_.level;
    return Status::success();
}

Status BTreeNode::loadHeader() noexcept {
    auto* layout = page_.layout();
    if (layout == nullptr) {
        return Status::corruption("BTreeNode: missing page layout");
    }

    serialization::BinaryReader reader(
        serialization::BufferView(layout->data, sizeof(layout->data)));

    std::uint32_t magic = 0;
    auto st = reader.read(magic);
    if (!st.ok()) return st;
    if (magic != kNodeBodyMagic && magic != 0) {
        return Status::corruption("BTreeNode: invalid body magic");
    }

    if (magic == 0) {
        config_.level = static_cast<std::uint32_t>(layout->reserved[0]);
        parent_ = format::PageReference::invalid();
        return Status::success();
    }

    st = reader.read(parent_);
    if (!st.ok()) return st;
    st = reader.read(config_.level);
    if (!st.ok()) return st;
    std::uint8_t keyTypeByte = 0;
    st = reader.read(keyTypeByte);
    if (!st.ok()) return st;
    config_.keyType = static_cast<KeyType>(keyTypeByte);
    std::uint8_t allowDup = 0;
    st = reader.read(allowDup);
    if (!st.ok()) return st;
    config_.allowDuplicates = allowDup != 0;
    st = reader.read(config_.binaryKeySize);
    if (!st.ok()) return st;
    return Status::success();
}

std::size_t BTreeNode::maxBodyBytes() noexcept {
    return sizeof(pages::IndexPageLayout::data);
}

std::size_t BTreeNode::entrySize(const BTreeNodeConfig& config, NodeType type) noexcept {
    const auto keySize = keyWireSize(config);
    if (type == NodeType::Leaf) {
        return keySize + format::PageReference::kSize;
    }
    return keySize + format::PageReference::kSize;
}

} // namespace btree
} // namespace quartz
