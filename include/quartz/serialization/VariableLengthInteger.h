#pragma once

#include "quartz/common/Status.h"

#include <cstddef>
#include <cstdint>

namespace quartz {
namespace serialization {

struct VarInt {

    static constexpr std::size_t kMaxVarInt32Bytes = 5;
    static constexpr std::size_t kMaxVarInt64Bytes = 10;

    // Encode unsigned 32-bit varint. Returns bytes written.
    static Status encodeU32(std::uint32_t value, std::uint8_t* out, std::size_t outSize,
                            std::size_t& bytesWritten) noexcept;

    // Decode unsigned 32-bit varint. Returns bytes consumed.
    static Status decodeU32(const std::uint8_t* in, std::size_t inSize,
                            std::uint32_t& value, std::size_t& bytesConsumed) noexcept;

    // Encode unsigned 64-bit varint
    static Status encodeU64(std::uint64_t value, std::uint8_t* out, std::size_t outSize,
                            std::size_t& bytesWritten) noexcept;

    // Decode unsigned 64-bit varint
    static Status decodeU64(const std::uint8_t* in, std::size_t inSize,
                            std::uint64_t& value, std::size_t& bytesConsumed) noexcept;

    // Encode signed 32-bit varint (ZigZag encoding)
    static Status encodeS32(std::int32_t value, std::uint8_t* out, std::size_t outSize,
                            std::size_t& bytesWritten) noexcept;

    // Decode signed 32-bit varint (ZigZag encoding)
    static Status decodeS32(const std::uint8_t* in, std::size_t inSize,
                            std::int32_t& value, std::size_t& bytesConsumed) noexcept;

    // Encode signed 64-bit varint (ZigZag encoding)
    static Status encodeS64(std::int64_t value, std::uint8_t* out, std::size_t outSize,
                            std::size_t& bytesWritten) noexcept;

    // Decode signed 64-bit varint (ZigZag encoding)
    static Status decodeS64(const std::uint8_t* in, std::size_t inSize,
                            std::int64_t& value, std::size_t& bytesConsumed) noexcept;

    // Compute the encoded size without encoding
    static std::size_t encodedSizeU32(std::uint32_t value) noexcept;
    static std::size_t encodedSizeU64(std::uint64_t value) noexcept;
    static std::size_t encodedSizeS32(std::int32_t value) noexcept;
    static std::size_t encodedSizeS64(std::int64_t value) noexcept;
};

} // namespace serialization
} // namespace quartz
