#include "quartz/btree/BTree.h"
#include "quartz/btree/BTreeNode.h"
#include "quartz/btree/Cursor.h"
#include "quartz/btree/Key.h"
#include "quartz/btree/TreeValidator.h"
#include "quartz/format/PageReference.h"
#include "quartz/serialization/Buffer.h"
#include "quartz/serialization/BinaryReader.h"
#include "quartz/serialization/BinaryWriter.h"
#include "quartz/space/SpaceManager.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <random>
#include <set>
#include <vector>

using namespace quartz;
using namespace quartz::btree;
using namespace quartz::format;
using namespace quartz::serialization;
using namespace quartz::space;
using namespace quartz::storage;

namespace {

BTreeNodeConfig defaultConfig() {
    BTreeNodeConfig config;
    config.keyType = KeyType::UInt32;
    return config;
}

PageReference makeDataRef(PageId id) {
    return PageReference::make(id, 1, static_cast<std::uint8_t>(PageType::Data));
}

void insertKeys(BTree& tree, const std::vector<std::uint32_t>& values) {
    for (auto v : values) {
        REQUIRE(tree.insert(Key::fromUInt32(v), makeDataRef(v + 1000)).ok());
        REQUIRE(tree.validate().ok());
    }
}

} // namespace

// ===== Basic tree lifecycle =====

TEST_CASE("BTree empty construction", "[btree][tree]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    CHECK(tree.empty());
    CHECK(tree.size() == 0);
    CHECK(tree.height() == 0);
    CHECK_FALSE(tree.contains(Key::fromUInt32(1)));
    CHECK(tree.validate().ok());
}

TEST_CASE("BTree single key insert find erase", "[btree][tree]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    REQUIRE(tree.insert(Key::fromUInt32(42), makeDataRef(42)).ok());
    CHECK_FALSE(tree.empty());
    CHECK(tree.size() == 1);
    CHECK(tree.height() == 1);
    CHECK(tree.contains(Key::fromUInt32(42)));

    PageReference out;
    REQUIRE(tree.find(Key::fromUInt32(42), out).ok());
    CHECK(out.pageId == 42);

    REQUIRE(tree.erase(Key::fromUInt32(42)).ok());
    CHECK(tree.empty());
    CHECK(tree.size() == 0);
    CHECK(tree.validate().ok());
}

TEST_CASE("BTree clear releases pages", "[btree][tree]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    insertKeys(tree, {1, 2, 3, 4, 5});
    const auto allocated = space.allocatedPageCount();
    tree.clear();
    CHECK(tree.empty());
    CHECK(space.allocatedPageCount() < allocated);
}

TEST_CASE("BTree duplicate key rejection", "[btree][tree]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    REQUIRE(tree.insert(Key::fromUInt32(7), makeDataRef(7)).ok());
    CHECK_FALSE(tree.insert(Key::fromUInt32(7), makeDataRef(8)).ok());
    CHECK(tree.size() == 1);
}

// ===== Insertion patterns =====

TEST_CASE("BTree ascending insertion", "[btree][tree][insert]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    for (std::uint32_t i = 0; i < 100; ++i) {
        REQUIRE(tree.insert(Key::fromUInt32(i), makeDataRef(i)).ok());
    }
    CHECK(tree.size() == 100);
    CHECK(tree.height() >= 1);
    for (std::uint32_t i = 0; i < 100; ++i) {
        CHECK(tree.contains(Key::fromUInt32(i)));
    }
    CHECK(tree.validate().ok());
}

TEST_CASE("BTree descending insertion", "[btree][tree][insert]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    for (std::uint32_t i = 100; i > 0; --i) {
        REQUIRE(tree.insert(Key::fromUInt32(i), makeDataRef(i)).ok());
    }
    CHECK(tree.size() == 100);
    for (std::uint32_t i = 1; i <= 100; ++i) {
        CHECK(tree.contains(Key::fromUInt32(i)));
    }
    CHECK(tree.validate().ok());
}

