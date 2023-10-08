#pragma once

#include "quartz/pages/BasePage.h"
#include "quartz/pages/HeaderPage.h"
#include "quartz/pages/FreeListPage.h"
#include "quartz/pages/DataPage.h"
#include "quartz/pages/IndexPage.h"
#include "quartz/pages/OverflowPage.h"
#include "quartz/pages/MetadataPage.h"
#include "quartz/storage/Page.h"
#include "quartz/storage/StorageConstants.h"
#include "quartz/serialization/BinaryReader.h"
#include "quartz/common/Status.h"

#include <cstdint>
#include <memory>

namespace quartz {
namespace pages {

class PageFactory {
public:
    PageFactory() = delete;

    static std::unique_ptr<BasePage> createPage(storage::Page page);
    static std::unique_ptr<BasePage> createPage(PageLayoutType type, storage::PageId id);
    static std::unique_ptr<BasePage> deserialize(PageLayoutType type,
                                                  serialization::BinaryReader& reader);
    static bool isSupportedType(PageLayoutType type) noexcept;
};

} // namespace pages
} // namespace quartz
