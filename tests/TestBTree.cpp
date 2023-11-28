#include "quartz/btree/BTreeNode.h"
#include "quartz/btree/BTreeStatistics.h"
#include "quartz/btree/Cursor.h"
#include "quartz/btree/InternalNode.h"
#include "quartz/btree/Key.h"
#include "quartz/btree/KeyComparator.h"
#include "quartz/btree/LeafNode.h"
#include "quartz/btree/NodeValidator.h"
#include "quartz/btree/SearchPath.h"
#include "quartz/btree/SplitMergePlanner.h"
#include "quartz/format/PageReference.h"
#include "quartz/pages/IndexPage.h"
#include "quartz/serialization/Buffer.h"
#include "quartz/serialization/BinaryReader.h"
#include "quartz/serialization/BinaryWriter.h"

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <string>
#include <unordered_set>
#include <vector>

using namespace quartz;
using namespace quartz::btree;
using namespace quartz::format;
using namespace quartz::serialization;

namespace {

BTreeNodeConfig u32Config() {
    BTreeNodeConfig config;
    config.keyType = KeyType::UInt32;
    config.level = 0;
    return config;
}

BTreeNodeConfig u64Config() {
    BTreeNodeConfig config;
    config.keyType = KeyType::UInt64;
    return config;
}

BTreeNodeConfig binaryConfig(std::uint16_t size) {
    BTreeNodeConfig config;
    config.keyType = KeyType::Binary;
    config.binaryKeySize = size;
    return config;
}

PageReference makeRef(storage::PageId id) {
    return PageReference::make(id, 1, static_cast<std::uint8_t>(storage::PageType::Data));
}

} // namespace

// ===== Key =====

TEST_CASE("Key uint32 construction and accessors", "[btree][key]") {
    auto key = Key::fromUInt32(42);
    CHECK(key.type() == KeyType::UInt32);
    CHECK(key.asUInt32() == 42);
    CHECK(key.binarySize() == 0);
}

TEST_CASE("Key uint64 construction and accessors", "[btree][key]") {
    auto key = Key::fromUInt64(9999999999ULL);
    CHECK(key.type() == KeyType::UInt64);
    CHECK(key.asUInt64() == 9999999999ULL);
}

TEST_CASE("Key binary construction", "[btree][key]") {
    std::vector<std::uint8_t> bytes = {0xDE, 0xAD, 0xBE, 0xEF};
    auto key = Key::fromBinary(bytes);
    CHECK(key.type() == KeyType::Binary);
    CHECK(key.binarySize() == 4);
    CHECK(key.binaryData()[0] == 0xDE);
    CHECK(key.binaryData()[3] == 0xEF);
    CHECK(key.binaryCopy() == bytes);
}

TEST_CASE("Key comparison operators uint32", "[btree][key]") {
    auto a = Key::fromUInt32(10);
    auto b = Key::fromUInt32(20);
    auto c = Key::fromUInt32(10);

    CHECK(a < b);
    CHECK(b > a);
    CHECK(a <= c);
    CHECK(a >= c);
    CHECK(a == c);
    CHECK(a != b);
    CHECK(KeyComparator::compare(a, b) < 0);
    CHECK(KeyComparator::compare(b, a) > 0);
    CHECK(KeyComparator::equal(a, c));
}

TEST_CASE("Key comparison operators uint64", "[btree][key]") {
    auto lo = Key::fromUInt64(100);
    auto hi = Key::fromUInt64(200);
    CHECK(lo < hi);
    CHECK(KeyComparator::less(lo, hi));
    CHECK(KeyComparator::greaterEqual(hi, lo));
}

TEST_CASE("Key comparison operators binary", "[btree][key]") {
    auto a = Key::fromBinary(std::vector<std::uint8_t>{1, 2, 3});
    auto b = Key::fromBinary(std::vector<std::uint8_t>{1, 2, 4});
  auto c = Key::fromBinary(std::vector<std::uint8_t>{1, 2});
    CHECK(a < b);
    CHECK(c < a);
}

TEST_CASE("Key cross-type ordering", "[btree][key]") {
    auto u32 = Key::fromUInt32(1);
    auto u64 = Key::fromUInt64(1);
    CHECK(u32 < u64);
}

TEST_CASE("Key serialization round-trip uint32", "[btree][key]") {
    auto original = Key::fromUInt32(12345);
    Buffer buf;
    BinaryWriter writer(buf);
    REQUIRE(original.serialize(writer).ok());

    BinaryReader reader{BufferView(buf)};
    Key restored;
    REQUIRE(restored.deserialize(reader).ok());
    CHECK(restored == original);
    CHECK(restored.serializedSize() == original.serializedSize());
}

TEST_CASE("Key serialization round-trip uint64", "[btree][key]") {
    auto original = Key::fromUInt64(0xFFFFFFFF00000001ULL);
    Buffer buf;
    BinaryWriter writer(buf);
    REQUIRE(original.serialize(writer).ok());

    BinaryReader reader{BufferView(buf)};
    Key restored;
    REQUIRE(restored.deserialize(reader).ok());
    CHECK(restored == original);
}

