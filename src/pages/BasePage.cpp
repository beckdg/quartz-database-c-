#include "quartz/pages/BasePage.h"

#include <cstring>

namespace quartz {
namespace pages {

BasePage::BasePage(storage::Page page)
    : page_(std::move(page)) {
}

Status BasePage::serialize(serialization::BinaryWriter& writer) const {
    return writer.writeBytes(page_.data(), page_.size());
}

Status BasePage::deserialize(serialization::BinaryReader& reader) {
    auto st = reader.readBytes(page_.data(), page_.size());
    if (!st.ok()) return st;
    // Sync the PageHeader member from the raw data buffer
    std::memcpy(&page_.header(), page_.data(), sizeof(storage::PageHeader));
    return Status::success();
}

void BasePage::reset(storage::PageId newId) {
    page_.reset(newId, static_cast<storage::PageType>(layoutType()));
}

void BasePage::clear() {
    page_.zeroFill();
}

std::string BasePage::toString() const {
    return std::string(page_.toString());
}

} // namespace pages
} // namespace quartz
