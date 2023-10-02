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
struct Superblock {

    // Identity
    std::uint32_t magic;                 // Must match kSuperblockMagic
    ObjectId      databaseId;            // Matches DatabaseHeader::databaseId

    // Page accounting
    std::uint64_t totalPages;            // Total pages in database
    std::uint64_t allocatedPages;        // Pages currently allocated
    std::uint64_t freePages;             // Pages currently free
    std::uint64_t reservedPages;         // Reserved system pages (typically 8)

    // Allocation state
    std::uint32_t firstFreePage;         // Page ID of first free page
    std::uint32_t lastAllocatedPage;     // Highest allocated page ID
    std::uint32_t freeListPage;          // Page ID of the free list root
    std::uint32_t metadataPage;          // Page ID of the metadata section

    // Feature and compatibility
    FeatureFlags::ValueType featureFlags;
    std::uint32_t minReaderVersion;      // Encoded as (major << 16) | minor

    // Reserved for future use
    std::uint64_t reserved[6];

    static constexpr std::size_t kSize = 128;

    static Superblock make(const ObjectId& dbId) noexcept {
        Superblock s{};
        s.magic = MagicNumbers::kSuperblockMagic;
        s.databaseId = dbId;
        s.totalPages = config::kInitialFileSize / config::kPageSize;
        s.allocatedPages = config::kReservedPages;
        s.freePages = s.totalPages - config::kReservedPages;
        s.reservedPages = config::kReservedPages;
        s.firstFreePage = config::kReservedPages;
        s.freeListPage = 2;
        s.metadataPage = 1;
        s.minReaderVersion = Versioning::encodeVersion(
            Versioning::kMajorVersion, Versioning::kMinorVersion);
        return s;
    }

    bool isValid() const noexcept {
        return magic == MagicNumbers::kSuperblockMagic;
    }

    bool isConsistent() const noexcept {
        if (!isValid()) return false;
        if (allocatedPages + freePages != totalPages) return false;
        return true;
    }
};
#pragma pack(pop)

static_assert(sizeof(Superblock) == Superblock::kSize,
              "Superblock must be exactly 128 bytes");

} // namespace format
} // namespace quartz
