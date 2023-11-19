#include "quartz/space/SpaceManager.h"

namespace quartz {
namespace space {

SpaceManager::SpaceManager()
    : extentAllocator_(std::make_unique<FirstFitPolicy>()) {
    initializeFreeSpace();
}

SpaceManager::SpaceManager(std::unique_ptr<AllocationPolicy> policy)
    : extentAllocator_(std::move(policy)) {
    initializeFreeSpace();
}

void SpaceManager::initializeFreeSpace() {
    // The FreeSpaceMap starts with all pages from kFirstDataPageId to kMaxPageCount as free.
    // Reserved pages (0 through kReservedPages-1) are never managed by the allocator.
    auto totalPages = static_cast<std::uint32_t>(storage::kMaxPageId - storage::kFirstDataPageId + 1);
    if (totalPages > 0) {
        extentAllocator_.freeSpaceMap().initialize(storage::kFirstDataPageId, totalPages);
    }
}

storage::PageId SpaceManager::allocatePage() {
    auto id = pageAllocator_.allocate();
    if (id != storage::kInvalidPageId) {
        auto st = extentAllocator_.freeSpaceMap().markAllocated(Extent{id, 1});
        if (!st.ok()) {
            // Rollback PageAllocator
            pageAllocator_.free(id);
            stats_.recordAllocation(false);
            return storage::kInvalidPageId;
        }
        stats_.recordAllocation(true);
    } else {
        stats_.recordAllocation(false);
    }
    return id;
}

bool SpaceManager::freePage(storage::PageId id) {
    if (!pageAllocator_.free(id)) {
        stats_.recordFree(false);
        return false;
    }
    auto st = extentAllocator_.freeSpaceMap().markFree(Extent{id, 1});
    stats_.recordFree(st.ok());
    return st.ok();
}

Extent SpaceManager::allocateExtent(std::uint32_t length, const AllocationHints& hints) {
    auto extent = extentAllocator_.allocate(length, hints);
    stats_.recordAllocation(extent.isValid());
    if (extent.isValid()) {
        stats_.update(extentAllocator_.freeSpaceMap(), storage::kMaxPageId + 1);
    }
    return extent;
}

Status SpaceManager::freeExtent(const Extent& extent) {
    auto st = extentAllocator_.release(extent);
    stats_.recordFree(st.ok());
    if (st.ok()) {
        stats_.update(extentAllocator_.freeSpaceMap(), storage::kMaxPageId + 1);
    }
    return st;
}

Status SpaceManager::reserveRange(storage::PageId start, std::uint32_t count) {
    if (start < storage::kFirstDataPageId) {
        return Status::invalidArgument("Cannot reserve reserved page range");
    }
    auto st = extentAllocator_.freeSpaceMap().markAllocated(Extent{start, count});
    if (st.ok()) {
        stats_.update(extentAllocator_.freeSpaceMap(), storage::kMaxPageId + 1);
    }
    return st;
}

std::uint32_t SpaceManager::allocatedPageCount() const noexcept {
    auto managedTotal = storage::kMaxPageId - storage::kFirstDataPageId + 1;
    return managedTotal - extentAllocator_.freeSpaceMap().countFree();
}

std::uint32_t SpaceManager::freePageCount() const noexcept {
    return extentAllocator_.freeSpaceMap().countFree();
}

void SpaceManager::reset() {
    pageAllocator_.reset();
    extentAllocator_.clear();
    initializeFreeSpace();
    stats_.reset();
}

} // namespace space
} // namespace quartz
