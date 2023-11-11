#include "quartz/pages/MetadataPage.h"

#include <cstring>

namespace quartz {
namespace pages {

MetadataPage::MetadataPage()
    : BasePage(storage::Page(0, storage::PageType::Metadata)) {
}

MetadataPage::MetadataPage(storage::Page page)
    : BasePage(std::move(page)) {
}

std::unique_ptr<BasePage> MetadataPage::clone() const {
    storage::Page newPage(id(), storage::PageType::Metadata);
    std::memcpy(newPage.data(), page().data(), page().size());
    return std::make_unique<MetadataPage>(std::move(newPage));
}

MetadataPageLayout* MetadataPage::mutableLayout() noexcept {
    return reinterpret_cast<MetadataPageLayout*>(page().payload());
}

const MetadataPageLayout* MetadataPage::layout() const noexcept {
    return reinterpret_cast<const MetadataPageLayout*>(page().payload());
}

std::uint32_t MetadataPage::entryCount() const noexcept {
    return layout()->entryCount;
}

void MetadataPage::setEntryCount(std::uint32_t count) noexcept {
    mutableLayout()->entryCount = count;
}

std::uint32_t MetadataPage::version() const noexcept {
    return layout()->version;
}

void MetadataPage::setVersion(std::uint32_t v) noexcept {
    mutableLayout()->version = v;
}

Status MetadataPage::validate() const {
    auto l = layout();
    if (!l->isValid()) {
        return Status::corruption("MetadataPage: invalid layout");
    }
    return Status::success();
}

void MetadataPage::reset(storage::PageId newId) {
    BasePage::reset(newId);
    *mutableLayout() = MetadataPageLayout{};
}

MetadataPage MetadataPage::create(storage::PageId pageId) {
    MetadataPage page;
    page.reset(pageId);
    return page;
}

} // namespace pages
} // namespace quartz
