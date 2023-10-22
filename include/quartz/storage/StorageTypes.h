#pragma once

#include <cstdint>

namespace quartz {
namespace storage {

enum class PageType : std::uint8_t {
    Invalid   = 0,
    Header    = 1,
    Metadata  = 2,
    FreeList  = 3,
    Data      = 4,
    Overflow  = 5,
    Index     = 6,
    Journal   = 7,
    Reserved  = 8
};

enum class PageFlag : std::uint16_t {
    None        = 0,
    Dirty       = 1 << 0,
    Free        = 1 << 1,
    Allocated   = 1 << 2,
    Immutable   = 1 << 3,
    Compressed  = 1 << 4,
    Encrypted   = 1 << 5
};

inline constexpr PageFlag operator|(PageFlag a, PageFlag b) {
    return static_cast<PageFlag>(
        static_cast<std::uint16_t>(a) | static_cast<std::uint16_t>(b));
}

inline constexpr PageFlag operator&(PageFlag a, PageFlag b) {
    return static_cast<PageFlag>(
        static_cast<std::uint16_t>(a) & static_cast<std::uint16_t>(b));
}

inline constexpr bool hasFlag(PageFlag value, PageFlag flag) {
    return (static_cast<std::uint16_t>(value) & static_cast<std::uint16_t>(flag)) != 0;
}

} // namespace storage
} // namespace quartz