TEST_CASE("Key serialization round-trip binary", "[btree][key]") {
    auto original = Key::fromBinary(std::vector<std::uint8_t>{9, 8, 7, 6, 5});
    Buffer buf;
    BinaryWriter writer(buf);
    REQUIRE(original.serialize(writer).ok());

    BinaryReader reader{BufferView(buf)};
    Key restored;
    REQUIRE(restored.deserialize(reader, 5).ok());
    CHECK(restored == original);
}

TEST_CASE("Key binary deserialize size mismatch fails", "[btree][key]") {
    auto original = Key::fromBinary(std::vector<std::uint8_t>{1, 2, 3});
    Buffer buf;
    BinaryWriter writer(buf);
    REQUIRE(original.serialize(writer).ok());

    BinaryReader reader{BufferView(buf)};
    Key restored;
    CHECK_FALSE(restored.deserialize(reader, 4).ok());
}

TEST_CASE("Key toString", "[btree][key]") {
    CHECK(Key::fromUInt32(7).toString().find("7") != std::string::npos);
    CHECK(Key::fromUInt64(100).toString().find("u64") != std::string::npos);
    CHECK(Key::fromBinary(std::vector<std::uint8_t>{1}).toString().find("bin") != std::string::npos);
}

TEST_CASE("Key hashing", "[btree][key]") {
    std::unordered_set<Key> keys;
    keys.insert(Key::fromUInt32(1));
    keys.insert(Key::fromUInt32(2));
    keys.insert(Key::fromUInt32(1));
    CHECK(keys.size() == 2);
    CHECK(std::hash<Key>{}(Key::fromUInt32(5)) == Key::fromUInt32(5).hash());
}

// ===== KeyComparator binary search =====

TEST_CASE("KeyComparator lower and upper bound", "[btree][key]") {
    std::vector<Key> keys = {
        Key::fromUInt32(10), Key::fromUInt32(20), Key::fromUInt32(30), Key::fromUInt32(40)};

    CHECK(KeyComparator::lowerBound(keys, Key::fromUInt32(20)) == 1);
    CHECK(KeyComparator::lowerBound(keys, Key::fromUInt32(25)) == 2);
    CHECK(KeyComparator::lowerBound(keys, Key::fromUInt32(5)) == 0);
    CHECK(KeyComparator::lowerBound(keys, Key::fromUInt32(50)) == 4);

    CHECK(KeyComparator::upperBound(keys, Key::fromUInt32(20)) == 2);
    CHECK(KeyComparator::upperBound(keys, Key::fromUInt32(25)) == 2);
    CHECK(KeyComparator::upperBound(keys, Key::fromUInt32(40)) == 4);

    CHECK(KeyComparator::find(keys, Key::fromUInt32(30)) == 2);
    CHECK(KeyComparator::find(keys, Key::fromUInt32(25)) == keys.size());
}

// ===== LeafNode =====

TEST_CASE("LeafNode create initializes metadata", "[btree][leaf]") {
    auto leaf = LeafNode::create(100, u32Config());
    CHECK(leaf.pageId() == 100);
    CHECK(leaf.nodeType() == NodeType::Leaf);
    CHECK(leaf.keyCount() == 0);
    CHECK(leaf.capacity() > 0);
    CHECK(leaf.validate().ok());
}

TEST_CASE("LeafNode insert and contains", "[btree][leaf]") {
    auto leaf = LeafNode::create(1, u32Config());
    REQUIRE(leaf.insert(Key::fromUInt32(30), makeRef(300)).ok());
    REQUIRE(leaf.insert(Key::fromUInt32(10), makeRef(100)).ok());
    REQUIRE(leaf.insert(Key::fromUInt32(20), makeRef(200)).ok());

    CHECK(leaf.keyCount() == 3);
    CHECK(leaf.contains(Key::fromUInt32(10)));
    CHECK(leaf.contains(Key::fromUInt32(20)));
    CHECK(leaf.contains(Key::fromUInt32(30)));
    CHECK_FALSE(leaf.contains(Key::fromUInt32(15)));

    CHECK(leaf.keyAt(0) == Key::fromUInt32(10));
    CHECK(leaf.keyAt(1) == Key::fromUInt32(20));
    CHECK(leaf.keyAt(2) == Key::fromUInt32(30));
    CHECK(leaf.referenceAt(0).pageId == 100);
}

TEST_CASE("LeafNode lower and upper bound", "[btree][leaf]") {
    auto leaf = LeafNode::create(2, u32Config());
    REQUIRE(leaf.insert(Key::fromUInt32(10), makeRef(1)).ok());
    REQUIRE(leaf.insert(Key::fromUInt32(30), makeRef(3)).ok());
    REQUIRE(leaf.insert(Key::fromUInt32(50), makeRef(5)).ok());

    CHECK(leaf.lowerBound(Key::fromUInt32(30)) == 1);
    CHECK(leaf.lowerBound(Key::fromUInt32(25)) == 2);
    CHECK(leaf.upperBound(Key::fromUInt32(30)) == 2);
    CHECK(leaf.find(Key::fromUInt32(30)) == 1);
}

TEST_CASE("LeafNode duplicate rejection", "[btree][leaf]") {
    auto leaf = LeafNode::create(3, u32Config());
    REQUIRE(leaf.insert(Key::fromUInt32(5), makeRef(5)).ok());
    CHECK_FALSE(leaf.insert(Key::fromUInt32(5), makeRef(6)).ok());
}

