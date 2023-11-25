#include "quartz/wal/LogSequenceNumber.h"

#include "quartz/serialization/BinaryReader.h"
#include "quartz/serialization/BinaryWriter.h"

#include <sstream>

namespace quartz {
namespace wal {

LogSequenceNumber::LogSequenceNumber(std::uint64_t value) noexcept
    : value_(value) {}

LogSequenceNumber LogSequenceNumber::invalid() noexcept {
    return LogSequenceNumber(kInvalidValue);
}

LogSequenceNumber LogSequenceNumber::initial() noexcept {
    return LogSequenceNumber(1);
}

bool LogSequenceNumber::isValid() const noexcept {
    return value_ != kInvalidValue;
}

std::uint64_t LogSequenceNumber::value() const noexcept {
    return value_;
}

LogSequenceNumber LogSequenceNumber::next() const noexcept {
    return LogSequenceNumber(value_ + 1);
}

bool LogSequenceNumber::operator==(const LogSequenceNumber& other) const noexcept {
    return value_ == other.value_;
}

bool LogSequenceNumber::operator!=(const LogSequenceNumber& other) const noexcept {
    return !(*this == other);
}

bool LogSequenceNumber::operator<(const LogSequenceNumber& other) const noexcept {
    return value_ < other.value_;
}

bool LogSequenceNumber::operator<=(const LogSequenceNumber& other) const noexcept {
    return value_ <= other.value_;
}

bool LogSequenceNumber::operator>(const LogSequenceNumber& other) const noexcept {
    return value_ > other.value_;
}

bool LogSequenceNumber::operator>=(const LogSequenceNumber& other) const noexcept {
    return value_ >= other.value_;
}

Status LogSequenceNumber::serialize(serialization::BinaryWriter& writer) const {
    return writer.write(value_);
}

Status LogSequenceNumber::deserialize(serialization::BinaryReader& reader) {
    return reader.read(value_);
}

std::string LogSequenceNumber::toString() const {
    std::ostringstream oss;
    if (!isValid()) {
        oss << "LSN(invalid)";
    } else {
        oss << "LSN(" << value_ << ")";
    }
    return oss.str();
}

} // namespace wal
} // namespace quartz
