#include "quartz/serialization/BinaryReader.h"
#include "quartz/serialization/Buffer.h"

#include <algorithm>

namespace quartz {
namespace serialization {

BinaryReader::BinaryReader(BufferView buffer, endian::Order order) noexcept
    : view_(buffer)
    , offset_(0)
    , order_(order) {
}

Status BinaryReader::seek(std::size_t position) noexcept {
    if (position > view_.size()) {
        return Status::invalidArgument("BinaryReader::seek: position out of range");
    }
    offset_ = position;
    return Status::success();
}

Status BinaryReader::skip(std::size_t count) noexcept {
    if (offset_ + count > view_.size()) {
        return Status::corruption("BinaryReader::skip: unexpected end of input");
    }
    offset_ += count;
    return Status::success();
}

Status BinaryReader::align(std::size_t alignment) noexcept {
    if (alignment == 0) return Status::success();
    auto misalignment = offset_ % alignment;
    if (misalignment != 0) {
        return skip(alignment - misalignment);
    }
    return Status::success();
}

Status BinaryReader::readBytes(void* dest, std::size_t count) noexcept {
    if (count == 0) return Status::success();
    if (offset_ + count > view_.size()) {
        return Status::corruption("BinaryReader: unexpected end of input");
    }
    std::memcpy(dest, view_.data() + offset_, count);
    offset_ += count;
    return Status::success();
}

Status BinaryReader::readBytes(Buffer& dest, std::size_t count) noexcept {
    if (offset_ + count > view_.size()) {
        return Status::corruption("BinaryReader: unexpected end of input");
    }
    auto st = dest.append(view_.data() + offset_, count);
    if (!st.ok()) return st;
    offset_ += count;
    return Status::success();
}

Status BinaryReader::readVarU32(std::uint32_t& value) noexcept {
    std::size_t consumed = 0;
    auto st = VarInt::decodeU32(view_.data() + offset_, remaining(), value, consumed);
    if (!st.ok()) return st;
    offset_ += consumed;
    return Status::success();
}

Status BinaryReader::readVarU64(std::uint64_t& value) noexcept {
    std::size_t consumed = 0;
    auto st = VarInt::decodeU64(view_.data() + offset_, remaining(), value, consumed);
    if (!st.ok()) return st;
    offset_ += consumed;
    return Status::success();
}

Status BinaryReader::readVarS32(std::int32_t& value) noexcept {
    std::size_t consumed = 0;
    auto st = VarInt::decodeS32(view_.data() + offset_, remaining(), value, consumed);
    if (!st.ok()) return st;
    offset_ += consumed;
    return Status::success();
}

Status BinaryReader::readVarS64(std::int64_t& value) noexcept {
    std::size_t consumed = 0;
    auto st = VarInt::decodeS64(view_.data() + offset_, remaining(), value, consumed);
    if (!st.ok()) return st;
    offset_ += consumed;
    return Status::success();
}

BufferView BinaryReader::remainingView() const noexcept {
    return view_.subview(offset_, remaining());
}

const std::uint8_t* BinaryReader::pointer() const noexcept {
    return view_.data() + offset_;
}

Status BinaryReader::ensure(std::size_t count) const noexcept {
    if (offset_ + count > view_.size()) {
        return Status::corruption("BinaryReader: unexpected end of input");
    }
    return Status::success();
}

BinaryReader BinaryReader::subReader(std::size_t count) const noexcept {
    auto sub = view_.subview(offset_, count);
    return BinaryReader(sub, order_);
}

} // namespace serialization
} // namespace quartz
