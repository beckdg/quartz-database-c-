#pragma once

#include "quartz/storage/StorageConstants.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>

namespace quartz {
namespace space {

struct Extent {
    storage::PageId start = storage::kInvalidPageId;
    std::uint32_t length = 0;

    bool isValid() const noexcept { return length > 0 && start != storage::kInvalidPageId; }

    storage::PageId firstPage() const noexcept { return start; }
    storage::PageId lastPage() const noexcept { return start + length - 1; }
    storage::PageId endPage() const noexcept { return start + length; }

    bool contains(storage::PageId page) const noexcept {
        return page >= start && page < (start + length);
    }

    bool contains(const Extent& other) const noexcept {
        return other.isValid() && other.start >= start &&
               (other.start + other.length) <= (start + length);
    }

    bool intersects(const Extent& other) const noexcept;

    bool adjacentTo(const Extent& other) const noexcept;

    bool canMerge(const Extent& other) const noexcept {
        return intersects(other) || adjacentTo(other);
    }

    static std::optional<Extent> merge(const Extent& a, const Extent& b);

    std::optional<std::pair<Extent, Extent>> split(storage::PageId atPage) const noexcept;

    bool operator==(const Extent& rhs) const noexcept {
        return start == rhs.start && length == rhs.length;
    }
    bool operator<(const Extent& rhs) const noexcept { return start < rhs.start; }
};

} // namespace space
} // namespace quartz
