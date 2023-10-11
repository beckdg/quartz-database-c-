#pragma once

#include "quartz/common/Config.h"
#include "quartz/storage/Page.h"
#include "quartz/storage/StorageConstants.h"
#include "quartz/storage/StorageTypes.h"
#include "quartz/common/Status.h"

#include <cstdint>
#include <memory>

namespace quartz {
namespace pages {

enum class PageLayoutType : std::uint8_t {
    Invalid       = 0,
    Header        = 1,
    Metadata      = 2,
    FreeList      = 3,
    Data          = 4,
    Index         = 5,
    Overflow      = 6
};

inline constexpr bool isValidLayoutType(PageLayoutType type) noexcept {
    return type >= PageLayoutType::Header && type <= PageLayoutType::Overflow;
}

/// Convert storage page type to semantic layout type (explicit mapping).
inline PageLayoutType toPageLayoutType(storage::PageType type) noexcept {
    switch (type) {
    case storage::PageType::Header:
        return PageLayoutType::Header;
    case storage::PageType::Metadata:
        return PageLayoutType::Metadata;
    case storage::PageType::FreeList:
        return PageLayoutType::FreeList;
    case storage::PageType::Data:
        return PageLayoutType::Data;
    case storage::PageType::Index:
        return PageLayoutType::Index;
    case storage::PageType::Overflow:
        return PageLayoutType::Overflow;
    default:
        return PageLayoutType::Invalid;
    }
}

/// Convert semantic layout type to storage page type.
inline storage::PageType toStoragePageType(PageLayoutType type) noexcept {
    switch (type) {
    case PageLayoutType::Header:
        return storage::PageType::Header;
    case PageLayoutType::Metadata:
        return storage::PageType::Metadata;
    case PageLayoutType::FreeList:
        return storage::PageType::FreeList;
    case PageLayoutType::Data:
        return storage::PageType::Data;
    case PageLayoutType::Index:
        return storage::PageType::Index;
    case PageLayoutType::Overflow:
        return storage::PageType::Overflow;
    default:
        return storage::PageType::Invalid;
    }
}

} // namespace pages
} // namespace quartz