TEST_CASE("LeafNode allows duplicates when configured", "[btree][leaf]") {
    BTreeNodeConfig config = u32Config();
    config.allowDuplicates = true;
    auto leaf = LeafNode::create(4, config);
    REQUIRE(leaf.insert(Key::fromUInt32(5), makeRef(5)).ok());
    REQUIRE(leaf.insert(Key::fromUInt32(5), makeRef(6)).ok());
    CHECK(leaf.keyCount() == 2);
}

TEST_CASE("LeafNode erase", "[btree][leaf]") {
    auto leaf = LeafNode::create(5, u32Config());
    REQUIRE(leaf.insert(Key::fromUInt32(1), makeRef(1)).ok());
    REQUIRE(leaf.insert(Key::fromUInt32(2), makeRef(2)).ok());
    REQUIRE(leaf.insert(Key::fromUInt32(3), makeRef(3)).ok());

    REQUIRE(leaf.erase(Key::fromUInt32(2)).ok());
    CHECK(leaf.keyCount() == 2);
    CHECK_FALSE(leaf.contains(Key::fromUInt32(2)));
    CHECK(leaf.contains(Key::fromUInt32(1)));
    CHECK(leaf.contains(Key::fromUInt32(3)));

    CHECK_FALSE(leaf.erase(Key::fromUInt32(99)).ok());
}

TEST_CASE("LeafNode eraseAt", "[btree][leaf]") {
    auto leaf = LeafNode::create(6, u32Config());
    REQUIRE(leaf.insert(Key::fromUInt32(1), makeRef(1)).ok());
    REQUIRE(leaf.insert(Key::fromUInt32(2), makeRef(2)).ok());
    REQUIRE(leaf.eraseAt(0).ok());
    CHECK(leaf.keyCount() == 1);
    CHECK(leaf.keyAt(0) == Key::fromUInt32(2));
}

TEST_CASE("LeafNode full node rejects insert", "[btree][leaf]") {
    auto leaf = LeafNode::create(7, u32Config());
    const auto cap = leaf.capacity();
    for (std::uint32_t i = 0; i < cap; ++i) {
        REQUIRE(leaf.insert(Key::fromUInt32(i * 2), makeRef(i + 10)).ok());
    }
    CHECK(leaf.keyCount() == cap);
    CHECK_FALSE(leaf.insert(Key::fromUInt32(9999), makeRef(999)).ok());
}

TEST_CASE("LeafNode occupancy statistics", "[btree][leaf]") {
    auto leaf = LeafNode::create(8, u32Config());
    REQUIRE(leaf.insert(Key::fromUInt32(1), makeRef(1)).ok());
    REQUIRE(leaf.insert(Key::fromUInt32(2), makeRef(2)).ok());

    auto stats = leaf.statistics();
    CHECK(stats.keyCount == 2);
    CHECK(stats.capacity == leaf.capacity());
    CHECK(stats.freeSlots == leaf.capacity() - 2);
    CHECK(stats.nodeType == NodeType::Leaf);
    CHECK(stats.occupancyPercent > 0.0);
    CHECK(stats.estimatedUtilization > 0.0);
    CHECK(stats.usedBytes > 0);
    CHECK(stats.freeBytes > 0);

    auto computed = computeStatistics(leaf);
    CHECK(computed.keyCount == stats.keyCount);
    CHECK(computed.capacity == stats.capacity);
}

TEST_CASE("LeafNode serialize deserialize round-trip", "[btree][leaf]") {
    auto original = LeafNode::create(20, u32Config());
    REQUIRE(original.insert(Key::fromUInt32(5), makeRef(50)).ok());
    REQUIRE(original.insert(Key::fromUInt32(15), makeRef(150)).ok());
    REQUIRE(original.insert(Key::fromUInt32(10), makeRef(100)).ok());

    Buffer buf;
    BinaryWriter writer(buf);
    REQUIRE(original.serialize(writer).ok());

    BinaryReader reader{BufferView(buf)};
    pages::IndexPage page;
    REQUIRE(page.deserialize(reader).ok());
    auto restored = LeafNode::fromPage(std::move(page));

    CHECK(restored.keyCount() == 3);
    CHECK(restored.contains(Key::fromUInt32(5)));
    CHECK(restored.contains(Key::fromUInt32(10)));
    CHECK(restored.contains(Key::fromUInt32(15)));
    CHECK(restored.validate().ok());
    CHECK(NodeValidator::validateLeaf(restored).ok());
}

TEST_CASE("LeafNode uint64 keys", "[btree][leaf]") {
    auto leaf = LeafNode::create(21, u64Config());
    REQUIRE(leaf.insert(Key::fromUInt64(1000), makeRef(1)).ok());
    REQUIRE(leaf.insert(Key::fromUInt64(2000), makeRef(2)).ok());
    CHECK(leaf.contains(Key::fromUInt64(2000)));
    CHECK(leaf.keyAt(0) == Key::fromUInt64(1000));
}

