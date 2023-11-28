#include "quartz/wal/LogWriter.h"

#include "quartz/serialization/BinaryWriter.h"
#include "quartz/serialization/Buffer.h"
#include "quartz/serialization/BufferView.h"
#include "quartz/wal/LogValidator.h"

namespace quartz {
namespace wal {

LogWriter::LogWriter(LogFile& file, LogStatisticsCollector& stats, std::size_t bufferCapacity)
    : file_(file)
    , stats_(stats)
    , buffer_(bufferCapacity)
    , nextLsn_(LogSequenceNumber::initial())
    , currentLsn_(LogSequenceNumber::invalid())
    , lastFlushedLsn_(LogSequenceNumber::invalid()) {}

Status LogWriter::appendRecord(LogRecord record) {
    record.setLsn(nextLsn_);
    currentLsn_ = nextLsn_;
    nextLsn_ = nextLsn_.next();

    auto st = LogValidator::validateRecordHeader(record);
    if (!st.ok()) return st;

    st = writeRecordToBuffer(record);
    if (!st.ok()) return st;

    const auto bytes = record.wireSize() + sizeof(std::uint32_t);
    stats_.recordWrite(bytes);
    stats_.updateLargestRecord(bytes);
    stats_.updateBufferUsage(buffer_.size());
    stats_.setCurrentLsn(currentLsn_);
    return Status::success();
}

Status LogWriter::appendBatch(std::vector<LogRecord> records) {
    for (auto& record : records) {
        auto st = appendRecord(std::move(record));
        if (!st.ok()) return st;
    }
    return Status::success();
}

Status LogWriter::flush() {
    auto st = flushBufferToFile();
    if (!st.ok()) return st;
    st = file_.flush();
    if (!st.ok()) return st;
    stats_.recordFlush();
    lastFlushedLsn_ = currentLsn_;
    stats_.setLastFlushedLsn(lastFlushedLsn_);
    return Status::success();
}

LogSequenceNumber LogWriter::currentLsn() const noexcept {
    return currentLsn_;
}

LogSequenceNumber LogWriter::lastFlushedLsn() const noexcept {
    return lastFlushedLsn_;
}

Status LogWriter::writeRecordToBuffer(const LogRecord& record) {
    serialization::Buffer buf;
    serialization::BinaryWriter writer(buf);
    std::uint32_t recordSize = 0;
    auto st = writer.write(recordSize);
    if (!st.ok()) return st;
    const auto bodyStart = writer.tell();
    st = record.serialize(writer);
    if (!st.ok()) return st;
    recordSize = static_cast<std::uint32_t>(writer.tell() - bodyStart);
    st = writer.overwriteAt(0, &recordSize, sizeof(recordSize));
    if (!st.ok()) return st;
    return buffer_.append(serialization::BufferView(buf));
}

Status LogWriter::flushBufferToFile() {
    return buffer_.flush([this](serialization::BufferView data) { return file_.append(data); });
}

Status LogWriter::restoreFromFile() {
    std::uint64_t offset = WalFileHeader::kSize;
    LogSequenceNumber maxLsn = LogSequenceNumber::invalid();

    while (offset < file_.fileSize()) {
        LogRecord record;
        std::uint64_t bytesRead = 0;
        auto st = file_.readRecord(offset, record, bytesRead);
        if (!st.ok()) {
            break;
        }
        maxLsn = record.lsn();
        offset += bytesRead;
    }

    if (maxLsn.isValid()) {
        currentLsn_ = maxLsn;
        nextLsn_ = maxLsn.next();
        lastFlushedLsn_ = maxLsn;
        stats_.setCurrentLsn(currentLsn_);
        stats_.setLastFlushedLsn(lastFlushedLsn_);
    }
    return Status::success();
}

} // namespace wal
} // namespace quartz