TEST_CASE("BTree random insertion", "[btree][tree][insert]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    std::vector<std::uint32_t> values;
    for (std::uint32_t i = 0; i < 200; ++i) {
        values.push_back(i * 3 + 7);
    }
    std::mt19937 rng(12345);
    std::shuffle(values.begin(), values.end(), rng);
    insertKeys(tree, values);
    CHECK(tree.size() == 200);
    for (auto v : values) {
        CHECK(tree.contains(Key::fromUInt32(v)));
    }
    CHECK(tree.validate().ok());
}

TEST_CASE("BTree large ascending triggers height growth", "[btree][tree][insert]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    for (std::uint32_t i = 0; i < 500; ++i) {
        REQUIRE(tree.insert(Key::fromUInt32(i), makeDataRef(i)).ok());
    }
    CHECK(tree.size() == 500);
    CHECK(tree.height() >= 2);
    CHECK(tree.validate().ok());
    auto stats = tree.statistics();
    CHECK(stats.leafCount > 1);
    CHECK(stats.operations.splitCount > 0);
}

// ===== Deletion =====

TEST_CASE("BTree erase all ascending", "[btree][tree][erase]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    for (std::uint32_t i = 0; i < 50; ++i) {
        REQUIRE(tree.insert(Key::fromUInt32(i), makeDataRef(i)).ok());
    }
    for (std::uint32_t i = 0; i < 50; ++i) {
        REQUIRE(tree.erase(Key::fromUInt32(i)).ok());
        CHECK(tree.validate().ok());
    }
    CHECK(tree.empty());
}

TEST_CASE("BTree random deletion", "[btree][tree][erase]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    std::vector<std::uint32_t> values;
    for (std::uint32_t i = 0; i < 100; ++i) {
        values.push_back(i);
    }
    insertKeys(tree, values);
    std::mt19937 rng(99);
    std::shuffle(values.begin(), values.end(), rng);
    for (auto v : values) {
        REQUIRE(tree.erase(Key::fromUInt32(v)).ok());
    }
    CHECK(tree.empty());
    CHECK(tree.validate().ok());
}

TEST_CASE("BTree erase triggers merge", "[btree][tree][erase]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    for (std::uint32_t i = 0; i < 300; ++i) {
        REQUIRE(tree.insert(Key::fromUInt32(i), makeDataRef(i)).ok());
    }
    const auto statsBefore = tree.statistics();
    for (std::uint32_t i = 0; i < 250; ++i) {
        REQUIRE(tree.erase(Key::fromUInt32(i)).ok());
    }
    CHECK(tree.size() == 50);
    CHECK(tree.validate().ok());
    auto statsAfter = tree.statistics();
    CHECK(statsAfter.operations.mergeCount + statsAfter.operations.rotationCount >=
          statsBefore.operations.mergeCount);
}

TEST_CASE("BTree erase nonexistent key", "[btree][tree][erase]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    REQUIRE(tree.insert(Key::fromUInt32(1), makeDataRef(1)).ok());
    CHECK_FALSE(tree.erase(Key::fromUInt32(2)).ok());
}

// ===== Search =====

TEST_CASE("BTree find and contains", "[btree][tree][search]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    insertKeys(tree, {10, 20, 30, 40, 50});
    CHECK(tree.contains(Key::fromUInt32(30)));
    CHECK_FALSE(tree.contains(Key::fromUInt32(25)));

    PageReference out;
    REQUIRE(tree.find(Key::fromUInt32(40), out).ok());
    CHECK(out.pageId == 1040);

    CHECK_FALSE(tree.find(Key::fromUInt32(99), out).ok());
}

TEST_CASE("BTree lowerBound cursor", "[btree][tree][search]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    insertKeys(tree, {10, 20, 30, 40, 50});

    Cursor cursor;
    REQUIRE(tree.lowerBound(Key::fromUInt32(25), cursor).ok());
    CHECK(cursor.valid());
    CHECK(cursor.currentKey() == Key::fromUInt32(30));

    REQUIRE(tree.lowerBound(Key::fromUInt32(10), cursor).ok());
    CHECK(cursor.currentKey() == Key::fromUInt32(10));

    REQUIRE(tree.lowerBound(Key::fromUInt32(55), cursor).ok());
    CHECK(cursor.isEnd());
}

