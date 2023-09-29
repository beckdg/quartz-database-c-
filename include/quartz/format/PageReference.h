#pragma once

#include "quartz/storage/StorageConstants.h"

#include <cstdint>
#include <cstring>

namespace quartz {
namespace format {

#pragma pack(push, 1)
struct PageReference {
    storage::PageId pageId = storage::kInvalidPageId;
    std::uint64_t generation = 0;
    std::uint8_t pageType = 0;
    std::uint8_t flags = 0;
    std::uint16_t reserved1 = 0;
    std::uint32_t checksum = 0;

    static constexpr std::size_t kSize = 20;

    bool isValid() const noexcept {
        return pageId != storage::kInvalidPageId;
    }

    bool operator==(const PageReference& other) const noexcept {
        return pageId == other.pageId && generation == other.generation &&
               pageType == other.pageType && flags == other.flags;
    }
    bool operator!=(const PageReference& other) const noexcept {
        return !(*this == other);
    }

    static PageReference make(storage::PageId id, std::uint64_t gen = 0,
                              std::uint8_t type = 0) noexcept {
        PageReference ref{};
        ref.pageId = id;
        ref.generation = gen;
        ref.pageType = type;
        return ref;
    }

    static PageReference invalid() noexcept {
        return PageReference{};
    }
};
#pragma pack(pop)

static_assert(sizeof(PageReference) == PageReference::kSize,
              "PageReference must be exactly 20 bytes");

} // namespace format
} // namespace quartz
