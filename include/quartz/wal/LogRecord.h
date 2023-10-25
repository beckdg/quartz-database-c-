#pragma once

#include "quartz/wal/LogSequenceNumber.h"
#include "quartz/wal/LogTypes.h"
#include "quartz/common/Status.h"
#include "quartz/serialization/BinaryReader.h"
#include "quartz/serialization/BinaryWriter.h"
#include "quartz/serialization/Buffer.h"
#include "quartz/storage/StorageConstants.h"

#include <cstdint>
#include <string>

namespace quartz {
namespace wal {

/// Binary log record with typed header and variable payload.
class LogRecord {
public:
    LogRecord() = default;
    LogRecord(LogRecord&&) noexcept = default;
    LogRecord& operator=(LogRecord&&) noexcept = default;
    LogRecord(const LogRecord&) = delete;
    LogRecord& operator=(const LogRecord&) = delete;

    static LogRecord make(LogRecordType type, storage::PageId pageId = storage::kInvalidPageId,
                          std::uint64_t transactionId = 0);

    LogRecordType type() const noexcept { return type_; }
    LogSequenceNumber lsn() const noexcept { return lsn_; }
    std::uint64_t timestamp() const noexcept { return timestamp_; }
    storage::PageId pageId() const noexcept { return pageId_; }
    std::uint64_t transactionId() const noexcept { return transactionId_; }
    std::uint32_t checksum() const noexcept { return checksum_; }
    const serialization::Buffer& payload() const noexcept { return payload_; }
    serialization::Buffer& payload() noexcept { return payload_; }

    void setType(LogRecordType type) noexcept { type_ = type; }
    void setLsn(LogSequenceNumber lsn) noexcept { lsn_ = lsn; }
    void setTimestamp(std::uint64_t ts) noexcept { timestamp_ = ts; }
    void setPageId(storage::PageId id) noexcept { pageId_ = id; }
    void setTransactionId(std::uint64_t id) noexcept { transactionId_ = id; }
    void setChecksum(std::uint32_t checksum) noexcept { checksum_ = checksum; }

    std::size_t wireSize() const noexcept;
    std::size_t payloadLength() const noexcept { return payload_.size(); }

    Status serialize(serialization::BinaryWriter& writer) const;
    Status deserialize(serialization::BinaryReader& reader);

    std::string toString() const;

private:
    LogRecordType type_ = LogRecordType::Invalid;
    LogSequenceNumber lsn_;
    std::uint64_t timestamp_ = 0;
    storage::PageId pageId_ = storage::kInvalidPageId;
    std::uint64_t transactionId_ = 0;
    std::uint32_t checksum_ = 0;
    std::uint32_t reserved0_ = 0;
    std::uint32_t reserved1_ = 0;
    serialization::Buffer payload_;
};

} // namespace wal
} // namespace quartz
