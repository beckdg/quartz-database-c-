#include "quartz/serialization/BinaryWriter.h"

#include <algorithm>

namespace quartz {
namespace serialization {

BinaryWriter::BinaryWriter(Buffer& buffer, endian::Order order) noexcept
    : buffer_(buffer)
    , offset_(buffer.size())
    , order_(order) {
}

std::size_t BinaryWriter::remainingCapacity() const noexcept {
    if (buffer_.capacity() < offset_) return 0;
    return buffer_.capacity() - offset_;
}

Status BinaryWriter::seek(std::size_t position) noexcept {
    if (position > buffer_.size()) {
        return Status::invalidArgument("BinaryWriter::seek: position out of range");
    }
    offset_ = position;
    return Status::success();
}

Status BinaryWriter::skip(std::size_t count) noexcept {
    auto st = ensureCapacity(offset_ + count);
    if (!st.ok()) return st;
    offset_ += count;
    if (offset_ > buffer_.size()) {
        buffer_.resize(offset_);
    }
    return Status::success();
}

Status BinaryWriter::align(std::size_t alignment) noexcept {
    if (alignment == 0) return Status::success();
    auto misalignment = offset_ % alignment;
    if (misalignment != 0) {
        return skip(alignment - misalignment);
    }
    return Status::success();
}

Status BinaryWriter::writeBytes(const void* src, std::size_t count) noexcept {
    if (count == 0) return Status::success();
    auto st = ensureCapacity(offset_ + count);
    if (!st.ok()) return st;
    auto* dest = buffer_.data() + offset_;
    std::memcpy(dest, src, count);
    offset_ += count;
    if (offset_ > buffer_.size()) {
        buffer_.resize(offset_);
    }
    return Status::success();
}

Status BinaryWriter::writeBytes(const Buffer& src) noexcept {
    return writeBytes(src.data(), src.size());
}

Status BinaryWriter::writeBytes(BufferView src) noexcept {
    return writeBytes(src.data(), src.size());
}

Status BinaryWriter::writeVarU32(std::uint32_t value) noexcept {
    std::uint8_t buf[VarInt::kMaxVarInt32Bytes];
    std::size_t written = 0;
    auto st = VarInt::encodeU32(value, buf, sizeof(buf), written);
    if (!st.ok()) return st;
    return writeBytes(buf, written);
}

Status BinaryWriter::writeVarU64(std::uint64_t value) noexcept {
    std::uint8_t buf[VarInt::kMaxVarInt64Bytes];
    std::size_t written = 0;
    auto st = VarInt::encodeU64(value, buf, sizeof(buf), written);
    if (!st.ok()) return st;
    return writeBytes(buf, written);
}

Status BinaryWriter::writeVarS32(std::int32_t value) noexcept {
    std::uint8_t buf[VarInt::kMaxVarInt32Bytes];
    std::size_t written = 0;
    auto st = VarInt::encodeS32(value, buf, sizeof(buf), written);
    if (!st.ok()) return st;
    return writeBytes(buf, written);
}

Status BinaryWriter::writeVarS64(std::int64_t value) noexcept {
    std::uint8_t buf[VarInt::kMaxVarInt64Bytes];
    std::size_t written = 0;
    auto st = VarInt::encodeS64(value, buf, sizeof(buf), written);
    if (!st.ok()) return st;
    return writeBytes(buf, written);
}

Status BinaryWriter::overwriteAt(std::size_t position, const void* src,
                                  std::size_t count) noexcept {
    if (position + count > buffer_.size()) {
        return Status::invalidArgument("BinaryWriter::overwriteAt: range out of bounds");
    }
    std::memcpy(buffer_.data() + position, src, count);
    return Status::success();
}

Status BinaryWriter::ensureCapacity(std::size_t needed) noexcept {
    if (needed <= buffer_.capacity()) {
        return Status::success();
    }
    auto newCap = std::max(buffer_.capacity() * 2, Buffer::kMinCapacity);
    while (newCap < needed) {
        newCap *= 2;
    }
    buffer_.reserve(newCap);
    return Status::success();
}

} // namespace serialization
} // namespace quartz
