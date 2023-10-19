#pragma once

#include "quartz/common/NonCopyable.h"
#include "quartz/storage/StorageConstants.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace quartz {
namespace storage {

struct AllocatorStats {
    std::uint32_t totalAllocated = 0;
    std::uint32_t totalFreed = 0;
    std::uint32_t peakAllocated = 0;
    std::uint32_t currentAllocated = 0;
    std::uint32_t freeListSize = 0;
    std::uint32_t nextPageId = kReservedPageCount;
};

class PageAllocator : private NonCopyable {
public:
    PageAllocator() noexcept;

    PageId allocate();
    bool free(PageId id) noexcept;
    void reset() noexcept;

    bool isAllocated(PageId id) const noexcept;
    std::uint32_t allocatedCount() const noexcept { return stats_.currentAllocated; }
    std::uint32_t peakCount() const noexcept { return stats_.peakAllocated; }

    const AllocatorStats& stats() const noexcept { return stats_; }

    std::size_t freeListCount() const noexcept { return freeList_.size(); }

private:
    std::vector<PageId> freeList_;
    PageId nextPageId_ = kReservedPageCount;
    AllocatorStats stats_;
};

} // namespace storage
} // namespace quartz
