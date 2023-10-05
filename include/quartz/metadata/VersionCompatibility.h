#pragma once

#include "quartz/format/FeatureFlags.h"
#include "quartz/format/Versioning.h"

#include <cstdint>

namespace quartz {
namespace metadata {

/// Runtime feature and version compatibility checks.
class VersionCompatibility {
public:
    static bool supportsFormat(std::uint32_t major, std::uint32_t minor) noexcept;
    static bool supportsFeatures(format::FeatureFlags flags) noexcept;
    static std::uint32_t currentFormatVersion() noexcept;
};

} // namespace metadata
} // namespace quartz
