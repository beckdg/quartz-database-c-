#include "quartz/wal/LogManager.h"

#include "quartz/wal/LogValidator.h"

namespace quartz {
namespace wal {

LogManager::LogManager()
    : writer_(file_, stats_)
    , reader_(file_, stats_) {}

Status LogManager::initialize(const std::string& path, bool create) {
    if (initialized_) {
        return Status::invalidArgument("LogManager: already initialized");
    }
    auto st = create ? file_.create(path) : file_.open(path, true);
    if (!st.ok()) return st;
    st = file_.validate();
    if (!st.ok()) return st;
    if (!create && file_.fileSize() > WalFileHeader::kSize) {
        st = writer_.restoreFromFile();
        if (!st.ok()) return st;
    }
    initialized_ = true;
    return reader_.rewind();
}

Status LogManager::shutdown() {
    if (!initialized_) {
        return Status::success();
    }
    auto st = flush();
    if (!st.ok()) return st;
    st = file_.close();
    initialized_ = false;
    return st;
}

Status LogManager::append(LogRecord record) {
    if (!initialized_) {
        return Status::invalidArgument("LogManager: not initialized");
    }
    return writer_.appendRecord(std::move(record));
}

Status LogManager::appendBatch(std::vector<LogRecord> records) {
    if (!initialized_) {
        return Status::invalidArgument("LogManager: not initialized");
    }
    return writer_.appendBatch(std::move(records));
}

Status LogManager::flush() {
    if (!initialized_) {
        return Status::invalidArgument("LogManager: not initialized");
    }
    return writer_.flush();
}

Status LogManager::readNext(LogRecord& record) {
    if (!initialized_) {
        return Status::invalidArgument("LogManager: not initialized");
    }
    return reader_.readNext(record);
}

Status LogManager::rewindReader() {
    if (!initialized_) {
        return Status::invalidArgument("LogManager: not initialized");
    }
    return reader_.rewind();
}

Status LogManager::seekReader(LogSequenceNumber lsn) {
    if (!initialized_) {
        return Status::invalidArgument("LogManager: not initialized");
    }
    return reader_.seekLsn(lsn);
}

LogStatistics LogManager::statistics() const {
    auto stats = stats_.snapshot();
    stats.currentLsn = writer_.currentLsn();
    stats.lastFlushedLsn = writer_.lastFlushedLsn();
    stats.bufferUsage = writer_.buffer().size();
    return stats;
}

Status LogManager::validate() const {
    if (!initialized_) {
        return Status::invalidArgument("LogManager: not initialized");
    }
    auto st = file_.validate();
    if (!st.ok()) return st;

    std::uint64_t offset = WalFileHeader::kSize;
    LogRecord record;
    LogSequenceNumber previous = LogSequenceNumber::invalid();
    while (offset < file_.fileSize()) {
        std::uint64_t bytesRead = 0;
        st = file_.readRecord(offset, record, bytesRead);
        if (!st.ok()) {
            return st;
        }
        st = LogValidator::validateRecord(record);
        if (!st.ok()) return st;
        st = LogValidator::validateLsnOrdering(previous, record.lsn());
        if (!st.ok()) return st;
        previous = record.lsn();
        offset += bytesRead;
    }
    return Status::success();
}

LogSequenceNumber LogManager::currentLsn() const noexcept {
    return writer_.currentLsn();
}

LogSequenceNumber LogManager::lastFlushedLsn() const noexcept {
    return writer_.lastFlushedLsn();
}

Status LogManager::restoreWriterState() {
    if (!initialized_) {
        return Status::invalidArgument("LogManager: not initialized");
    }
    return writer_.restoreFromFile();
}

} // namespace wal
} // namespace quartz
