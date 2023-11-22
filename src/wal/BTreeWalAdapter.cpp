#include "quartz/wal/BTreeWalAdapter.h"

#include "quartz/serialization/BinaryWriter.h"
#include "quartz/serialization/Buffer.h"
#include "quartz/wal/LogTypes.h"

namespace quartz {
namespace wal {

BTreeWalAdapter::BTreeWalAdapter(LogManager& manager)
    : manager_(manager) {}

Status BTreeWalAdapter::appendKeyRecord(LogRecordType type, storage::PageId pageId,
                                        const btree::Key& key, format::PageReference ref) {
    auto record = LogRecord::make(type, pageId);
    serialization::Buffer buf;
    serialization::BinaryWriter writer(buf);
    auto st = key.serialize(writer);
    if (!st.ok()) return st;
    if (ref.isValid()) {
        st = writer.write(ref);
        if (!st.ok()) return st;
    }
    record.payload() = std::move(buf);
    return manager_.append(std::move(record));
}

Status BTreeWalAdapter::onInsert(const btree::Key& key, format::PageReference value) {
    return appendKeyRecord(LogRecordType::PageUpdate, value.pageId, key, value);
}

Status BTreeWalAdapter::onErase(const btree::Key& key) {
    return appendKeyRecord(LogRecordType::PageDelete, storage::kInvalidPageId, key);
}

Status BTreeWalAdapter::onPageAllocate(storage::PageId pageId) {
    auto record = LogRecord::make(LogRecordType::Allocation, pageId);
    return manager_.append(std::move(record));
}

Status BTreeWalAdapter::onPageDeallocate(storage::PageId pageId) {
    auto record = LogRecord::make(LogRecordType::Deallocation, pageId);
    return manager_.append(std::move(record));
}

Status BTreeWalAdapter::onNodeSplit(storage::PageId leftId, storage::PageId rightId,
                                    const btree::Key& promotedKey) {
    auto record = LogRecord::make(LogRecordType::NodeSplit, leftId);
    serialization::Buffer buf;
    serialization::BinaryWriter writer(buf);
    auto st = writer.write(rightId);
    if (!st.ok()) return st;
    st = promotedKey.serialize(writer);
    if (!st.ok()) return st;
    record.payload() = std::move(buf);
    return manager_.append(std::move(record));
}

Status BTreeWalAdapter::onNodeMerge(storage::PageId survivorId, storage::PageId removedId) {
    auto record = LogRecord::make(LogRecordType::NodeMerge, survivorId);
    serialization::Buffer buf;
    serialization::BinaryWriter writer(buf);
    auto st = writer.write(removedId);
    if (!st.ok()) return st;
    record.payload() = std::move(buf);
    return manager_.append(std::move(record));
}

} // namespace wal
} // namespace quartz
