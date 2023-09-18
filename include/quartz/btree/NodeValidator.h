#pragma once

#include "quartz/btree/BTreeNode.h"
#include "quartz/common/Status.h"

namespace quartz {
namespace btree {

class LeafNode;
class InternalNode;

/// Validates B-tree node invariants and serialization integrity.
class NodeValidator {
public:
    static Status validate(const BTreeNode& node);
    static Status validateLeaf(const LeafNode& node);
    static Status validateInternal(const InternalNode& node);
    static Status validateSerializedBody(serialization::BinaryReader& reader,
                                       const BTreeNodeConfig& config,
                                       NodeType type,
                                       std::uint32_t keyCount);
};

} // namespace btree
} // namespace quartz
