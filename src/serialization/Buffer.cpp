#include "quartz/serialization/Buffer.h"

#include <algorithm>
#include <stdexcept>

namespace quartz {
namespace serialization {

Buffer::Buffer(std::size_t initialCapacity) {
    buffer_.reserve(std::max(initialCapacity, kMinCapacity));
}

Buffer::Buffer(Buffer&& other) noexcept
    : buffer_(std::move(other.buffer_)) {
}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this != &other) {
        buffer_ = std::move(other.buffer_);
    }
    return *this;
}

void Buffer::reserve(std::size_t newCapacity) {
    if (newCapacity > buffer_.capacity()) {
        buffer_.reserve(newCapacity);
    }
}

void Buffer::resize(std::size_t newSize) {
    buffer_.resize(newSize);
}

void Buffer::clear() noexcept {
    buffer_.clear();
}

Status Buffer::append(const void* src, std::size_t count) {
    if (count == 0) return Status::success();
    auto needed = buffer_.size() + count;
    if (needed > buffer_.capacity()) {
        grow(needed);
    }
    auto oldSize = buffer_.size();
    buffer_.resize(oldSize + count);
    std::memcpy(buffer_.data() + oldSize, src, count);
    return Status::success();
}

Status Buffer::append(const Buffer& other) {
    return append(other.data(), other.size());
}

Status Buffer::append(std::string_view data) {
    return append(data.data(), data.size());
}

std::uint8_t& Buffer::at(std::size_t index) {
    if (index >= buffer_.size()) {
        throw std::out_of_range("Buffer::at: index out of range");
    }
    return buffer_[index];
}

const std::uint8_t& Buffer::at(std::size_t index) const {
    if (index >= buffer_.size()) {
        throw std::out_of_range("Buffer::at: index out of range");
    }
    return buffer_[index];
}

void Buffer::swap(Buffer& other) noexcept {
    buffer_.swap(other.buffer_);
}

bool Buffer::operator==(const Buffer& other) const {
    return buffer_ == other.buffer_;
}

void Buffer::grow(std::size_t minCapacity) {
    auto newCap = std::max(buffer_.capacity() * 2, kMinCapacity);
    while (newCap < minCapacity) {
        newCap *= 2;
    }
    buffer_.reserve(newCap);
}

} // namespace serialization
} // namespace quartz
