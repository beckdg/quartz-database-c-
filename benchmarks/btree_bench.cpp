#include "quartz/btree/BTree.h"
#include "quartz/btree/Key.h"
#include "quartz/format/PageReference.h"
#include "quartz/instrumentation/Timer.h"
#include "quartz/space/SpaceManager.h"
#include "quartz/storage/StorageTypes.h"

#include <cstdint>
#include <iostream>

using namespace quartz;

int main() {
    space::SpaceManager space;
    btree::BTreeNodeConfig config;
    config.keyType = btree::KeyType::UInt32;
    auto tree = btree::BTree::create(space, config);

    instrumentation::Timer timer;
    timer.start();

    constexpr std::uint32_t kCount = 10000;
    for (std::uint32_t i = 0; i < kCount; ++i) {
        auto ref = format::PageReference::make(
            i + 1000, 1, static_cast<std::uint8_t>(storage::PageType::Data));
        if (!tree.insert(btree::Key::fromUInt32(i), ref).ok()) {
            return 1;
        }
    }
    timer.stop();

    std::cout << "BTree insert " << kCount << " keys: " << timer.toString() << "\n";
    std::cout << "  height=" << tree.height() << ", size=" << tree.size() << "\n";
    return 0;
}
