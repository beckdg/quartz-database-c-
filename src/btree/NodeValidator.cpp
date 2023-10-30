#include "quartz/btree/NodeValidator.h"

#include "quartz/btree/InternalNode.h"
#include "quartz/btree/LeafNode.h"

namespace quartz {
namespace btree {

Status NodeValidator::validate(const BTreeNode& node) {
    return node.validate();
}

Status NodeValidator::validateLeaf(const LeafNode& node) {
    return node.validate();
}

Status NodeValidator::validateInternal(const InternalNode& node) {
    return node.validate();
}

Status NodeValidator::validateSerializedBody(serialization::BinaryReader& reader,
                                             const BTreeNodeConfig& config,
                                             NodeType type,
                                             std::uint32_t keyCount) {
    if (type == NodeType::Leaf) {
        for (std::uint32_t i = 0; i < keyCount; ++i) {
            Key key;
            auto st = key.deserialize(reader, config.binaryKeySize);
            if (!st.ok()) return st;
            format::PageReference ref;
            st = reader.read(ref);
            if (!st.ok()) return st;
        }
        return Status::success();
    }

    format::PageReference leftChild;
    auto st = reader.read(leftChild);
    if (!st.ok()) return st;

    for (std::uint32_t i = 0; i < keyCount; ++i) {
        Key key;
        st = key.deserialize(reader, config.binaryKeySize);
        if (!st.ok()) return st;
        format::PageReference rightChild;
        st = reader.read(rightChild);
        if (!st.ok()) return st;
    }
    return Status::success();
}

} // namespace btree
} // namespace quartz
