#include "quartz/pages/PageStatistics.h"
#include "quartz/pages/PageLayouts.h"

#include <cstring>
#include <sstream>

namespace quartz {
namespace pages {

namespace {

PageStats computeBase(const BasePage& page) {
    PageStats stats{};
    stats.totalSize = page.size();
    stats.headerOverhead = sizeof(storage::PageHeader);
    stats.payloadSize = page.payloadSize();
    return stats;
}

} // anonymous namespace

PageStats PageStatistics::compute(const BasePage& page) {
    auto stats = computeBase(page);
    stats.reservedBytes = 0;
    stats.usedBytes = 0;
    stats.freeBytes = page.payloadSize();
    stats.utilizationPercent = 0.0;
    return stats;
}

PageStats PageStatistics::compute(const HeaderPage& page) {
    auto stats = computeBase(page);
    auto* l = page.layout();
    stats.reservedBytes = sizeof(l->reserved);
    stats.usedBytes = sizeof(l->databaseHeader) + sizeof(l->superblockPageId) +
                      sizeof(l->flags) + sizeof(l->formatVersion);
    stats.freeBytes = stats.reservedBytes;
    stats.utilizationPercent = (static_cast<double>(stats.usedBytes) /
                                static_cast<double>(stats.payloadSize)) * 100.0;
    return stats;
}

PageStats PageStatistics::compute(const FreeListPage& page) {
    auto stats = computeBase(page);
    auto& l = *reinterpret_cast<const FreeListPageLayout*>(page.page().payload());
    stats.reservedBytes = sizeof(l.reserved);
    stats.usedBytes = sizeof(l.freeCount) + sizeof(l.capacity) +
                      l.freeCount * sizeof(storage::PageId);
    stats.freeBytes = (l.capacity - l.freeCount) * sizeof(storage::PageId);
    stats.utilizationPercent = (static_cast<double>(stats.usedBytes) /
                                static_cast<double>(stats.payloadSize)) * 100.0;
    return stats;
}

PageStats PageStatistics::compute(const DataPage& page) {
    auto stats = computeBase(page);
    auto& l = *reinterpret_cast<const DataPageLayout*>(page.page().payload());
    stats.reservedBytes = sizeof(l.reserved1) + sizeof(l.reserved2);
    stats.usedBytes = sizeof(l.freeSpaceOffset) + sizeof(l.slotCount);
    stats.freeBytes = l.availableSpace();
    stats.utilizationPercent = (static_cast<double>(stats.usedBytes) /
                                static_cast<double>(stats.payloadSize)) * 100.0;
    return stats;
}

PageStats PageStatistics::compute(const OverflowPage& page) {
    auto stats = computeBase(page);
    auto& l = *reinterpret_cast<const OverflowPageLayout*>(page.page().payload());
    stats.reservedBytes = sizeof(l.reserved);
    stats.usedBytes = sizeof(l.nextPageId) + sizeof(l.payloadSize) + l.payloadSize;
    stats.freeBytes = l.remainingCapacity();
    stats.utilizationPercent = (static_cast<double>(stats.usedBytes) /
                                static_cast<double>(stats.payloadSize)) * 100.0;
    return stats;
}

PageStats PageStatistics::compute(const IndexPage& page) {
    auto stats = computeBase(page);
    auto& l = *reinterpret_cast<const IndexPageLayout*>(page.page().payload());
    stats.reservedBytes = sizeof(l.reserved);
    stats.usedBytes = sizeof(l.nodeType) + sizeof(l.keyCount) +
                      sizeof(l.capacity) + sizeof(l.flags);
    stats.freeBytes = stats.payloadSize - stats.usedBytes - stats.reservedBytes;
    stats.utilizationPercent = (static_cast<double>(stats.usedBytes) /
                                static_cast<double>(stats.payloadSize)) * 100.0;
    return stats;
}

PageStats PageStatistics::compute(const MetadataPage& page) {
    auto stats = computeBase(page);
    auto& l = *reinterpret_cast<const MetadataPageLayout*>(page.page().payload());
    stats.reservedBytes = sizeof(l.reserved);
    stats.usedBytes = sizeof(l.entryCount) + sizeof(l.version);
    stats.freeBytes = stats.payloadSize - stats.usedBytes - stats.reservedBytes;
    stats.utilizationPercent = (static_cast<double>(stats.usedBytes) /
                                static_cast<double>(stats.payloadSize)) * 100.0;
    return stats;
}

std::string PageStatistics::toString(const PageStats& stats) {
    std::ostringstream oss;
    oss << "Page Statistics:\n"
        << "  Total:       " << stats.totalSize << " bytes\n"
        << "  Header:      " << stats.headerOverhead << " bytes\n"
        << "  Payload:     " << stats.payloadSize << " bytes\n"
        << "  Reserved:    " << stats.reservedBytes << " bytes\n"
        << "  Used:        " << stats.usedBytes << " bytes\n"
        << "  Free:        " << stats.freeBytes << " bytes\n"
        << "  Utilization: " << stats.utilizationPercent << "%\n";
    return oss.str();
}

} // namespace pages
} // namespace quartz
