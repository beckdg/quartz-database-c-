#pragma once

#include "quartz/wal/LogBuffer.h"
#include "quartz/wal/LogFile.h"
#include "quartz/wal/LogRecord.h"
#include "quartz/wal/LogSequenceNumber.h"
#include "quartz/wal/LogStatistics.h"
#include "quartz/common/Status.h"

#include <vector>

namespace quartz {
namespace wal {

/// Serializes and appends log records with LSN assignment.
class LogWriter {
public:
    LogWriter(LogFile& file, LogStatisticsCollector& stats, std::size_t bufferCapacity = 64 * 1024);

    Status appendRecord(LogRecord record);
    Status appendBatch(std::vector<LogRecord> records);
    Status flush();

    LogSequenceNumber currentLsn() const noexcept;
    LogSequenceNumber lastFlushedLsn() const noexcept;

    /// Restores LSN counters by scanning the on-disk log.
    Status restoreFromFile();

    LogBuffer& buffer() noexcept { return buffer_; }
    const LogBuffer& buffer() const noexcept { return buffer_; }

private:
    Status writeRecordToBuffer(const LogRecord& record);
    Status flushBufferToFile();

    LogFile& file_;
    LogStatisticsCollector& stats_;
    LogBuffer buffer_;
    LogSequenceNumber nextLsn_;
    LogSequenceNumber currentLsn_;
    LogSequenceNumber lastFlushedLsn_;
};

} // namespace wal
} // namespace quartz
