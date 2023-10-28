#include "quartz/btree/BTree.h"

#include "quartz/btree/Cursor.h"
#include "quartz/btree/InternalNode.h"
#include "quartz/btree/KeyComparator.h"
#include "quartz/btree/LeafNode.h"
#include "quartz/btree/Rebalance.h"
#include "quartz/btree/TreeValidator.h"
#include "quartz/space/SpaceManager.h"
#include "quartz/storage/StorageTypes.h"

namespace quartz {
namespace btree {

BTree::BTree(space::SpaceManager& space, BTreeNodeConfig config)
    : space_(space), config_(config) {}

BTree::~BTree() {
    clear();
}

BTree BTree::create(space::SpaceManager& space, BTreeNodeConfig config) {
    return BTree(space, config);
}

void BTree::clear() {
    for (const auto& entry : leaves_) {
        space_.freePage(entry.first);
    }
    for (const auto& entry : internals_) {
        space_.freePage(entry.first);
    }
    leaves_.clear();
    internals_.clear();
    rootId_ = storage::kInvalidPageId;
    size_ = 0;
    height_ = 0;
    opStats_.reset();
}

bool BTree::empty() const noexcept {
    return rootId_ == storage::kInvalidPageId;
}

std::size_t BTree::size() const noexcept {
    return size_;
}

std::uint32_t BTree::height() const noexcept {
    return height_;
}

storage::PageId BTree::rootId() const noexcept {
    return rootId_;
}

bool BTree::isLeaf(storage::PageId id) const {
    return leaves_.find(id) != leaves_.end();
}

bool BTree::isInternal(storage::PageId id) const {
    return internals_.find(id) != internals_.end();
}

LeafNode& BTree::leaf(storage::PageId id) {
    return leaves_.at(id);
}

InternalNode& BTree::internal(storage::PageId id) {
    return internals_.at(id);
}

const LeafNode& BTree::leaf(storage::PageId id) const {
    return leaves_.at(id);
}

const InternalNode& BTree::internal(storage::PageId id) const {
    return internals_.at(id);
}

format::PageReference BTree::makeIndexRef(storage::PageId id) const {
    return format::PageReference::make(id, 1,
                                       static_cast<std::uint8_t>(storage::PageType::Index));
}

void BTree::setRoot(storage::PageId id) noexcept {
    rootId_ = id;
}

void BTree::setHeight(std::uint32_t h) noexcept {
    height_ = h;
}

void BTree::setWalSink(BTreeWalSink* sink) noexcept {
    walSink_ = sink;
}

Status BTree::walOnInsert(const Key& key, format::PageReference value) {
    if (walSink_ == nullptr) return Status::success();
    return walSink_->onInsert(key, value);
}

Status BTree::walOnErase(const Key& key) {
    if (walSink_ == nullptr) return Status::success();
    return walSink_->onErase(key);
}

Status BTree::walOnPageAllocate(storage::PageId pageId) {
    if (walSink_ == nullptr) return Status::success();
    return walSink_->onPageAllocate(pageId);
}

Status BTree::walOnPageDeallocate(storage::PageId pageId) {
    if (walSink_ == nullptr) return Status::success();
    return walSink_->onPageDeallocate(pageId);
}

Status BTree::walOnNodeSplit(storage::PageId leftId, storage::PageId rightId, const Key& promoted) {
    if (walSink_ == nullptr) return Status::success();
    return walSink_->onNodeSplit(leftId, rightId, promoted);
}

Status BTree::walOnNodeMerge(storage::PageId survivorId, storage::PageId removedId) {
    if (walSink_ == nullptr) return Status::success();
    return walSink_->onNodeMerge(survivorId, removedId);
}

Status BTree::allocateLeaf(LeafNode*& out, std::uint32_t level) {
    const auto pageId = space_.allocatePage();
    if (pageId == storage::kInvalidPageId) {
        return Status::outOfMemory("BTree: failed to allocate leaf page");
    }
    opStats_.recordAllocation();

    BTreeNodeConfig nodeConfig = config_;
    nodeConfig.level = level;
    auto [iter, inserted] = leaves_.emplace(pageId, LeafNode::create(pageId, nodeConfig));
    if (!inserted) {
        return Status::corruption("BTree: duplicate leaf page id");
    }
    out = &iter->second;
    return walOnPageAllocate(pageId);
}

Status BTree::allocateInternal(InternalNode*& out, std::uint32_t level) {
    const auto pageId = space_.allocatePage();
    if (pageId == storage::kInvalidPageId) {
        return Status::outOfMemory("BTree: failed to allocate internal page");
    }
    opStats_.recordAllocation();

    BTreeNodeConfig nodeConfig = config_;
    nodeConfig.level = level;
    auto [iter, inserted] = internals_.emplace(pageId, InternalNode::create(pageId, nodeConfig));
    if (!inserted) {
        return Status::corruption("BTree: duplicate internal page id");
    }
    out = &iter->second;
    return walOnPageAllocate(pageId);
}

void BTree::releaseNode(storage::PageId id) {
    (void)walOnPageDeallocate(id);
    if (isLeaf(id)) {
        leaves_.erase(id);
    } else if (isInternal(id)) {
        internals_.erase(id);
    }
    space_.freePage(id);
}

Status BTree::descendToLeaf(const Key& key, SearchPath& path, LeafNode*& leaf,
                            std::size_t& index, bool exactMatch) const {
    if (empty()) {
        return Status::invalidArgument("BTree: tree is empty");
    }

    storage::PageId current = rootId_;
    while (true) {
        if (isLeaf(current)) {
            leaf = const_cast<LeafNode*>(&this->leaf(current));
            if (exactMatch) {
                index = leaf->find(key);
                if (index >= leaf->keyCount() || !KeyComparator::equal(leaf->keyAt(index), key)) {
                    return Status::invalidArgument("BTree: key not found");
                }
            } else {
                index = leaf->lowerBound(key);
            }
            return Status::success();
        }

        const auto& node = internal(current);
        const auto childIndex = node.lowerBound(key);
        const auto childRef = node.childAt(childIndex);
        if (!childRef.isValid()) {
            return Status::corruption("BTree: invalid child reference");
        }

        SearchPathEntry entry;
        entry.pageId = current;
        entry.childIndex = static_cast<std::uint32_t>(childIndex);
        entry.pageRef = makeIndexRef(current);
        path.push(entry);
        current = childRef.pageId;
    }
}

Status BTree::descendForInsert(const Key& key, SearchPath& path, LeafNode*& leaf,
                               std::size_t& index) const {
    auto st = descendToLeaf(key, path, leaf, index, false);
    if (!st.ok()) {
        return st;
    }
    if (!config_.allowDuplicates && index < leaf->keyCount() &&
        KeyComparator::equal(leaf->keyAt(index), key)) {
        return Status::invalidArgument("BTree: duplicate key");
    }
    return Status::success();
}

bool BTree::contains(const Key& key) const {
    format::PageReference ignored;
    return find(key, ignored).ok();
}

Status BTree::find(const Key& key, format::PageReference& out) const {
    SearchPath path;
    LeafNode* leaf = nullptr;
    std::size_t index = 0;
    auto st = descendToLeaf(key, path, leaf, index, true);
    if (!st.ok()) {
        return st;
    }
    out = leaf->referenceAt(index);
    return Status::success();
}

Status BTree::lowerBound(const Key& key, Cursor& cursor) const {
    if (empty()) {
        cursor.bindTree(this, true);
        return Status::success();
    }
    cursor.bindTree(this, false);
    SearchPath path;
    LeafNode* leaf = nullptr;
    std::size_t index = 0;
    auto st = descendToLeaf(key, path, leaf, index, false);
    if (!st.ok()) {
        return st;
    }
    cursor.leafId_ = leaf->pageId();
    cursor.index_ = index;
    if (index >= leaf->keyCount()) {
        return cursor.advanceToNextLeaf();
    }
    return Status::success();
}

Status BTree::upperBound(const Key& key, Cursor& cursor) const {
    if (empty()) {
        cursor.bindTree(this, true);
        return Status::success();
    }
    cursor.bindTree(this, false);
    SearchPath path;
    LeafNode* leaf = nullptr;
    std::size_t index = 0;
    auto st = descendToLeaf(key, path, leaf, index, false);
    if (!st.ok()) {
        return st;
    }
    index = leaf->upperBound(key);
    cursor.leafId_ = leaf->pageId();
    cursor.index_ = index;
    if (index >= leaf->keyCount()) {
        return cursor.advanceToNextLeaf();
    }
    return Status::success();
}

Status BTree::insert(const Key& key, format::PageReference value) {
    if (!value.isValid()) {
        return Status::invalidArgument("BTree: invalid page reference");
    }

    if (empty()) {
        LeafNode* leaf = nullptr;
        auto st = allocateLeaf(leaf, 0);
        if (!st.ok()) {
            return st;
        }
        st = leaf->insert(key, value);
        if (!st.ok()) {
            return st;
        }
        rootId_ = leaf->pageId();
        height_ = 1;
        ++size_;
        return walOnInsert(key, value);
    }

    SearchPath path;
    LeafNode* leaf = nullptr;
    std::size_t index = 0;
    auto st = descendForInsert(key, path, leaf, index);
    if (!st.ok()) {
        return st;
    }

    if (leaf->keyCount() < leaf->capacity()) {
        st = leaf->insert(key, value);
        if (!st.ok()) {
            return st;
        }
        ++size_;
        return walOnInsert(key, value);
    }

    st = Rebalance::splitAndInsertLeaf(*this, *leaf, path, key, value);
    if (st.ok()) {
        ++size_;
        st = walOnInsert(key, value);
    }
    return st;
}

Status BTree::erase(const Key& key) {
    if (empty()) {
        return Status::invalidArgument("BTree: key not found");
    }

    SearchPath path;
    LeafNode* leaf = nullptr;
    std::size_t index = 0;
    auto st = descendToLeaf(key, path, leaf, index, true);
    if (!st.ok()) {
        return st;
    }

    st = leaf->eraseAt(index);
    if (!st.ok()) {
        return st;
    }
    --size_;

    st = walOnErase(key);
    if (!st.ok()) {
        return st;
    }

    if (path.empty()) {
        if (leaf->keyCount() == 0) {
            releaseNode(leaf->pageId());
            rootId_ = storage::kInvalidPageId;
            height_ = 0;
        }
        return Status::success();
    }

    if (!isUnderfull(leaf->keyCount(), leaf->capacity(), false)) {
        return Status::success();
    }

    return Rebalance::fixLeafUnderflow(*this, *leaf, path);
}

Status BTree::collapseRootIfNeeded() {
    if (empty()) {
        return Status::success();
    }
    if (!isInternal(rootId_)) {
        return Status::success();
    }

  auto& root = internal(rootId_);
    if (root.keyCount() == 0 && root.children().size() == 1) {
        const auto newRoot = root.childAt(0).pageId;
        releaseNode(rootId_);
        rootId_ = newRoot;
        if (height_ > 0) {
            --height_;
        }
    }
    return Status::success();
}

Status BTree::validate() const {
    return TreeValidator::validate(*this);
}

TreeStatistics BTree::statistics() const {
    TreeStatistics stats;
    stats.keyCount = size_;
    stats.height = height_;
    stats.maxDepth = height_;
    stats.operations = opStats_.operations();

    if (empty()) {
        return stats;
    }

    double occupancySum = 0.0;
    for (const auto& entry : leaves_) {
        ++stats.leafCount;
        occupancySum += entry.second.occupancyPercent();
    }
    for (const auto& entry : internals_) {
        ++stats.internalCount;
        occupancySum += entry.second.occupancyPercent();
    }
    stats.nodeCount = stats.leafCount + stats.internalCount;
    if (stats.nodeCount > 0) {
        stats.averageOccupancy = occupancySum / static_cast<double>(stats.nodeCount);
    }
    return stats;
}

Cursor BTree::begin() const {
    Cursor cursor;
    cursor.bindTree(this, false);
    (void)cursor.seekBegin();
    return cursor;
}

Cursor BTree::end() const {
    Cursor cursor;
    cursor.bindTree(this, true);
    return cursor;
}

Status BTree::serialize(serialization::BinaryWriter& writer) const {
    auto st = writer.write(rootId_);
    if (!st.ok()) return st;
    st = writer.write(static_cast<std::uint64_t>(size_));
    if (!st.ok()) return st;
    st = writer.write(height_);
    if (!st.ok()) return st;

    const auto keyType = static_cast<std::uint8_t>(config_.keyType);
    st = writer.write(keyType);
    if (!st.ok()) return st;
    st = writer.write(config_.binaryKeySize);
    if (!st.ok()) return st;
    const auto allowDup = static_cast<std::uint8_t>(config_.allowDuplicates ? 1 : 0);
    st = writer.write(allowDup);
    if (!st.ok()) return st;

    const std::uint32_t nodeCount =
        static_cast<std::uint32_t>(leaves_.size() + internals_.size());
    st = writer.write(nodeCount);
    if (!st.ok()) return st;

    for (const auto& entry : leaves_) {
        st = entry.second.serialize(writer);
        if (!st.ok()) return st;
    }
    for (const auto& entry : internals_) {
        st = entry.second.serialize(writer);
        if (!st.ok()) return st;
    }
    return Status::success();
}

Status BTree::deserialize(serialization::BinaryReader& reader) {
    clear();

    auto st = reader.read(rootId_);
    if (!st.ok()) return st;
    std::uint64_t storedSize = 0;
    st = reader.read(storedSize);
    if (!st.ok()) return st;
    size_ = static_cast<std::size_t>(storedSize);
    st = reader.read(height_);
    if (!st.ok()) return st;

    std::uint8_t keyTypeByte = 0;
    st = reader.read(keyTypeByte);
    if (!st.ok()) return st;
    config_.keyType = static_cast<KeyType>(keyTypeByte);
    st = reader.read(config_.binaryKeySize);
    if (!st.ok()) return st;
    std::uint8_t allowDup = 0;
    st = reader.read(allowDup);
    if (!st.ok()) return st;
    config_.allowDuplicates = allowDup != 0;

    std::uint32_t nodeCount = 0;
    st = reader.read(nodeCount);
    if (!st.ok()) return st;

    for (std::uint32_t i = 0; i < nodeCount; ++i) {
        pages::IndexPage page;
        st = page.deserialize(reader);
        if (!st.ok()) return st;
        const auto id = page.id();
        const auto type = static_cast<NodeType>(page.nodeType());
        if (type == NodeType::Leaf) {
            leaves_.emplace(id, LeafNode::fromPage(std::move(page)));
        } else if (type == NodeType::Internal) {
            internals_.emplace(id, InternalNode::fromPage(std::move(page)));
        } else {
            return Status::corruption("BTree: unknown node type in snapshot");
        }
    }
    return Status::success();
}

} // namespace btree
} // namespace quartz
