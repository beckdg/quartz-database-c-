#pragma once

#include "quartz/common/NonCopyable.h"
#include "quartz/common/Status.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <vector>

namespace quartz {
namespace serialization {

class Buffer : private NonCopyable {
public:
    Buffer() noexcept = default;
    explicit Buffer(std::size_t initialCapacity);
    ~Buffer() noexcept = default;

    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    void reserve(std::size_t newCapacity);
    void resize(std::size_t newSize);
    void clear() noexcept;

    Status append(const void* src, std::size_t count);
    Status append(const Buffer& other);
    Status append(std::string_view data);

    template <typename T>
    Status appendValue(const T& value) {
        return append(&value, sizeof(T));
    }

    std::uint8_t* data() noexcept { return buffer_.data(); }
    const std::uint8_t* data() const noexcept { return buffer_.data(); }

    std::size_t size() const noexcept { return buffer_.size(); }
    std::size_t capacity() const noexcept { return buffer_.capacity(); }
    bool empty() const noexcept { return buffer_.empty(); }

    std::uint8_t& at(std::size_t index);
    const std::uint8_t& at(std::size_t index) const;

    std::uint8_t& operator[](std::size_t index) { return buffer_[index]; }
    const std::uint8_t& operator[](std::size_t index) const { return buffer_[index]; }

    std::uint8_t* begin() noexcept { return buffer_.data(); }
    const std::uint8_t* begin() const noexcept { return buffer_.data(); }
    std::uint8_t* end() noexcept { return buffer_.data() + buffer_.size(); }
    const std::uint8_t* end() const noexcept { return buffer_.data() + buffer_.size(); }

    std::string_view toStringView() const noexcept {
        return std::string_view(reinterpret_cast<const char*>(buffer_.data()), buffer_.size());
    }

    void swap(Buffer& other) noexcept;

    bool operator==(const Buffer& other) const;
    bool operator!=(const Buffer& other) const { return !(*this == other); }

    static constexpr std::size_t kMinCapacity = 64;

private:
    void grow(std::size_t minCapacity);

    std::vector<std::uint8_t> buffer_;
};

} // namespace serialization
} // namespace quartz
