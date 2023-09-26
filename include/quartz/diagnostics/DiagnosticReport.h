#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace quartz {
namespace diagnostics {

/// Severity for diagnostic findings.
enum class FindingSeverity : std::uint8_t {
    Info = 0,
    Warning = 1,
    Error = 2,
};

/// A single diagnostic finding.
struct DiagnosticFinding {
    FindingSeverity severity = FindingSeverity::Info;
    std::string component;
    std::string message;
};

/// Aggregated diagnostic report.
struct DiagnosticReport {
    std::vector<DiagnosticFinding> findings;
    bool passed() const noexcept;

    void add(FindingSeverity severity, const std::string& component, const std::string& message);
};

} // namespace diagnostics
} // namespace quartz
