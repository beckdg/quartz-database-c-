#pragma once

#include "quartz/pages/BasePage.h"
#include "quartz/pages/HeaderPage.h"
#include "quartz/pages/FreeListPage.h"
#include "quartz/pages/DataPage.h"
#include "quartz/pages/IndexPage.h"
#include "quartz/pages/OverflowPage.h"
#include "quartz/pages/MetadataPage.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace quartz {
namespace pages {

struct PageStats {
    std::size_t totalSize;
    std::size_t headerOverhead;
    std::size_t payloadSize;
    std::size_t reservedBytes;
    std::size_t usedBytes;
    double utilizationPercent;
    std::size_t freeBytes;
};

class PageStatistics {
public:
    PageStatistics() = delete;

    static PageStats compute(const BasePage& page);
    static PageStats compute(const HeaderPage& page);
    static PageStats compute(const FreeListPage& page);
    static PageStats compute(const DataPage& page);
    static PageStats compute(const OverflowPage& page);
    static PageStats compute(const IndexPage& page);
    static PageStats compute(const MetadataPage& page);

    static std::string toString(const PageStats& stats);
};

} // namespace pages
} // namespace quartz
