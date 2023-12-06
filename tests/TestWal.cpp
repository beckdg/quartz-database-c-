#include "quartz/btree/BTree.h"
#include "quartz/btree/Key.h"
#include "quartz/format/PageReference.h"
#include "quartz/serialization/BinaryReader.h"
#include "quartz/serialization/BinaryWriter.h"
#include "quartz/serialization/Buffer.h"
#include "quartz/space/SpaceManager.h"
#include "quartz/wal/BTreeWalAdapter.h"
#include "quartz/wal/LogBuffer.h"
#include "quartz/wal/LogFile.h"
#include "quartz/wal/LogManager.h"
#include "quartz/wal/LogReader.h"
#include "quartz/wal/LogRecord.h"
#include "quartz/wal/LogSequenceNumber.h"
#include "quartz/wal/LogTypes.h"
#include "quartz/wal/LogValidator.h"
#include "quartz/wal/LogWriter.h"

#include <catch2/catch_test_macros.hpp>

#include <cstring>

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using namespace quartz;
using namespace quartz::btree;
using namespace quartz::format;
using namespace quartz::serialization;
using namespace quartz::space;
using namespace quartz::storage;
using namespace quartz::wal;

namespace {

std::string walTestPath(const char* name) {
    return fs::temp_directory_path().string() + "/quartzdb_wal_" + name;
}

void cleanupWal(const std::string& path) {
    std::error_code ec;
    fs::remove(fs::path(path), ec);
}

BTreeNodeConfig defaultBTreeConfig() {
    BTreeNodeConfig config;
    config.keyType = KeyType::UInt32;
    return config;
}

PageReference makeDataRef(PageId id) {
    return PageReference::make(id, 1, static_cast<std::uint8_t>(PageType::Data));
}

LogRecord makeTestRecord(LogRecordType type, PageId pageId, std::size_t payloadSize = 0) {
    auto record = LogRecord::make(type, pageId);
    if (payloadSize > 0) {
        record.payload().resize(payloadSize);
        std::memset(record.payload().data(), 0xAB, payloadSize);
    }
    return record;
}

std::vector<LogRecord> readAllRecords(LogManager& manager) {
    std::vector<LogRecord> records;
    REQUIRE(manager.rewindReader().ok());
    LogRecord record;
    while (manager.readNext(record).ok()) {
        records.push_back(std::move(record));
    }
    return records;
}

} // namespace

// ===== LogSequenceNumber =====

TEST_CASE("LSN invalid and initial values", "[wal][lsn]") {
    const auto invalid = LogSequenceNumber::invalid();
    CHECK_FALSE(invalid.isValid());
    CHECK(invalid.value() == 0);
    CHECK(invalid.toString() == "LSN(invalid)");

    const auto initial = LogSequenceNumber::initial();
    CHECK(initial.isValid());
    CHECK(initial.value() == 1);
    CHECK(initial.toString() == "LSN(1)");
}

TEST_CASE("LSN comparison and ordering", "[wal][lsn]") {
    const LogSequenceNumber a(5);
    const LogSequenceNumber b(10);
    const LogSequenceNumber c(10);

    CHECK(a < b);
    CHECK(a <= b);
    CHECK(b > a);
    CHECK(b >= a);
    CHECK(b == c);
    CHECK_FALSE(b != c);
    CHECK(a != b);
}

TEST_CASE("LSN increment", "[wal][lsn]") {
    auto lsn = LogSequenceNumber::initial();
    for (std::uint64_t expected = 1; expected <= 100; ++expected) {
        CHECK(lsn.value() == expected);
        lsn = lsn.next();
    }
}

TEST_CASE("LSN serialization round-trip", "[wal][lsn]") {
    Buffer buf;
    BinaryWriter writer(buf);

    const LogSequenceNumber original(4242);
    REQUIRE(original.serialize(writer).ok());

    BinaryReader reader{BufferView(buf)};
    LogSequenceNumber restored;
    REQUIRE(restored.deserialize(reader).ok());
    CHECK(restored == original);
    CHECK(restored.toString() == "LSN(4242)");
}

// ===== LogRecord =====

