#pragma once

#include "quartz/btree/BTree.h"
#include "quartz/common/Status.h"
#include "quartz/recovery/RecoveryTypes.h"
#include "quartz/wal/LogManager.h"
#include "quartz/wal/LogSequenceNumber.h"

namespace quartz {
namespace recovery {

/// Replays WAL records into a B-tree without emitting new log entries.
class LogReplayer {
public:
    /// Replays records with LSN strictly greater than \p afterLsn.
    static Status replay(wal::LogManager& wal, btree::BTree& tree, wal::LogSequenceNumber afterLsn,
                         RecoveryResult& result);

    /// Applies a single record to the tree (logical records only).
    static Status applyRecord(const wal::LogRecord& record, btree::BTree& tree);
};

} // namespace recovery
} // namespace quartz
