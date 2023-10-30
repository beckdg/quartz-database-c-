#include "quartz/btree/LeafNode.h"

#include "quartz/pages/PageLayouts.h"

#include <cstring>

namespace quartz {
namespace btree {

namespace {

constexpr std::size_t kBodyHeaderSize = sizeof(std::uint32_t) + format::PageReference::kSize +
                                        sizeof(std::uint32_t) + sizeof(std::uint8_t) +
                                        sizeof(std::uint8_t) + sizeof(std::uint16_t);

std::uint8_t* entryArea(pages::IndexPageLayout* layout) noexcept {
    return layout->data + kBodyHeaderSize;
}

std::size_t entryAreaSize() noexcept {
    return sizeof(pages::IndexPageLayout::data) - kBodyHeaderSize;
}

} // namespace

LeafNode::LeafNode() = default;

LeafNode::LeafNode(pages::IndexPage page)
    : BTreeNode(std::move(page)) {
    (void)loadEntries();
}

LeafNode LeafNode::create(storage::PageId pageId, const BTreeNodeConfig& config) {
    LeafNode node(pages::IndexPage::create(pageId));
    node.initializeHeader(NodeType::Leaf, config);
    return node;
}

LeafNode LeafNode::fromPage(pages::IndexPage page) {
    return LeafNode(std::move(page));
}

bool LeafNode::contains(const Key& key) const {
    return find(key) < keys_.size();
}

std::size_t LeafNode::lowerBound(const Key& key) const {
    return KeyComparator::lowerBound(keys_, key);
}

std::size_t LeafNode::upperBound(const Key& key) const {
    return KeyComparator::upperBound(keys_, key);
}

std::size_t LeafNode::find(const Key& key) const {
    return KeyComparator::find(keys_, key);
}

Status LeafNode::insert(const Key& key, format::PageReference ref) {
    if (!ref.isValid()) {
        return Status::invalidArgument("LeafNode: invalid page reference");
    }
    if (keyCount() >= capacity()) {
        return Status::invalidArgument("LeafNode: node is full");
    }

    const std::size_t pos = lowerBound(key);
    if (!hasFlag(flags(), NodeFlags::AllowDuplicates) && pos < keys_.size() &&
        KeyComparator::equal(keys_[pos], key)) {
        return Status::invalidArgument("LeafNode: duplicate key");
    }

    keys_.insert(keys_.begin() + static_cast<std::ptrdiff_t>(pos), key);
    refs_.insert(refs_.begin() + static_cast<std::ptrdiff_t>(pos), ref);
    page_.setKeyCount(static_cast<std::uint32_t>(keys_.size()));
    return syncEntries();
}

Status LeafNode::erase(const Key& key) {
    const std::size_t pos = find(key);
    if (pos >= keys_.size()) {
        return Status::invalidArgument("LeafNode: key not found");
    }
    return eraseAt(pos);
}

Status LeafNode::eraseAt(std::size_t index) {
    if (index >= keys_.size()) {
        return Status::invalidArgument("LeafNode: index out of range");
    }
    keys_.erase(keys_.begin() + static_cast<std::ptrdiff_t>(index));
    refs_.erase(refs_.begin() + static_cast<std::ptrdiff_t>(index));
    page_.setKeyCount(static_cast<std::uint32_t>(keys_.size()));
    return syncEntries();
}

Key LeafNode::keyAt(std::size_t index) const {
    return keys_.at(index);
}

format::PageReference LeafNode::referenceAt(std::size_t index) const {
    return refs_.at(index);
}

Status LeafNode::assignEntries(std::vector<Key> keys, std::vector<format::PageReference> refs) {
    if (keys.size() != refs.size()) {
        return Status::invalidArgument("LeafNode: key/reference size mismatch");
    }
    if (keys.size() > capacity()) {
        return Status::invalidArgument("LeafNode: exceeds capacity");
    }
    keys_ = std::move(keys);
    refs_ = std::move(refs);
    page_.setKeyCount(static_cast<std::uint32_t>(keys_.size()));
    return syncEntries();
}

Status LeafNode::loadEntries() {
    keys_.clear();
    refs_.clear();

    auto* layout = page_.layout();
    if (layout == nullptr) {
        return Status::corruption("LeafNode: missing layout");
    }

    auto st = loadHeader();
    if (!st.ok()) return st;

    serialization::BinaryReader reader(
        serialization::BufferView(entryArea(layout), entryAreaSize()));

    for (std::uint32_t i = 0; i < keyCount(); ++i) {
        Key key;
        st = key.deserialize(reader, config_.binaryKeySize);
        if (!st.ok()) return st;
        format::PageReference ref;
        st = reader.read(ref);
        if (!st.ok()) return st;
        keys_.push_back(key);
        refs_.push_back(ref);
    }
    return Status::success();
}

Status LeafNode::syncEntries() {
    auto st = syncHeader();
    if (!st.ok()) return st;

    auto* layout = page_.layout();
    if (layout == nullptr) {
        return Status::corruption("LeafNode: missing layout");
    }

    serialization::Buffer buf;
    serialization::BinaryWriter writer(buf);

    for (std::size_t i = 0; i < keys_.size(); ++i) {
        st = keys_[i].serialize(writer);
        if (!st.ok()) return st;
        st = writer.write(refs_[i]);
        if (!st.ok()) return st;
    }

    const auto bytes = writer.tell();
    if (bytes > entryAreaSize()) {
        return Status::corruption("LeafNode: entry data exceeds page capacity");
    }

    std::memset(entryArea(layout), 0, entryAreaSize());
    if (bytes > 0) {
        std::memcpy(entryArea(layout), buf.data(), bytes);
    }
    page_.setKeyCount(static_cast<std::uint32_t>(keys_.size()));
    return Status::success();
}

Status LeafNode::validate() const {
    auto st = BTreeNode::validate();
    if (!st.ok()) return st;
    if (nodeType() != NodeType::Leaf) {
        return Status::corruption("LeafNode: wrong node type");
    }
    if (keys_.size() != refs_.size()) {
        return Status::corruption("LeafNode: key/reference count mismatch");
    }
    if (keys_.size() != keyCount()) {
        return Status::corruption("LeafNode: header key count mismatch");
    }
    for (std::size_t i = 1; i < keys_.size(); ++i) {
        if (!KeyComparator::less(keys_[i - 1], keys_[i])) {
            if (!hasFlag(flags(), NodeFlags::AllowDuplicates) ||
                !KeyComparator::equal(keys_[i - 1], keys_[i])) {
                return Status::corruption("LeafNode: keys not sorted");
            }
        }
    }
    return Status::success();
}

Status LeafNode::serializeBody(serialization::BinaryWriter& writer) const {
    for (std::size_t i = 0; i < keys_.size(); ++i) {
        auto st = keys_[i].serialize(writer);
        if (!st.ok()) return st;
        st = writer.write(refs_[i]);
        if (!st.ok()) return st;
    }
    return Status::success();
}

Status LeafNode::deserializeBody(serialization::BinaryReader& reader) {
    keys_.clear();
    refs_.clear();
    for (std::uint32_t i = 0; i < keyCount(); ++i) {
        Key key;
        auto st = key.deserialize(reader, config_.binaryKeySize);
        if (!st.ok()) return st;
        format::PageReference ref;
        st = reader.read(ref);
        if (!st.ok()) return st;
        keys_.push_back(key);
        refs_.push_back(ref);
    }
    return Status::success();
}

} // namespace btree
} // namespace quartz
