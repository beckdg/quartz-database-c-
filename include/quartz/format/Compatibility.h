#pragma once

#include "quartz/common/Status.h"
#include "quartz/format/FeatureFlags.h"
#include "quartz/format/Versioning.h"

#include <string>

namespace quartz {
namespace format {

class Compatibility {
public:
    struct Requirements {
        Versioning::Version minReaderVersion;
        Versioning::Version minWriterVersion;
        FeatureFlags requiredFeatures;
        FeatureFlags prohibitedFeatures;
    };

    static Status checkReaderCompatibility(const Versioning::Version& headerVersion,
                                            const Versioning::Version& readerVersion) noexcept {
        if (headerVersion.major != readerVersion.major) {
            return Status::invalidArgument(
                "Reader major version mismatch: file=" +
                Versioning::toString(headerVersion) +
                ", reader=" + Versioning::toString(readerVersion));
        }
        if (headerVersion.minor > readerVersion.minor) {
            return Status::invalidArgument(
                "File requires newer minor version: file=" +
                Versioning::toString(headerVersion) +
                ", reader=" + Versioning::toString(readerVersion));
        }
        return Status::success();
    }

    static Status checkWriterCompatibility(const Versioning::Version& headerVersion,
                                            const Versioning::Version& writerVersion) noexcept {
        if (headerVersion.major != writerVersion.major) {
            return Status::invalidArgument(
                "Writer major version mismatch: file=" +
                Versioning::toString(headerVersion) +
                ", writer=" + Versioning::toString(writerVersion));
        }
        return Status::success();
    }

    static Status checkFeatureCompatibility(const FeatureFlags& fileFlags,
                                              const FeatureFlags& readerFlags) noexcept {
        auto st = fileFlags.validate();
        if (!st.ok()) return st;

        if (fileFlags.has(FeatureFlags::kCompression) &&
            !readerFlags.has(FeatureFlags::kCompression)) {
            return Status::invalidArgument(
                "File uses compression but reader does not support it");
        }
        if (fileFlags.has(FeatureFlags::kEncryption) &&
            !readerFlags.has(FeatureFlags::kEncryption)) {
            return Status::invalidArgument(
                "File uses encryption but reader does not support it");
        }
        if (fileFlags.has(FeatureFlags::kLargePages) &&
            !readerFlags.has(FeatureFlags::kLargePages)) {
            return Status::invalidArgument(
                "File uses large pages but reader does not support it");
        }
        if (fileFlags.has(FeatureFlags::kExtendedIds) &&
            !readerFlags.has(FeatureFlags::kExtendedIds)) {
            return Status::invalidArgument(
                "File uses extended IDs but reader does not support it");
        }
        return Status::success();
    }

    static bool isDowngrade(const Versioning::Version& oldVersion,
                             const Versioning::Version& newVersion) noexcept {
        if (oldVersion.major != newVersion.major) return oldVersion.major > newVersion.major;
        if (oldVersion.minor != newVersion.minor) return oldVersion.minor > newVersion.minor;
        return oldVersion.patch > newVersion.patch;
    }

    static bool isUpgrade(const Versioning::Version& oldVersion,
                           const Versioning::Version& newVersion) noexcept {
        if (oldVersion.major != newVersion.major) return oldVersion.major < newVersion.major;
        if (oldVersion.minor != newVersion.minor) return oldVersion.minor < newVersion.minor;
        return oldVersion.patch < newVersion.patch;
    }

    static bool canRead(const Requirements& requirements,
                         const Versioning::Version& readerVersion,
                         const FeatureFlags& readerFlags) noexcept {
        if (!Versioning::isReadable(requirements.minReaderVersion, readerVersion)) {
            return false;
        }
        if (readerFlags.hasAny(requirements.prohibitedFeatures.value())) {
            return false;
        }
        return true;
    }
};

} // namespace format
} // namespace quartz
