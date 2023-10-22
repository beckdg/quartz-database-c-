#pragma once

#include <cstdint>
#include <cstring>

#if defined(_MSC_VER)
#include <cstdlib>
#endif

namespace quartz {
namespace endian {

enum class Order {
    Little,
    Big
};

inline bool isLittleEndian() noexcept {
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
    return __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__;
#elif defined(_WIN32)
    return true;
#else
    constexpr uint16_t test = 0x0001;
    uint8_t byte = 0;
    std::memcpy(&byte, &test, sizeof(byte));
    return byte == 0x01;
#endif
}

inline bool isBigEndian() noexcept {
    return !isLittleEndian();
}

inline Order nativeOrder() noexcept {
    return isLittleEndian() ? Order::Little : Order::Big;
}

inline uint16_t swap16(uint16_t value) noexcept {
#if defined(_MSC_VER)
    return _byteswap_ushort(value);
#elif defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap16(value);
#else
    return static_cast<uint16_t>((value << 8) | (value >> 8));
#endif
}

inline uint32_t swap32(uint32_t value) noexcept {
#if defined(_MSC_VER)
    return _byteswap_ulong(value);
#elif defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap32(value);
#else
    return ((value >> 24) & 0x000000FFu) | ((value >> 8) & 0x0000FF00u) |
           ((value << 8) & 0x00FF0000u) | ((value << 24) & 0xFF000000u);
#endif
}

inline uint64_t swap64(uint64_t value) noexcept {
#if defined(_MSC_VER)
    return _byteswap_uint64(value);
#elif defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap64(value);
#else
    uint32_t lo = static_cast<uint32_t>(value & 0xFFFFFFFFu);
    uint32_t hi = static_cast<uint32_t>(value >> 32);
    return (static_cast<uint64_t>(swap32(lo)) << 32) | static_cast<uint64_t>(swap32(hi));
#endif
}

inline uint16_t readLE16(const void* ptr) noexcept {
    uint16_t value;
    std::memcpy(&value, ptr, sizeof(value));
    if (isBigEndian()) { value = swap16(value); }
    return value;
}

inline uint32_t readLE32(const void* ptr) noexcept {
    uint32_t value;
    std::memcpy(&value, ptr, sizeof(value));
    if (isBigEndian()) { value = swap32(value); }
    return value;
}

inline uint64_t readLE64(const void* ptr) noexcept {
    uint64_t value;
    std::memcpy(&value, ptr, sizeof(value));
    if (isBigEndian()) { value = swap64(value); }
    return value;
}

inline uint16_t readBE16(const void* ptr) noexcept {
    uint16_t value;
    std::memcpy(&value, ptr, sizeof(value));
    if (isLittleEndian()) { value = swap16(value); }
    return value;
}

inline uint32_t readBE32(const void* ptr) noexcept {
    uint32_t value;
    std::memcpy(&value, ptr, sizeof(value));
    if (isLittleEndian()) { value = swap32(value); }
    return value;
}

inline uint64_t readBE64(const void* ptr) noexcept {
    uint64_t value;
    std::memcpy(&value, ptr, sizeof(value));
    if (isLittleEndian()) { value = swap64(value); }
    return value;
}

inline void writeLE16(void* ptr, uint16_t value) noexcept {
    if (isBigEndian()) { value = swap16(value); }
    std::memcpy(ptr, &value, sizeof(value));
}

inline void writeLE32(void* ptr, uint32_t value) noexcept {
    if (isBigEndian()) { value = swap32(value); }
    std::memcpy(ptr, &value, sizeof(value));
}

inline void writeLE64(void* ptr, uint64_t value) noexcept {
    if (isBigEndian()) { value = swap64(value); }
    std::memcpy(ptr, &value, sizeof(value));
}

inline void writeBE16(void* ptr, uint16_t value) noexcept {
    if (isLittleEndian()) { value = swap16(value); }
    std::memcpy(ptr, &value, sizeof(value));
}

inline void writeBE32(void* ptr, uint32_t value) noexcept {
    if (isLittleEndian()) { value = swap32(value); }
    std::memcpy(ptr, &value, sizeof(value));
}

inline void writeBE64(void* ptr, uint64_t value) noexcept {
    if (isLittleEndian()) { value = swap64(value); }
    std::memcpy(ptr, &value, sizeof(value));
}

} // namespace endian
} // namespace quartz
