#pragma once

#include "quartz/common/Status.h"
#include "quartz/space/SpaceManager.h"

namespace quartz {
namespace maintenance {

/// Reclaims free space and compacts fragmentation metadata.
class Vacuum {
public:
    explicit Vacuum(space::SpaceManager& space);

    Status run();
    std::uint32_t pagesReclaimed() const noexcept { return pagesReclaimed_; }

private:
    space::SpaceManager& space_;
    std::uint32_t pagesReclaimed_ = 0;
};

} // namespace maintenance
} // namespace quartz
