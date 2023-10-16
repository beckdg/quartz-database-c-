#pragma once

#include "quartz/common/NonCopyable.h"
#include "quartz/common/Status.h"
#include "quartz/space/AllocationHints.h"
#include "quartz/space/AllocationPolicy.h"
#include "quartz/space/Extent.h"
#include "quartz/space/FreeSpaceMap.h"

#include <memory>

namespace quartz {
namespace space {

class ExtentAllocator : private NonCopyable {
public:
    explicit ExtentAllocator(std::unique_ptr<AllocationPolicy> policy);

    Extent allocate(std::uint32_t length, const AllocationHints& hints = {});
    Status release(const Extent& extent);

    FreeSpaceMap& freeSpaceMap() noexcept { return fsm_; }
    const FreeSpaceMap& freeSpaceMap() const noexcept { return fsm_; }

    const AllocationPolicy& policy() const noexcept { return *policy_; }
    void setPolicy(std::unique_ptr<AllocationPolicy> policy);

    std::uint32_t freePageCount() const noexcept { return fsm_.countFree(); }
    std::uint32_t freeExtentCount() const noexcept { return fsm_.countFreeExtents(); }

    void clear() noexcept;

private:
    FreeSpaceMap fsm_;
    std::unique_ptr<AllocationPolicy> policy_;
};

} // namespace space
} // namespace quartz
