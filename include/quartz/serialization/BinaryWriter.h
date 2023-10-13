#pragma once

#include "quartz/common/Status.h"
#include "quartz/serialization/Buffer.h"
#include "quartz/serialization/BufferView.h"
#include "quartz/serialization/VariableLengthInteger.h"
#include "quartz/util/Endian.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace quartz {
namespace serialization {

class BinaryWriter {
public:
    explicit BinaryWriter(Buffer& buffer, endian::Order order = endian::Order::Little) noexcept;

    std::size_t tell() const noexcept { return offset_; }
    std::size_t remainingCapacity() const noexcept;

    Status seek(std::size_t position) noexcept;
    Status skip(std::size_t count) noexcept;
    Status align(std::size_t alignment) noexcept;

    Status writeBytes(const void* src, std::size_t count) noexcept;
    Status writeBytes(const Buffer& src) noexcept;
    Status writeBytes(BufferView src) noexcept;

    template <typename T>
    Status write(const T& value) noexcept {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
        return writeBytes(&value, sizeof(T));
    }

    template <typename T>
    Status writeLE(T value) noexcept {
        static_assert(std::is_trivially_copyable_v<T> && sizeof(T) <= 8,
                      "writeLE requires trivially copyable type of at most 8 bytes");
        std::uint64_t raw = 0;
        std::memcpy(&raw, &value, sizeof(T));
        endian::writeLE64(&raw, raw);
        return writeBytes(&raw, sizeof(T));
    }

    template <typename T>
    Status writeBE(T value) noexcept {
        static_assert(std::is_trivially_copyable_v<T> && sizeof(T) <= 8,
                      "writeBE requires trivially copyable type of at most 8 bytes");
        std::uint64_t raw = 0;
        std::memcpy(&raw, &value, sizeof(T));
        endian::writeBE64(&raw, raw);
        return writeBytes(&raw, sizeof(T));
    }

    Status writeVarU32(std::uint32_t value) noexcept;
    Status writeVarU64(std::uint64_t value) noexcept;
    Status writeVarS32(std::int32_t value) noexcept;
    Status writeVarS64(std::int64_t value) noexcept;

    Status overwriteAt(std::size_t position, const void* src, std::size_t count) noexcept;

    Buffer& buffer() noexcept { return buffer_; }
    const Buffer& buffer() const noexcept { return buffer_; }

private:
    Status ensureCapacity(std::size_t needed) noexcept;

    Buffer& buffer_;
    std::size_t offset_ = 0;
    endian::Order order_ = endian::Order::Little;
};

} // namespace serialization
} // namespace quartz
