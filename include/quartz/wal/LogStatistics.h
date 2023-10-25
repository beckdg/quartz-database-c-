#pragma once

#include "quartz/wal/LogSequenceNumber.h"

#include <cstddef>
#include <cstdint>

namespace quartz {
namespace wal {

/// Aggregate metrics for WAL activity.
struct LogStatistics {
    std::uint64_t recordCount = 0;
    std::uint64_t bytesWritten = 0;
    std::uint64_t bytesRead = 0;
    std::uint64_t flushCount = 0;
    std::size_t bufferUsage = 0;
    std::size_t largestRecord = 0;
    double averageRecordSize = 0.0;
    LogSequenceNumber currentLsn;
    LogSequenceNumber lastFlushedLsn;
};

/// Tracks WAL operation counters.
class LogStatisticsCollector {
public:
    void recordWrite(std::size_t bytes, std::size_t recordCount = 1) noexcept;
    void recordRead(std::size_t bytes) noexcept;
    void recordFlush() noexcept;
    void updateBufferUsage(std::size_t usage) noexcept;
    void updateLargestRecord(std::size_t size) noexcept;
    void setCurrentLsn(LogSequenceNumber lsn) noexcept;
    void setLastFlushedLsn(LogSequenceNumber lsn) noexcept;

    LogStatistics snapshot() const noexcept;

    void reset() noexcept;

private:
    LogStatistics stats_;
};

} // namespace wal
} // namespace quartz
