#pragma once

#include "quartz/common/Status.h"

#include <cstdint>
#include <string>

namespace quartz {
namespace format {

class FeatureFlags {
public:
    using ValueType = std::uint64_t;

    static constexpr ValueType kNone            = 0;
    static constexpr ValueType kChecksums       = ValueType{1} << 0;
    static constexpr ValueType kCompression     = ValueType{1} << 1;
    static constexpr ValueType kEncryption      = ValueType{1} << 2;
    static constexpr ValueType kJournaling      = ValueType{1} << 3;
    static constexpr ValueType kLargePages      = ValueType{1} << 4;
    static constexpr ValueType kExtendedIds     = ValueType{1} << 5;
    static constexpr ValueType kCustomPageSize  = ValueType{1} << 6;
    static constexpr ValueType kMetadataRegion  = ValueType{1} << 7;
    static constexpr ValueType kUserFlags       = ValueType{1} << 8;

    static constexpr ValueType kReservedMask    = ~(kChecksums | kCompression | kEncryption |
                                                     kJournaling | kLargePages | kExtendedIds |
                                                     kCustomPageSize | kMetadataRegion | kUserFlags);

    static constexpr ValueType kRequiredFlags   = kNone;

    FeatureFlags() noexcept = default;
    explicit FeatureFlags(ValueType flags) noexcept : flags_(flags) {}

    bool has(ValueType flag) const noexcept { return (flags_ & flag) == flag; }
    bool hasAny(ValueType flag) const noexcept { return (flags_ & flag) != 0; }

    void set(ValueType flag) noexcept { flags_ |= flag; }
    void clear(ValueType flag) noexcept { flags_ &= ~flag; }

    ValueType value() const noexcept { return flags_; }
    void setValue(ValueType v) noexcept { flags_ = v; }

    bool hasReservedBits() const noexcept { return (flags_ & kReservedMask) != 0; }

    Status validate() const noexcept {
        if (hasReservedBits()) {
            return Status::invalidArgument("FeatureFlags: reserved bits are set");
        }
        return Status::success();
    }

    bool operator==(const FeatureFlags& other) const noexcept {
        return flags_ == other.flags_;
    }
    bool operator!=(const FeatureFlags& other) const noexcept {
        return !(*this == other);
    }

    std::string toString() const;

private:
    ValueType flags_ = kNone;
};

} // namespace format
} // namespace quartz
