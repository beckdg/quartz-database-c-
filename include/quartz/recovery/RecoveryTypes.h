#pragma once

#include "quartz/wal/LogSequenceNumber.h"

#include <cstdint>

namespace quartz {
namespace recovery {

/// Checkpoint payload format version.
inline constexpr std::uint32_t kCheckpointFormatVersion = 1;

/// Magic for serialized checkpoint payloads ("CHPK").
inline constexpr std::uint32_t kCheckpointMagic = 0x4348504Bu;

/// Result of a recovery operation.
struct RecoveryResult {
    bool recoveredFromCheckpoint = false;
    wal::LogSequenceNumber checkpointLsn;
    wal::LogSequenceNumber lastReplayedLsn;
    std::uint64_t recordsReplayed = 0;
    std::uint64_t recordsSkipped = 0;
};

} // namespace recovery
} // namespace quartz
