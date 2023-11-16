#include "quartz/recovery/RecoveryValidator.h"

#include "quartz/recovery/CheckpointManager.h"
#include "quartz/recovery/LogReplayer.h"
#include "quartz/space/SpaceManager.h"

namespace quartz {
namespace recovery {

Status RecoveryValidator::validate(wal::LogManager& wal, const btree::BTree& tree) {
    auto st = wal.validate();
    if (!st.ok()) return st;
    return tree.validate();
}

Status RecoveryValidator::verifyLogicalConsistency(wal::LogManager& wal, const btree::BTree& tree) {
    space::SpaceManager scratchSpace;
    auto scratchTree = btree::BTree::create(scratchSpace, tree.config());

    wal::LogSequenceNumber checkpointLsn = wal::LogSequenceNumber::invalid();
    CheckpointPayload payload;
    auto st = CheckpointManager::findLastCheckpoint(wal, payload, checkpointLsn);
    if (st.ok()) {
        st = CheckpointManager::restoreTree(payload, scratchTree);
        if (!st.ok()) return st;
    }

    RecoveryResult result;
    st = LogReplayer::replay(wal, scratchTree, checkpointLsn, result);
    if (!st.ok()) return st;

    if (scratchTree.size() != tree.size()) {
        return Status::corruption("RecoveryValidator: tree size mismatch after replay");
    }
    st = scratchTree.validate();
    if (!st.ok()) return st;
    return wal.rewindReader();
}

} // namespace recovery
} // namespace quartz
