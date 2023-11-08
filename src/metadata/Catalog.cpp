#include "quartz/metadata/Catalog.h"

namespace quartz {
namespace metadata {

Status Catalog::registerSchema(const format::SchemaDescriptor& schema) {
    if (!schema.isValid()) {
        return Status::invalidArgument("Catalog: invalid schema descriptor");
    }
    schemas_[schema.schemaId] = schema;
    return Status::success();
}

Status Catalog::registerMetadata(const format::MetadataDescriptor& metadata) {
    if (!metadata.isValid()) {
        return Status::invalidArgument("Catalog: invalid metadata descriptor");
    }
    metadata_[metadata.typeId] = metadata;
    return Status::success();
}

Status Catalog::findSchema(std::uint32_t schemaId, format::SchemaDescriptor& out) const {
    const auto it = schemas_.find(schemaId);
    if (it == schemas_.end()) {
        return Status::invalidArgument("Catalog: schema not found");
    }
    out = it->second;
    return Status::success();
}

Status Catalog::findMetadata(std::uint32_t typeId, format::MetadataDescriptor& out) const {
    const auto it = metadata_.find(typeId);
    if (it == metadata_.end()) {
        return Status::invalidArgument("Catalog: metadata not found");
    }
    out = it->second;
    return Status::success();
}

std::vector<std::uint32_t> Catalog::schemaIds() const {
    std::vector<std::uint32_t> ids;
    ids.reserve(schemas_.size());
    for (const auto& entry : schemas_) {
        ids.push_back(entry.first);
    }
    return ids;
}

std::vector<std::uint32_t> Catalog::metadataTypeIds() const {
    std::vector<std::uint32_t> ids;
    ids.reserve(metadata_.size());
    for (const auto& entry : metadata_) {
        ids.push_back(entry.first);
    }
    return ids;
}

void Catalog::clear() noexcept {
    schemas_.clear();
    metadata_.clear();
}

} // namespace metadata
} // namespace quartz