TEST_CASE("LogRecord make assigns type and timestamp", "[wal][record]") {
    auto record = LogRecord::make(LogRecordType::PageUpdate, 42);
    CHECK(record.type() == LogRecordType::PageUpdate);
    CHECK(record.pageId() == 42);
    CHECK(record.timestamp() > 0);
    CHECK(record.payload().empty());
}

TEST_CASE("LogRecord serialization round-trip", "[wal][record]") {
    auto original = makeTestRecord(LogRecordType::Allocation, 7, 32);
    original.setLsn(LogSequenceNumber(99));
    original.setTransactionId(1234);

    Buffer buf;
    BinaryWriter writer(buf);
    REQUIRE(original.serialize(writer).ok());

    BinaryReader reader{BufferView(buf)};
    LogRecord restored;
    REQUIRE(restored.deserialize(reader).ok());

    CHECK(restored.type() == original.type());
    CHECK(restored.lsn() == original.lsn());
    CHECK(restored.pageId() == original.pageId());
    CHECK(restored.transactionId() == original.transactionId());
    CHECK(restored.payloadLength() == 32);
    CHECK(restored.payload().size() == 32);
}

TEST_CASE("LogRecord all supported types serialize", "[wal][record]") {
    const LogRecordType types[] = {
        LogRecordType::PageCreate,     LogRecordType::PageUpdate,   LogRecordType::PageDelete,
        LogRecordType::NodeSplit,        LogRecordType::NodeMerge,    LogRecordType::Allocation,
        LogRecordType::Deallocation,     LogRecordType::MetadataUpdate,
        LogRecordType::CheckpointMarker,
    };

    for (const auto type : types) {
        auto record = LogRecord::make(type, 1);
        Buffer buf;
        BinaryWriter writer(buf);
        REQUIRE(record.serialize(writer).ok());
        BinaryReader reader{BufferView(buf)};
        LogRecord restored;
        REQUIRE(restored.deserialize(reader).ok());
        CHECK(restored.type() == type);
        CHECK(isValidRecordType(restored.type()));
    }
}

TEST_CASE("LogRecord large payload round-trip", "[wal][record]") {
    constexpr std::size_t kLarge = 8192;
    auto record = makeTestRecord(LogRecordType::PageUpdate, 3, kLarge);

    Buffer buf;
    BinaryWriter writer(buf);
    REQUIRE(record.serialize(writer).ok());
    CHECK(buf.size() >= kLarge);

    BinaryReader reader{BufferView(buf)};
    LogRecord restored;
    REQUIRE(restored.deserialize(reader).ok());
    CHECK(restored.payloadLength() == kLarge);
}

TEST_CASE("LogRecord toString contains key fields", "[wal][record]") {
    auto record = LogRecord::make(LogRecordType::NodeSplit, 5);
    record.setLsn(LogSequenceNumber(3));
    const auto text = record.toString();
    const bool hasSplit = text.find("NodeSplit") != std::string::npos ||
                          text.find("type=4") != std::string::npos;
    CHECK(hasSplit);
    CHECK(text.find("LSN(3)") != std::string::npos);
}

// ===== LogBuffer =====

TEST_CASE("LogBuffer append and size", "[wal][buffer]") {
    LogBuffer buffer(256);
    CHECK(buffer.empty());
    CHECK(buffer.size() == 0);
    CHECK(buffer.capacity() == 256);
    CHECK(buffer.remainingCapacity() == 256);

    const std::uint8_t data[] = {1, 2, 3, 4, 5};
    REQUIRE(buffer.append(BufferView(data, sizeof(data))).ok());
    CHECK(buffer.size() == 5);
    CHECK_FALSE(buffer.empty());
    CHECK(buffer.remainingCapacity() == 251);
}

TEST_CASE("LogBuffer reserve and clear", "[wal][buffer]") {
    LogBuffer buffer(128);
    REQUIRE(buffer.reserve(64).ok());
    REQUIRE(buffer.append(BufferView(reinterpret_cast<const std::uint8_t*>("hello"), 5)).ok());
    CHECK(buffer.size() == 5);
    buffer.clear();
    CHECK(buffer.empty());
    CHECK(buffer.size() == 0);
}

