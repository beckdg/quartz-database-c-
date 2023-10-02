#pragma once

#include "quartz/storage/StorageConstants.h"

#include <cstdint>

namespace quartz {
namespace format {

#pragma pack(push, 1)
struct SchemaDescriptor {
    std::uint32_t schemaId;          // Unique schema identifier
    std::uint32_t version;           // Schema version number
    storage::PageId rootPage;        // Page ID of the schema root
    std::uint32_t flags;             // Schema flags (reserved)
    std::uint32_t fieldCount;        // Number of fields in schema
    std::uint64_t reserved[1];       // Reserved for future use
    std::uint32_t reserved2;         // Reserved for future use

    static constexpr std::size_t kSize = 32;

    bool isValid() const noexcept {
        return schemaId != 0 && rootPage != storage::kInvalidPageId;
    }

    static SchemaDescriptor make(std::uint32_t id,
                                  storage::PageId page,
                                  std::uint32_t fields) noexcept {
        SchemaDescriptor d{};
        d.schemaId = id;
        d.version = 1;
        d.rootPage = page;
        d.fieldCount = fields;
        return d;
    }
};
#pragma pack(pop)

static_assert(sizeof(SchemaDescriptor) == SchemaDescriptor::kSize,
              "SchemaDescriptor must be exactly 32 bytes");

} // namespace format
} // namespace quartz
