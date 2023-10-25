#pragma once

#include "quartz/wal/LogFile.h"
#include "quartz/wal/LogReader.h"
#include "quartz/wal/LogRecord.h"
#include "quartz/wal/LogStatistics.h"
#include "quartz/wal/LogWriter.h"
#include "quartz/common/NonCopyable.h"
#include "quartz/common/Status.h"

#include <string>
#include <vector>

namespace quartz {
namespace wal {

/// Top-level WAL coordinator for append, flush, and read operations.
class LogManager : private NonCopyable {
public:
    LogManager();

    Status initialize(const std::string& path, bool create = true);
    Status shutdown();

    bool isInitialized() const noexcept { return initialized_; }

    Status append(LogRecord record);
    Status appendBatch(std::vector<LogRecord> records);
    Status flush();

    Status readNext(LogRecord& record);
    Status rewindReader();
    Status seekReader(LogSequenceNumber lsn);

    LogStatistics statistics() const;
    Status validate() const;

    LogSequenceNumber currentLsn() const noexcept;
    LogSequenceNumber lastFlushedLsn() const noexcept;

    /// Restores writer LSN state from the on-disk log.
    Status restoreWriterState();

    LogFile& file() noexcept { return file_; }
    const LogFile& file() const noexcept { return file_; }
    LogWriter& writer() noexcept { return writer_; }
    LogReader& reader() noexcept { return reader_; }

private:
    bool initialized_ = false;
    LogFile file_;
    LogStatisticsCollector stats_;
    LogWriter writer_;
    LogReader reader_;
};

} // namespace wal
} // namespace quartz
