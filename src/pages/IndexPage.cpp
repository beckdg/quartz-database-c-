#include "quartz/pages/IndexPage.h"

#include <cstring>

namespace quartz {
namespace pages {

IndexPage::IndexPage()
    : BasePage(storage::Page(0, storage::PageType::Index)) {
}

IndexPage::IndexPage(storage::Page page)
    : BasePage(std::move(page)) {
}

std::unique_ptr<BasePage> IndexPage::clone() const {
    storage::Page newPage(id(), storage::PageType::Index);
    std::memcpy(newPage.data(), page().data(), page().size());
    return std::make_unique<IndexPage>(std::move(newPage));
}

const IndexPageLayout* IndexPage::layout() const noexcept {
    return reinterpret_cast<const IndexPageLayout*>(page().payload());
}

IndexPageLayout* IndexPage::layout() noexcept {
    return mutableLayout();
}

IndexPageLayout* IndexPage::mutableLayout() noexcept {
    return reinterpret_cast<IndexPageLayout*>(page().payload());
}

std::uint32_t IndexPage::nodeType() const noexcept {
    return layout()->nodeType;
}

void IndexPage::setNodeType(std::uint32_t type) noexcept {
    mutableLayout()->nodeType = type;
}

std::uint32_t IndexPage::keyCount() const noexcept {
    return layout()->keyCount;
}

void IndexPage::setKeyCount(std::uint32_t count) noexcept {
    mutableLayout()->keyCount = count;
}

std::uint32_t IndexPage::capacity() const noexcept {
    return layout()->capacity;
}

void IndexPage::setCapacity(std::uint32_t cap) noexcept {
    mutableLayout()->capacity = cap;
}

std::uint32_t IndexPage::flags() const noexcept {
    return layout()->flags;
}

void IndexPage::setFlags(std::uint32_t f) noexcept {
    mutableLayout()->flags = f;
}

Status IndexPage::validate() const {
    auto l = layout();
    if (!l->isValid()) {
        return Status::corruption("IndexPage: invalid layout");
    }
    return Status::success();
}

void IndexPage::reset(storage::PageId newId) {
    BasePage::reset(newId);
    *mutableLayout() = IndexPageLayout{};
}

IndexPage IndexPage::create(storage::PageId pageId) {
    IndexPage page;
    page.reset(pageId);
    return page;
}

} // namespace pages
} // namespace quartz
