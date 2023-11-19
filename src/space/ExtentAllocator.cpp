#include "quartz/space/ExtentAllocator.h"

namespace quartz {
namespace space {

ExtentAllocator::ExtentAllocator(std::unique_ptr<AllocationPolicy> policy)
    : policy_(std::move(policy)) {
}

Extent ExtentAllocator::allocate(std::uint32_t length, const AllocationHints& hints) {
    if (length == 0) return {};

    auto extent = policy_->allocate(fsm_, length, hints);
    if (extent.isValid()) {
        auto st = fsm_.markAllocated(extent);
        if (!st.ok()) {
            extent = {};
        }
    }
    return extent;
}

Status ExtentAllocator::release(const Extent& extent) {
    if (!extent.isValid()) {
        return Status::invalidArgument("ExtentAllocator::release: invalid extent");
    }
    return fsm_.markFree(extent);
}

void ExtentAllocator::setPolicy(std::unique_ptr<AllocationPolicy> policy) {
    policy_ = std::move(policy);
}

void ExtentAllocator::clear() noexcept {
    fsm_.clear();
}

} // namespace space
} // namespace quartz
