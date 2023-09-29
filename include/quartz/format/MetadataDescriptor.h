#pragma once

#include "quartz/storage/StorageConstants.h"

#include <cstdint>

namespace quartz {
namespace format {

#pragma pack(push, 1)
struct MetadataDescriptor {
    std::uint32_t magic;             // Section magic number
    std::uint32_t typeId;            // Metadata type identifier
    std::uint32_t version;           // Metadata version
    storage::PageId startPage;       // First page of this metadata section
    std::uint32_t pageCount;         // Number of pages used
    std::uint64_t byteSize;          // Total byte size of metadata
    std::uint32_t flags;             // Section flags (reserved)
    std::uint64_t reserved[2];       // Reserved for future use

    static constexpr std::size_t kSize = 48;

    bool isValid() const noexcept {
        return magic != 0 && startPage != storage::kInvalidPageId;
    }

    static MetadataDescriptor make(std::uint32_t sectionMagic,
                                    std::uint32_t type,
                                    storage::PageId page,
                                    std::uint32_t count,
                                    std::uint64_t size) noexcept {
        MetadataDescriptor d{};
        d.magic = sectionMagic;
        d.typeId = type;
        d.version = 1;
        d.startPage = page;
        d.pageCount = count;
        d.byteSize = size;
        return d;
    }
};
#pragma pack(pop)

static_assert(sizeof(MetadataDescriptor) == MetadataDescriptor::kSize,
              "MetadataDescriptor must be exactly 48 bytes");

} // namespace format
} // namespace quartz
