#pragma once

#include "quartz/btree/BTree.h"
#include "quartz/common/Status.h"
#include "quartz/space/SpaceManager.h"
#include "quartz/storage/DatabaseFile.h"
#include "quartz/wal/BTreeWalAdapter.h"
#include "quartz/wal/LogManager.h"

namespace quartz {
namespace core {

/// Non-owning view of active database subsystems.
struct DatabaseContext {
    storage::DatabaseFile* file = nullptr;
    space::SpaceManager* space = nullptr;
    btree::BTree* tree = nullptr;
    wal::LogManager* wal = nullptr;
    wal::BTreeWalAdapter* walAdapter = nullptr;
};

} // namespace core
} // namespace quartz
