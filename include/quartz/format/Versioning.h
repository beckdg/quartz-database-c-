#pragma once

#include "quartz/common/Status.h"

#include <cstdint>
#include <string>

namespace quartz {
namespace format {

class Versioning {
public:
    static constexpr std::uint32_t kMajorVersion = 1;
    static constexpr std::uint32_t kMinorVersion = 0;
    static constexpr std::uint32_t kPatchVersion = 0;

    struct Version {
        std::uint32_t major;
        std::uint32_t minor;
        std::uint32_t patch;

        bool operator==(const Version& other) const noexcept {
            return major == other.major && minor == other.minor && patch == other.patch;
        }
        bool operator!=(const Version& other) const noexcept { return !(*this == other); }

        bool isCompatible(const Version& required) const noexcept {
            return major == required.major && minor >= required.minor;
        }
    };

    static Version current() noexcept {
        return Version{kMajorVersion, kMinorVersion, kPatchVersion};
    }

    static Status validate(const Version& version) noexcept {
        if (version.major != kMajorVersion) {
            return Status::invalidArgument(
                "Version major mismatch: expected " + std::to_string(kMajorVersion) +
                ", got " + std::to_string(version.major));
        }
        if (version.minor > kMinorVersion) {
            return Status::invalidArgument(
                "Version minor too new: reader supports up to " +
                std::to_string(kMinorVersion) + ", got " + std::to_string(version.minor));
        }
        return Status::success();
    }

    static bool isReadable(const Version& writerVersion,
                           const Version& readerVersion) noexcept {
        return writerVersion.major == readerVersion.major &&
               writerVersion.minor <= readerVersion.minor;
    }

    static std::string toString(const Version& v) {
        return std::to_string(v.major) + "." +
               std::to_string(v.minor) + "." +
               std::to_string(v.patch);
    }

    static std::uint32_t encodeVersion(std::uint32_t major, std::uint32_t minor) noexcept {
        return (major << 16) | (minor & 0xFFFF);
    }

    static Version decodeVersion(std::uint32_t encoded) noexcept {
        return Version{encoded >> 16, encoded & 0xFFFF, 0};
    }
};

} // namespace format
} // namespace quartz