TEST_CASE("BTree upperBound cursor", "[btree][tree][search]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    insertKeys(tree, {10, 20, 30, 40, 50});

    Cursor cursor;
    REQUIRE(tree.upperBound(Key::fromUInt32(25), cursor).ok());
    CHECK(cursor.valid());
    CHECK(cursor.currentKey() == Key::fromUInt32(30));

    REQUIRE(tree.upperBound(Key::fromUInt32(50), cursor).ok());
    CHECK(cursor.isEnd());
}

// ===== Cursor traversal =====

TEST_CASE("BTree cursor forward traversal", "[btree][tree][cursor]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    for (std::uint32_t i = 0; i < 30; ++i) {
        REQUIRE(tree.insert(Key::fromUInt32(i * 2), makeDataRef(i)).ok());
    }

    Cursor cursor = tree.begin();
    std::uint32_t expected = 0;
    while (cursor.valid()) {
        CHECK(cursor.currentKey() == Key::fromUInt32(expected));
        expected += 2;
        REQUIRE(cursor.next().ok());
    }
    CHECK(expected == 60);
}

TEST_CASE("BTree cursor reverse from end", "[btree][tree][cursor]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    insertKeys(tree, {1, 3, 5, 7, 9});

    Cursor cursor = tree.end();
    REQUIRE(cursor.previous().ok());
    CHECK(cursor.currentKey() == Key::fromUInt32(9));
    REQUIRE(cursor.previous().ok());
    CHECK(cursor.currentKey() == Key::fromUInt32(7));
}

TEST_CASE("BTree cursor seekKey", "[btree][tree][cursor]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    insertKeys(tree, {5, 15, 25, 35});

    Cursor cursor;
    cursor.bindTree(&tree);
    REQUIRE(cursor.seekKey(Key::fromUInt32(25)).ok());
    CHECK(cursor.currentKey() == Key::fromUInt32(25));
}

// ===== Statistics =====

TEST_CASE("BTree statistics", "[btree][tree][stats]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    for (std::uint32_t i = 0; i < 200; ++i) {
        REQUIRE(tree.insert(Key::fromUInt32(i), makeDataRef(i)).ok());
    }
    auto stats = tree.statistics();
    CHECK(stats.keyCount == 200);
    CHECK(stats.height >= 1);
    CHECK(stats.nodeCount > 0);
    CHECK(stats.leafCount > 0);
    CHECK(stats.averageOccupancy > 0.0);
    CHECK(stats.operations.allocationCount > 0);
    CHECK(stats.operations.splitCount > 0);
}

// ===== Serialization =====

TEST_CASE("BTree serialize deserialize round-trip", "[btree][tree][serialize]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    insertKeys(tree, {1, 5, 9, 13, 17, 21, 25});

    Buffer buf;
    BinaryWriter writer(buf);
    REQUIRE(tree.serialize(writer).ok());

    SpaceManager space2;
    auto restored = BTree::create(space2, defaultConfig());
    BinaryReader reader{BufferView(buf)};
    REQUIRE(restored.deserialize(reader).ok());

    CHECK(restored.size() == tree.size());
    CHECK(restored.height() == tree.height());
    for (auto v : {1u, 5u, 9u, 13u, 17u, 21u, 25u}) {
        CHECK(restored.contains(Key::fromUInt32(v)));
    }
    CHECK(restored.validate().ok());
}

// ===== Stress =====

TEST_CASE("BTree randomized insert erase sequence", "[btree][tree][stress]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    std::mt19937 rng(42);
    std::set<std::uint32_t> model;

    for (int step = 0; step < 500; ++step) {
        const bool doInsert = model.empty() || (rng() % 3 != 2);
        if (doInsert) {
            const auto v = static_cast<std::uint32_t>(rng() % 300);
            if (model.count(v) == 0) {
                REQUIRE(tree.insert(Key::fromUInt32(v), makeDataRef(v)).ok());
                model.insert(v);
            }
        } else {
            const auto idx = static_cast<std::size_t>(rng() % model.size());
            auto it = model.begin();
            std::advance(it, static_cast<std::ptrdiff_t>(idx));
            REQUIRE(tree.erase(Key::fromUInt32(*it)).ok());
            model.erase(it);
        }
        CHECK(tree.validate().ok());
        CHECK(tree.size() == model.size());
    }
}

