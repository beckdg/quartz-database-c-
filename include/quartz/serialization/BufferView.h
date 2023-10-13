#pragma once

#include "quartz/serialization/Buffer.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string_view>

namespace quartz {
namespace serialization {

class BufferView {
public:
    BufferView() noexcept = default;
    BufferView(const std::uint8_t* data, std::size_t size) noexcept;

    explicit BufferView(const Buffer& buffer) noexcept;
    BufferView(std::string_view data) noexcept;

    BufferView(const BufferView&) noexcept = default;
    BufferView& operator=(const BufferView&) noexcept = default;

    const std::uint8_t* data() const noexcept { return data_; }
    std::size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }

    const std::uint8_t& operator[](std::size_t index) const;
    const std::uint8_t& at(std::size_t index) const;

    const std::uint8_t* begin() const noexcept { return data_; }
    const std::uint8_t* end() const noexcept { return data_ + size_; }

    BufferView subview(std::size_t offset, std::size_t count) const;
    BufferView slice(std::size_t offset) const;

    void remove_prefix(std::size_t n);
    void remove_suffix(std::size_t n);

    std::string_view toString() const noexcept {
        return std::string_view(reinterpret_cast<const char*>(data_), size_);
    }

    bool operator==(const BufferView& other) const noexcept;
    bool operator!=(const BufferView& other) const noexcept { return !(*this == other); }

    static BufferView fromRaw(const void* data, std::size_t size) noexcept {
        return BufferView(static_cast<const std::uint8_t*>(data), size);
    }

private:
    const std::uint8_t* data_ = nullptr;
    std::size_t size_ = 0;
};

} // namespace serialization
} // namespace quartz
