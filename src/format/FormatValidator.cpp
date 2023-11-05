#include "quartz/format/FormatValidator.h"

namespace quartz {
namespace format {

Status FormatValidator::validateHeader(const DatabaseHeader& header) noexcept {
    if (!header.isValid()) {
        return Status::corruption("DatabaseHeader: invalid magic, version, or page size");
    }
    auto st = validateMagic(header.magic);
    if (!st.ok()) return st;
    st = validatePageSize(header.pageSize);
    if (!st.ok()) return st;
    if (header.majorVersion != Versioning::kMajorVersion) {
        return Status::invalidArgument(
            "DatabaseHeader: unsupported major version " +
            std::to_string(header.majorVersion));
    }
    if (header.minorVersion > Versioning::kMinorVersion) {
        return Status::invalidArgument(
            "DatabaseHeader: minor version " +
            std::to_string(header.minorVersion) +
            " is too new, reader supports up to " +
            std::to_string(Versioning::kMinorVersion));
    }
    if (header.databaseId.isNil()) {
        return Status::corruption("DatabaseHeader: nil database ID");
    }
    return Status::success();
}

Status FormatValidator::validateSuperblock(const Superblock& superblock) noexcept {
    if (!superblock.isValid()) {
        return Status::corruption("Superblock: invalid magic");
    }
    if (!superblock.isConsistent()) {
        return Status::corruption("Superblock: inconsistent page counts");
    }
    if (superblock.totalPages == 0) {
        return Status::corruption("Superblock: zero total pages");
    }
    return Status::success();
}

Status FormatValidator::validateMagic(std::uint32_t magic) noexcept {
    if (!MagicNumbers::isValid(magic)) {
        return Status::corruption("Invalid magic number: 0x" +
                                  std::to_string(magic));
    }
    return Status::success();
}

Status FormatValidator::validateVersion(const Versioning::Version& version) noexcept {
    return Versioning::validate(version);
}

Status FormatValidator::validatePageSize(std::uint32_t pageSize) noexcept {
    if (pageSize < 512) {
        return Status::invalidArgument("Page size too small: " +
                                       std::to_string(pageSize));
    }
    if (pageSize > 65536) {
        return Status::invalidArgument("Page size too large: " +
                                       std::to_string(pageSize));
    }
    if ((pageSize & (pageSize - 1)) != 0) {
        return Status::invalidArgument("Page size not a power of two: " +
                                       std::to_string(pageSize));
    }
    return Status::success();
}

Status FormatValidator::validateFeatureFlags(const FeatureFlags& flags) noexcept {
    return flags.validate();
}

Status FormatValidator::validateMetadataDescriptor(
    const MetadataDescriptor& desc) noexcept {
    if (!desc.isValid()) {
        return Status::corruption("MetadataDescriptor: invalid descriptor");
    }
    if (!MagicNumbers::isRecognized(desc.magic)) {
        return Status::corruption("MetadataDescriptor: unrecognized magic 0x" +
                                  std::to_string(desc.magic));
    }
    if (desc.pageCount == 0) {
        return Status::corruption("MetadataDescriptor: zero page count");
    }
    return Status::success();
}

Status FormatValidator::validateSchemaDescriptor(
    const SchemaDescriptor& desc) noexcept {
    if (!desc.isValid()) {
        return Status::corruption("SchemaDescriptor: invalid descriptor");
    }
    return Status::success();
}

} // namespace format
} // namespace quartz
