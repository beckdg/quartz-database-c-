#include "quartz/space/FreeSpaceMap.h"

#include <algorithm>

namespace quartz {
namespace space {

void FreeSpaceMap::initialize(storage::PageId start, std::uint32_t length) {
    extents_.clear();
    if (length > 0) {
        extents_.push_back(Extent{start, length});
    }
}

Status FreeSpaceMap::markAllocated(const Extent& range) {
    if (!range.isValid()) {
        return Status::invalidArgument("FreeSpaceMap::markAllocated: invalid extent");
    }

    if (extents_.empty()) {
        return Status::invalidArgument("FreeSpaceMap::markAllocated: no free extents available");
    }

    // Find first extent that might overlap
    auto it = std::lower_bound(extents_.begin(), extents_.end(), range,
        [](const Extent& e, const Extent& r) { return e.endPage() <= r.start; });

    bool changed = false;
    while (it != extents_.end() && it->start < range.endPage()) {
        Extent current = *it;

        if (range.contains(current)) {
            // Range fully covers this extent — remove it
            auto pagesAffected = current.length;
            it = extents_.erase(it);
            changed = true;
            (void)pagesAffected;
        } else if (current.contains(range)) {
            // Extent fully contains the range — split it
            auto splitResult = current.split(range.start);
            if (splitResult) {
                auto [left, rightEnd] = *splitResult;
                // Remove range's pages from the right part
                auto rightLen = rightEnd.length - range.length;
                *it = left;
                if (rightLen > 0) {
                    auto right = Extent{range.endPage(), rightLen};
                    extents_.insert(it + 1, right);
                }
                if (!left.isValid()) {
                    it = extents_.erase(it);
                }
            }
            return Status::success();
        } else if (current.start < range.start && current.endPage() > range.start) {
            // Range overlaps the end of current extent
            auto newLen = static_cast<std::uint32_t>(range.start - current.start);
            it->length = newLen;
            ++it;
            changed = true;
        } else if (current.start >= range.start && current.start < range.endPage()) {
            // Range overlaps the start of current extent
            auto overlap = static_cast<std::uint32_t>(range.endPage() - current.start);
            auto remaining = current.length - overlap;
            if (remaining > 0) {
                auto shifted = Extent{range.endPage(), remaining};
                *it = shifted;
            } else {
                it = extents_.erase(it);
            }
            changed = true;
        } else {
            ++it;
        }
    }

    if (!changed) {
        return Status::invalidArgument(
            "FreeSpaceMap::markAllocated: extent range does not overlap any free extent");
    }

    return Status::success();
}

Status FreeSpaceMap::markFree(const Extent& range) {
    if (!range.isValid()) {
        return Status::invalidArgument("FreeSpaceMap::markFree: invalid extent");
    }

    // Find insertion point: first extent whose start is >= range.start
    auto it = std::lower_bound(extents_.begin(), extents_.end(), range);

    // Try merging with the extent before the insertion point
    if (it != extents_.begin()) {
        auto prev = it - 1;
        if (prev->canMerge(range)) {
            auto merged = Extent::merge(*prev, range);
            if (merged) {
                *prev = *merged;
                it = prev + 1;
                // Continue to right-merge
                while (it != extents_.end()) {
                    auto& leftExt = *(it - 1);
                    if (leftExt.canMerge(*it)) {
                        auto m = Extent::merge(leftExt, *it);
                        if (m) {
                            leftExt = *m;
                            it = extents_.erase(it);
                            continue;
                        }
                    }
                    break;
                }
                return Status::success();
            }
        }
    }

    // No merge with left: insert the range
    it = extents_.insert(it, range);
    ++it;

    // Merge with following extents if adjacent/overlapping
    while (it != extents_.end()) {
        auto& leftExt = *(it - 1);
        if (leftExt.canMerge(*it)) {
            auto merged = Extent::merge(leftExt, *it);
            if (merged) {
                leftExt = *merged;
                it = extents_.erase(it);
                continue;
            }
        }
        break;
    }

    return Status::success();
}

void FreeSpaceMap::clear() noexcept {
    extents_.clear();
}

Extent FreeSpaceMap::findFreeRange(std::uint32_t length, bool preferBeginning) const {
    if (extents_.empty() || length == 0) return {};

    if (preferBeginning) {
        for (const auto& ext : extents_) {
            if (ext.length >= length) {
                return Extent{ext.start, length};
            }
        }
    } else {
        for (auto it = extents_.rbegin(); it != extents_.rend(); ++it) {
            if (it->length >= length) {
                auto offset = it->length - length;
                return Extent{it->start + offset, length};
            }
        }
    }

    return {};
}

Extent FreeSpaceMap::findExactRange(std::uint32_t length) const {
    for (const auto& ext : extents_) {
        if (ext.length == length) {
            return ext;
        }
    }
    return {};
}

Extent FreeSpaceMap::findBestRange(std::uint32_t length) const {
    const Extent* best = nullptr;
    for (const auto& ext : extents_) {
        if (ext.length >= length) {
            if (!best || ext.length < best->length) {
                best = &ext;
            }
        }
    }
    if (best) {
        return Extent{best->start, length};
    }
    return {};
}

std::uint32_t FreeSpaceMap::countFree() const noexcept {
    std::uint32_t total = 0;
    for (const auto& ext : extents_) {
        total += ext.length;
    }
    return total;
}

bool FreeSpaceMap::contains(storage::PageId page) const noexcept {
    auto it = std::lower_bound(extents_.begin(), extents_.end(), Extent{page, 1});
    if (it != extents_.end() && it->start == page) return true;
    if (it != extents_.begin()) {
        auto prev = it - 1;
        if (prev->contains(page)) return true;
    }
    return false;
}

Status FreeSpaceMap::validate() const noexcept {
    if (extents_.empty()) return Status::success();

    for (std::size_t i = 1; i < extents_.size(); ++i) {
        const auto& prev = extents_[i - 1];
        const auto& curr = extents_[i];

        if (!prev.isValid()) {
            return Status::corruption("FreeSpaceMap: invalid extent at index " +
                                      std::to_string(i - 1));
        }
        if (!curr.isValid()) {
            return Status::corruption("FreeSpaceMap: invalid extent at index " +
                                      std::to_string(i));
        }
        if (prev.start >= curr.start) {
            return Status::corruption("FreeSpaceMap: extents not sorted");
        }
        if (prev.intersects(curr)) {
            return Status::corruption("FreeSpaceMap: overlapping extents");
        }
        if (prev.adjacentTo(curr)) {
            return Status::corruption("FreeSpaceMap: adjacent extents should be merged");
        }
    }

    return Status::success();
}

} // namespace space
} // namespace quartz
