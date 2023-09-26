#pragma once

#include "quartz/btree/TreeStatistics.h"
#include "quartz/space/SpaceStatistics.h"
#include "quartz/wal/LogStatistics.h"

#include <cstdint>

namespace quartz {
namespace core {

/// Aggregate engine statistics from all subsystems.
struct DatabaseStatistics {
    std::uint64_t openCount = 0;
    std::uint64_t checkpointCount = 0;
    std::uint64_t recoveryCount = 0;
    btree::TreeStatistics btree;
    space::SpaceStats space;
    wal::LogStatistics wal;
};

} // namespace core
} // namespace quartz
