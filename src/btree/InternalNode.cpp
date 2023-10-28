#include "quartz/btree/InternalNode.h"

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

InternalNode::InternalNode() = default;

InternalNode::InternalNode(pages::IndexPage page)
    : BTreeNode(std::move(page)) {
    (void)loadEntries();
}

InternalNode InternalNode::create(storage::PageId pageId, const BTreeNodeConfig& config) {
    InternalNode node(pages::IndexPage::create(pageId));
    node.initializeHeader(NodeType::Internal, config);
    node.children_.clear();
    node.children_.push_back(format::PageReference::invalid());
    return node;
}

InternalNode InternalNode::fromPage(pages::IndexPage page) {
    return InternalNode(std::move(page));
}

std::size_t InternalNode::lowerBound(const Key& key) const {
    return KeyComparator::lowerBound(keys_, key);
}

std::size_t InternalNode::upperBound(const Key& key) const {
    return KeyComparator::upperBound(keys_, key);
}

std::size_t InternalNode::find(const Key& key) const {
    return KeyComparator::find(keys_, key);
}

Status InternalNode::insert(std::size_t index, const Key& key, format::PageReference rightChild) {
    if (!rightChild.isValid()) {
        return Status::invalidArgument("InternalNode: invalid right child reference");
    }
    if (index > keys_.size()) {
        return Status::invalidArgument("InternalNode: index out of range");
    }
    if (keys_.size() >= capacity()) {
        return Status::invalidArgument("InternalNode: node is full");
    }
    if (!hasFlag(flags(), NodeFlags::AllowDuplicates)) {
        for (const auto& existing : keys_) {
            if (KeyComparator::equal(existing, key)) {
                return Status::invalidArgument("InternalNode: duplicate key");
            }
        }
    }

    keys_.insert(keys_.begin() + static_cast<std::ptrdiff_t>(index), key);
    children_.insert(children_.begin() + static_cast<std::ptrdiff_t>(index + 1), rightChild);
    page_.setKeyCount(static_cast<std::uint32_t>(keys_.size()));
    return syncEntries();
}

Status InternalNode::eraseAt(std::size_t index) {
    if (index >= keys_.size()) {
        return Status::invalidArgument("InternalNode: index out of range");
    }
    keys_.erase(keys_.begin() + static_cast<std::ptrdiff_t>(index));
    children_.erase(children_.begin() + static_cast<std::ptrdiff_t>(index + 1));
    page_.setKeyCount(static_cast<std::uint32_t>(keys_.size()));
    return syncEntries();
}

Key InternalNode::keyAt(std::size_t index) const {
    return keys_.at(index);
}

format::PageReference InternalNode::childAt(std::size_t index) const {
    return children_.at(index);
}

Status InternalNode::setKeyAt(std::size_t index, const Key& key) {
    if (index >= keys_.size()) {
        return Status::invalidArgument("InternalNode: key index out of range");
    }
    keys_[index] = key;
    return syncEntries();
}

Status InternalNode::assignEntries(std::vector<Key> keys,
                                   std::vector<format::PageReference> children) {
    if (children.size() != keys.size() + 1) {
        return Status::invalidArgument("InternalNode: child count mismatch");
    }
    if (keys.size() > capacity()) {
        return Status::invalidArgument("InternalNode: exceeds capacity");
    }
    keys_ = std::move(keys);
    children_ = std::move(children);
    page_.setKeyCount(static_cast<std::uint32_t>(keys_.size()));
    return syncEntries();
}