TEST_CASE("LeafNode binary keys", "[btree][leaf]") {
    auto leaf = LeafNode::create(22, binaryConfig(3));
    auto k1 = Key::fromBinary(std::vector<std::uint8_t>{1, 1, 1});
    auto k2 = Key::fromBinary(std::vector<std::uint8_t>{2, 2, 2});
    REQUIRE(leaf.insert(k1, makeRef(1)).ok());
    REQUIRE(leaf.insert(k2, makeRef(2)).ok());
    CHECK(leaf.contains(k2));
    CHECK(leaf.keyAt(0) == k1);
}

// ===== InternalNode =====

TEST_CASE("InternalNode create", "[btree][internal]") {
    auto node = InternalNode::create(30, u32Config());
    CHECK(node.nodeType() == NodeType::Internal);
    CHECK(node.keyCount() == 0);
    CHECK(node.capacity() > 0);
    CHECK(node.children().size() == 1);
    CHECK(node.validate().ok());
}

TEST_CASE("InternalNode insert and child layout", "[btree][internal]") {
    auto node = InternalNode::create(31, u32Config());
    REQUIRE(node.insert(0, Key::fromUInt32(20), makeRef(200)).ok());
    REQUIRE(node.insert(0, Key::fromUInt32(10), makeRef(100)).ok());

    CHECK(node.keyCount() == 2);
    CHECK(node.keyAt(0) == Key::fromUInt32(10));
    CHECK(node.keyAt(1) == Key::fromUInt32(20));
    CHECK(node.children().size() == 3);
    CHECK(node.childAt(1).pageId == 100);
    CHECK(node.childAt(2).pageId == 200);
}

TEST_CASE("InternalNode binary search", "[btree][internal]") {
    auto node = InternalNode::create(32, u32Config());
    REQUIRE(node.insert(0, Key::fromUInt32(30), makeRef(3)).ok());
    REQUIRE(node.insert(0, Key::fromUInt32(10), makeRef(1)).ok());
    REQUIRE(node.insert(1, Key::fromUInt32(20), makeRef(2)).ok());

    CHECK(node.lowerBound(Key::fromUInt32(20)) == 1);
    CHECK(node.upperBound(Key::fromUInt32(20)) == 2);
    CHECK(node.find(Key::fromUInt32(30)) == 2);
}

TEST_CASE("InternalNode eraseAt", "[btree][internal]") {
    auto node = InternalNode::create(33, u32Config());
    REQUIRE(node.insert(0, Key::fromUInt32(20), makeRef(2)).ok());
    REQUIRE(node.insert(0, Key::fromUInt32(10), makeRef(1)).ok());
    REQUIRE(node.eraseAt(0).ok());
    CHECK(node.keyCount() == 1);
    CHECK(node.keyAt(0) == Key::fromUInt32(20));
    CHECK(node.children().size() == 2);
}

TEST_CASE("InternalNode serialize round-trip", "[btree][internal]") {
    auto original = InternalNode::create(34, u32Config());
    REQUIRE(original.insert(0, Key::fromUInt32(50), makeRef(5)).ok());
    REQUIRE(original.insert(0, Key::fromUInt32(25), makeRef(25)).ok());

    Buffer buf;
    BinaryWriter writer(buf);
    REQUIRE(original.serialize(writer).ok());

    BinaryReader reader{BufferView(buf)};
    pages::IndexPage page;
    REQUIRE(page.deserialize(reader).ok());
    auto restored = InternalNode::fromPage(std::move(page));

    CHECK(restored.keyCount() == 2);
    CHECK(restored.keyAt(0) == Key::fromUInt32(25));
    CHECK(restored.keyAt(1) == Key::fromUInt32(50));
    CHECK(restored.validate().ok());
    CHECK(NodeValidator::validateInternal(restored).ok());
}

TEST_CASE("InternalNode duplicate rejection", "[btree][internal]") {
    auto node = InternalNode::create(35, u32Config());
    REQUIRE(node.insert(0, Key::fromUInt32(10), makeRef(1)).ok());
    CHECK_FALSE(node.insert(0, Key::fromUInt32(10), makeRef(2)).ok());
}

// ===== Cursor =====

TEST_CASE("Cursor leaf traversal", "[btree][cursor]") {
    auto leaf = LeafNode::create(40, u32Config());
    REQUIRE(leaf.insert(Key::fromUInt32(1), makeRef(1)).ok());
    REQUIRE(leaf.insert(Key::fromUInt32(2), makeRef(2)).ok());
    REQUIRE(leaf.insert(Key::fromUInt32(3), makeRef(3)).ok());

    Cursor cursor;
    cursor.bindLeaf(&leaf);
    CHECK_FALSE(cursor.valid());

    REQUIRE(cursor.seek(0).ok());
    CHECK(cursor.valid());
    CHECK(cursor.currentKey() == Key::fromUInt32(1));
    CHECK(cursor.currentReference().pageId == 1);

    REQUIRE(cursor.next().ok());
    CHECK(cursor.currentKey() == Key::fromUInt32(2));

    REQUIRE(cursor.seek(2).ok());
    REQUIRE(cursor.previous().ok());
    CHECK(cursor.currentKey() == Key::fromUInt32(2));

    REQUIRE(cursor.seekKey(Key::fromUInt32(3)).ok());
    CHECK(cursor.position() == 2);

    cursor.reset();
    CHECK_FALSE(cursor.valid());
}

