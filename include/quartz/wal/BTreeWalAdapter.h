#pragma once

#include "quartz/btree/BTreeWalSink.h"
#include "quartz/wal/LogManager.h"

namespace quartz {
namespace wal {

/// Adapts LogManager to the BTreeWalSink interface.
class BTreeWalAdapter final : public btree::BTreeWalSink {
public:
    explicit BTreeWalAdapter(LogManager& manager);

    Status onInsert(const btree::Key& key, format::PageReference value) override;
    Status onErase(const btree::Key& key) override;
    Status onPageAllocate(storage::PageId pageId) override;
    Status onPageDeallocate(storage::PageId pageId) override;
    Status onNodeSplit(storage::PageId leftId, storage::PageId rightId,
                       const btree::Key& promotedKey) override;
    Status onNodeMerge(storage::PageId survivorId, storage::PageId removedId) override;

private:
    LogManager& manager_;

    Status appendKeyRecord(LogRecordType type, storage::PageId pageId, const btree::Key& key,
                         format::PageReference ref = format::PageReference::invalid());
};

} // namespace wal
} // namespace quartz