TEST_CASE("LogBuffer flush invokes sink", "[wal][buffer]") {
    LogBuffer buffer(64);
    const std::uint8_t payload[] = {0xDE, 0xAD};
    REQUIRE(buffer.append(BufferView(payload, sizeof(payload))).ok());

    std::size_t flushed = 0;
    REQUIRE(buffer.flush([&](BufferView data) {
        flushed = data.size();
        return Status::success();
    }).ok());
    CHECK(flushed == 2);
    CHECK(buffer.empty());
}

TEST_CASE("LogBuffer capacity exceeded", "[wal][buffer]") {
    LogBuffer buffer(4);
    const std::uint8_t data[8] = {};
    CHECK_FALSE(buffer.append(BufferView(data, sizeof(data))).ok());
}

// ===== LogFile =====

TEST_CASE("LogFile create open validate close", "[wal][file]") {
    auto path = walTestPath("create");
    cleanupWal(path);

    LogFile file;
    REQUIRE(file.create(path).ok());
    CHECK(file.isOpen());
    CHECK(file.validate().ok());
    CHECK(file.fileSize() >= WalFileHeader::kSize);
    CHECK(file.dataOffset() == WalFileHeader::kSize);

    REQUIRE(file.close().ok());
    CHECK_FALSE(file.isOpen());
    cleanupWal(path);
}

TEST_CASE("LogFile append and read record", "[wal][file]") {
    auto path = walTestPath("append_read");
    cleanupWal(path);

    LogFile file;
    REQUIRE(file.create(path).ok());

    auto record = makeTestRecord(LogRecordType::Allocation, 10, 16);
    record.setLsn(LogSequenceNumber(1));
    REQUIRE(file.appendRecord(record).ok());
    REQUIRE(file.flush().ok());
    CHECK(file.fileSize() > WalFileHeader::kSize);

    LogRecord readBack;
    std::uint64_t bytesRead = 0;
    REQUIRE(file.readRecord(WalFileHeader::kSize, readBack, bytesRead).ok());
    CHECK(bytesRead > 0);
    CHECK(readBack.type() == LogRecordType::Allocation);
    CHECK(readBack.pageId() == 10);
    CHECK(readBack.lsn() == LogSequenceNumber(1));

    REQUIRE(file.close().ok());
    cleanupWal(path);
}

TEST_CASE("LogFile empty log has header only", "[wal][file]") {
    auto path = walTestPath("empty");
    cleanupWal(path);

    LogFile file;
    REQUIRE(file.create(path).ok());
    CHECK(file.fileSize() == WalFileHeader::kSize);
    REQUIRE(file.close().ok());
    cleanupWal(path);
}

// ===== LogWriter / LogReader =====

TEST_CASE("LogWriter assigns monotonic LSNs", "[wal][writer]") {
    auto path = walTestPath("writer_lsn");
    cleanupWal(path);

    LogFile file;
    REQUIRE(file.create(path).ok());
    LogStatisticsCollector stats;
    LogWriter writer(file, stats);

    for (std::uint64_t i = 1; i <= 20; ++i) {
        auto record = makeTestRecord(LogRecordType::PageUpdate, static_cast<PageId>(i));
        REQUIRE(writer.appendRecord(std::move(record)).ok());
        CHECK(writer.currentLsn().value() == i);
    }

    REQUIRE(writer.flush().ok());
    CHECK(writer.lastFlushedLsn().value() == 20);
    REQUIRE(file.close().ok());
    cleanupWal(path);
}

TEST_CASE("LogWriter batch append", "[wal][writer]") {
    auto path = walTestPath("writer_batch");
    cleanupWal(path);

    LogFile file;
    REQUIRE(file.create(path).ok());
    LogStatisticsCollector stats;
    LogWriter writer(file, stats);

    std::vector<LogRecord> batch;
    for (int i = 0; i < 50; ++i) {
        batch.push_back(makeTestRecord(LogRecordType::MetadataUpdate, static_cast<PageId>(i)));
    }
    REQUIRE(writer.appendBatch(std::move(batch)).ok());
    REQUIRE(writer.flush().ok());

    LogReader reader(file, stats);
    REQUIRE(reader.rewind().ok());
    LogRecord record;
    std::uint64_t count = 0;
    while (reader.readNext(record).ok()) {
        ++count;
        CHECK(record.lsn().value() == count);
    }
    CHECK(count == 50);

    REQUIRE(file.close().ok());
    cleanupWal(path);
}

