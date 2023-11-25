#include "quartz/wal/LogStatistics.h"

namespace quartz {
namespace wal {

void LogStatisticsCollector::recordWrite(std::size_t bytes, std::size_t recordCount) noexcept {
    stats_.recordCount += recordCount;
    stats_.bytesWritten += bytes;
    if (stats_.recordCount > 0) {
        stats_.averageRecordSize =
            static_cast<double>(stats_.bytesWritten) / static_cast<double>(stats_.recordCount);
    }
}

void LogStatisticsCollector::recordRead(std::size_t bytes) noexcept {
    stats_.bytesRead += bytes;
}

void LogStatisticsCollector::recordFlush() noexcept {
    ++stats_.flushCount;
}

void LogStatisticsCollector::updateBufferUsage(std::size_t usage) noexcept {
    stats_.bufferUsage = usage;
}

void LogStatisticsCollector::updateLargestRecord(std::size_t size) noexcept {
    if (size > stats_.largestRecord) {
        stats_.largestRecord = size;
    }
}

void LogStatisticsCollector::setCurrentLsn(LogSequenceNumber lsn) noexcept {
    stats_.currentLsn = lsn;
}

void LogStatisticsCollector::setLastFlushedLsn(LogSequenceNumber lsn) noexcept {
    stats_.lastFlushedLsn = lsn;
}

LogStatistics LogStatisticsCollector::snapshot() const noexcept {
    return stats_;
}

void LogStatisticsCollector::reset() noexcept {
    stats_ = {};
}

} // namespace wal
} // namespace quartz
