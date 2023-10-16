#pragma once

#include "quartz/space/AllocationHints.h"
#include "quartz/space/Extent.h"

#include <memory>
#include <string>

namespace quartz {
namespace space {

class FreeSpaceMap;

class AllocationPolicy {
public:
    virtual ~AllocationPolicy() = default;

    virtual Extent allocate(FreeSpaceMap& fsm, std::uint32_t length,
                            const AllocationHints& hints) = 0;
    virtual std::string name() const = 0;
};

class FirstFitPolicy : public AllocationPolicy {
public:
    Extent allocate(FreeSpaceMap& fsm, std::uint32_t length,
                    const AllocationHints& hints) override;
    std::string name() const override { return "FirstFit"; }
};

class BestFitPolicy : public AllocationPolicy {
public:
    Extent allocate(FreeSpaceMap& fsm, std::uint32_t length,
                    const AllocationHints& hints) override;
    std::string name() const override { return "BestFit"; }
};

class SequentialPolicy : public AllocationPolicy {
public:
    SequentialPolicy() : cursor_(storage::kFirstDataPageId) {}

    Extent allocate(FreeSpaceMap& fsm, std::uint32_t length,
                    const AllocationHints& hints) override;
    std::string name() const override { return "Sequential"; }

    void resetCursor() noexcept { cursor_ = storage::kFirstDataPageId; }
    storage::PageId cursor() const noexcept { return cursor_; }

private:
    storage::PageId cursor_;
};

} // namespace space
} // namespace quartz
