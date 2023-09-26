#pragma once

#include "quartz/common/Status.h"
#include "quartz/diagnostics/DiagnosticReport.h"

namespace quartz {
namespace core {
class Database;
}

namespace diagnostics {

/// Cross-subsystem consistency validation for an open database.
class ConsistencyChecker {
public:
    static Status check(core::Database& db);
    static DiagnosticReport analyze(const core::Database& db);
};

} // namespace diagnostics
} // namespace quartz
