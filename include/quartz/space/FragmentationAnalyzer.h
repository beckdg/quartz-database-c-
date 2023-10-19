#pragma once

#include "quartz/space/Extent.h"
#include "quartz/storage/StorageConstants.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace quartz {
namespace space {

class FreeSpaceMap;

struct FragmentationReport {
    double fragmentationPercent = 0.0;
    std::uint32_t largestFreeExtent = 0;
    std::uint32_t smallestFreeExtent = 0;
    double averageFreeExtentSize = 0.0;
    std::uint32_t extentCount = 0;
    std::uint32_t totalFreePages = 0;
    double allocationDensity = 0.0;
    std::uint32_t maxContiguousAllocation = 0;
    std::string recommendation;
};

class FragmentationAnalyzer {
public:
    static FragmentationReport analyze(const FreeSpaceMap& fsm, std::uint32_t totalPages);

    static double calculateFragmentation(const FreeSpaceMap& fsm, std::uint32_t totalPages);
    static std::uint32_t largestFreeExtent(const FreeSpaceMap& fsm);
    static std::uint32_t totalFreePages(const FreeSpaceMap& fsm);
    static double averageFreeExtentSize(const FreeSpaceMap& fsm);
    static double allocationDensity(const FreeSpaceMap& fsm, std::uint32_t totalPages);
    static std::uint32_t maxContiguousAllocation(const FreeSpaceMap& fsm);
};

} // namespace space
} // namespace quartz
