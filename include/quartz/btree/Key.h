#pragma once

#include "quartz/btree/BTreeTypes.h"
#include "quartz/common/Status.h"
#include "quartz/serialization/BinaryReader.h"
#include "quartz/serialization/BinaryWriter.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace quartz {
namespace btree {

/// Tagged key value supporting uint32, uint64, and fixed-size binary payloads.
/// Uses a closed tagged representation without virtual inheritance.
class Key {
public:
    Key() noexcept;

    static Key fromUInt32(std::uint32_t value) noexcept;
    static Key fromUInt64(std::uint64_t value) noexcept;
    static Key fromBinary(const std::uint8_t* data, std::uint16_t size);
    static Key fromBinary(const std::vector<std::uint8_t>& data);

    KeyType type() const noexcept { return type_; }
    std::uint16_t binarySize() const noexcept { return binarySize_; }

    std::uint32_t asUInt32() const noexcept;
    std::uint64_t asUInt64() const noexcept;
    const std::uint8_t* binaryData() const noexcept;
    std::vector<std::uint8_t> binaryCopy() const;

    /// Three-way comparison. Keys of different types are ordered by KeyType value.
    int compare(const Key& other) const noexcept;

    bool operator==(const Key& other) const noexcept;
    bool operator!=(const Key& other) const noexcept;
    bool operator<(const Key& other) const noexcept;
    bool operator<=(const Key& other) const noexcept;
    bool operator>(const Key& other) const noexcept;
    bool operator>=(const Key& other) const noexcept;

    /// Serialized size on the wire for this key instance.
    std::size_t serializedSize() const noexcept;

    /// Serialize key type tag and payload.
    Status serialize(serialization::BinaryWriter& writer) const;

    /// Deserialize key. For binary keys, binaryKeySize must match the configured fixed size.
    Status deserialize(serialization::BinaryReader& reader, std::uint16_t binaryKeySize = 0);

    std::string toString() const;

    std::size_t hash() const noexcept;

private:
    KeyType type_ = KeyType::UInt32;
    std::uint16_t binarySize_ = 0;
    union Storage {
        std::uint32_t u32;
        std::uint64_t u64;
        std::uint8_t binary[kMaxBinaryKeySize];
    } storage_{};
};

} // namespace btree
} // namespace quartz

namespace std {

template <>
struct hash<quartz::btree::Key> {
    std::size_t operator()(const quartz::btree::Key& key) const noexcept {
        return key.hash();
    }
};

} // namespace std
