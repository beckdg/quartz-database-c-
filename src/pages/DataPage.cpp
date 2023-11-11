#include "quartz/pages/DataPage.h"

#include <cstring>

namespace quartz {
namespace pages {

DataPage::DataPage()
    : BasePage(storage::Page(0, storage::PageType::Data)) {
}

DataPage::DataPage(storage::Page page)
    : BasePage(std::move(page)) {
}

std::unique_ptr<BasePage> DataPage::clone() const {
    storage::Page newPage(id(), storage::PageType::Data);
    std::memcpy(newPage.data(), page().data(), page().size());
    return std::make_unique<DataPage>(std::move(newPage));
}

DataPageLayout* DataPage::mutableLayout() noexcept {
    return reinterpret_cast<DataPageLayout*>(page().payload());
}

const DataPageLayout* DataPage::layout() const noexcept {
    return reinterpret_cast<const DataPageLayout*>(page().payload());
}

std::uint16_t DataPage::freeSpaceOffset() const noexcept {
    return layout()->freeSpaceOffset;
}

void DataPage::setFreeSpaceOffset(std::uint16_t offset) noexcept {
    mutableLayout()->freeSpaceOffset = offset;
}

std::uint16_t DataPage::slotCount() const noexcept {
    return layout()->slotCount;
}

void DataPage::setSlotCount(std::uint16_t count) noexcept {
    mutableLayout()->slotCount = count;
}

std::size_t DataPage::availableSpace() const noexcept {
    return layout()->availableSpace();
}

Status DataPage::validate() const {
    auto l = layout();
    if (!l->isValid()) {
        return Status::corruption("DataPage: invalid layout");
    }
    return Status::success();
}

void DataPage::reset(storage::PageId newId) {
    BasePage::reset(newId);
    *mutableLayout() = DataPageLayout{};
}

DataPage DataPage::create(storage::PageId pageId) {
    DataPage page;
    page.reset(pageId);
    return page;
}

} // namespace pages
} // namespace quartz
