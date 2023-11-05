#include "quartz/format/ObjectId.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <random>
#include <sstream>

namespace quartz {
namespace format {

ObjectId ObjectId::generate() {
    static thread_local std::mt19937_64 rng(std::random_device{}());

    ObjectId id;
    id.parts_.high = rng();
    id.parts_.low = rng();

    // Set variant bits (RFC 4122 variant)
    id.bytes_[8] = (id.bytes_[8] & 0x3F) | 0x80;

    // Set version bits (random version 4)
    id.bytes_[6] = (id.bytes_[6] & 0x0F) | 0x40;

    return id;
}

ObjectId ObjectId::fromString(const std::string& str) {
    ObjectId id;
    auto st = fromString(str, id);
    // If parsing fails, return nil
    if (!st.ok()) {
        return ObjectId();
    }
    return id;
}

Status ObjectId::fromString(const std::string& str, ObjectId& out) noexcept {
    // Expected format: 00000000-0000-0000-0000-000000000000
    //                  8-4-4-4-12 hex digits with hyphens
    if (str.size() != 36) {
        return Status::invalidArgument("ObjectId: invalid string length (expected 36)");
    }
    if (str[8] != '-' || str[13] != '-' || str[18] != '-' || str[23] != '-') {
        return Status::invalidArgument("ObjectId: invalid separator positions");
    }

    auto hexToNybble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };

    // Collect hex digits (skip hyphens)
    int hexPos = 0;
    for (std::size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '-') continue;
        int nybble = hexToNybble(str[i]);
        if (nybble < 0) {
            return Status::invalidArgument(
                "ObjectId: invalid hex character at position " +
                std::to_string(i));
        }
        if (hexPos % 2 == 0) {
            out.bytes_[hexPos / 2] = static_cast<std::uint8_t>(nybble << 4);
        } else {
            out.bytes_[hexPos / 2] |= static_cast<std::uint8_t>(nybble);
        }
        ++hexPos;
    }

    if (hexPos != 32) {
        return Status::corruption("ObjectId: unexpected hex digit count");
    }

    return Status::success();
}

bool ObjectId::isNil() const noexcept {
    return parts_.high == 0 && parts_.low == 0;
}

std::string ObjectId::toString() const {
    static constexpr const char* hex = "0123456789abcdef";
    std::string result(36, '-');
    std::size_t outIdx = 0;
    for (int i = 0; i < 16; ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            ++outIdx;
        }
        result[outIdx++] = hex[(bytes_[i] >> 4) & 0x0F];
        result[outIdx++] = hex[bytes_[i] & 0x0F];
    }
    return result;
}

int ObjectId::compare(const ObjectId& other) const noexcept {
    if (parts_.high != other.parts_.high) {
        return parts_.high < other.parts_.high ? -1 : 1;
    }
    if (parts_.low != other.parts_.low) {
        return parts_.low < other.parts_.low ? -1 : 1;
    }
    return 0;
}

bool ObjectId::operator==(const ObjectId& other) const noexcept {
    return parts_.high == other.parts_.high && parts_.low == other.parts_.low;
}

bool ObjectId::operator<(const ObjectId& other) const noexcept {
    if (parts_.high != other.parts_.high) return parts_.high < other.parts_.high;
    return parts_.low < other.parts_.low;
}

} // namespace format
} // namespace quartz
