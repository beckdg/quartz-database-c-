#include "quartz/wal/LogRecord.h"

#include <chrono>
#include <sstream>

namespace quartz {
namespace wal {

namespace {

std::uint64_t currentTimestamp() noexcept {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(now).count());
}

} // namespace

LogRecord LogRecord::make(LogRecordType type, storage::PageId pageId, std::uint64_t transactionId) {
    LogRecord record;
    record.type_ = type;
    record.pageId_ = pageId;
    record.transactionId_ = transactionId;
    record.timestamp_ = currentTimestamp();
    return record;
}

std::size_t LogRecord::wireSize() const noexcept {
    return sizeof(std::uint8_t) + sizeof(std::uint32_t) + sizeof(std::uint64_t) * 2 +
           sizeof(storage::PageId) + sizeof(std::uint64_t) + sizeof(std::uint32_t) * 3 +
           payload_.size();
}

Status LogRecord::serialize(serialization::BinaryWriter& writer) const {
    const auto typeByte = static_cast<std::uint8_t>(type_);
    auto st = writer.write(typeByte);
    if (!st.ok()) return st;
    st = lsn_.serialize(writer);
    if (!st.ok()) return st;
    st = writer.write(timestamp_);
    if (!st.ok()) return st;
    st = writer.write(pageId_);
    if (!st.ok()) return st;
    st = writer.write(transactionId_);
    if (!st.ok()) return st;
    const auto payloadLen = static_cast<std::uint32_t>(payload_.size());
    st = writer.write(payloadLen);
    if (!st.ok()) return st;
    st = writer.write(checksum_);
    if (!st.ok()) return st;
    st = writer.write(reserved0_);
    if (!st.ok()) return st;
    st = writer.write(reserved1_);
    if (!st.ok()) return st;
    if (payloadLen > 0) {
        st = writer.writeBytes(payload_);
        if (!st.ok()) return st;
    }
    return Status::success();
}

Status LogRecord::deserialize(serialization::BinaryReader& reader) {
    std::uint8_t typeByte = 0;
    auto st = reader.read(typeByte);
    if (!st.ok()) return st;
    type_ = static_cast<LogRecordType>(typeByte);
    st = lsn_.deserialize(reader);
    if (!st.ok()) return st;
    st = reader.read(timestamp_);
    if (!st.ok()) return st;
    st = reader.read(pageId_);
    if (!st.ok()) return st;
    st = reader.read(transactionId_);
    if (!st.ok()) return st;
    std::uint32_t payloadLen = 0;
    st = reader.read(payloadLen);
    if (!st.ok()) return st;
    st = reader.read(checksum_);
    if (!st.ok()) return st;
    st = reader.read(reserved0_);
    if (!st.ok()) return st;
    st = reader.read(reserved1_);
    if (!st.ok()) return st;
    payload_.clear();
    if (payloadLen > 0) {
        payload_.resize(payloadLen);
        st = reader.readBytes(payload_.data(), payloadLen);
        if (!st.ok()) return st;
    }
    return Status::success();
}

std::string LogRecord::toString() const {
    std::ostringstream oss;
    oss << "LogRecord(type=" << static_cast<unsigned>(type_)
        << ", lsn=" << lsn_.toString()
        << ", pageId=" << pageId_
        << ", txn=" << transactionId_
        << ", payload=" << payload_.size() << " bytes)";
    return oss.str();
}

} // namespace wal
} // namespace quartz
