#pragma once

#include "quartz/common/NonCopyable.h"
#include "quartz/common/Status.h"
#include "quartz/space/AllocationHints.h"
#include "quartz/space/AllocationPolicy.h"
#include "quartz/space/Extent.h"
#include "quartz/space/ExtentAllocator.h"
#include "quartz/space/SpaceStatistics.h"
#include "quartz/storage/PageAllocator.h"

#include <memory>

namespace quartz {
namespace space {

class SpaceManager : private NonCopyable {
public:
    SpaceManager();
    explicit SpaceManager(std::unique_ptr<AllocationPolicy> policy);
    ~SpaceManager() = default;

    storage::PageId allocatePage();
    bool freePage(storage::PageId id);

    Extent allocateExtent(std::uint32_t length, const AllocationHints& hints = {});
    Status freeExtent(const Extent& extent);

    Status reserveRange(storage::PageId start, std::uint32_t count);

    std::uint32_t allocatedPageCount() const noexcept;
    std::uint32_t freePageCount() const noexcept;

    ExtentAllocator& extentAllocator() noexcept { return extentAllocator_; }
    const ExtentAllocator& extentAllocator() const noexcept { return extentAllocator_; }

    storage::PageAllocator& pageAllocator() noexcept { return pageAllocator_; }
    const storage::PageAllocator& pageAllocator() const noexcept { return pageAllocator_; }

    SpaceStatisticsCollector& statistics() noexcept { return stats_; }
    const SpaceStatisticsCollector& statistics() const noexcept { return stats_; }

    void reset();

private:
    void initializeFreeSpace();

    storage::PageAllocator pageAllocator_;
    ExtentAllocator extentAllocator_;
    SpaceStatisticsCollector stats_;
};

} // namespace space
} // namespace quartz
