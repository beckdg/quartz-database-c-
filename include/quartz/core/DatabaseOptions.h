#pragma once

#include "quartz/btree/BTreeTypes.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace quartz {
namespace core {

/// Runtime configuration for a Database instance.
struct DatabaseOptions {
    std::string dataPath;
    bool createIfMissing = true;
    bool enableWal = true;
    bool recoverOnOpen = true;
    bool truncateWalOnCheckpoint = false;
    std::size_t walBufferCapacity = 64 * 1024;
    btree::BTreeNodeConfig btreeConfig;
};

} // namespace core
} // namespace quartz