TEST_CASE("LogReader seek and rewind", "[wal][reader]") {
    auto path = walTestPath("reader_seek");
    cleanupWal(path);

    LogFile file;
    REQUIRE(file.create(path).ok());
    LogStatisticsCollector stats;
    LogWriter writer(file, stats);

    for (int i = 0; i < 10; ++i) {
        REQUIRE(writer.appendRecord(makeTestRecord(LogRecordType::Allocation, static_cast<PageId>(i))).ok());
    }
    REQUIRE(writer.flush().ok());

    LogReader reader(file, stats);
    REQUIRE(reader.seekLsn(LogSequenceNumber(5)).ok());
    LogRecord record;
    REQUIRE(reader.readNext(record).ok());
    CHECK(record.lsn() == LogSequenceNumber(5));

    REQUIRE(reader.rewind().ok());
    REQUIRE(reader.readNext(record).ok());
    CHECK(record.lsn() == LogSequenceNumber(1));

    REQUIRE(file.close().ok());
    cleanupWal(path);
}

TEST_CASE("LogReader end of log", "[wal][reader]") {
    auto path = walTestPath("reader_eof");
    cleanupWal(path);

    LogFile file;
    REQUIRE(file.create(path).ok());
    LogStatisticsCollector stats;
    LogWriter writer(file, stats);
    REQUIRE(writer.appendRecord(makeTestRecord(LogRecordType::PageCreate, 1)).ok());
    REQUIRE(writer.flush().ok());

    LogReader reader(file, stats);
    REQUIRE(reader.rewind().ok());
    LogRecord record;
    REQUIRE(reader.readNext(record).ok());
    CHECK_FALSE(reader.readNext(record).ok());
    CHECK(reader.endOfLog());

    REQUIRE(file.close().ok());
    cleanupWal(path);
}

// ===== LogValidator =====

TEST_CASE("LogValidator rejects invalid record type", "[wal][validator]") {
    auto record = LogRecord::make(LogRecordType::Invalid, 1);
    record.setLsn(LogSequenceNumber(1));
    CHECK_FALSE(LogValidator::validateRecordHeader(record).ok());
}

TEST_CASE("LogValidator LSN ordering", "[wal][validator]") {
    CHECK(LogValidator::validateLsnOrdering(LogSequenceNumber::invalid(), LogSequenceNumber(1)).ok());
    CHECK(LogValidator::validateLsnOrdering(LogSequenceNumber(1), LogSequenceNumber(2)).ok());
    CHECK_FALSE(LogValidator::validateLsnOrdering(LogSequenceNumber(5), LogSequenceNumber(5)).ok());
    CHECK_FALSE(LogValidator::validateLsnOrdering(LogSequenceNumber(10), LogSequenceNumber(3)).ok());
}

TEST_CASE("LogValidator rejects missing timestamp", "[wal][validator]") {
    auto record = LogRecord::make(LogRecordType::PageUpdate, 1);
    record.setTimestamp(0);
    record.setLsn(LogSequenceNumber(1));
    CHECK_FALSE(LogValidator::validateRecordHeader(record).ok());
}

// ===== LogManager =====

TEST_CASE("LogManager lifecycle append flush read", "[wal][manager]") {
    auto path = walTestPath("manager");
    cleanupWal(path);

    LogManager manager;
    CHECK_FALSE(manager.isInitialized());
    REQUIRE(manager.initialize(path).ok());
    CHECK(manager.isInitialized());

    for (int i = 0; i < 25; ++i) {
        REQUIRE(manager.append(makeTestRecord(LogRecordType::PageUpdate, static_cast<PageId>(i), 8)).ok());
    }
    REQUIRE(manager.flush().ok());

    const auto stats = manager.statistics();
    CHECK(stats.recordCount == 25);
    CHECK(stats.bytesWritten > 0);
    CHECK(stats.flushCount >= 1);
    CHECK(stats.currentLsn.value() == 25);
    CHECK(stats.lastFlushedLsn.value() == 25);

    auto records = readAllRecords(manager);
    CHECK(records.size() == 25);
    for (std::size_t i = 0; i < records.size(); ++i) {
        CHECK(records[i].lsn().value() == i + 1);
    }

    CHECK(manager.validate().ok());
    REQUIRE(manager.shutdown().ok());
    CHECK_FALSE(manager.isInitialized());
    cleanupWal(path);
}

