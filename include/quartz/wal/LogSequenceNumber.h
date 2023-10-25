#pragma once

#include "quartz/common/Status.h"
#include "quartz/serialization/BinaryReader.h"
#include "quartz/serialization/BinaryWriter.h"

#include <cstdint>
#include <string>

namespace quartz {
namespace wal {

/// Monotonically increasing log sequence identifier.
class LogSequenceNumber {
public:
    static constexpr std::uint64_t kInvalidValue = 0;

    LogSequenceNumber() noexcept = default;
    explicit LogSequenceNumber(std::uint64_t value) noexcept;

    static LogSequenceNumber invalid() noexcept;
    static LogSequenceNumber initial() noexcept;

    bool isValid() const noexcept;
    std::uint64_t value() const noexcept;

    LogSequenceNumber next() const noexcept;

    bool operator==(const LogSequenceNumber& other) const noexcept;
    bool operator!=(const LogSequenceNumber& other) const noexcept;
    bool operator<(const LogSequenceNumber& other) const noexcept;
    bool operator<=(const LogSequenceNumber& other) const noexcept;
    bool operator>(const LogSequenceNumber& other) const noexcept;
    bool operator>=(const LogSequenceNumber& other) const noexcept;

    Status serialize(serialization::BinaryWriter& writer) const;
    Status deserialize(serialization::BinaryReader& reader);

    std::string toString() const;

private:
    std::uint64_t value_ = kInvalidValue;
};

} // namespace wal
} // namespace quartz
