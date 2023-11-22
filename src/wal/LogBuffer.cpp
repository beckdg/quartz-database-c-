#include "quartz/wal/LogBuffer.h"

#include "quartz/serialization/BufferView.h"

namespace quartz {
namespace wal {

LogBuffer::LogBuffer(std::size_t capacity)
    : capacity_(capacity > 0 ? capacity : 64 * 1024) {
    buffer_.reserve(capacity_);
}

Status LogBuffer::append(serialization::BufferView data) {
    if (buffer_.size() + data.size() > capacity_) {
        return Status::outOfMemory("LogBuffer: capacity exceeded");
    }
    return buffer_.append(data.data(), data.size());
}

Status LogBuffer::reserve(std::size_t additionalBytes) {
    if (buffer_.size() + additionalBytes > capacity_) {
        return Status::outOfMemory("LogBuffer: reserve exceeds capacity");
    }
    buffer_.reserve(buffer_.size() + additionalBytes);
    return Status::success();
}

Status LogBuffer::flush(const std::function<Status(serialization::BufferView)>& sink) {
    if (buffer_.empty()) {
        return Status::success();
    }
    auto st = sink(serialization::BufferView(buffer_));
    if (!st.ok()) {
        return st;
    }
    buffer_.clear();
    return Status::success();
}

void LogBuffer::clear() noexcept {
    buffer_.clear();
}

std::size_t LogBuffer::size() const noexcept {
    return buffer_.size();
}

std::size_t LogBuffer::capacity() const noexcept {
    return capacity_;
}

std::size_t LogBuffer::remainingCapacity() const noexcept {
    return capacity_ > buffer_.size() ? capacity_ - buffer_.size() : 0;
}

bool LogBuffer::empty() const noexcept {
    return buffer_.empty();
}

} // namespace wal
} // namespace quartz
