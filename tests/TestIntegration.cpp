#include "quartz/core/Database.h"
#include "quartz/diagnostics/ConsistencyChecker.h"
#include "quartz/diagnostics/IntegrityScanner.h"
#include "quartz/instrumentation/Counter.h"
#include "quartz/instrumentation/Profiler.h"
#include "quartz/instrumentation/Timer.h"
#include "quartz/maintenance/BulkLoader.h"
#include "quartz/maintenance/Vacuum.h"
#include "quartz/metadata/Catalog.h"
#include "quartz/metadata/VersionCompatibility.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <vector>

using namespace quartz;

TEST_CASE("Timer measures elapsed time", "[instrumentation]") {
    instrumentation::Timer timer;
    timer.start();
    timer.stop();
    CHECK(timer.elapsedMilliseconds() >= 0.0);
    CHECK_FALSE(timer.toString().empty());
}

TEST_CASE("Counter increments", "[instrumentation]") {
    instrumentation::Counter counter("ops");
    counter.increment(5);
    CHECK(counter.value() == 5);
    counter.reset();
    CHECK(counter.value() == 0);
}

TEST_CASE("Profiler summary", "[instrumentation]") {
    instrumentation::Profiler profiler;
    profiler.counter("hits").increment(3);
    auto& t = profiler.timer("work");
    t.start();
    t.stop();
    CHECK(profiler.summary().find("hits") != std::string::npos);
}

TEST_CASE("Catalog registers schemas", "[metadata]") {
    metadata::Catalog catalog;
    auto schema = format::SchemaDescriptor::make(1, 10, 4);
    REQUIRE(catalog.registerSchema(schema).ok());
    format::SchemaDescriptor found;
    REQUIRE(catalog.findSchema(1, found).ok());
    CHECK(found.schemaId == 1);
}

TEST_CASE("VersionCompatibility supports current format", "[metadata]") {
    CHECK(metadata::VersionCompatibility::supportsFormat(1, 0));
    CHECK_FALSE(metadata::VersionCompatibility::supportsFormat(99, 0));
}

TEST_CASE("BulkLoader loads sorted keys", "[maintenance]") {
    space::SpaceManager space;
    auto tree = btree::BTree::create(space);
    maintenance::BulkLoader loader(tree);

    std::vector<btree::Key> keys;
    std::vector<format::PageReference> refs;
    for (std::uint32_t i = 0; i < 20; ++i) {
        keys.push_back(btree::Key::fromUInt32(i));
        refs.push_back(format::PageReference::make(
            i, 1, static_cast<std::uint8_t>(storage::PageType::Data)));
    }
    REQUIRE(loader.loadSorted(keys, refs).ok());
    CHECK(loader.keysLoaded() == 20);
    CHECK(tree.size() == 20);
}

TEST_CASE("Vacuum runs on space manager", "[maintenance]") {
    space::SpaceManager space;
    maintenance::Vacuum vacuum(space);
    CHECK(vacuum.run().ok());
}

TEST_CASE("ConsistencyChecker analyze open database", "[diagnostics]") {
    const auto path =
        (std::filesystem::temp_directory_path() / "quartzdb_diag.qdb").string();
    std::error_code ec;
    std::filesystem::remove(path, ec);
    std::filesystem::remove(path + ".wal", ec);

    core::Database db;
    REQUIRE(db.open(path, true).ok());
    auto report = diagnostics::ConsistencyChecker::analyze(db);
    CHECK(report.passed());

    diagnostics::IntegrityScanner scanner(db.file());
    CHECK(scanner.scan().ok());
    CHECK(scanner.report().passed());

    REQUIRE(db.close().ok());
    std::filesystem::remove(path, ec);
    std::filesystem::remove(path + ".wal", ec);
}
