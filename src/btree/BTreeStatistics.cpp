#include "quartz/btree/BTreeNode.h"
#include "quartz/btree/BTreeStatistics.h"

namespace quartz {
namespace btree {

BTreeStatistics computeStatistics(const BTreeNode& node) {
    return node.statistics();
}

} // namespace btree
} // namespace quartz
