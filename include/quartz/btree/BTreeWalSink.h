#pragma once

#include "quartz/btree/Key.h"
#include "quartz/common/Status.h"
#include "quartz/format/PageReference.h"
#include "quartz/storage/StorageConstants.h"

namespace quartz {
namespace btree {

/// Abstract sink for optional WAL record emission from B-tree operations.
/// Implemented by the WAL layer; keeps B-tree independent of wal:: types.
class BTreeWalSink {
public:
    virtual ~BTreeWalSink() = default;

    virtual Status onInsert(const Key& key, format::PageReference value) = 0;
    virtual Status onErase(const Key& key) = 0;
    virtual Status onPageAllocate(storage::PageId pageId) = 0;
    virtual Status onPageDeallocate(storage::PageId pageId) = 0;
    virtual Status onNodeSplit(storage::PageId leftId, storage::PageId rightId,
                               const Key& promotedKey) = 0;
    virtual Status onNodeMerge(storage::PageId survivorId, storage::PageId removedId) = 0;
};

} // namespace btree
} // namespace quartz