Status InternalNode::loadEntries() {
    keys_.clear();
    children_.clear();

    auto* layout = page_.layout();
    if (layout == nullptr) {
        return Status::corruption("InternalNode: missing layout");
    }

    auto st = loadHeader();
    if (!st.ok()) return st;

    serialization::BinaryReader reader(
        serialization::BufferView(entryArea(layout), entryAreaSize()));

    if (keyCount() == 0) {
        format::PageReference leftChild;
        st = reader.read(leftChild);
        if (!st.ok()) return st;
        children_.push_back(leftChild);
        return Status::success();
    }

    format::PageReference leftChild;
    st = reader.read(leftChild);
    if (!st.ok()) return st;
    children_.push_back(leftChild);

    for (std::uint32_t i = 0; i < keyCount(); ++i) {
        Key key;
        st = key.deserialize(reader, config_.binaryKeySize);
        if (!st.ok()) return st;
        format::PageReference rightChild;
        st = reader.read(rightChild);
        if (!st.ok()) return st;
        keys_.push_back(key);
        children_.push_back(rightChild);
    }
    return Status::success();
}

Status InternalNode::syncEntries() {
    auto st = syncHeader();
    if (!st.ok()) return st;

    auto* layout = page_.layout();
    if (layout == nullptr) {
        return Status::corruption("InternalNode: missing layout");
    }

    serialization::Buffer buf;
    serialization::BinaryWriter writer(buf);

    if (!children_.empty()) {
        st = writer.write(children_.front());
        if (!st.ok()) return st;
    } else {
        st = writer.write(format::PageReference::invalid());
        if (!st.ok()) return st;
    }

    for (std::size_t i = 0; i < keys_.size(); ++i) {
        st = keys_[i].serialize(writer);
        if (!st.ok()) return st;
        st = writer.write(children_[i + 1]);
        if (!st.ok()) return st;
    }

    const auto bytes = writer.tell();
    if (bytes > entryAreaSize()) {
        return Status::corruption("InternalNode: entry data exceeds page capacity");
    }

    std::memset(entryArea(layout), 0, entryAreaSize());
    if (bytes > 0) {
        std::memcpy(entryArea(layout), buf.data(), bytes);
    }
    page_.setKeyCount(static_cast<std::uint32_t>(keys_.size()));
    return Status::success();
}

Status InternalNode::validate() const {
    auto st = BTreeNode::validate();
    if (!st.ok()) return st;
    if (nodeType() != NodeType::Internal) {
        return Status::corruption("InternalNode: wrong node type");
    }
    if (children_.size() != keys_.size() + 1) {
        return Status::corruption("InternalNode: child count mismatch");
    }
    if (keys_.size() != keyCount()) {
        return Status::corruption("InternalNode: header key count mismatch");
    }
    for (std::size_t i = 1; i < keys_.size(); ++i) {
        if (!KeyComparator::less(keys_[i - 1], keys_[i])) {
            if (!hasFlag(flags(), NodeFlags::AllowDuplicates) ||
                !KeyComparator::equal(keys_[i - 1], keys_[i])) {
                return Status::corruption("InternalNode: keys not sorted");
            }
        }
    }
    return Status::success();
}

Status InternalNode::serializeBody(serialization::BinaryWriter& writer) const {
    if (!children_.empty()) {
        auto st = writer.write(children_.front());
        if (!st.ok()) return st;
    } else {
        auto st = writer.write(format::PageReference::invalid());
        if (!st.ok()) return st;
    }

    for (std::size_t i = 0; i < keys_.size(); ++i) {
        auto st = keys_[i].serialize(writer);
        if (!st.ok()) return st;
        st = writer.write(children_[i + 1]);
        if (!st.ok()) return st;
    }
    return Status::success();
}

Status InternalNode::deserializeBody(serialization::BinaryReader& reader) {
    keys_.clear();
    children_.clear();

    format::PageReference leftChild;
    auto st = reader.read(leftChild);
    if (!st.ok()) return st;
    children_.push_back(leftChild);

    for (std::uint32_t i = 0; i < keyCount(); ++i) {
        Key key;
        st = key.deserialize(reader, config_.binaryKeySize);
        if (!st.ok()) return st;
        format::PageReference rightChild;
        st = reader.read(rightChild);
        if (!st.ok()) return st;
        keys_.push_back(key);
        children_.push_back(rightChild);
    }
    return Status::success();
}

} // namespace btree
} // namespace quartz