TEST_CASE("Cursor internal traversal", "[btree][cursor]") {
    auto node = InternalNode::create(41, u32Config());
    REQUIRE(node.insert(0, Key::fromUInt32(10), makeRef(10)).ok());

    Cursor cursor;
    cursor.bindInternal(&node);
    REQUIRE(cursor.seek(0).ok());
    CHECK(cursor.currentKey() == Key::fromUInt32(10));
    CHECK(cursor.currentReference().pageId == 10);
}

TEST_CASE("Cursor invalid operations", "[btree][cursor]") {
    Cursor cursor;
    CHECK_FALSE(cursor.seek(0).ok());
    CHECK_FALSE(cursor.next().ok());
    CHECK_FALSE(cursor.previous().ok());
}

// ===== SearchPath =====

TEST_CASE("SearchPath stack operations", "[btree][searchpath]") {
    SearchPath path;
    CHECK(path.empty());
    CHECK(path.depth() == 0);

    path.reserve(8);
    SearchPathEntry e1{10, 0, makeRef(10)};
    SearchPathEntry e2{20, 1, makeRef(20)};
    path.push(e1);
    path.push(e2);

    CHECK_FALSE(path.empty());
    CHECK(path.depth() == 2);
    CHECK(path.top().pageId == 20);
    SearchPathEntry parentFrame;
    CHECK(path.parentEntry(parentFrame).ok());
    CHECK(parentFrame.pageId == 10);
    CHECK(path.at(0).pageId == 10);

    path.pop();
    CHECK(path.depth() == 1);
    CHECK(path.top().pageId == 10);

    path.clear();
    CHECK(path.empty());
}

// ===== NodeValidator =====

TEST_CASE("NodeValidator rejects unsorted leaf", "[btree][validator]") {
    auto leaf = LeafNode::create(50, u32Config());
    leaf.indexPage().setCapacity(10);
    leaf.indexPage().setKeyCount(2);
    CHECK_FALSE(NodeValidator::validate(leaf).ok());
}

TEST_CASE("NodeValidator serialized body leaf", "[btree][validator]") {
    Buffer buf;
    BinaryWriter writer(buf);
    REQUIRE(Key::fromUInt32(1).serialize(writer).ok());
    REQUIRE(writer.write(makeRef(1)).ok());
    REQUIRE(Key::fromUInt32(2).serialize(writer).ok());
    REQUIRE(writer.write(makeRef(2)).ok());

    BinaryReader reader{BufferView(buf)};
    CHECK(NodeValidator::validateSerializedBody(reader, u32Config(), NodeType::Leaf, 2).ok());
}

TEST_CASE("NodeValidator serialized body malformed", "[btree][validator]") {
    Buffer buf;
    BinaryWriter writer(buf);
    REQUIRE(Key::fromUInt32(1).serialize(writer).ok());

    BinaryReader reader{BufferView(buf)};
    CHECK_FALSE(NodeValidator::validateSerializedBody(reader, u32Config(), NodeType::Leaf, 2).ok());
}

// ===== Split / Merge Planning =====

TEST_CASE("SplitMergePlanner leaf split", "[btree][split]") {
    auto leaf = LeafNode::create(60, u32Config());
    for (std::uint32_t i = 1; i <= 10; ++i) {
        REQUIRE(leaf.insert(Key::fromUInt32(i * 10), makeRef(i)).ok());
    }

    auto plan = SplitMergePlanner::planLeafSplit(leaf);
    CHECK(plan.feasible);
    CHECK(plan.splitPosition > 0);
    CHECK(plan.splitPosition < leaf.keyCount());
    CHECK(plan.promotedKey == leaf.keyAt(plan.splitPosition));
    CHECK(plan.leftOccupancy > 0.0);
    CHECK(plan.rightOccupancy > 0.0);
}

TEST_CASE("SplitMergePlanner internal split", "[btree][split]") {
    auto node = InternalNode::create(61, u32Config());
    for (std::uint32_t i = 0; i < 6; ++i) {
        REQUIRE(node.insert(i, Key::fromUInt32((i + 1) * 10), makeRef(i + 100)).ok());
    }

    auto plan = SplitMergePlanner::planInternalSplit(node);
    CHECK(plan.feasible);
    CHECK(plan.splitPosition > 0);
    CHECK(plan.promotedKey == node.keyAt(plan.splitPosition));
}

TEST_CASE("SplitMergePlanner leaf merge feasible", "[btree][merge]") {
    auto left = LeafNode::create(62, u32Config());
    auto right = LeafNode::create(63, u32Config());
    REQUIRE(left.insert(Key::fromUInt32(1), makeRef(1)).ok());
    REQUIRE(left.insert(Key::fromUInt32(2), makeRef(2)).ok());
    REQUIRE(right.insert(Key::fromUInt32(3), makeRef(3)).ok());

    auto plan = SplitMergePlanner::planLeafMerge(left, right);
    CHECK(plan.feasible);
    CHECK(plan.resultingKeyCount == 3);
    CHECK(plan.combinedOccupancy > 0.0);
}

