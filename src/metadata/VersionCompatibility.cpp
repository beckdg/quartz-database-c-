#include "quartz/metadata/VersionCompatibility.h"

#include "quartz/common/Config.h"

namespace quartz {
namespace metadata {

bool VersionCompatibility::supportsFormat(std::uint32_t major, std::uint32_t minor) noexcept {
    if (major > static_cast<std::uint32_t>(config::kVersionMajor)) {
        return false;
    }
    if (major == static_cast<std::uint32_t>(config::kVersionMajor) &&
        minor > static_cast<std::uint32_t>(config::kVersionMinor)) {
        return false;
    }
    return true;
}

bool VersionCompatibility::supportsFeatures(format::FeatureFlags flags) noexcept {
    (void)flags;
    return true;
}

std::uint32_t VersionCompatibility::currentFormatVersion() noexcept {
    return format::Versioning::encodeVersion(static_cast<std::uint32_t>(config::kVersionMajor),
                                             static_cast<std::uint32_t>(config::kVersionMinor));
}

} // namespace metadata
} // namespace quartz
