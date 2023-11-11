#include "quartz/pages/FreeListPage.h"

#include <cstring>

namespace quartz {
namespace pages {

FreeListPage::FreeListPage()
    : BasePage(storage::Page(0, storage::PageType::FreeList)) {
}

FreeListPage::FreeListPage(storage::Page page)
    : BasePage(std::move(page)) {
}

std::unique_ptr<BasePage> FreeListPage::clone() const {
    storage::Page newPage(id(), storage::PageType::FreeList);
    std::memcpy(newPage.data(), page().data(), page().size());
    return std::make_unique<FreeListPage>(std::move(newPage));
}

FreeListPageLayout* FreeListPage::mutableLayout() noexcept {
    return reinterpret_cast<FreeListPageLayout*>(page().payload());
}

const FreeListPageLayout* FreeListPage::layout() const noexcept {
    return reinterpret_cast<const FreeListPageLayout*>(page().payload());
}

std::uint32_t FreeListPage::freeCount() const noexcept {
    return layout()->freeCount;
}

std::uint32_t FreeListPage::capacity() const noexcept {
    return layout()->capacity;
}

bool FreeListPage::isFull() const noexcept {
    return freeCount() >= capacity();
}

bool FreeListPage::isEmpty() const noexcept {
    return freeCount() == 0;
}

storage::PageId FreeListPage::freePage(std::size_t index) const noexcept {
    if (index >= freeCount()) return storage::kInvalidPageId;
    return layout()->freePages[index];
}

Status FreeListPage::setFreePage(std::size_t index, storage::PageId pageId) noexcept {
    if (index >= capacity()) {
        return Status::invalidArgument("FreeListPage: index out of range");
    }
    mutableLayout()->freePages[index] = pageId;
    return Status::success();
}

Status FreeListPage::addFreePage(storage::PageId pageId) noexcept {
    auto l = mutableLayout();
    if (l->freeCount >= l->capacity) {
        return Status::invalidArgument("FreeListPage: free list is full");
    }
    l->freePages[l->freeCount] = pageId;
    ++l->freeCount;
    return Status::success();
}

void FreeListPage::clearFreePages() noexcept {
    auto l = mutableLayout();
    l->freeCount = 0;
}

Status FreeListPage::validate() const {
    auto l = layout();
    if (!l->isValid()) {
        return Status::corruption("FreeListPage: invalid layout");
    }
    return Status::success();
}

void FreeListPage::reset(storage::PageId newId) {
    BasePage::reset(newId);
    auto l = mutableLayout();
    *l = FreeListPageLayout{};
    l->capacity = 1000;
}

FreeListPage FreeListPage::create(storage::PageId pageId) {
    FreeListPage page;
    page.reset(pageId);
    return page;
}

} // namespace pages
} // namespace quartz