TEST_CASE("LogManager append before init fails", "[wal][manager]") {
    LogManager manager;
    CHECK_FALSE(manager.append(makeTestRecord(LogRecordType::Allocation, 1)).ok());
}

TEST_CASE("LogManager reopen existing log", "[wal][manager]") {
    auto path = walTestPath("reopen");
    cleanupWal(path);

    {
        LogManager manager;
        REQUIRE(manager.initialize(path).ok());
        REQUIRE(manager.append(makeTestRecord(LogRecordType::Allocation, 1)).ok());
        REQUIRE(manager.flush().ok());
        REQUIRE(manager.shutdown().ok());
    }

    {
        LogManager manager;
        REQUIRE(manager.initialize(path, false).ok());
        auto records = readAllRecords(manager);
        CHECK(records.size() == 1);
        CHECK(records[0].type() == LogRecordType::Allocation);
        REQUIRE(manager.shutdown().ok());
    }
    cleanupWal(path);
}

// ===== BTree WAL integration =====

TEST_CASE("BTree works without WAL sink", "[wal][btree]") {
    SpaceManager space;
    auto tree = BTree::create(space, defaultBTreeConfig());
    CHECK(tree.walSink() == nullptr);

    for (std::uint32_t i = 0; i < 100; ++i) {
        REQUIRE(tree.insert(Key::fromUInt32(i), makeDataRef(i + 1000)).ok());
    }
    CHECK(tree.size() == 100);
    CHECK(tree.validate().ok());
}

TEST_CASE("BTree with WAL adapter emits records", "[wal][btree]") {
    auto path = walTestPath("btree_wal");
    cleanupWal(path);

    LogManager manager;
    REQUIRE(manager.initialize(path).ok());
    BTreeWalAdapter adapter(manager);

    SpaceManager space;
    auto tree = BTree::create(space, defaultBTreeConfig());
    tree.setWalSink(&adapter);

  for (std::uint32_t i = 0; i < 80; ++i) {
        REQUIRE(tree.insert(Key::fromUInt32(i), makeDataRef(i + 5000)).ok());
    }
    REQUIRE(manager.flush().ok());

    auto records = readAllRecords(manager);
    CHECK(records.size() > 80);

    bool sawAllocation = false;
    bool sawPageUpdate = false;
    for (const auto& r : records) {
        if (r.type() == LogRecordType::Allocation) sawAllocation = true;
        if (r.type() == LogRecordType::PageUpdate) sawPageUpdate = true;
        CHECK(isValidRecordType(r.type()));
        CHECK(r.lsn().isValid());
    }
    CHECK(sawAllocation);
    CHECK(sawPageUpdate);
    CHECK(manager.validate().ok());

    REQUIRE(manager.shutdown().ok());
    cleanupWal(path);
}

TEST_CASE("BTree erase with WAL emits delete records", "[wal][btree]") {
    auto path = walTestPath("btree_erase");
    cleanupWal(path);

    LogManager manager;
    REQUIRE(manager.initialize(path).ok());
    BTreeWalAdapter adapter(manager);

    SpaceManager space;
    auto tree = BTree::create(space, defaultBTreeConfig());
    tree.setWalSink(&adapter);

    for (std::uint32_t i = 0; i < 30; ++i) {
        REQUIRE(tree.insert(Key::fromUInt32(i), makeDataRef(i)).ok());
    }
    REQUIRE(manager.flush().ok());
    const auto afterInsert = manager.statistics().recordCount;

    for (std::uint32_t i = 0; i < 10; ++i) {
        REQUIRE(tree.erase(Key::fromUInt32(i)).ok());
    }
    REQUIRE(manager.flush().ok());
    CHECK(manager.statistics().recordCount > afterInsert);

    auto records = readAllRecords(manager);
  std::size_t deletes = 0;
    for (const auto& r : records) {
        if (r.type() == LogRecordType::PageDelete) ++deletes;
    }
    CHECK(deletes >= 10);

    REQUIRE(manager.shutdown().ok());
    cleanupWal(path);
}

