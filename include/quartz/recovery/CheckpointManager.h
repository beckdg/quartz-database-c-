#pragma once

#include "quartz/btree/BTree.h"
#include "quartz/common/Status.h"
#include "quartz/recovery/CheckpointRecord.h"
#include "quartz/wal/LogManager.h"
#include "quartz/wal/LogSequenceNumber.h"

namespace quartz {
namespace recovery {

/// Creates and restores WAL checkpoints containing B-tree snapshots.
class CheckpointManager {
public:
    /// Serializes the tree and appends a CheckpointMarker record.
    static Status createCheckpoint(btree::BTree& tree, wal::LogManager& wal,
                                   wal::LogSequenceNumber& outLsn);

    /// Scans the WAL for the last checkpoint payload.
    static Status findLastCheckpoint(wal::LogManager& wal, CheckpointPayload& outPayload,
                                     wal::LogSequenceNumber& outLsn);

    /// Restores a B-tree from a checkpoint payload.
    static Status restoreTree(const CheckpointPayload& payload, btree::BTree& tree);

    /// Truncates the WAL file after the given LSN (post-checkpoint cleanup).
    static Status truncateAfterCheckpoint(wal::LogManager& wal, wal::LogSequenceNumber lsn);
};

} // namespace recovery
} // namespace quartz
