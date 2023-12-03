#include "quartz/btree/BTree.h"
#include "quartz/btree/Key.h"
#include "quartz/format/PageReference.h"
#include "quartz/recovery/CheckpointManager.h"
#include "quartz/recovery/LogReplayer.h"
#include "quartz/recovery/RecoveryManager.h"
#include "quartz/recovery/RecoveryValidator.h"
#include "quartz/space/SpaceManager.h"
#include "quartz/wal/BTreeWalAdapter.h"
#include "quartz/wal/LogManager.h"
#include "quartz/wal/LogRecord.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

using namespace quartz;
using namespace quartz::btree;
using namespace quartz::format;
using namespace quartz::recovery;
using namespace quartz::space;
using namespace quartz::storage;
using namespace quartz::wal;

namespace {

std::string recoveryPath(const char* name) {
    return fs::temp_directory_path().string() + "/quartzdb_recovery_" + name;
}

void cleanup(const std::string& path) {
    std::error_code ec;
    fs::remove(fs::path(path), ec);
}

PageReference makeRef(PageId id) {
    return PageReference::make(id, 1, static_cast<std::uint8_t>(PageType::Data));
}

} // namespace

TEST_CASE("LogReplayer applies PageUpdate and PageDelete", "[recovery]") {
    SpaceManager space;
    BTreeNodeConfig config;
    config.keyType = KeyType::UInt32;
    auto tree = BTree::create(space, config);

    auto insertRecord = LogRecord::make(LogRecordType::PageUpdate, 10);
    serialization::Buffer buf;
    serialization::BinaryWriter writer(buf);
    REQUIRE(Key::fromUInt32(42).serialize(writer).ok());
    REQUIRE(writer.write(makeRef(10)).ok());
    insertRecord.payload() = std::move(buf);
    insertRecord.setLsn(LogSequenceNumber(1));
    REQUIRE(LogReplayer::applyRecord(insertRecord, tree).ok());
    CHECK(tree.contains(Key::fromUInt32(42)));

    auto deleteRecord = LogRecord::make(LogRecordType::PageDelete);
    serialization::Buffer delBuf;
    serialization::BinaryWriter delWriter(delBuf);
    REQUIRE(Key::fromUInt32(42).serialize(delWriter).ok());
    deleteRecord.payload() = std::move(delBuf);
    deleteRecord.setLsn(LogSequenceNumber(2));
    REQUIRE(LogReplayer::applyRecord(deleteRecord, tree).ok());
    CHECK_FALSE(tree.contains(Key::fromUInt32(42)));
}

TEST_CASE("Checkpoint create and restore round-trip", "[recovery]") {
    auto path = recoveryPath("checkpoint");
    cleanup(path);

    LogManager wal;
    REQUIRE(wal.initialize(path).ok());

    SpaceManager space;
    BTreeNodeConfig config;
    config.keyType = KeyType::UInt32;
    auto tree = BTree::create(space, config);
    BTreeWalAdapter adapter(wal);
    tree.setWalSink(&adapter);

    for (std::uint32_t i = 0; i < 50; ++i) {
        REQUIRE(tree.insert(Key::fromUInt32(i), makeRef(i + 100)).ok());
    }
    REQUIRE(wal.flush().ok());

    LogSequenceNumber cpLsn;
    REQUIRE(CheckpointManager::createCheckpoint(tree, wal, cpLsn).ok());
    CHECK(cpLsn.isValid());

    SpaceManager restoredSpace;
    auto restored = BTree::create(restoredSpace, config);
    CheckpointPayload payload;
    LogSequenceNumber foundLsn;
    REQUIRE(CheckpointManager::findLastCheckpoint(wal, payload, foundLsn).ok());
    REQUIRE(CheckpointManager::restoreTree(payload, restored).ok());
    CHECK(restored.size() == 50);
    CHECK(restored.contains(Key::fromUInt32(25)));

    REQUIRE(wal.shutdown().ok());
    cleanup(path);
}

TEST_CASE("RecoveryManager replays after checkpoint", "[recovery]") {
    auto path = recoveryPath("full");
    cleanup(path);

    LogManager wal;
    REQUIRE(wal.initialize(path).ok());

    SpaceManager space;
    BTreeNodeConfig config;
    config.keyType = KeyType::UInt32;
    auto tree = BTree::create(space, config);
    BTreeWalAdapter adapter(wal);
    tree.setWalSink(&adapter);

    for (std::uint32_t i = 0; i < 100; ++i) {
        REQUIRE(tree.insert(Key::fromUInt32(i), makeRef(i)).ok());
    }
    REQUIRE(wal.flush().ok());

    LogSequenceNumber cpLsn;
    REQUIRE(RecoveryManager::checkpoint(tree, wal, false, cpLsn).ok());

    for (std::uint32_t i = 100; i < 150; ++i) {
        REQUIRE(tree.insert(Key::fromUInt32(i), makeRef(i)).ok());
    }
    REQUIRE(wal.flush().ok());

    SpaceManager recoveredSpace;
    auto recovered = BTree::create(recoveredSpace, config);
    RecoveryResult result;
    REQUIRE(RecoveryManager::recover(wal, recovered, result).ok());
    CHECK(result.recoveredFromCheckpoint);
    CHECK(recovered.size() == 150);
    CHECK(recovered.contains(Key::fromUInt32(149)));
    CHECK(RecoveryValidator::validate(wal, recovered).ok());

    REQUIRE(wal.shutdown().ok());
    cleanup(path);
}

TEST_CASE("Recovery from empty WAL", "[recovery]") {
    auto path = recoveryPath("empty");
    cleanup(path);

    LogManager wal;
    REQUIRE(wal.initialize(path).ok());

    SpaceManager space;
    auto tree = BTree::create(space);
    RecoveryResult result;
    REQUIRE(RecoveryManager::recover(wal, tree, result).ok());
    CHECK(tree.empty());
    CHECK(result.recordsReplayed == 0);

    REQUIRE(wal.shutdown().ok());
    cleanup(path);
}

TEST_CASE("CheckpointPayload serialization round-trip", "[recovery]") {
    CheckpointPayload original;
    original.lsn = LogSequenceNumber(7);
    original.treeSize = 42;
    original.treeHeight = 3;
    REQUIRE(original.btreeSnapshot.append(reinterpret_cast<const std::uint8_t*>("snapshot"), 8).ok());

    serialization::Buffer buf;
    serialization::BinaryWriter writer(buf);
    REQUIRE(original.serialize(writer).ok());

    serialization::BinaryReader reader{serialization::BufferView(buf)};
    CheckpointPayload restored;
    REQUIRE(restored.deserialize(reader).ok());
    CHECK(restored.lsn == original.lsn);
    CHECK(restored.treeSize == original.treeSize);
    CHECK(restored.btreeSnapshot.size() == 8);
}
