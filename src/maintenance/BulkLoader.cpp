#include "quartz/maintenance/BulkLoader.h"

namespace quartz {
namespace maintenance {

BulkLoader::BulkLoader(btree::BTree& tree)
    : tree_(tree) {}

Status BulkLoader::loadSorted(const std::vector<btree::Key>& keys,
                              const std::vector<format::PageReference>& values) {
    if (keys.size() != values.size()) {
        return Status::invalidArgument("BulkLoader: key/value count mismatch");
    }
    keysLoaded_ = 0;
    for (std::size_t i = 0; i < keys.size(); ++i) {
        auto st = tree_.insert(keys[i], values[i]);
        if (!st.ok()) return st;
        ++keysLoaded_;
    }
    return tree_.validate();
}

} // namespace maintenance
} // namespace quartz
