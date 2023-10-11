#pragma once

#include "quartz/pages/BasePage.h"
#include "quartz/pages/PageLayouts.h"
#include "quartz/common/Status.h"

namespace quartz {
namespace pages {

class PageValidator {
public:
    PageValidator() = delete;

    static Status validatePage(const BasePage& page);
    static Status validateLayoutType(PageLayoutType type) noexcept;
    static Status validateReservedFields(const HeaderPageLayout& layout) noexcept;
    static Status validateReservedFields(const FreeListPageLayout& layout) noexcept;
    static Status validateReservedFields(const DataPageLayout& layout) noexcept;
    static Status validateReservedFields(const OverflowPageLayout& layout) noexcept;
    static Status validateReservedFields(const IndexPageLayout& layout) noexcept;
    static Status validateReservedFields(const MetadataPageLayout& layout) noexcept;
};

} // namespace pages
} // namespace quartz