TEST_CASE("BTree split triggers NodeSplit WAL records", "[wal][btree]") {
    auto path = walTestPath("btree_split");
    cleanupWal(path);

    LogManager manager;
    REQUIRE(manager.initialize(path).ok());
    BTreeWalAdapter adapter(manager);

    SpaceManager space;
    auto tree = BTree::create(space, defaultBTreeConfig());
    tree.setWalSink(&adapter);

    for (std::uint32_t i = 0; i < 200; ++i) {
        REQUIRE(tree.insert(Key::fromUInt32(i), makeDataRef(i)).ok());
    }
    REQUIRE(manager.flush().ok());

    auto records = readAllRecords(manager);
    std::size_t splits = 0;
    for (const auto& r : records) {
        if (r.type() == LogRecordType::NodeSplit) ++splits;
    }
    CHECK(splits > 0);
    CHECK(tree.height() > 1);

    REQUIRE(manager.shutdown().ok());
    cleanupWal(path);
}

// ===== Stress =====

TEST_CASE("WAL stress append many records", "[wal][stress]") {
    auto path = walTestPath("stress");
    cleanupWal(path);

    LogManager manager;
    REQUIRE(manager.initialize(path).ok());

    constexpr int kCount = 500;
    for (int i = 0; i < kCount; ++i) {
        auto record = makeTestRecord(LogRecordType::PageUpdate, static_cast<PageId>(i % 100),
                                     static_cast<std::size_t>((i % 64) + 1));
        REQUIRE(manager.append(std::move(record)).ok());
        if (i % 50 == 49) {
            REQUIRE(manager.flush().ok());
        }
    }
    REQUIRE(manager.flush().ok());
    CHECK(manager.validate().ok());

    auto records = readAllRecords(manager);
    CHECK(records.size() == static_cast<std::size_t>(kCount));
    for (std::size_t i = 0; i < records.size(); ++i) {
        CHECK(records[i].lsn().value() == i + 1);
        CHECK(records[i].type() == LogRecordType::PageUpdate);
    }

    const auto stats = manager.statistics();
    CHECK(stats.recordCount == static_cast<std::uint64_t>(kCount));
    CHECK(stats.averageRecordSize > 0.0);
    CHECK(stats.largestRecord > 0);

    REQUIRE(manager.shutdown().ok());
    cleanupWal(path);
}

TEST_CASE("WAL statistics track read and write bytes", "[wal][stats]") {
    auto path = walTestPath("stats");
    cleanupWal(path);

    LogManager manager;
    REQUIRE(manager.initialize(path).ok());

    for (int i = 0; i < 10; ++i) {
        REQUIRE(manager.append(makeTestRecord(LogRecordType::CheckpointMarker, 0, 4)).ok());
    }
    REQUIRE(manager.flush().ok());

    const auto beforeRead = manager.statistics();
    CHECK(beforeRead.bytesWritten > 0);
    CHECK(beforeRead.bufferUsage == 0);

    (void)readAllRecords(manager);
    const auto afterRead = manager.statistics();
    CHECK(afterRead.bytesRead > 0);
    CHECK(afterRead.bytesRead >= beforeRead.bytesWritten);

    REQUIRE(manager.shutdown().ok());
    cleanupWal(path);
}

// ===== Corruption / invalid =====

TEST_CASE("LogReader fails on truncated file", "[wal][corruption]") {
    auto path = walTestPath("truncated");
    cleanupWal(path);

    LogFile file;
    REQUIRE(file.create(path).ok());
    LogStatisticsCollector stats;
    LogWriter writer(file, stats);
    REQUIRE(writer.appendRecord(makeTestRecord(LogRecordType::Allocation, 1)).ok());
    REQUIRE(writer.flush().ok());

    REQUIRE(file.truncate(WalFileHeader::kSize + 4).ok());

    LogReader reader(file, stats);
    REQUIRE(reader.rewind().ok());
    LogRecord record;
    CHECK_FALSE(reader.readNext(record).ok());

    REQUIRE(file.close().ok());
    cleanupWal(path);
}

