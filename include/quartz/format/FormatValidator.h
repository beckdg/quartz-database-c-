#pragma once

#include "quartz/common/Status.h"
#include "quartz/format/DatabaseHeader.h"
#include "quartz/format/FeatureFlags.h"
#include "quartz/format/MagicNumbers.h"
#include "quartz/format/MetadataDescriptor.h"
#include "quartz/format/SchemaDescriptor.h"
#include "quartz/format/Superblock.h"
#include "quartz/format/Versioning.h"

namespace quartz {
namespace format {

class FormatValidator {
public:
    static Status validateHeader(const DatabaseHeader& header) noexcept;
    static Status validateSuperblock(const Superblock& superblock) noexcept;
    static Status validateMagic(std::uint32_t magic) noexcept;
    static Status validateVersion(const Versioning::Version& version) noexcept;
    static Status validatePageSize(std::uint32_t pageSize) noexcept;
    static Status validateFeatureFlags(const FeatureFlags& flags) noexcept;
    static Status validateMetadataDescriptor(const MetadataDescriptor& desc) noexcept;
    static Status validateSchemaDescriptor(const SchemaDescriptor& desc) noexcept;

private:
    FormatValidator() = delete;
};

} // namespace format
} // namespace quartz