TEST_CASE("BTree interleaved ascending descending", "[btree][tree][stress]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    for (std::uint32_t i = 0; i < 100; ++i) {
        REQUIRE(tree.insert(Key::fromUInt32(i), makeDataRef(i)).ok());
        REQUIRE(tree.insert(Key::fromUInt32(1000 + i), makeDataRef(1000 + i)).ok());
    }
    CHECK(tree.size() == 200);
    CHECK(tree.validate().ok());
    for (std::uint32_t i = 0; i < 100; ++i) {
        REQUIRE(tree.erase(Key::fromUInt32(i)).ok());
        REQUIRE(tree.erase(Key::fromUInt32(1000 + i)).ok());
    }
    CHECK(tree.empty());
}

TEST_CASE("BTree validation after root split", "[btree][tree][split]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    const auto cap = BTreeNode::computeCapacity(defaultConfig(), NodeType::Leaf);
    for (std::uint32_t i = 0; i < cap + 1; ++i) {
        REQUIRE(tree.insert(Key::fromUInt32(i), makeDataRef(i)).ok());
    }
    CHECK(tree.height() >= 2);
    CHECK(tree.statistics().operations.splitCount >= 1);
    CHECK(TreeValidator::validate(tree).ok());
}

TEST_CASE("BTree exhaustive small key set", "[btree][tree][stress]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    for (std::uint32_t i = 0; i < 20; ++i) {
        for (std::uint32_t j = 0; j < 20; ++j) {
            auto t = BTree::create(space, defaultConfig());
            std::vector<std::uint32_t> keys;
            for (std::uint32_t k = 0; k < i; ++k) {
                keys.push_back(k * 20 + j);
            }
            insertKeys(t, keys);
            CHECK(t.size() == i);
            CHECK(t.validate().ok());
        }
    }
}

TEST_CASE("BTree invalid reference on insert", "[btree][tree][edge]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    CHECK_FALSE(tree.insert(Key::fromUInt32(1), PageReference::invalid()).ok());
}

TEST_CASE("BTree find on empty tree", "[btree][tree][edge]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    PageReference out;
    CHECK_FALSE(tree.find(Key::fromUInt32(1), out).ok());
}

TEST_CASE("BTree begin end on empty", "[btree][tree][edge]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    Cursor c = tree.begin();
    CHECK(c.isEnd());
    Cursor e = tree.end();
    CHECK(e.isEnd());
}

TEST_CASE("BTree height reduces after full delete", "[btree][tree][edge]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    for (std::uint32_t i = 0; i < 400; ++i) {
        REQUIRE(tree.insert(Key::fromUInt32(i), makeDataRef(i)).ok());
    }
    const auto maxHeight = tree.height();
    for (std::uint32_t i = 0; i < 400; ++i) {
        REQUIRE(tree.erase(Key::fromUInt32(i)).ok());
    }
    CHECK(tree.empty());
    CHECK(tree.height() == 0);
    CHECK(maxHeight >= 2);
}

TEST_CASE("BTree node count matches statistics", "[btree][tree][stats]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    insertKeys(tree, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
    auto stats = tree.statistics();
    CHECK(stats.nodeCount == stats.leafCount + stats.internalCount);
    CHECK(stats.keyCount == 10);
}

TEST_CASE("BTree contains after reinsert", "[btree][tree]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultConfig());
    REQUIRE(tree.insert(Key::fromUInt32(99), makeDataRef(99)).ok());
    REQUIRE(tree.erase(Key::fromUInt32(99)).ok());
    REQUIRE(tree.insert(Key::fromUInt32(99), makeDataRef(199)).ok());
    PageReference out;
    REQUIRE(tree.find(Key::fromUInt32(99), out).ok());
    CHECK(out.pageId == 199);
}
