#pragma once

#include "quartz/storage/StorageConstants.h"
#include "quartz/storage/StorageTypes.h"

#include <cstdint>
#include <cstring>

namespace quartz {
namespace storage {

#pragma pack(push, 1)
struct PageHeader {

    std::uint32_t magic;            // File format magic number
    std::uint32_t version;          // Format version
    PageId        pageId;           // Unique page identifier
    PageType      pageType;         // Type of this page
    std::uint8_t  flags;            // Generic flags (reserved)
    std::uint16_t payloadSize;      // Bytes of actual payload in this page
    std::uint16_t reserved1;        // Reserved for future use
    std::uint32_t timestamp;        // Timestamp placeholder (future)
    std::uint32_t checksum;         // Checksum placeholder (future)
    std::uint64_t generation;       // Monotonically increasing generation
    std::uint32_t reserved2[7];     // Future expansion
    std::uint16_t reservedPad;      // Pad to 64 bytes

    static constexpr std::size_t kSize = 64;

    static PageHeader make(PageId id, PageType type) noexcept {
        PageHeader h{};
        h.magic = config::kMagicNumber;
        h.version = config::kFileFormatVersion;
        h.pageId = id;
        h.pageType = type;
        h.payloadSize = static_cast<std::uint16_t>(kPagePayloadSize);
        return h;
    }

    bool isValid() const noexcept {
        return magic == config::kMagicNumber &&
               pageType != PageType::Invalid;
    }
};
#pragma pack(pop)

static_assert(sizeof(PageHeader) == PageHeader::kSize,
              "PageHeader must be exactly 64 bytes");

} // namespace storage
} // namespace quartz
