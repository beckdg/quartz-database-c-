#include "quartz/space/FragmentationAnalyzer.h"
#include "quartz/space/FreeSpaceMap.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace quartz {
namespace space {

FragmentationReport FragmentationAnalyzer::analyze(const FreeSpaceMap& fsm,
                                                    std::uint32_t totalPages) {
    FragmentationReport report;

    const auto& extents = fsm.extents();
    report.extentCount = static_cast<std::uint32_t>(extents.size());
    report.totalFreePages = fsm.countFree();
    report.largestFreeExtent = largestFreeExtent(fsm);
    report.averageFreeExtentSize = averageFreeExtentSize(fsm);
    report.fragmentationPercent = calculateFragmentation(fsm, totalPages);
    report.allocationDensity = allocationDensity(fsm, totalPages);
    report.maxContiguousAllocation = maxContiguousAllocation(fsm);

    // Smallest non-zero extent
    report.smallestFreeExtent = 0;
    if (!extents.empty()) {
        report.smallestFreeExtent = extents.front().length;
        for (const auto& ext : extents) {
            if (ext.length < report.smallestFreeExtent) {
                report.smallestFreeExtent = ext.length;
            }
        }
    }

    // Generate recommendation
    if (report.fragmentationPercent > 50.0) {
        report.recommendation = "High fragmentation: consider defragmentation";
    } else if (report.fragmentationPercent > 25.0) {
        report.recommendation = "Moderate fragmentation: monitor growth";
    } else {
        report.recommendation = "Low fragmentation: space is well-organized";
    }

    return report;
}

double FragmentationAnalyzer::calculateFragmentation(const FreeSpaceMap& fsm,
                                                      std::uint32_t /*totalPages*/) {
    auto freePages = fsm.countFree();
    if (freePages == 0) return 0.0;
    if (fsm.countFreeExtents() <= 1) return 0.0;

    auto largest = largestFreeExtent(fsm);
    return 100.0 * (1.0 - static_cast<double>(largest) / static_cast<double>(freePages));
}

std::uint32_t FragmentationAnalyzer::largestFreeExtent(const FreeSpaceMap& fsm) {
    std::uint32_t largest = 0;
    for (const auto& ext : fsm.extents()) {
        if (ext.length > largest) largest = ext.length;
    }
    return largest;
}

std::uint32_t FragmentationAnalyzer::totalFreePages(const FreeSpaceMap& fsm) {
    return fsm.countFree();
}

double FragmentationAnalyzer::averageFreeExtentSize(const FreeSpaceMap& fsm) {
    auto count = fsm.countFreeExtents();
    if (count == 0) return 0.0;
    return static_cast<double>(fsm.countFree()) / static_cast<double>(count);
}

double FragmentationAnalyzer::allocationDensity(const FreeSpaceMap& fsm,
                                                  std::uint32_t totalPages) {
    if (totalPages == 0) return 0.0;
    auto freePages = fsm.countFree();
    auto usedPages = totalPages - freePages;
    return static_cast<double>(usedPages) / static_cast<double>(totalPages);
}

std::uint32_t FragmentationAnalyzer::maxContiguousAllocation(const FreeSpaceMap& fsm) {
    return largestFreeExtent(fsm);
}

} // namespace space
} // namespace quartz
