#include "quartz/serialization/BufferView.h"

#include <algorithm>
#include <stdexcept>

namespace quartz {
namespace serialization {

BufferView::BufferView(const std::uint8_t* data, std::size_t size) noexcept
    : data_(data)
    , size_(size) {
}

BufferView::BufferView(const Buffer& buffer) noexcept
    : data_(buffer.data())
    , size_(buffer.size()) {
}

BufferView::BufferView(std::string_view data) noexcept
    : data_(reinterpret_cast<const std::uint8_t*>(data.data()))
    , size_(data.size()) {
}

const std::uint8_t& BufferView::operator[](std::size_t index) const {
    return data_[index];
}

const std::uint8_t& BufferView::at(std::size_t index) const {
    if (index >= size_) {
        throw std::out_of_range("BufferView::at: index out of range");
    }
    return data_[index];
}

BufferView BufferView::subview(std::size_t offset, std::size_t count) const {
    if (offset > size_) {
        return BufferView();
    }
    auto actualCount = std::min(count, size_ - offset);
    return BufferView(data_ + offset, actualCount);
}

BufferView BufferView::slice(std::size_t offset) const {
    if (offset > size_) {
        return BufferView();
    }
    return BufferView(data_ + offset, size_ - offset);
}

void BufferView::remove_prefix(std::size_t n) {
    auto actual = std::min(n, size_);
    data_ += actual;
    size_ -= actual;
}

void BufferView::remove_suffix(std::size_t n) {
    auto actual = std::min(n, size_);
    size_ -= actual;
}

bool BufferView::operator==(const BufferView& other) const noexcept {
    if (size_ != other.size_) return false;
    if (data_ == other.data_) return true;
    return std::memcmp(data_, other.data_, size_) == 0;
}

} // namespace serialization
} // namespace quartz
