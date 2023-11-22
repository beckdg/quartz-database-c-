#include "quartz/storage/PageAllocator.h"

#include <algorithm>

namespace quartz {
namespace storage {

PageAllocator::PageAllocator() noexcept
    : nextPageId_(kReservedPageCount) {
}

PageId PageAllocator::allocate() {
    // Reuse a freed page if available
    if (!freeList_.empty()) {
        auto id = freeList_.back();
        freeList_.pop_back();
        stats_.totalAllocated++;
        stats_.currentAllocated++;
        if (stats_.currentAllocated > stats_.peakAllocated) {
            stats_.peakAllocated = stats_.currentAllocated;
        }
        stats_.freeListSize = static_cast<std::uint32_t>(freeList_.size());
        return id;
    }

    // Allocate a new page ID
    if (nextPageId_ > kMaxPageId) {
        return kInvalidPageId;
    }

    auto id = nextPageId_++;
    stats_.totalAllocated++;
    stats_.currentAllocated++;
    if (stats_.currentAllocated > stats_.peakAllocated) {
        stats_.peakAllocated = stats_.currentAllocated;
    }
    stats_.freeListSize = static_cast<std::uint32_t>(freeList_.size());
    return id;
}

bool PageAllocator::free(PageId id) noexcept {
    if (id == kInvalidPageId || id < kReservedPageCount) {
        return false;
    }
    if (id >= nextPageId_) {
        return false;
    }

    // Check if already freed
    auto it = std::find(freeList_.begin(), freeList_.end(), id);
    if (it != freeList_.end()) {
        return false;
    }

    freeList_.push_back(id);
    stats_.totalFreed++;
    stats_.currentAllocated--;
    stats_.freeListSize = static_cast<std::uint32_t>(freeList_.size());
    return true;
}

void PageAllocator::reset() noexcept {
    freeList_.clear();
    nextPageId_ = kReservedPageCount;
    stats_ = AllocatorStats{};
}

bool PageAllocator::isAllocated(PageId id) const noexcept {
    if (id == kInvalidPageId) {
        return false;
    }
    if (id >= nextPageId_) {
        return false;
    }
    if (id < kReservedPageCount) {
        return true;
    }
    auto it = std::find(freeList_.begin(), freeList_.end(), id);
    return it == freeList_.end();
}

} // namespace storage
} // namespace quartz
