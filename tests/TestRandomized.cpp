#include "quartz/btree/BTree.h"
#include "quartz/btree/Key.h"
#include "quartz/format/PageReference.h"
#include "quartz/space/SpaceManager.h"
#include "quartz/wal/BTreeWalAdapter.h"
#include "quartz/wal/LogManager.h"
#include "quartz/recovery/RecoveryManager.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <random>
#include <set>
#include <vector>

namespace fs = std::filesystem;

using namespace quartz;

TEST_CASE("Randomized insert erase with WAL recovery", "[randomized]") {
    auto walPath = (fs::temp_directory_path() / "quartzdb_random.wal").string();
    std::error_code ec;
    fs::remove(walPath, ec);

    wal::LogManager wal;
    REQUIRE(wal.initialize(walPath).ok());

    space::SpaceManager space;
    auto tree = btree::BTree::create(space);
    wal::BTreeWalAdapter adapter(wal);
    tree.setWalSink(&adapter);

    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> opDist(0, 1);
    std::uniform_int_distribution<std::uint32_t> keyDist(0, 200);

    std::set<std::uint32_t> model;
    for (int step = 0; step < 500; ++step) {
        const auto key = keyDist(rng);
        if (opDist(rng) == 0 || model.find(key) == model.end()) {
            auto ref = format::PageReference::make(
                key + 1000, 1, static_cast<std::uint8_t>(storage::PageType::Data));
            if (tree.insert(btree::Key::fromUInt32(key), ref).ok()) {
                model.insert(key);
            }
        } else {
            if (tree.erase(btree::Key::fromUInt32(key)).ok()) {
                model.erase(key);
            }
        }
    }
    REQUIRE(wal.flush().ok());

    space::SpaceManager recoveredSpace;
    auto recovered = btree::BTree::create(recoveredSpace);
    recovery::RecoveryResult result;
    REQUIRE(recovery::RecoveryManager::recover(wal, recovered, result).ok());
    CHECK(recovered.size() == model.size());

    for (auto k : model) {
        CHECK(recovered.contains(btree::Key::fromUInt32(k)));
    }

    REQUIRE(wal.shutdown().ok());
    fs::remove(walPath, ec);
}

TEST_CASE("Stress checkpoint and recovery cycles", "[randomized][stress]") {
    auto walPath = (fs::temp_directory_path() / "quartzdb_stress_cp.wal").string();
    std::error_code ec;

    for (int cycle = 0; cycle < 5; ++cycle) {
        fs::remove(walPath, ec);

        wal::LogManager wal;
        REQUIRE(wal.initialize(walPath, true).ok());

        space::SpaceManager space;
        auto tree = btree::BTree::create(space);
        wal::BTreeWalAdapter adapter(wal);
        tree.setWalSink(&adapter);

        for (std::uint32_t i = 0; i < 50; ++i) {
            auto ref = format::PageReference::make(
                i, 1, static_cast<std::uint8_t>(storage::PageType::Data));
            REQUIRE(tree.insert(btree::Key::fromUInt32(i + static_cast<std::uint32_t>(cycle * 50)), ref).ok());
        }
        REQUIRE(wal.flush().ok());

        wal::LogSequenceNumber cpLsn;
        REQUIRE(recovery::RecoveryManager::checkpoint(tree, wal, false, cpLsn).ok());

        space::SpaceManager recoveredSpace;
        auto recovered = btree::BTree::create(recoveredSpace);
        recovery::RecoveryResult result;
        REQUIRE(recovery::RecoveryManager::recover(wal, recovered, result).ok());
        CHECK(recovered.size() == 50);

        REQUIRE(wal.shutdown().ok());
    }
}
