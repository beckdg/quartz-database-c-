#pragma once

#include "quartz/common/Config.h"
#include "quartz/format/DatabaseHeader.h"
#include "quartz/format/FeatureFlags.h"
#include "quartz/format/Superblock.h"
#include "quartz/storage/StorageConstants.h"

#include <cstdint>

namespace quartz {
namespace pages {

inline constexpr std::size_t kPagePayloadSize = storage::kPagePayloadSize;

#pragma pack(push, 1)

struct HeaderPageLayout {
    format::DatabaseHeader databaseHeader;
    storage::PageId        superblockPageId;
    std::uint64_t          flags;
    std::uint32_t          formatVersion;
    std::uint64_t          reserved[486];

    static constexpr std::size_t kSize = kPagePayloadSize;

    bool isValid() const noexcept {
        return databaseHeader.isValid() && formatVersion > 0;
    }
};
static_assert(sizeof(HeaderPageLayout) == HeaderPageLayout::kSize,
              "HeaderPageLayout must fill exactly one page payload");

struct FreeListPageLayout {
    std::uint32_t    freeCount;
    std::uint32_t    capacity;
    std::uint64_t    reserved[3];
    storage::PageId  freePages[1000];

    static constexpr std::size_t kSize = kPagePayloadSize;

    bool isValid() const noexcept {
        return capacity > 0 && freeCount <= capacity;
    }
};
static_assert(sizeof(FreeListPageLayout) == FreeListPageLayout::kSize,
              "FreeListPageLayout must fill exactly one page payload");

struct DataPageLayout {
    std::uint16_t freeSpaceOffset;
    std::uint16_t slotCount;
    std::uint32_t reserved1;
    std::uint64_t reserved2[2];
    std::uint8_t  data[4008];

    static constexpr std::size_t kSize = kPagePayloadSize;

    bool isValid() const noexcept {
        return freeSpaceOffset <= sizeof(data);
    }
    std::size_t availableSpace() const noexcept {
        return sizeof(data) - freeSpaceOffset;
    }
};
static_assert(sizeof(DataPageLayout) == DataPageLayout::kSize,
              "DataPageLayout must fill exactly one page payload");

struct OverflowPageLayout {
    storage::PageId nextPageId;
    std::uint32_t   payloadSize;
    std::uint64_t   reserved[2];
    std::uint8_t    data[4008];

    static constexpr std::size_t kSize = kPagePayloadSize;

    bool isValid() const noexcept {
        return payloadSize <= sizeof(data);
    }
    std::size_t remainingCapacity() const noexcept {
        return sizeof(data) - payloadSize;
    }
};
static_assert(sizeof(OverflowPageLayout) == OverflowPageLayout::kSize,
              "OverflowPageLayout must fill exactly one page payload");

struct IndexPageLayout {
    std::uint32_t nodeType;
    std::uint32_t keyCount;
    std::uint32_t capacity;
    std::uint32_t flags;
    std::uint64_t reserved[3];
    std::uint8_t  data[3992];

    static constexpr std::size_t kSize = kPagePayloadSize;

    bool isValid() const noexcept {
        return capacity > 0 && keyCount <= capacity;
    }
};
static_assert(sizeof(IndexPageLayout) == IndexPageLayout::kSize,
              "IndexPageLayout must fill exactly one page payload");

struct MetadataPageLayout {
    std::uint32_t entryCount;
    std::uint32_t version;
    std::uint64_t reserved[3];
    std::uint8_t  data[4000];

    static constexpr std::size_t kSize = kPagePayloadSize;

    bool isValid() const noexcept {
        return version > 0;
    }
};
static_assert(sizeof(MetadataPageLayout) == MetadataPageLayout::kSize,
              "MetadataPageLayout must fill exactly one page payload");

#pragma pack(pop)

} // namespace pages
} // namespace quartz
