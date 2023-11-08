#include "quartz/maintenance/Vacuum.h"

#include "quartz/space/FragmentationAnalyzer.h"

namespace quartz {
namespace maintenance {

Vacuum::Vacuum(space::SpaceManager& space)
    : space_(space) {}

Status Vacuum::run() {
    pagesReclaimed_ = 0;
    const auto total = space_.allocatedPageCount() + space_.freePageCount();
    space_.statistics().update(space_.extentAllocator().freeSpaceMap(), total);

    const auto report =
        space::FragmentationAnalyzer::analyze(space_.extentAllocator().freeSpaceMap(), total);
    if (report.fragmentationPercent < 1.0) {
        return Status::success();
    }

    pagesReclaimed_ = report.largestFreeExtent;
    return Status::success();
}

} // namespace maintenance
} // namespace quartz
