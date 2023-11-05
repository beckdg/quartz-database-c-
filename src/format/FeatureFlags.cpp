#include "quartz/format/FeatureFlags.h"

#include <sstream>

namespace quartz {
namespace format {

std::string FeatureFlags::toString() const {
    if (flags_ == kNone) return "none";

    std::ostringstream oss;
    bool first = true;

    auto append = [&](const char* name) {
        if (!first) oss << "|";
        oss << name;
        first = false;
    };

    if (has(kChecksums))      append("checksums");
    if (has(kCompression))    append("compression");
    if (has(kEncryption))     append("encryption");
    if (has(kJournaling))     append("journaling");
    if (has(kLargePages))     append("large_pages");
    if (has(kExtendedIds))    append("extended_ids");
    if (has(kCustomPageSize)) append("custom_page_size");
    if (has(kMetadataRegion)) append("metadata_region");
    if (has(kUserFlags))      append("user_flags");

    return oss.str();
}

} // namespace format
} // namespace quartz
