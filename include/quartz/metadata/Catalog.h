#pragma once

#include "quartz/format/MetadataDescriptor.h"
#include "quartz/format/SchemaDescriptor.h"
#include "quartz/common/Status.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace quartz {
namespace metadata {

/// In-memory catalog of schema and metadata descriptors keyed by identifier.
class Catalog {
public:
    Status registerSchema(const format::SchemaDescriptor& schema);
    Status registerMetadata(const format::MetadataDescriptor& metadata);

    Status findSchema(std::uint32_t schemaId, format::SchemaDescriptor& out) const;
    Status findMetadata(std::uint32_t typeId, format::MetadataDescriptor& out) const;

    std::vector<std::uint32_t> schemaIds() const;
    std::vector<std::uint32_t> metadataTypeIds() const;

    std::size_t schemaCount() const noexcept { return schemas_.size(); }
    std::size_t metadataCount() const noexcept { return metadata_.size(); }

    void clear() noexcept;

private:
    std::unordered_map<std::uint32_t, format::SchemaDescriptor> schemas_;
    std::unordered_map<std::uint32_t, format::MetadataDescriptor> metadata_;
};

} // namespace metadata
} // namespace quartz
