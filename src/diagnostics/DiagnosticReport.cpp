#include "quartz/diagnostics/DiagnosticReport.h"

namespace quartz {
namespace diagnostics {

bool DiagnosticReport::passed() const noexcept {
    for (const auto& finding : findings) {
        if (finding.severity == FindingSeverity::Error) {
            return false;
        }
    }
    return true;
}

void DiagnosticReport::add(FindingSeverity severity, const std::string& component,
                           const std::string& message) {
    findings.push_back(DiagnosticFinding{severity, component, message});
}

} // namespace diagnostics
} // namespace quartz
