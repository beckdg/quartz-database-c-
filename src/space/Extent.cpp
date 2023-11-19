#include "quartz/space/Extent.h"

#include <algorithm>

namespace quartz {
namespace space {

bool Extent::intersects(const Extent& other) const noexcept {
    if (!isValid() || !other.isValid()) return false;
    return start < other.endPage() && other.start < endPage();
}

bool Extent::adjacentTo(const Extent& other) const noexcept {
    if (!isValid() || !other.isValid()) return false;
    return endPage() == other.start || other.endPage() == start;
}

std::optional<Extent> Extent::merge(const Extent& a, const Extent& b) {
    if (!a.isValid() || !b.isValid()) return std::nullopt;
    auto newStart = std::min(a.start, b.start);
    auto aEnd = a.start + a.length;
    auto bEnd = b.start + b.length;
    auto newEnd = std::max(aEnd, bEnd);
    if (newEnd <= newStart) return std::nullopt;
    return Extent{newStart, static_cast<std::uint32_t>(newEnd - newStart)};
}

std::optional<std::pair<Extent, Extent>> Extent::split(storage::PageId atPage) const noexcept {
    if (!isValid() || atPage <= start || atPage >= (start + length)) {
        return std::nullopt;
    }
    auto leftLen = static_cast<std::uint32_t>(atPage - start);
    auto rightLen = static_cast<std::uint32_t>(start + length - atPage);
    return std::pair<Extent, Extent>{Extent{start, leftLen}, Extent{atPage, rightLen}};
}

} // namespace space
} // namespace quartz
