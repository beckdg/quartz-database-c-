#pragma once

#include "quartz/common/Status.h"
#include "quartz/diagnostics/DiagnosticReport.h"
#include "quartz/storage/DatabaseFile.h"

namespace quartz {
namespace diagnostics {

/// Offline integrity scan of database file pages.
class IntegrityScanner {
public:
  explicit IntegrityScanner(storage::DatabaseFile& file);

    Status scan();
    const DiagnosticReport& report() const noexcept { return report_; }

private:
    storage::DatabaseFile& file_;
    DiagnosticReport report_;
};

} // namespace diagnostics
} // namespace quartz
