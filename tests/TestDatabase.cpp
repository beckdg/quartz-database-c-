#include "quartz/btree/Key.h"
#include "quartz/core/Database.h"
#include "quartz/format/PageReference.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

using namespace quartz;
using namespace quartz::btree;
using namespace quartz::core;
using namespace quartz::format;
using namespace quartz::storage;

namespace {

std::string dbPath(const char* name) {
    return fs::temp_directory_path().string() + "/quartzdb_db_" + name + ".qdb";
}

void cleanup(const std::string& path) {
    std::error_code ec;
    fs::remove(fs::path(path), ec);
    fs::remove(fs::path(path + ".wal"), ec);
}

PageReference makeRef(PageId id) {
    return PageReference::make(id, 1, static_cast<std::uint8_t>(PageType::Data));
}

} // namespace

TEST_CASE("Database open insert find close", "[database]") {
    auto path = dbPath("basic");
    cleanup(path);

    DatabaseOptions options;
    options.enableWal = true;
    options.recoverOnOpen = true;

    Database db(options);
    REQUIRE(db.open(path, true).ok());
    CHECK(db.isOpen());

    REQUIRE(db.insert(Key::fromUInt32(1), makeRef(100)).ok());
    REQUIRE(db.insert(Key::fromUInt32(2), makeRef(200)).ok());
    CHECK(db.contains(Key::fromUInt32(1)));

    PageReference out;
    REQUIRE(db.find(Key::fromUInt32(2), out).ok());
    CHECK(out.pageId == 200);

    CHECK(db.validate().ok());
    REQUIRE(db.close().ok());
    CHECK_FALSE(db.isOpen());
    cleanup(path);
}

TEST_CASE("Database recovery after reopen", "[database]") {
    auto path = dbPath("reopen");
    cleanup(path);

    {
        DatabaseOptions options;
        Database db(options);
        REQUIRE(db.open(path, true).ok());
        for (std::uint32_t i = 0; i < 30; ++i) {
            REQUIRE(db.insert(Key::fromUInt32(i), makeRef(i + 500)).ok());
        }
        REQUIRE(db.checkpoint(false).ok());
        for (std::uint32_t i = 30; i < 40; ++i) {
            REQUIRE(db.insert(Key::fromUInt32(i), makeRef(i + 500)).ok());
        }
        REQUIRE(db.close().ok());
    }

    {
        DatabaseOptions options;
        Database db(options);
        REQUIRE(db.open(path, false).ok());
        CHECK(db.tree().size() == 40);
        CHECK(db.contains(Key::fromUInt32(39)));
        CHECK(db.lastRecovery().recoveredFromCheckpoint);
        REQUIRE(db.close().ok());
    }
    cleanup(path);
}

TEST_CASE("Database statistics", "[database]") {
    auto path = dbPath("stats");
    cleanup(path);

    Database db;
    REQUIRE(db.open(path, true).ok());
    REQUIRE(db.insert(Key::fromUInt32(7), makeRef(7)).ok());

    const auto stats = db.statistics();
    CHECK(stats.btree.keyCount >= 1);
    CHECK(stats.openCount >= 1);

    REQUIRE(db.close().ok());
    cleanup(path);
}

TEST_CASE("Database without WAL", "[database]") {
    auto path = dbPath("nowal");
    cleanup(path);

    DatabaseOptions options;
    options.enableWal = false;
    Database db(options);
    REQUIRE(db.open(path, true).ok());
    REQUIRE(db.insert(Key::fromUInt32(99), makeRef(99)).ok());
    CHECK(db.contains(Key::fromUInt32(99)));
    REQUIRE(db.close().ok());
    cleanup(path);
}