TEST_CASE("LogValidator rejects invalid LSN in record", "[wal][corruption]") {
    auto record = LogRecord::make(LogRecordType::PageUpdate, 1);
    record.setLsn(LogSequenceNumber::invalid());
    CHECK_FALSE(LogValidator::validateRecord(record).ok());
}

TEST_CASE("LSN exhaustive pairwise ordering", "[wal][lsn]") {
    for (std::uint64_t a = 1; a <= 40; ++a) {
        for (std::uint64_t b = 1; b <= 40; ++b) {
            const LogSequenceNumber lsa(a);
            const LogSequenceNumber lsb(b);
            CHECK((lsa < lsb) == (a < b));
            CHECK((lsa == lsb) == (a == b));
            CHECK((lsa > lsb) == (a > b));
        }
    }
}

TEST_CASE("LogRecord payload size matrix round-trip", "[wal][record]") {
    const std::size_t sizes[] = {0, 1, 3, 7, 15, 31, 63, 127, 255, 511, 1023};
    const LogRecordType types[] = {LogRecordType::PageUpdate, LogRecordType::NodeSplit,
                                   LogRecordType::Allocation};

    for (const auto type : types) {
        for (const auto size : sizes) {
            auto original = makeTestRecord(type, static_cast<PageId>(size % 17), size);
            original.setLsn(LogSequenceNumber(static_cast<std::uint64_t>(size + 1)));

            Buffer buf;
            BinaryWriter writer(buf);
            REQUIRE(original.serialize(writer).ok());

            BinaryReader reader{BufferView(buf)};
            LogRecord restored;
            REQUIRE(restored.deserialize(reader).ok());
            CHECK(restored.type() == type);
            CHECK(restored.payloadLength() == size);
            CHECK(restored.lsn() == original.lsn());
        }
    }
}

TEST_CASE("WAL high-volume append validates monotonic LSNs", "[wal][stress]") {
    auto path = walTestPath("high_volume");
    cleanupWal(path);

    LogManager manager;
    REQUIRE(manager.initialize(path).ok());

    constexpr int kRecords = 400;
    for (int i = 0; i < kRecords; ++i) {
        auto record = makeTestRecord(LogRecordType::MetadataUpdate, static_cast<PageId>(i % 50),
                                     static_cast<std::size_t>(i % 32));
        REQUIRE(manager.append(std::move(record)).ok());
    }
    REQUIRE(manager.flush().ok());

    auto records = readAllRecords(manager);
    REQUIRE(records.size() == static_cast<std::size_t>(kRecords));
    for (std::size_t i = 0; i < records.size(); ++i) {
        CHECK(records[i].lsn().value() == i + 1);
        CHECK(records[i].timestamp() > 0);
        CHECK(isValidRecordType(records[i].type()));
    }
    CHECK(manager.validate().ok());

    REQUIRE(manager.shutdown().ok());
    cleanupWal(path);
}

TEST_CASE("LogBuffer incremental append tracks size", "[wal][buffer]") {
    LogBuffer buffer(4096);
    std::size_t total = 0;
    for (int i = 1; i <= 100; ++i) {
        const std::uint8_t byte = static_cast<std::uint8_t>(i);
        REQUIRE(buffer.append(BufferView(&byte, 1)).ok());
        total += 1;
        CHECK(buffer.size() == total);
        CHECK(buffer.remainingCapacity() == buffer.capacity() - total);
    }
    CHECK_FALSE(buffer.empty());
    buffer.clear();
    CHECK(buffer.empty());
    CHECK(buffer.size() == 0);
}

TEST_CASE("LogWriter buffer flushes on capacity pressure", "[wal][writer]") {
    auto path = walTestPath("buffer_flush");
    cleanupWal(path);

    LogFile file;
    REQUIRE(file.create(path).ok());
    LogStatisticsCollector stats;
    LogWriter writer(file, stats, 256);

    for (int i = 0; i < 30; ++i) {
        auto record = makeTestRecord(LogRecordType::PageUpdate, static_cast<PageId>(i), 32);
        REQUIRE(writer.appendRecord(std::move(record)).ok());
    }
    REQUIRE(writer.flush().ok());
    CHECK(writer.lastFlushedLsn().value() == 30);
    CHECK(file.fileSize() > WalFileHeader::kSize);

    REQUIRE(file.close().ok());
    cleanupWal(path);
}
