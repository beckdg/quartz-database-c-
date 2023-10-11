#pragma once

#include "quartz/btree/BTree.h"
#include "quartz/common/Status.h"
#include "quartz/recovery/RecoveryTypes.h"
#include "quartz/wal/LogManager.h"

namespace quartz {
namespace recovery {

/// Orchestrates crash recovery from WAL checkpoints and incremental replay.
class RecoveryManager {
public:
    /// Performs full recovery: restore checkpoint (if any) and replay subsequent records.
    static Status recover(wal::LogManager& wal, btree::BTree& tree, RecoveryResult& result);

    /// Creates a checkpoint, optionally truncating the WAL afterward.
    static Status checkpoint(btree::BTree& tree, wal::LogManager& wal, bool truncateWal,
                             wal::LogSequenceNumber& outLsn);
};

} // namespace recovery
} // namespace quartz
