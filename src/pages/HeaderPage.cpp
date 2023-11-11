#include "quartz/pages/HeaderPage.h"

#include <cstring>

namespace quartz {
namespace pages {

HeaderPage::HeaderPage()
    : BasePage(storage::Page(storage::kHeaderPageId, storage::PageType::Header)) {
}

HeaderPage::HeaderPage(storage::Page page)
    : BasePage(std::move(page)) {
}

std::unique_ptr<BasePage> HeaderPage::clone() const {
    storage::Page newPage(id(), storage::PageType::Header);
    std::memcpy(newPage.data(), page().data(), page().size());
    return std::make_unique<HeaderPage>(std::move(newPage));
}

void HeaderPage::initFromFormat(const format::DatabaseHeader& dbHeader) {
    auto l = mutableLayout();
    l->databaseHeader = dbHeader;
    l->superblockPageId = 1;
    l->flags = 0;
    l->formatVersion = format::Versioning::encodeVersion(
        format::Versioning::kMajorVersion, format::Versioning::kMinorVersion);
}

const HeaderPageLayout* HeaderPage::layout() const noexcept {
    return reinterpret_cast<const HeaderPageLayout*>(page().payload());
}

HeaderPageLayout* HeaderPage::layout() noexcept {
    return mutableLayout();
}

HeaderPageLayout* HeaderPage::mutableLayout() noexcept {
    return reinterpret_cast<HeaderPageLayout*>(page().payload());
}

const format::DatabaseHeader& HeaderPage::databaseHeader() const noexcept {
    return layout()->databaseHeader;
}

storage::PageId HeaderPage::superblockPageId() const noexcept {
    return layout()->superblockPageId;
}

void HeaderPage::setSuperblockPageId(storage::PageId id) noexcept {
    mutableLayout()->superblockPageId = id;
}

std::uint64_t HeaderPage::flags() const noexcept {
    return layout()->flags;
}

void HeaderPage::setFlags(std::uint64_t f) noexcept {
    mutableLayout()->flags = f;
}

std::uint32_t HeaderPage::formatVersion() const noexcept {
    return layout()->formatVersion;
}

void HeaderPage::setFormatVersion(std::uint32_t v) noexcept {
    mutableLayout()->formatVersion = v;
}

Status HeaderPage::validate() const {
    auto l = layout();
    if (!l->isValid()) {
        return Status::corruption("HeaderPage: invalid layout");
    }
    return Status::success();
}

void HeaderPage::reset(storage::PageId newId) {
    BasePage::reset(newId);
    *mutableLayout() = HeaderPageLayout{};
}

HeaderPage HeaderPage::create(std::uint32_t majorVersion,
                               std::uint32_t minorVersion,
                               std::uint64_t featureFlags) {
    HeaderPage page;
    auto l = page.mutableLayout();
    l->databaseHeader.magic = format::MagicNumbers::kDatabaseMagic;
    l->databaseHeader.majorVersion = majorVersion;
    l->databaseHeader.minorVersion = minorVersion;
    l->databaseHeader.pageSize = static_cast<std::uint32_t>(config::kPageSize);
    l->superblockPageId = 1;
    l->flags = featureFlags;
    l->formatVersion = format::Versioning::encodeVersion(majorVersion, minorVersion);
    return page;
}

} // namespace pages
} // namespace quartz
