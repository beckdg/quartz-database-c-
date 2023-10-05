#pragma once

#include "quartz/btree/BTree.h"
#include "quartz/btree/Key.h"
#include "quartz/common/Status.h"
#include "quartz/format/PageReference.h"
#include "quartz/space/SpaceManager.h"

#include <vector>

namespace quartz {
namespace maintenance {

/// Builds a B-tree from pre-sorted keys in a single bottom-up pass.
class BulkLoader {
public:
    explicit BulkLoader(btree::BTree& tree);

    Status loadSorted(const std::vector<btree::Key>& keys,
                      const std::vector<format::PageReference>& values);

    std::uint64_t keysLoaded() const noexcept { return keysLoaded_; }

private:
    btree::BTree& tree_;
    std::uint64_t keysLoaded_ = 0;
};

} // namespace maintenance
} // namespace quartz