TEST_CASE("SplitMergePlanner leaf merge infeasible", "[btree][merge]") {
    auto left = LeafNode::create(64, u32Config());
    auto right = LeafNode::create(65, u32Config());
    const auto cap = left.capacity();
    for (std::uint32_t i = 0; i < cap - 1; ++i) {
        REQUIRE(left.insert(Key::fromUInt32(i), makeRef(i)).ok());
    }
    for (std::uint32_t i = 0; i < 3; ++i) {
        REQUIRE(right.insert(Key::fromUInt32(1000 + i), makeRef(100 + i)).ok());
    }

    auto plan = SplitMergePlanner::planLeafMerge(left, right);
    CHECK_FALSE(plan.feasible);
}

TEST_CASE("SplitMergePlanner internal merge", "[btree][merge]") {
    auto left = InternalNode::create(66, u32Config());
    auto right = InternalNode::create(67, u32Config());
    REQUIRE(left.insert(0, Key::fromUInt32(10), makeRef(1)).ok());
    REQUIRE(right.insert(0, Key::fromUInt32(20), makeRef(2)).ok());

    auto plan = SplitMergePlanner::planInternalMerge(left, right);
    CHECK(plan.feasible);
    CHECK(plan.resultingKeyCount == 2);
}

// ===== BTreeNode capacity =====

TEST_CASE("BTreeNode computeCapacity", "[btree][node]") {
    const auto leafCap = BTreeNode::computeCapacity(u32Config(), NodeType::Leaf);
    const auto internalCap = BTreeNode::computeCapacity(u32Config(), NodeType::Internal);
    CHECK(leafCap > 0);
    CHECK(internalCap > 0);
    CHECK(leafCap >= internalCap);
}

TEST_CASE("BTreeNode parent and level", "[btree][node]") {
    auto leaf = LeafNode::create(70, u32Config());
    leaf.setLevel(2);
    leaf.setParent(makeRef(99));
    CHECK(leaf.level() == 2);
    CHECK(leaf.parent().pageId == 99);
}

// ===== Malformed / edge cases =====

TEST_CASE("LeafNode invalid page reference on insert", "[btree][edge]") {
    auto leaf = LeafNode::create(80, u32Config());
    CHECK_FALSE(leaf.insert(Key::fromUInt32(1), PageReference::invalid()).ok());
}

TEST_CASE("LeafNode eraseAt out of range", "[btree][edge]") {
    auto leaf = LeafNode::create(81, u32Config());
    CHECK_FALSE(leaf.eraseAt(0).ok());
}

TEST_CASE("Key deserialize truncated buffer", "[btree][edge]") {
    Buffer buf;
    BinaryWriter writer(buf);
    std::uint8_t typeByte = static_cast<std::uint8_t>(KeyType::UInt32);
    REQUIRE(writer.write(typeByte).ok());

    BinaryReader reader{BufferView(buf)};
    Key key;
    CHECK_FALSE(key.deserialize(reader).ok());
}

TEST_CASE("LeafNode body deserialize truncated", "[btree][edge]") {
    Buffer buf;
    BinaryReader reader{BufferView(buf)};
    CHECK_FALSE(NodeValidator::validateSerializedBody(reader, u32Config(), NodeType::Leaf, 1).ok());
}

TEST_CASE("Split plan empty node", "[btree][edge]") {
    auto leaf = LeafNode::create(82, u32Config());
    auto plan = SplitMergePlanner::planLeafSplit(leaf);
    CHECK_FALSE(plan.feasible);
}

TEST_CASE("InternalNode insert out of range index", "[btree][edge]") {
    auto node = InternalNode::create(83, u32Config());
    CHECK_FALSE(node.insert(5, Key::fromUInt32(1), makeRef(1)).ok());
}

TEST_CASE("LeafNode free slots and occupancy percent", "[btree][edge]") {
    auto leaf = LeafNode::create(84, u32Config());
    CHECK(leaf.freeSlots() == leaf.capacity());
    CHECK(leaf.occupancyPercent() == 0.0);
    REQUIRE(leaf.insert(Key::fromUInt32(1), makeRef(1)).ok());
    CHECK(leaf.freeSlots() == leaf.capacity() - 1);
    CHECK(leaf.occupancyPercent() > 0.0);
}

TEST_CASE("LeafNode sync preserves order after reload", "[btree][edge]") {
    auto leaf = LeafNode::create(85, u32Config());
    std::vector<std::uint32_t> values = {50, 10, 30, 20, 40};
    for (auto v : values) {
        REQUIRE(leaf.insert(Key::fromUInt32(v), makeRef(v)).ok());
    }
    REQUIRE(leaf.syncEntries().ok());

    storage::Page clonedPage(leaf.pageId(), storage::PageType::Index);
    std::memcpy(clonedPage.data(), leaf.indexPage().page().data(),
                leaf.indexPage().page().size());
    auto fromClone = LeafNode::fromPage(pages::IndexPage(std::move(clonedPage)));
    CHECK(fromClone.keyCount() == 5);
    CHECK(fromClone.keyAt(0) == Key::fromUInt32(10));
    CHECK(fromClone.keyAt(4) == Key::fromUInt32(50));
}

