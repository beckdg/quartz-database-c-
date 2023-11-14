#include "quartz/pages/PageValidator.h"
#include "quartz/format/FormatValidator.h"

namespace quartz {
namespace pages {

Status PageValidator::validatePage(const BasePage& page) {
    auto type = page.layoutType();
    auto st = validateLayoutType(type);
    if (!st.ok()) return st;

    if (!page.isValid()) {
        return Status::corruption("Page: invalid page header");
    }

    return page.validate();
}

Status PageValidator::validateLayoutType(PageLayoutType type) noexcept {
    if (!isValidLayoutType(type)) {
        return Status::invalidArgument("PageValidator: unsupported page layout type");
    }
    return Status::success();
}

Status PageValidator::validateReservedFields(const HeaderPageLayout& layout) noexcept {
    for (auto r : layout.reserved) {
        if (r != 0) {
            return Status::corruption("HeaderPage: reserved field non-zero");
        }
    }
    return Status::success();
}

Status PageValidator::validateReservedFields(const FreeListPageLayout& layout) noexcept {
    for (auto r : layout.reserved) {
        if (r != 0) {
            return Status::corruption("FreeListPage: reserved field non-zero");
        }
    }
    return Status::success();
}

Status PageValidator::validateReservedFields(const DataPageLayout& layout) noexcept {
    if (layout.reserved1 != 0) {
        return Status::corruption("DataPage: reserved1 field non-zero");
    }
    for (auto r : layout.reserved2) {
        if (r != 0) {
            return Status::corruption("DataPage: reserved2 field non-zero");
        }
    }
    return Status::success();
}

Status PageValidator::validateReservedFields(const OverflowPageLayout& layout) noexcept {
    for (auto r : layout.reserved) {
        if (r != 0) {
            return Status::corruption("OverflowPage: reserved field non-zero");
        }
    }
    return Status::success();
}

Status PageValidator::validateReservedFields(const IndexPageLayout& layout) noexcept {
    for (auto r : layout.reserved) {
        if (r != 0) {
            return Status::corruption("IndexPage: reserved field non-zero");
        }
    }
    return Status::success();
}

Status PageValidator::validateReservedFields(const MetadataPageLayout& layout) noexcept {
    for (auto r : layout.reserved) {
        if (r != 0) {
            return Status::corruption("MetadataPage: reserved field non-zero");
        }
    }
    return Status::success();
}

} // namespace pages
} // namespace quartz
