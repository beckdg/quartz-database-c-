#pragma once

#include "quartz/recovery/RecoveryTypes.h"
#include "quartz/common/Status.h"
#include "quartz/serialization/BinaryReader.h"
#include "quartz/serialization/BinaryWriter.h"
#include "quartz/serialization/Buffer.h"
#include "quartz/wal/LogSequenceNumber.h"

#include <cstdint>

namespace quartz {
namespace recovery {

/// Serialized checkpoint payload embedded in a WAL CheckpointMarker record.
struct CheckpointPayload {
    std::uint32_t magic = kCheckpointMagic;
    std::uint32_t version = kCheckpointFormatVersion;
    wal::LogSequenceNumber lsn;
    std::uint64_t treeSize = 0;
    std::uint32_t treeHeight = 0;
    serialization::Buffer btreeSnapshot;

    Status serialize(serialization::BinaryWriter& writer) const;
    Status deserialize(serialization::BinaryReader& reader);

    Status validate() const;
};

} // namespace recovery
} // namespace quartz
