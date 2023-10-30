#include "quartz/btree/Key.h"

#include <cstring>
#include <sstream>

namespace quartz {
namespace btree {

Key::Key() noexcept {
    storage_.u32 = 0;
}

Key Key::fromUInt32(std::uint32_t value) noexcept {
    Key key;
    key.type_ = KeyType::UInt32;
    key.binarySize_ = 0;
    key.storage_.u32 = value;
    return key;
}

Key Key::fromUInt64(std::uint64_t value) noexcept {
    Key key;
    key.type_ = KeyType::UInt64;
    key.binarySize_ = 0;
    key.storage_.u64 = value;
    return key;
}

Key Key::fromBinary(const std::uint8_t* data, std::uint16_t size) {
    Key key;
    key.type_ = KeyType::Binary;
    key.binarySize_ = size;
    if (size > 0 && data != nullptr) {
        std::memcpy(key.storage_.binary, data, size);
    }
    return key;
}

Key Key::fromBinary(const std::vector<std::uint8_t>& data) {
    return fromBinary(data.data(), static_cast<std::uint16_t>(data.size()));
}

std::uint32_t Key::asUInt32() const noexcept {
    return storage_.u32;
}

std::uint64_t Key::asUInt64() const noexcept {
    return storage_.u64;
}

const std::uint8_t* Key::binaryData() const noexcept {
    return storage_.binary;
}

std::vector<std::uint8_t> Key::binaryCopy() const {
    return std::vector<std::uint8_t>(storage_.binary, storage_.binary + binarySize_);
}

int Key::compare(const Key& other) const noexcept {
    if (type_ != other.type_) {
        return static_cast<int>(type_) < static_cast<int>(other.type_) ? -1 : 1;
    }

    switch (type_) {
    case KeyType::UInt32:
        if (storage_.u32 < other.storage_.u32) return -1;
        if (storage_.u32 > other.storage_.u32) return 1;
        return 0;
    case KeyType::UInt64:
        if (storage_.u64 < other.storage_.u64) return -1;
        if (storage_.u64 > other.storage_.u64) return 1;
        return 0;
    case KeyType::Binary: {
        const std::uint16_t minSize =
            binarySize_ < other.binarySize_ ? binarySize_ : other.binarySize_;
        for (std::uint16_t i = 0; i < minSize; ++i) {
            if (storage_.binary[i] < other.storage_.binary[i]) return -1;
            if (storage_.binary[i] > other.storage_.binary[i]) return 1;
        }
        if (binarySize_ < other.binarySize_) return -1;
        if (binarySize_ > other.binarySize_) return 1;
        return 0;
    }
  default:
        return 0;
    }
}

bool Key::operator==(const Key& other) const noexcept {
    return compare(other) == 0;
}

bool Key::operator!=(const Key& other) const noexcept {
    return !(*this == other);
}

bool Key::operator<(const Key& other) const noexcept {
    return compare(other) < 0;
}

bool Key::operator<=(const Key& other) const noexcept {
    return compare(other) <= 0;
}

bool Key::operator>(const Key& other) const noexcept {
    return compare(other) > 0;
}

bool Key::operator>=(const Key& other) const noexcept {
    return compare(other) >= 0;
}

std::size_t Key::serializedSize() const noexcept {
    switch (type_) {
    case KeyType::UInt32:
        return 1 + sizeof(std::uint32_t);
    case KeyType::UInt64:
        return 1 + sizeof(std::uint64_t);
    case KeyType::Binary:
        return 1 + sizeof(std::uint16_t) + binarySize_;
    default:
        return 1;
    }
}

Status Key::serialize(serialization::BinaryWriter& writer) const {
    const auto typeByte = static_cast<std::uint8_t>(type_);
    auto st = writer.write(typeByte);
    if (!st.ok()) return st;

    switch (type_) {
    case KeyType::UInt32:
        return writer.write(storage_.u32);
    case KeyType::UInt64:
        return writer.write(storage_.u64);
    case KeyType::Binary: {
        st = writer.write(binarySize_);
        if (!st.ok()) return st;
        if (binarySize_ > 0) {
            return writer.writeBytes(storage_.binary, binarySize_);
        }
        return Status::success();
    }
    default:
        return Status::invalidArgument("Key: unsupported key type");
    }
}

Status Key::deserialize(serialization::BinaryReader& reader, std::uint16_t binaryKeySize) {
    std::uint8_t typeByte = 0;
    auto st = reader.read(typeByte);
    if (!st.ok()) return st;

    type_ = static_cast<KeyType>(typeByte);
    binarySize_ = 0;

    switch (type_) {
    case KeyType::UInt32:
        return reader.read(storage_.u32);
    case KeyType::UInt64:
        return reader.read(storage_.u64);
    case KeyType::Binary: {
        std::uint16_t size = 0;
        st = reader.read(size);
        if (!st.ok()) return st;
        if (binaryKeySize > 0 && size != binaryKeySize) {
            return Status::corruption("Key: binary key size mismatch");
        }
        if (size > kMaxBinaryKeySize) {
            return Status::corruption("Key: binary key exceeds maximum size");
        }
        binarySize_ = size;
        if (size > 0) {
            return reader.readBytes(storage_.binary, size);
        }
        return Status::success();
    }
    default:
        return Status::corruption("Key: unknown key type");
    }
}

std::string Key::toString() const {
    std::ostringstream oss;
    switch (type_) {
    case KeyType::UInt32:
        oss << "Key(u32:" << storage_.u32 << ")";
        break;
    case KeyType::UInt64:
        oss << "Key(u64:" << storage_.u64 << ")";
        break;
    case KeyType::Binary:
        oss << "Key(bin:" << static_cast<unsigned>(binarySize_) << ":[";
        for (std::uint16_t i = 0; i < binarySize_; ++i) {
            if (i > 0) oss << ',';
            oss << static_cast<unsigned>(storage_.binary[i]);
        }
        oss << "])";
        break;
    default:
        oss << "Key(invalid)";
        break;
    }
    return oss.str();
}

std::size_t Key::hash() const noexcept {
    std::size_t h = static_cast<std::size_t>(type_);
    switch (type_) {
    case KeyType::UInt32:
        h ^= static_cast<std::size_t>(storage_.u32) + 0x9e3779b9 + (h << 6) + (h >> 2);
        break;
    case KeyType::UInt64:
        h ^= static_cast<std::size_t>(storage_.u64) + 0x9e3779b9 + (h << 6) + (h >> 2);
        break;
    case KeyType::Binary:
        for (std::uint16_t i = 0; i < binarySize_; ++i) {
            h ^= static_cast<std::size_t>(storage_.binary[i]) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        break;
    default:
        break;
    }
    return h;
}

} // namespace btree
} // namespace quartz
