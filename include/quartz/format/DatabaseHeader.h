#pragma once

#include "quartz/common/Config.h"
#include "quartz/common/Status.h"
#include "quartz/format/FeatureFlags.h"
#include "quartz/format/MagicNumbers.h"
#include "quartz/format/ObjectId.h"
#include "quartz/format/Versioning.h"
#include <cstdint>

namespace quartz {
namespace format {

#pragma pack(push, 1)
struct DatabaseHeader {

    // Fixed-size header section (first 128 bytes)
    std::uint32_t magic;                 // Must match kDatabaseMagic
    std::uint32_t majorVersion;          // Format major version
    std::uint32_t minorVersion;          // Format minor version
    std::uint32_t pageSize;              // Page size in bytes (typically 4096)
    ObjectId     databaseId;             // Unique 128-bit database identifier
    std::uint64_t creationTimestamp;     // Unix timestamp of database creation
    std::uint64_t modificationTimestamp; // Unix timestamp of last modification
    FeatureFlags::ValueType featureFlags; // Feature flag bitfield
    std::uint32_t superblockPageId;      // Page ID containing the superblock
    std::uint32_t headerChecksum;        // CRC-32 of bytes 0-119 (future)
    std::uint64_t reserved[8];           // Reserved expansion space (64 bytes)

    static constexpr std::size_t kSize = 128;

    static DatabaseHeader make() noexcept {
        DatabaseHeader h{};
        h.magic = MagicNumbers::kDatabaseMagic;
        h.majorVersion = Versioning::kMajorVersion;
        h.minorVersion = Versioning::kMinorVersion;
        h.pageSize = config::kPageSize;
        h.databaseId = ObjectId::generate();
        h.superblockPageId = 1;
        return h;
    }

    bool isValid() const noexcept {
        return magic == MagicNumbers::kDatabaseMagic &&
               majorVersion == Versioning::kMajorVersion &&
               pageSize == config::kPageSize;
    }

    Versioning::Version version() const noexcept {
        return Versioning::Version{majorVersion, minorVersion, 0};
    }

    FeatureFlags flags() const noexcept {
        return FeatureFlags(featureFlags);
    }
};
#pragma pack(pop)

static_assert(sizeof(DatabaseHeader) == DatabaseHeader::kSize,
              "DatabaseHeader must be exactly 128 bytes");

} // namespace format
} // namespace quartz
