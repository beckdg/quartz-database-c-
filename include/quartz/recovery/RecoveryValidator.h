#pragma once

#include "quartz/btree/BTree.h"
#include "quartz/common/Status.h"
#include "quartz/recovery/RecoveryTypes.h"
#include "quartz/wal/LogManager.h"

namespace quartz {
namespace recovery {

/// Validates post-recovery consistency between WAL and B-tree state.
class RecoveryValidator {
public:
    /// Verifies the WAL is internally consistent and the tree passes validation.
    static Status validate(wal::LogManager& wal, const btree::BTree& tree);

    /// Replays the WAL into a fresh tree and compares sizes with the live tree.
    static Status verifyLogicalConsistency(wal::LogManager& wal, const btree::BTree& tree);
};

} // namespace recovery
} // namespace quartz
