#pragma once

#include "quartz/common/NonCopyable.h"
#include "quartz/common/Status.h"
#include "quartz/serialization/Buffer.h"
#include "quartz/serialization/BufferView.h"

#include <cstddef>
#include <cstdint>
#include <functional>

namespace quartz {
namespace wal {

/// In-memory buffer for batching log records before file flush.
class LogBuffer : private NonCopyable {
public:
    explicit LogBuffer(std::size_t capacity = 64 * 1024);

    Status append(serialization::BufferView data);
    Status reserve(std::size_t additionalBytes);
    Status flush(const std::function<Status(serialization::BufferView)>& sink);
    void clear() noexcept;

    std::size_t size() const noexcept;
    std::size_t capacity() const noexcept;
    std::size_t remainingCapacity() const noexcept;
    bool empty() const noexcept;

    const serialization::Buffer& data() const noexcept { return buffer_; }

private:
    serialization::Buffer buffer_;
    std::size_t capacity_;
};

} // namespace wal
} // namespace quartz