TEST_CASE("Cursor seek key not found", "[btree][edge]") {
    auto leaf = LeafNode::create(86, u32Config());
    REQUIRE(leaf.insert(Key::fromUInt32(1), makeRef(1)).ok());
    Cursor cursor;
    cursor.bindLeaf(&leaf);
    CHECK_FALSE(cursor.seekKey(Key::fromUInt32(2)).ok());
}

TEST_CASE("SearchPath parent requires depth", "[btree][edge]") {
    SearchPath path;
    path.push({1, 0, makeRef(1)});
    SearchPathEntry parent;
    CHECK_FALSE(path.parentEntry(parent).ok());
}

TEST_CASE("Multiple leaf nodes independent", "[btree][edge]") {
    auto a = LeafNode::create(90, u32Config());
    auto b = LeafNode::create(91, u32Config());
    REQUIRE(a.insert(Key::fromUInt32(1), makeRef(1)).ok());
    REQUIRE(b.insert(Key::fromUInt32(2), makeRef(2)).ok());
    CHECK(a.contains(Key::fromUInt32(1)));
    CHECK_FALSE(a.contains(Key::fromUInt32(2)));
    CHECK(b.contains(Key::fromUInt32(2)));
}

TEST_CASE("BTreeNode serialize full page size", "[btree][edge]") {
    auto leaf = LeafNode::create(92, u32Config());
    Buffer buf;
    BinaryWriter writer(buf);
    REQUIRE(leaf.serialize(writer).ok());
    CHECK(buf.size() == storage::kPageSize);
}

TEST_CASE("Key comparison exhaustive uint32 ordering", "[btree][key]") {
    for (std::uint32_t i = 0; i < 20; ++i) {
        for (std::uint32_t j = 0; j < 20; ++j) {
            auto ki = Key::fromUInt32(i);
            auto kj = Key::fromUInt32(j);
            if (i < j) {
                CHECK(ki < kj);
                CHECK_FALSE(ki == kj);
            } else if (i == j) {
                CHECK(ki == kj);
                CHECK_FALSE(ki < kj);
            } else {
                CHECK(ki > kj);
            }
        }
    }
}

TEST_CASE("LeafNode binary search positions across range", "[btree][leaf]") {
    auto leaf = LeafNode::create(93, u32Config());
    for (std::uint32_t i = 1; i <= 8; ++i) {
        REQUIRE(leaf.insert(Key::fromUInt32(i * 10), makeRef(i * 10)).ok());
    }
    CHECK(leaf.lowerBound(Key::fromUInt32(5)) == 0);
    CHECK(leaf.lowerBound(Key::fromUInt32(10)) == 0);
    CHECK(leaf.lowerBound(Key::fromUInt32(15)) == 1);
    CHECK(leaf.lowerBound(Key::fromUInt32(80)) == 7);
    CHECK(leaf.lowerBound(Key::fromUInt32(85)) == 8);
    CHECK(leaf.upperBound(Key::fromUInt32(10)) == 1);
    CHECK(leaf.upperBound(Key::fromUInt32(80)) == 8);
    CHECK(leaf.find(Key::fromUInt32(50)) == 4);
    CHECK(leaf.find(Key::fromUInt32(99)) == leaf.keys().size());
}

TEST_CASE("LeafNode incremental insert maintains sorted order", "[btree][leaf]") {
    auto leaf = LeafNode::create(94, u32Config());
    std::vector<std::uint32_t> input = {42, 7, 99, 1, 55, 23, 88, 12, 67, 34};
    for (auto v : input) {
        REQUIRE(leaf.insert(Key::fromUInt32(v), makeRef(v)).ok());
    }
    for (std::size_t i = 1; i < leaf.keys().size(); ++i) {
        CHECK(leaf.keys()[i - 1] < leaf.keys()[i]);
    }
}

TEST_CASE("LeafNode erase all keys one by one", "[btree][leaf]") {
    auto leaf = LeafNode::create(95, u32Config());
    for (std::uint32_t i = 0; i < 5; ++i) {
        REQUIRE(leaf.insert(Key::fromUInt32(i), makeRef(i)).ok());
    }
    for (std::uint32_t i = 0; i < 5; ++i) {
        REQUIRE(leaf.erase(Key::fromUInt32(i)).ok());
        CHECK(leaf.keyCount() == 4 - i);
    }
    CHECK(leaf.keyCount() == 0);
    CHECK(leaf.occupancyPercent() == 0.0);
}

TEST_CASE("InternalNode fill and child integrity", "[btree][internal]") {
    auto node = InternalNode::create(96, u32Config());
    const auto cap = node.capacity();
    for (std::uint32_t i = 0; i < cap; ++i) {
        REQUIRE(node.insert(i, Key::fromUInt32(i + 1), makeRef(100 + i)).ok());
    }
    CHECK(node.keyCount() == cap);
    CHECK(node.children().size() == cap + 1);
    for (std::uint32_t i = 0; i < cap; ++i) {
        CHECK(node.keyAt(i) == Key::fromUInt32(i + 1));
        CHECK(node.childAt(i + 1).pageId == 100 + i);
    }
    CHECK_FALSE(node.insert(0, Key::fromUInt32(999), makeRef(999)).ok());
}

