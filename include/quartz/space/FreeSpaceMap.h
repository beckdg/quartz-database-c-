#pragma once

#include "quartz/common/NonCopyable.h"
#include "quartz/common/Status.h"
#include "quartz/space/Extent.h"
#include "quartz/storage/StorageConstants.h"

#include <cstdint>
#include <vector>

namespace quartz {
namespace space {

class FreeSpaceMap : private NonCopyable {
public:
    FreeSpaceMap() = default;

    void initialize(storage::PageId start, std::uint32_t length);

    Status markAllocated(const Extent& range);
    Status markFree(const Extent& range);

    void clear() noexcept;

    Extent findFreeRange(std::uint32_t length, bool preferBeginning = true) const;
    Extent findExactRange(std::uint32_t length) const;
    Extent findBestRange(std::uint32_t length) const;

    std::uint32_t countFree() const noexcept;
    std::uint32_t countFreeExtents() const noexcept { return static_cast<std::uint32_t>(extents_.size()); }
    bool contains(storage::PageId page) const noexcept;

    const std::vector<Extent>& extents() const noexcept { return extents_; }

    Status validate() const noexcept;

private:
    std::vector<Extent> extents_;
};

} // namespace space
} // namespace quartz
