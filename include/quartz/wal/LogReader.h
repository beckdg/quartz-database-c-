#pragma once

#include "quartz/wal/LogFile.h"
#include "quartz/wal/LogRecord.h"
#include "quartz/wal/LogSequenceNumber.h"
#include "quartz/wal/LogStatistics.h"
#include "quartz/common/Status.h"

namespace quartz {
namespace wal {

/// Iterates log records from a WAL file.
class LogReader {
public:
    explicit LogReader(LogFile& file, LogStatisticsCollector& stats);

    Status rewind();
    Status readNext(LogRecord& record);
    Status seekLsn(LogSequenceNumber lsn);

    bool endOfLog() const noexcept;
    std::uint64_t offset() const noexcept { return offset_; }
    LogSequenceNumber lastLsn() const noexcept { return lastLsn_; }

private:
    LogFile& file_;
    LogStatisticsCollector& stats_;
    std::uint64_t offset_ = WalFileHeader::kSize;
    LogSequenceNumber lastLsn_;
    bool endOfLog_ = false;
};

} // namespace wal
} // namespace quartz
