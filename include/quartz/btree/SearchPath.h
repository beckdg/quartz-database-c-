#pragma once

#include "quartz/common/Status.h"
#include "quartz/format/PageReference.h"
#include "quartz/storage/StorageConstants.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace quartz {
namespace btree {

/// One frame in a future tree traversal path.
struct SearchPathEntry {
    storage::PageId pageId = storage::kInvalidPageId;
    std::uint32_t childIndex = 0;
    format::PageReference pageRef;
};

/// Stack of parent nodes for future top-down traversal.
class SearchPath {
public:
    void clear() noexcept;
    void reserve(std::size_t capacity);
    void push(const SearchPathEntry& entry);
    void pop();
    bool empty() const noexcept;
    std::size_t depth() const noexcept;

    const SearchPathEntry& top() const;
    const SearchPathEntry& at(std::size_t index) const;

    /// Returns the parent frame when depth >= 2.
    Status parentEntry(SearchPathEntry& out) const noexcept;

private:
    std::vector<SearchPathEntry> entries_;
};

} // namespace btree
} // namespace quartz
