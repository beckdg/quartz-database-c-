#include "quartz/wal/LogReader.h"

#include "quartz/wal/LogValidator.h"

namespace quartz {
namespace wal {

LogReader::LogReader(LogFile& file, LogStatisticsCollector& stats)
    : file_(file), stats_(stats) {}

Status LogReader::rewind() {
    offset_ = WalFileHeader::kSize;
    lastLsn_ = LogSequenceNumber::invalid();
    endOfLog_ = false;
    return Status::success();
}

Status LogReader::readNext(LogRecord& record) {
    if (endOfLog_) {
        return Status::invalidArgument("LogReader: end of log");
    }
    if (offset_ >= file_.fileSize()) {
        endOfLog_ = true;
        return Status::invalidArgument("LogReader: end of log");
    }

    std::uint64_t bytesRead = 0;
    auto st = file_.readRecord(offset_, record, bytesRead);
    if (!st.ok()) {
        endOfLog_ = true;
        return st;
    }

    st = LogValidator::validateRecord(record);
    if (!st.ok()) return st;
    st = LogValidator::validateLsnOrdering(lastLsn_, record.lsn());
    if (!st.ok()) return st;

    lastLsn_ = record.lsn();
    offset_ += bytesRead;
    stats_.recordRead(bytesRead);
    return Status::success();
}

Status LogReader::seekLsn(LogSequenceNumber lsn) {
    auto st = rewind();
    if (!st.ok()) return st;

    LogRecord record;
    while (!endOfLog_) {
        const auto recordOffset = offset_;
        st = readNext(record);
        if (!st.ok()) {
            if (endOfLog_) break;
            return st;
        }
        if (record.lsn() == lsn) {
            offset_ = recordOffset;
            lastLsn_ = lsn.value() > 1 ? LogSequenceNumber(lsn.value() - 1)
                                       : LogSequenceNumber::invalid();
            endOfLog_ = false;
            return Status::success();
        }
        if (record.lsn() > lsn) {
            return Status::invalidArgument("LogReader: LSN not found");
        }
    }
    return Status::invalidArgument("LogReader: LSN not found");
}

bool LogReader::endOfLog() const noexcept {
    return endOfLog_ || offset_ >= file_.fileSize();
}

} // namespace wal
} // namespace quartz
