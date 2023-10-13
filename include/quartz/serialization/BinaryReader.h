#pragma once

#include "quartz/common/Status.h"
#include "quartz/serialization/BufferView.h"
#include "quartz/serialization/VariableLengthInteger.h"
#include "quartz/util/Endian.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace quartz {
namespace serialization {

class BinaryReader {
public:
    explicit BinaryReader(BufferView buffer, endian::Order order = endian::Order::Little) noexcept;

    std::size_t tell() const noexcept { return offset_; }
    std::size_t remaining() const noexcept { return view_.size() - offset_; }
    bool hasMore() const noexcept { return offset_ < view_.size(); }

    Status seek(std::size_t position) noexcept;
    Status skip(std::size_t count) noexcept;
    Status align(std::size_t alignment) noexcept;

    Status readBytes(void* dest, std::size_t count) noexcept;
    Status readBytes(Buffer& dest, std::size_t count) noexcept;

    template <typename T>
    Status read(T& value) noexcept {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
        return readBytes(&value, sizeof(T));
    }

    template <typename T>
    Status readLE(T& value) noexcept {
        static_assert(std::is_trivially_copyable_v<T> && sizeof(T) <= 8,
                      "readLE requires trivially copyable type of at most 8 bytes");
        std::uint64_t raw = 0;
        auto st = readBytes(&raw, sizeof(T));
        if (!st.ok()) return st;
        value = static_cast<T>(endian::readLE64(&raw));
        return Status::success();
    }

    template <typename T>
    Status readBE(T& value) noexcept {
        static_assert(std::is_trivially_copyable_v<T> && sizeof(T) <= 8,
                      "readBE requires trivially copyable type of at most 8 bytes");
        std::uint64_t raw = 0;
        auto st = readBytes(&raw, sizeof(T));
        if (!st.ok()) return st;
        value = static_cast<T>(endian::readBE64(&raw));
        return Status::success();
    }

    Status readVarU32(std::uint32_t& value) noexcept;
    Status readVarU64(std::uint64_t& value) noexcept;
    Status readVarS32(std::int32_t& value) noexcept;
    Status readVarS64(std::int64_t& value) noexcept;

    BufferView view() const noexcept { return view_; }
    BufferView remainingView() const noexcept;

    const std::uint8_t* pointer() const noexcept;
    Status ensure(std::size_t count) const noexcept;

    BinaryReader subReader(std::size_t count) const noexcept;

private:
    BufferView view_;
    std::size_t offset_ = 0;
    endian::Order order_ = endian::Order::Little;
};

} // namespace serialization
} // namespace quartz
