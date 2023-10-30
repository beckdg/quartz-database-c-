#include "quartz/btree/SearchPath.h"

#include "quartz/common/Status.h"

namespace quartz {
namespace btree {

void SearchPath::clear() noexcept {
    entries_.clear();
}

void SearchPath::reserve(std::size_t capacity) {
    entries_.reserve(capacity);
}

void SearchPath::push(const SearchPathEntry& entry) {
    entries_.push_back(entry);
}

void SearchPath::pop() {
    if (!entries_.empty()) {
        entries_.pop_back();
    }
}

bool SearchPath::empty() const noexcept {
    return entries_.empty();
}

std::size_t SearchPath::depth() const noexcept {
    return entries_.size();
}

const SearchPathEntry& SearchPath::top() const {
    return entries_.back();
}

Status SearchPath::parentEntry(SearchPathEntry& out) const noexcept {
    if (entries_.size() < 2) {
        return Status::invalidArgument("SearchPath: no parent frame");
    }
    out = entries_[entries_.size() - 2];
    return Status::success();
}

const SearchPathEntry& SearchPath::at(std::size_t index) const {
    return entries_.at(index);
}

} // namespace btree
} // namespace quartz
