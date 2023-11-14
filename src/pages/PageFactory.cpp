#include "quartz/pages/PageFactory.h"

#include <cstring>

namespace quartz {
namespace pages {

std::unique_ptr<BasePage> PageFactory::createPage(storage::Page page) {
    auto type = toPageLayoutType(page.type());
    switch (type) {
        case PageLayoutType::Header:
            return std::make_unique<HeaderPage>(std::move(page));
        case PageLayoutType::FreeList:
            return std::make_unique<FreeListPage>(std::move(page));
        case PageLayoutType::Data:
            return std::make_unique<DataPage>(std::move(page));
        case PageLayoutType::Index:
            return std::make_unique<IndexPage>(std::move(page));
        case PageLayoutType::Overflow:
            return std::make_unique<OverflowPage>(std::move(page));
        case PageLayoutType::Metadata:
            return std::make_unique<MetadataPage>(std::move(page));
        default:
            return nullptr;
    }
}

std::unique_ptr<BasePage> PageFactory::createPage(PageLayoutType type, storage::PageId id) {
    switch (type) {
        case PageLayoutType::Header:
            return std::make_unique<HeaderPage>(
                storage::Page(id, storage::PageType::Header));
        case PageLayoutType::FreeList:
            return std::make_unique<FreeListPage>(
                storage::Page(id, storage::PageType::FreeList));
        case PageLayoutType::Data:
            return std::make_unique<DataPage>(
                storage::Page(id, storage::PageType::Data));
        case PageLayoutType::Index:
            return std::make_unique<IndexPage>(
                storage::Page(id, storage::PageType::Index));
        case PageLayoutType::Overflow:
            return std::make_unique<OverflowPage>(
                storage::Page(id, storage::PageType::Overflow));
        case PageLayoutType::Metadata:
            return std::make_unique<MetadataPage>(
                storage::Page(id, storage::PageType::Metadata));
        default:
            return nullptr;
    }
}

std::unique_ptr<BasePage> PageFactory::deserialize(PageLayoutType type,
                                                    serialization::BinaryReader& reader) {
    auto page = createPage(type, storage::kInvalidPageId);
    if (!page) {
        return nullptr;
    }
    auto st = page->deserialize(reader);
    if (!st.ok()) {
        return nullptr;
    }
    return page;
}

bool PageFactory::isSupportedType(PageLayoutType type) noexcept {
    return type >= PageLayoutType::Header && type <= PageLayoutType::Overflow;
}

} // namespace pages
} // namespace quartz
