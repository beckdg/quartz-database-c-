#include "quartz/btree/BTree.h"
#include "quartz/btree/Key.h"
#include "quartz/format/PageReference.h"
#include "quartz/instrumentation/Timer.h"
#include "quartz/recovery/RecoveryManager.h"
#include "quartz/space/SpaceManager.h"
#include "quartz/wal/BTreeWalAdapter.h"
#include "quartz/wal/LogManager.h"

#include <cstdint>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

using namespace quartz;

int main() {
    const auto walPath = (fs::temp_directory_path() / "quartzdb_bench_recovery.wal").string();
    std::error_code ec;
    fs::remove(walPath, ec);

    space::SpaceManager space;
    btree::BTreeNodeConfig config;
    config.keyType = btree::KeyType::UInt32;
    auto tree = btree::BTree::create(space, config);

    wal::LogManager wal;
    if (!wal.initialize(walPath).ok()) {
        return 1;
    }
    wal::BTreeWalAdapter adapter(wal);
    tree.setWalSink(&adapter);

    for (std::uint32_t i = 0; i < 1000; ++i) {
        auto ref = format::PageReference::make(
            i + 500, 1, static_cast<std::uint8_t>(storage::PageType::Data));
        if (!tree.insert(btree::Key::fromUInt32(i), ref).ok()) {
            return 1;
        }
    }
    if (!wal.flush().ok()) {
        return 1;
    }

    wal::LogSequenceNumber cpLsn;
    if (!recovery::RecoveryManager::checkpoint(tree, wal, false, cpLsn).ok()) {
        return 1;
    }

    for (std::uint32_t i = 1000; i < 1500; ++i) {
        auto ref = format::PageReference::make(
            i + 500, 1, static_cast<std::uint8_t>(storage::PageType::Data));
        if (!tree.insert(btree::Key::fromUInt32(i), ref).ok()) {
            return 1;
        }
    }
    if (!wal.flush().ok()) {
        return 1;
    }

    space::SpaceManager recoveredSpace;
    auto recoveredTree = btree::BTree::create(recoveredSpace, config);

    instrumentation::Timer timer;
    timer.start();
    recovery::RecoveryResult result;
    if (!recovery::RecoveryManager::recover(wal, recoveredTree, result).ok()) {
        return 1;
    }
    timer.stop();

    std::cout << "Recovery replay: " << timer.toString() << "\n";
    std::cout << "  replayed=" << result.recordsReplayed
              << ", checkpoint=" << result.recoveredFromCheckpoint
              << ", size=" << recoveredTree.size() << "\n";

    (void)wal.shutdown();
    fs::remove(walPath, ec);
    return 0;
}
