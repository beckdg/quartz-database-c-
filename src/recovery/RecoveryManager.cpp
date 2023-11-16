#include "quartz/recovery/RecoveryManager.h"

#include "quartz/recovery/CheckpointManager.h"
#include "quartz/recovery/LogReplayer.h"
#include "quartz/recovery/RecoveryValidator.h"

namespace quartz {
namespace recovery {

Status RecoveryManager::recover(wal::LogManager& wal, btree::BTree& tree, RecoveryResult& result) {
    result = {};

    wal::LogSequenceNumber checkpointLsn = wal::LogSequenceNumber::invalid();
    CheckpointPayload payload;

    auto st = CheckpointManager::findLastCheckpoint(wal, payload, checkpointLsn);
    if (st.ok()) {
        st = CheckpointManager::restoreTree(payload, tree);
        if (!st.ok()) return st;
        result.recoveredFromCheckpoint = true;
        result.checkpointLsn = checkpointLsn;
    } else {
        tree.clear();
    }

    st = LogReplayer::replay(wal, tree, checkpointLsn, result);
    if (!st.ok()) return st;

    st = wal.restoreWriterState();
    if (!st.ok()) return st;

    st = wal.rewindReader();
    if (!st.ok()) return st;

    return RecoveryValidator::validate(wal, tree);
}

Status RecoveryManager::checkpoint(btree::BTree& tree, wal::LogManager& wal, bool truncateWal,
                                   wal::LogSequenceNumber& outLsn) {
    auto st = CheckpointManager::createCheckpoint(tree, wal, outLsn);
    if (!st.ok()) return st;
    if (truncateWal) {
        st = CheckpointManager::truncateAfterCheckpoint(wal, outLsn);
        if (!st.ok()) return st;
        st = wal.restoreWriterState();
        if (!st.ok()) return st;
        st = wal.rewindReader();
    }
    return st;
}

} // namespace recovery
} // namespace quartz
