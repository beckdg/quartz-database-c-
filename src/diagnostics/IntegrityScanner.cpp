#include "quartz/diagnostics/IntegrityScanner.h"

#include "quartz/common/Config.h"
#include "quartz/pages/HeaderPage.h"

namespace quartz {
namespace diagnostics {

IntegrityScanner::IntegrityScanner(storage::DatabaseFile& file)
    : file_(file) {}

Status IntegrityScanner::scan() {
    report_ = {};
    if (!file_.isOpen()) {
        report_.add(FindingSeverity::Error, "file", "not open");
        return Status::invalidArgument("IntegrityScanner: file not open");
    }

    if (file_.fileSize() < config::kPageSize) {
        report_.add(FindingSeverity::Error, "file", "file too small");
        return Status::corruption("IntegrityScanner: file too small");
    }

    storage::Page page(0, storage::PageType::Header);
    auto st = file_.seek(0);
    if (!st.ok()) return st;
    st = file_.readPage(page);
    if (!st.ok()) {
        report_.add(FindingSeverity::Error, "header", st.toString());
        return st;
    }

    pages::HeaderPage headerPage(std::move(page));
    st = headerPage.validate();
    if (!st.ok()) {
        report_.add(FindingSeverity::Error, "header", st.toString());
        return st;
    }

    report_.add(FindingSeverity::Info, "header", "valid");
    const auto pageCount = file_.fileSize() / config::kPageSize;
    report_.add(FindingSeverity::Info, "file",
                "pages=" + std::to_string(pageCount) + ", size=" + std::to_string(file_.fileSize()));
    return Status::success();
}

} // namespace diagnostics
} // namespace quartz
