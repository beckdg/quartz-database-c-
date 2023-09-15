#pragma once

#include <cstdint>

namespace quartz {
namespace btree {

/// On-disk magic for B-tree node body sections stored in IndexPageLayout::data.
inline constexpr std::uint32_t kNodeBodyMagic = 0x45525442; // "BTRE" in little-endian

/// Maximum supported fixed binary key length in bytes.
inline constexpr std::uint16_t kMaxBinaryKeySize = 256;

/// Discriminator for key payload encoding.
enum class KeyType : std::uint8_t {
    UInt32 = 1,
    UInt64 = 2,
    Binary = 3,
};

/// B-tree node kind stored in IndexPageLayout::nodeType.
enum class NodeType : std::uint32_t {
    Leaf     = 1,
    Internal = 2,
};

/// Node-level flags stored in IndexPageLayout::flags.
enum class NodeFlags : std::uint32_t {
    None              = 0,
    AllowDuplicates   = 1u << 0,
};

inline NodeFlags operator|(NodeFlags a, NodeFlags b) noexcept {
    return static_cast<NodeFlags>(static_cast<std::uint32_t>(a) |
                                  static_cast<std::uint32_t>(b));
}

inline NodeFlags operator&(NodeFlags a, NodeFlags b) noexcept {
    return static_cast<NodeFlags>(static_cast<std::uint32_t>(a) &
                                  static_cast<std::uint32_t>(b));
}

inline bool hasFlag(NodeFlags flags, NodeFlags bit) noexcept {
    return (flags & bit) == bit;
}

/// Configuration applied when creating or interpreting a node.
struct BTreeNodeConfig {
    KeyType keyType = KeyType::UInt32;
    std::uint16_t binaryKeySize = 0;
    bool allowDuplicates = false;
    std::uint32_t level = 0;
};

/// Minimum key count before a non-root node is considered underfull.
inline std::uint32_t minKeyCount(std::uint32_t capacity, bool isRoot) noexcept {
    if (isRoot) {
        return 0;
    }
    return capacity / 2;
}

/// Returns true when a non-root node has fallen below minimum occupancy.
inline bool isUnderfull(std::uint32_t keyCount, std::uint32_t capacity, bool isRoot) noexcept {
    return !isRoot && keyCount < minKeyCount(capacity, false);
}

} // namespace btree
} // namespace quartz
