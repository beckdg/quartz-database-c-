#pragma once

#include "quartz/btree/Key.h"

#include <cstddef>
#include <vector>

namespace quartz {
namespace btree {

/// Comparison utilities for keys and key containers.
class KeyComparator {
public:
    /// Ascending three-way comparison.
    static int compare(const Key& lhs, const Key& rhs) noexcept {
        return lhs.compare(rhs);
    }

    static bool equal(const Key& lhs, const Key& rhs) noexcept {
        return lhs == rhs;
    }

    static bool less(const Key& lhs, const Key& rhs) noexcept {
        return lhs < rhs;
    }

    static bool lessEqual(const Key& lhs, const Key& rhs) noexcept {
        return lhs <= rhs;
    }

    static bool greater(const Key& lhs, const Key& rhs) noexcept {
        return lhs > rhs;
    }

    static bool greaterEqual(const Key& lhs, const Key& rhs) noexcept {
        return lhs >= rhs;
    }

    /// Binary-search compatible lower bound: first index where keys[i] >= key.
    template <typename Container>
    static std::size_t lowerBound(const Container& keys, const Key& key) {
        std::size_t lo = 0;
        std::size_t hi = keys.size();
        while (lo < hi) {
            const std::size_t mid = lo + (hi - lo) / 2;
            if (less(keys[mid], key)) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return lo;
    }

    /// Binary-search compatible upper bound: first index where keys[i] > key.
    template <typename Container>
    static std::size_t upperBound(const Container& keys, const Key& key) {
        std::size_t lo = 0;
        std::size_t hi = keys.size();
        while (lo < hi) {
            const std::size_t mid = lo + (hi - lo) / 2;
            if (lessEqual(keys[mid], key)) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return lo;
    }

    /// Find exact match index or return keys.size() if not found.
    template <typename Container>
    static std::size_t find(const Container& keys, const Key& key) {
        const std::size_t pos = lowerBound(keys, key);
        if (pos < keys.size() && equal(keys[pos], key)) {
            return pos;
        }
        return keys.size();
    }
};

} // namespace btree
} // namespace quartz