TEST_CASE("InternalNode lower bound across separators", "[btree][internal]") {
    auto node = InternalNode::create(97, u32Config());
    REQUIRE(node.insert(0, Key::fromUInt32(20), makeRef(2)).ok());
    REQUIRE(node.insert(0, Key::fromUInt32(10), makeRef(1)).ok());
    REQUIRE(node.insert(2, Key::fromUInt32(40), makeRef(4)).ok());
    REQUIRE(node.insert(2, Key::fromUInt32(30), makeRef(3)).ok());

    CHECK(node.lowerBound(Key::fromUInt32(5)) == 0);
    CHECK(node.lowerBound(Key::fromUInt32(10)) == 0);
    CHECK(node.lowerBound(Key::fromUInt32(15)) == 1);
    CHECK(node.lowerBound(Key::fromUInt32(35)) == 3);
    CHECK(node.upperBound(Key::fromUInt32(30)) == 3);
}

TEST_CASE("Key hash distribution basic", "[btree][key]") {
    std::unordered_set<std::size_t> hashes;
    for (std::uint32_t i = 0; i < 50; ++i) {
        hashes.insert(Key::fromUInt32(i).hash());
    }
    CHECK(hashes.size() == 50);
}

TEST_CASE("Key binary fixed size serialization sizes", "[btree][key]") {
    auto key = Key::fromBinary(std::vector<std::uint8_t>{1, 2, 3, 4});
    CHECK(key.serializedSize() == 1 + 2 + 4);
    Buffer buf;
    BinaryWriter writer(buf);
    REQUIRE(key.serialize(writer).ok());
    CHECK(buf.size() == key.serializedSize());
}

TEST_CASE("LeafNode body serialize deserialize", "[btree][leaf]") {
    auto leaf = LeafNode::create(98, u32Config());
    REQUIRE(leaf.insert(Key::fromUInt32(7), makeRef(7)).ok());
    REQUIRE(leaf.insert(Key::fromUInt32(3), makeRef(3)).ok());

    Buffer buf;
    BinaryWriter writer(buf);
    REQUIRE(leaf.serializeBody(writer).ok());

    BinaryReader reader{BufferView(buf)};
    LeafNode decoded(pages::IndexPage::create(98));
    decoded.indexPage().setKeyCount(leaf.keyCount());
    REQUIRE(decoded.deserializeBody(reader).ok());
    CHECK(decoded.keys().size() == 2);
    CHECK(decoded.keys()[0] == Key::fromUInt32(3));
}

TEST_CASE("SearchPath multiple depths parent chain", "[btree][searchpath]") {
    SearchPath path;
    for (storage::PageId id = 1; id <= 5; ++id) {
        path.push({id, 0, makeRef(id)});
    }
    CHECK(path.depth() == 5);
    SearchPathEntry parentFrame;
    CHECK(path.parentEntry(parentFrame).ok());
    CHECK(parentFrame.pageId == 4);
    CHECK(path.at(2).pageId == 3);
    path.pop();
    path.pop();
    CHECK(path.depth() == 3);
    CHECK(path.top().pageId == 3);
}

TEST_CASE("SplitMergePlanner empty internal split", "[btree][split]") {
    auto node = InternalNode::create(99, u32Config());
    auto plan = SplitMergePlanner::planInternalSplit(node);
    CHECK_FALSE(plan.feasible);
}

TEST_CASE("BTreeNodeConfig binary capacity scales with key size", "[btree][node]") {
    const auto small = BTreeNode::computeCapacity(binaryConfig(4), NodeType::Leaf);
    const auto large = BTreeNode::computeCapacity(binaryConfig(64), NodeType::Leaf);
    CHECK(small > large);
    CHECK(small > 0);
    CHECK(large > 0);
}

TEST_CASE("NodeValidator validate polymorphic", "[btree][validator]") {
    auto leaf = LeafNode::create(100, u32Config());
    REQUIRE(leaf.insert(Key::fromUInt32(1), makeRef(1)).ok());
    BTreeNode& base = leaf;
    CHECK(NodeValidator::validate(base).ok());
}

TEST_CASE("Cursor next reaches end", "[btree][cursor]") {
    auto leaf = LeafNode::create(101, u32Config());
    REQUIRE(leaf.insert(Key::fromUInt32(1), makeRef(1)).ok());
    Cursor cursor;
    cursor.bindLeaf(&leaf);
    REQUIRE(cursor.seek(0).ok());
    CHECK_FALSE(cursor.next().ok());
}

TEST_CASE("LeafNode reference round-trip after serialize", "[btree][leaf]") {
    auto leaf = LeafNode::create(102, u32Config());
    auto ref = makeRef(555);
    ref.generation = 42;
    REQUIRE(leaf.insert(Key::fromUInt32(99), ref).ok());

    Buffer buf;
    BinaryWriter writer(buf);
    REQUIRE(leaf.serialize(writer).ok());

    BinaryReader reader{BufferView(buf)};
    pages::IndexPage page;
    REQUIRE(page.deserialize(reader).ok());
    auto restored = LeafNode::fromPage(std::move(page));
    CHECK(restored.referenceAt(0).pageId == 555);
    CHECK(restored.referenceAt(0).generation == 42);
}
