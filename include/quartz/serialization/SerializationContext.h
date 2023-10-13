#pragma once

#include "quartz/util/Endian.h"

#include <cstdint>

namespace quartz {
namespace serialization {

class SerializationContext {
public:
    static constexpr std::uint32_t kDefaultVersion = 1;
    static constexpr std::uint32_t kLatestVersion = 1;

    SerializationContext() noexcept = default;

    explicit SerializationContext(std::uint32_t version,
                                  endian::Order byteOrder = endian::nativeOrder(),
                                  bool strict = true) noexcept
        : version_(version)
        , byteOrder_(byteOrder)
        , strict_(strict) {}

    std::uint32_t version() const noexcept { return version_; }
    void setVersion(std::uint32_t v) noexcept { version_ = v; }

    endian::Order byteOrder() const noexcept { return byteOrder_; }
    void setByteOrder(endian::Order order) noexcept { byteOrder_ = order; }

    bool strict() const noexcept { return strict_; }
    void setStrict(bool s) noexcept { strict_ = s; }

    bool isVersionSupported(std::uint32_t v) const noexcept {
        return v >= kDefaultVersion && v <= kLatestVersion;
    }

    bool allowsField(std::uint32_t fieldVersion) const noexcept {
        return fieldVersion <= version_;
    }

private:
    std::uint32_t version_ = kLatestVersion;
    endian::Order byteOrder_ = endian::Order::Little;
    bool strict_ = true;
};

} // namespace serialization
} // namespace quartz
