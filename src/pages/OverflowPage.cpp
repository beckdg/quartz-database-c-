#include "quartz/pages/OverflowPage.h"

#include <cstring>

namespace quartz {
namespace pages {

OverflowPage::OverflowPage()
    : BasePage(storage::Page(0, storage::PageType::Overflow)) {
}

OverflowPage::OverflowPage(storage::Page page)
    : BasePage(std::move(page)) {
}

std::unique_ptr<BasePage> OverflowPage::clone() const {
    storage::Page newPage(id(), storage::PageType::Overflow);
    std::memcpy(newPage.data(), page().data(), page().size());
    return std::make_unique<OverflowPage>(std::move(newPage));
}

OverflowPageLayout* OverflowPage::mutableLayout() noexcept {
    return reinterpret_cast<OverflowPageLayout*>(page().payload());
}

const OverflowPageLayout* OverflowPage::layout() const noexcept {
    return reinterpret_cast<const OverflowPageLayout*>(page().payload());
}

storage::PageId OverflowPage::nextPageId() const noexcept {
    return layout()->nextPageId;
}

void OverflowPage::setNextPageId(storage::PageId id) noexcept {
    mutableLayout()->nextPageId = id;
}

bool OverflowPage::hasNextPage() const noexcept {
    return nextPageId() != storage::kInvalidPageId;
}

std::uint32_t OverflowPage::payloadSize() const noexcept {
    return layout()->payloadSize;
}

void OverflowPage::setPayloadSize(std::uint32_t size) noexcept {
    mutableLayout()->payloadSize = size;
}

std::size_t OverflowPage::remainingCapacity() const noexcept {
    return layout()->remainingCapacity();
}

Status OverflowPage::validate() const {
    auto l = layout();
    if (!l->isValid()) {
        return Status::corruption("OverflowPage: invalid layout");
    }
    return Status::success();
}

void OverflowPage::reset(storage::PageId newId) {
    BasePage::reset(newId);
    auto l = mutableLayout();
    *l = OverflowPageLayout{};
    l->nextPageId = storage::kInvalidPageId;
}

OverflowPage OverflowPage::create(storage::PageId pageId) {
    OverflowPage page;
    page.reset(pageId);
    return page;
}

} // namespace pages
} // namespace quartz
