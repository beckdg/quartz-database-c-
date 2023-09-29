#pragma once

#include "quartz/common/Status.h"

#include <cstdint>
#include <cstring>
#include <ostream>
#include <string>
#include <string_view>

namespace quartz {
namespace format {

class ObjectId {
public:
    static constexpr std::size_t kSize = 16;

    ObjectId() noexcept : bytes_{} {}

    static ObjectId generate();
    static ObjectId fromString(const std::string& str);
    static Status fromString(const std::string& str, ObjectId& out) noexcept;

    static ObjectId nil() noexcept { return ObjectId(); }
    bool isNil() const noexcept;

    std::uint64_t high() const noexcept { return parts_.high; }
    std::uint64_t low() const noexcept { return parts_.low; }

    std::string toString() const;

    int compare(const ObjectId& other) const noexcept;

    bool operator==(const ObjectId& other) const noexcept;
    bool operator!=(const ObjectId& other) const noexcept { return !(*this == other); }
    bool operator<(const ObjectId& other) const noexcept;
    bool operator>(const ObjectId& other) const noexcept { return other < *this; }
    bool operator<=(const ObjectId& other) const noexcept { return !(other < *this); }
    bool operator>=(const ObjectId& other) const noexcept { return !(*this < other); }

    const std::uint8_t* data() const noexcept { return bytes_; }
    std::size_t size() const noexcept { return kSize; }

    friend std::ostream& operator<<(std::ostream& os, const ObjectId& id) {
        os << id.toString();
        return os;
    }

    struct Parts {
        std::uint64_t high;
        std::uint64_t low;
    };

private:
    union {
        std::uint8_t bytes_[kSize];
        Parts parts_;
    };
};

static_assert(sizeof(ObjectId) == ObjectId::kSize,
              "ObjectId must be exactly 16 bytes");

} // namespace format
} // namespace quartz
