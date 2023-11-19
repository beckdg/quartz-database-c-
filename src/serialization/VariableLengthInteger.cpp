#include "quartz/serialization/VariableLengthInteger.h"

namespace quartz {
namespace serialization {

namespace {

Status encodeImpl(std::uint64_t value, std::uint8_t* out, std::size_t outSize,
                  std::size_t& bytesWritten, std::size_t maxBytes) noexcept {
    bytesWritten = 0;
    std::uint8_t buf[10];
    std::size_t i = 0;

    do {
        if (i >= maxBytes) {
            return Status::invalidArgument("VarInt: value too large for encoding");
        }
        auto byte = static_cast<std::uint8_t>(value & 0x7F);
        value >>= 7;
        if (value != 0) {
            byte |= 0x80;
        }
        buf[i++] = byte;
    } while (value != 0);

    if (i > outSize) {
        return Status::invalidArgument("VarInt: output buffer too small");
    }

    for (std::size_t j = 0; j < i; ++j) {
        out[j] = buf[j];
    }
    bytesWritten = i;
    return Status::success();
}

Status decodeImpl(const std::uint8_t* in, std::size_t inSize,
                  std::uint64_t& value, std::size_t& bytesConsumed,
                  std::size_t maxBytes) noexcept {
    value = 0;
    bytesConsumed = 0;
    std::size_t shift = 0;

    for (std::size_t i = 0; i < maxBytes; ++i) {
        if (i >= inSize) {
            return Status::corruption("VarInt: unexpected end of input");
        }

        auto byte = in[i];
        value |= (static_cast<std::uint64_t>(byte & 0x7F) << shift);
        shift += 7;
        bytesConsumed = i + 1;

        if ((byte & 0x80) == 0) {
            return Status::success();
        }

        if (shift >= 64 && (byte & 0x7F) != 0) {
            return Status::corruption("VarInt: overflow");
        }
    }

    return Status::corruption("VarInt: malformed input");
}

std::size_t encodedSizeImpl(std::uint64_t value) noexcept {
    std::size_t count = 1;
    while (value >= 0x80) {
        value >>= 7;
        ++count;
    }
    return count;
}

} // anonymous namespace

Status VarInt::encodeU32(std::uint32_t value, std::uint8_t* out, std::size_t outSize,
                         std::size_t& bytesWritten) noexcept {
    return encodeImpl(value, out, outSize, bytesWritten, kMaxVarInt32Bytes);
}

Status VarInt::decodeU32(const std::uint8_t* in, std::size_t inSize,
                         std::uint32_t& value, std::size_t& bytesConsumed) noexcept {
    std::uint64_t tmp = 0;
    auto st = decodeImpl(in, inSize, tmp, bytesConsumed, kMaxVarInt32Bytes);
    if (!st.ok()) return st;
    if (tmp > UINT32_MAX) {
        return Status::corruption("VarInt: value too large for uint32");
    }
    value = static_cast<std::uint32_t>(tmp);
    return Status::success();
}

Status VarInt::encodeU64(std::uint64_t value, std::uint8_t* out, std::size_t outSize,
                         std::size_t& bytesWritten) noexcept {
    return encodeImpl(value, out, outSize, bytesWritten, kMaxVarInt64Bytes);
}

Status VarInt::decodeU64(const std::uint8_t* in, std::size_t inSize,
                         std::uint64_t& value, std::size_t& bytesConsumed) noexcept {
    return decodeImpl(in, inSize, value, bytesConsumed, kMaxVarInt64Bytes);
}

Status VarInt::encodeS32(std::int32_t value, std::uint8_t* out, std::size_t outSize,
                         std::size_t& bytesWritten) noexcept {
    // ZigZag: (n << 1) ^ (n >> 31)
    auto zigzag = static_cast<std::uint32_t>((static_cast<std::uint32_t>(value) << 1) ^
                                              (static_cast<std::uint32_t>(value) >> 31));
    return encodeU32(zigzag, out, outSize, bytesWritten);
}

Status VarInt::decodeS32(const std::uint8_t* in, std::size_t inSize,
                         std::int32_t& value, std::size_t& bytesConsumed) noexcept {
    std::uint32_t zigzag = 0;
    auto st = decodeU32(in, inSize, zigzag, bytesConsumed);
    if (!st.ok()) return st;
    value = static_cast<std::int32_t>((zigzag >> 1) ^ (-static_cast<std::int32_t>(zigzag & 1)));
    return Status::success();
}

Status VarInt::encodeS64(std::int64_t value, std::uint8_t* out, std::size_t outSize,
                         std::size_t& bytesWritten) noexcept {
    // ZigZag: (n << 1) ^ (n >> 63)
    auto zigzag = static_cast<std::uint64_t>((static_cast<std::uint64_t>(value) << 1) ^
                                              (static_cast<std::uint64_t>(value) >> 63));
    return encodeU64(zigzag, out, outSize, bytesWritten);
}

Status VarInt::decodeS64(const std::uint8_t* in, std::size_t inSize,
                         std::int64_t& value, std::size_t& bytesConsumed) noexcept {
    std::uint64_t zigzag = 0;
    auto st = decodeU64(in, inSize, zigzag, bytesConsumed);
    if (!st.ok()) return st;
    value = static_cast<std::int64_t>((zigzag >> 1) ^ (-static_cast<std::int64_t>(zigzag & 1)));
    return Status::success();
}

std::size_t VarInt::encodedSizeU32(std::uint32_t value) noexcept {
    return encodedSizeImpl(value);
}

std::size_t VarInt::encodedSizeU64(std::uint64_t value) noexcept {
    return encodedSizeImpl(value);
}

std::size_t VarInt::encodedSizeS32(std::int32_t value) noexcept {
    auto zigzag = static_cast<std::uint32_t>((static_cast<std::uint32_t>(value) << 1) ^
                                              (static_cast<std::uint32_t>(value) >> 31));
    return encodedSizeImpl(zigzag);
}

std::size_t VarInt::encodedSizeS64(std::int64_t value) noexcept {
    auto zigzag = static_cast<std::uint64_t>((static_cast<std::uint64_t>(value) << 1) ^
                                              (static_cast<std::uint64_t>(value) >> 63));
    return encodedSizeImpl(zigzag);
}

} // namespace serialization
} // namespace quartz
