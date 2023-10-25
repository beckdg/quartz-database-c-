#pragma once

#include <cstdint>

namespace quartz {
namespace wal {

/// WAL file format version.
inline constexpr std::uint32_t kWalFormatVersion = 1;

/// Minimum bytes between consecutive records for alignment.
inline constexpr std::uint32_t kLogRecordAlignment = 8;

/// Discriminator for log record payloads.
enum class LogRecordType : std::uint8_t {
    Invalid          = 0,
    PageCreate       = 1,
    PageUpdate       = 2,
    PageDelete       = 3,
    NodeSplit        = 4,
    NodeMerge        = 5,
    Allocation       = 6,
    Deallocation     = 7,
    MetadataUpdate   = 8,
    CheckpointMarker = 9,
};

inline bool isValidRecordType(LogRecordType type) noexcept {
    return type > LogRecordType::Invalid && type <= LogRecordType::CheckpointMarker;
}

} // namespace wal
} // namespace quartz
