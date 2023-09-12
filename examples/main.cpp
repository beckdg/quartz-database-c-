#include "quartz/common/Version.h"
#include "quartz/common/Status.h"
#include "quartz/common/Logger.h"
#include "quartz/storage/Page.h"
#include "quartz/storage/PageAllocator.h"
#include "quartz/storage/DatabaseFile.h"
#include "quartz/storage/StorageConstants.h"
#include "quartz/serialization/Buffer.h"
#include "quartz/serialization/BufferView.h"
#include "quartz/serialization/BinaryWriter.h"
#include "quartz/serialization/BinaryReader.h"
#include "quartz/serialization/VariableLengthInteger.h"
#include "quartz/serialization/SerializationContext.h"
#include "quartz/serialization/SerializationTraits.h"
#include "quartz/serialization/Serializer.h"
#include "quartz/format/DatabaseHeader.h"
#include "quartz/format/Superblock.h"
#include "quartz/format/ObjectId.h"
#include "quartz/format/PageReference.h"
#include "quartz/format/FeatureFlags.h"
#include "quartz/format/Versioning.h"
#include "quartz/format/MagicNumbers.h"
#include "quartz/format/FormatValidator.h"
#include "quartz/format/MetadataDescriptor.h"
#include "quartz/format/SchemaDescriptor.h"
#include "quartz/pages/HeaderPage.h"
#include "quartz/pages/FreeListPage.h"
#include "quartz/pages/DataPage.h"
#include "quartz/pages/IndexPage.h"
#include "quartz/pages/OverflowPage.h"
#include "quartz/pages/MetadataPage.h"
#include "quartz/pages/PageFactory.h"
#include "quartz/pages/PageValidator.h"
#include "quartz/pages/PageStatistics.h"
#include "quartz/space/Extent.h"
#include "quartz/space/AllocationPolicy.h"
#include "quartz/space/FreeSpaceMap.h"
#include "quartz/space/ExtentAllocator.h"
#include "quartz/space/SpaceManager.h"
#include "quartz/space/FragmentationAnalyzer.h"
#include "quartz/btree/BTree.h"
#include "quartz/btree/LeafNode.h"
#include "quartz/btree/Cursor.h"
#include "quartz/btree/NodeValidator.h"
#include "quartz/btree/TreeValidator.h"
#include "quartz/btree/BTreeStatistics.h"
#include "quartz/wal/BTreeWalAdapter.h"
#include "quartz/wal/LogManager.h"
#include "quartz/wal/LogRecord.h"
#include "quartz/wal/LogTypes.h"

#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;
namespace ser = quartz::serialization;

#define QZ(S) static_cast<void>((S))

int main() {
    std::cout << "QuartzDB Example\n"
              << "================\n\n";

    // Print version
    std::cout << "Version: " << quartz::Version::string() << "\n";
    std::cout << "Build:   " << quartz::Version::buildInfo() << "\n\n";

    // Create and inspect a Status
    auto ok = quartz::Status::success();
    std::cout << "Status::success()->ok()  = " << (ok.ok() ? "true" : "false") << "\n";
    std::cout << "Status::success()->toString() = \"" << ok.toString() << "\"\n\n";

    auto err = quartz::Status::invalidArgument("example error");
    std::cout << "Status::invalidArgument()->ok()  = " << (err.ok() ? "true" : "false") << "\n";
    std::cout << "Status::invalidArgument()->toString() = \"" << err.toString() << "\"\n\n";

    // Demonstrate various error codes
    auto errors = {
        quartz::Status::ioError("disk write failed"),
        quartz::Status::corruption("page checksum mismatch"),
        quartz::Status::outOfMemory("buffer allocation failed"),
        quartz::Status::unknown("unexpected state")
    };

    for (const auto& s : errors) {
        std::cout << "  " << s.toString() << "\n";
    }
    std::cout << "\n";

    // Logger demonstration
    quartz::Logger log;
    log.setLevel(quartz::Logger::Level::Debug);

    log.info("QuartzDB example started");
    log.debug("This is a debug message");
    log.warning("This is a warning");
    log.error("This is an error");

    // Storage demo: Page
    std::cout << "\n--- Page Layer ---\n";
    quartz::storage::Page page(100, quartz::storage::PageType::Data);
    std::cout << "Page id  : " << page.id() << "\n";
    std::cout << "Page type: " << static_cast<int>(page.type()) << "\n";
    std::cout << "Page size: " << page.size() << " bytes\n";
    std::cout << "Header sz: " << quartz::config::kPageHeaderSize << " bytes\n";
    std::cout << "Payload sz: " << page.payloadSize() << " bytes\n\n";

    // Storage demo: PageAllocator
    std::cout << "--- Page Allocator ---\n";
    quartz::storage::PageAllocator alloc;
    auto id1 = alloc.allocate();
    auto id2 = alloc.allocate();
    auto id3 = alloc.allocate();
    std::cout << "Allocated pages: " << id1 << ", " << id2 << ", " << id3 << "\n";
    alloc.free(id2);
    auto recycled = alloc.allocate();
    std::cout << "Recycled page: " << recycled << " (expect " << id2 << ")\n";
    std::cout << "Allocator stats: " << alloc.stats().currentAllocated << " current, "
              << alloc.stats().totalAllocated << " total, "
              << alloc.stats().peakAllocated << " peak, "
              << alloc.stats().freeListSize << " free\n\n";

    // Storage demo: DatabaseFile
    std::cout << "--- Database File ---\n";
    auto tempPath = fs::temp_directory_path().string() + "/quartzdb_example.db";
    {
        quartz::storage::DatabaseFile db(tempPath, true);
        if (db.isOpen()) {
            std::cout << "Created database file: " << tempPath << "\n";

            // Write a page
            quartz::storage::Page writePage(0, quartz::storage::PageType::Data);
            const char* message = "Hello from QuartzDB!";
            std::memcpy(writePage.payload(), message, std::strlen(message) + 1);
            auto ws = db.writePage(writePage);
            std::cout << "Write status: " << (ws.ok() ? "OK" : ws.toString()) << "\n";
            std::cout << "File size: " << db.fileSize() << " bytes\n";
        }
    }

    // Reopen and read
    {
        quartz::storage::DatabaseFile db(tempPath, false);
        if (db.isOpen()) {
            quartz::storage::Page readPage(0, quartz::storage::PageType::Data);
            auto rs = db.readPage(readPage);
            if (rs.ok()) {
                std::cout << "Read page " << readPage.id() << " (type="
                          << static_cast<int>(readPage.type()) << ")\n";
                std::cout << "Payload: " << reinterpret_cast<const char*>(readPage.payload()) << "\n";
            } else {
                std::cout << "Read failed: " << rs.toString() << "\n";
            }
        }
    }

    // Cleanup temp db file
    std::error_code ec;
    fs::remove(fs::path(tempPath), ec);

    // Serialization demo: Buffer + BinaryWriter + BinaryReader
    std::cout << "\n--- Serialization Layer ---\n";
    {
        ser::Buffer buf;
        ser::BinaryWriter writer(buf);
        ser::SerializationContext sctx(1);

        // Write various data types
        QZ(writer.write<std::uint32_t>(0xDEADBEEF));
        QZ(writer.write<std::uint16_t>(0xCAFE));
        QZ(writer.write<std::uint8_t>(0x42));
        QZ(writer.writeVarU32(12345));
        QZ(writer.writeVarS32(-42));
        QZ(writer.writeLE<float>(3.1415f));

        // Write a string via SerializationTraits
        std::string greeting = "Hello from Serialization!";
        QZ(ser::serialize(writer, greeting, sctx));

        // Write a second string
        std::string response = "Round-trip works!";
        QZ(ser::serialize(writer, response, sctx));

        std::cout << "Wrote " << buf.size() << " bytes to buffer\n";

        // Read back
        ser::BufferView view(buf);
        ser::BinaryReader reader(view);

        std::uint32_t u32;
        QZ(reader.read(u32));
        std::uint16_t u16; QZ(reader.read(u16));
        std::uint8_t  u8;  QZ(reader.read(u8));
        std::uint32_t varU; QZ(reader.readVarU32(varU));
        std::int32_t  varS; QZ(reader.readVarS32(varS));
        float f;            QZ(reader.readLE(f));

        std::string g, r;
        QZ(ser::deserialize(reader, g, sctx));
        QZ(ser::deserialize(reader, r, sctx));

        std::cout << "Read back: u32=0x" << std::hex << u32 << std::dec
                  << ", u16=0x" << std::hex << u16 << std::dec
                  << ", u8=0x" << std::hex << static_cast<int>(u8) << std::dec << "\n";
        std::cout << "VarU32=" << varU << ", VarS32=" << varS << "\n";
        std::cout << "Float=" << f << "\n";
        std::cout << "String1=\"" << g << "\"\n";
        std::cout << "String2=\"" << r << "\"\n";

        // Demonstrate VarInt encoded size
        std::cout << "VarInt sizes: uint32(12345)=" << ser::VarInt::encodedSizeU32(12345)
                  << " bytes, int32(-42)=" << ser::VarInt::encodedSizeS32(-42) << " bytes\n";

        // Demonstrate Serializer header
        ser::Buffer hdrBuf;
        ser::BinaryWriter hdrWriter(hdrBuf);
        auto hdrSt = ser::Serializer::writeHeader(hdrWriter, sctx);
        std::cout << "Serializer header: " << hdrBuf.size() << " bytes ("
                  << (hdrSt.ok() ? "OK" : "FAIL") << ")\n";

        ser::BufferView hdrView(hdrBuf);
        ser::BinaryReader hdrReader(hdrView);
        ser::SerializationContext ctxOut;
        auto readSt = ser::Serializer::readHeader(hdrReader, ctxOut);
        std::cout << "Header read: " << (readSt.ok() ? "OK" : readSt.toString()) << "\n";
        if (readSt.ok()) {
            std::cout << "Header version: " << ctxOut.version() << "\n";
        }
    }

    // Format layer demo
    std::cout << "\n--- Format Layer ---\n";
    {
        // Create a database header
        auto dbHeader = quartz::format::DatabaseHeader::make();
        std::cout << "DatabaseHeader: magic=0x" << std::hex << dbHeader.magic << std::dec
                  << ", version=" << dbHeader.majorVersion << "." << dbHeader.minorVersion
                  << ", pageSize=" << dbHeader.pageSize << "\n";
        std::cout << "  DB ID: " << dbHeader.databaseId << "\n";

        // Validate it
        auto vSt = quartz::format::FormatValidator::validateHeader(dbHeader);
        std::cout << "  Validation: " << (vSt.ok() ? "OK" : vSt.toString()) << "\n";

        // Create a superblock
        auto sb = quartz::format::Superblock::make(dbHeader.databaseId);
        std::cout << "Superblock: totalPages=" << sb.totalPages
                  << ", reserved=" << sb.reservedPages
                  << ", freeListPage=" << sb.freeListPage << "\n";

        // Validate superblock
        auto sbSt = quartz::format::FormatValidator::validateSuperblock(sb);
        std::cout << "  Validation: " << (sbSt.ok() ? "OK" : sbSt.toString()) << "\n";

        // Generate ObjectIds
        auto oid1 = quartz::format::ObjectId::generate();
        auto oid2 = quartz::format::ObjectId::generate();
        std::cout << "ObjectId1: " << oid1 << "\n";
        std::cout << "ObjectId2: " << oid2 << "\n";
        std::cout << "  Unique: " << (oid1 != oid2 ? "yes" : "no") << "\n";

        // PageReference
        auto pageRef = quartz::format::PageReference::make(100, 1, 4);
        std::cout << "PageReference: pageId=" << pageRef.pageId
                  << ", generation=" << pageRef.generation
                  << ", type=" << static_cast<int>(pageRef.pageType) << "\n";

        // Feature flags
        quartz::format::FeatureFlags features(
            quartz::format::FeatureFlags::kChecksums |
            quartz::format::FeatureFlags::kJournaling);
        std::cout << "Feature flags: " << features.toString() << "\n";

        // Version info
        auto curVer = quartz::format::Versioning::current();
        std::cout << "Format version: " << quartz::format::Versioning::toString(curVer) << "\n";

        // Serialize and deserialize the header
        ser::Buffer hdrBuf;
        ser::BinaryWriter hdrWriter(hdrBuf);
        ser::SerializationContext sctx(1);
        auto serializeSt = ser::serialize(hdrWriter, dbHeader, sctx);
        std::cout << "Serialized header: " << hdrBuf.size() << " bytes ("
                  << (serializeSt.ok() ? "OK" : serializeSt.toString()) << ")\n";

        ser::BufferView hdrView(hdrBuf);
        ser::BinaryReader hdrReader(hdrView);
        quartz::format::DatabaseHeader restored;
        auto deserializeSt = ser::deserialize(hdrReader, restored, sctx);
        std::cout << "Deserialized: " << (deserializeSt.ok() ? "OK" : deserializeSt.toString())
                  << "\n";
        if (deserializeSt.ok()) {
            std::cout << "  Match: " << (restored.databaseId == dbHeader.databaseId ? "yes" : "no")
                      << "\n";
        }

        // Metadata descriptor
        auto md = quartz::format::MetadataDescriptor::make(
            quartz::format::MagicNumbers::kMetadataMagic, 1, 10, 4, 16384);
        std::cout << "MetadataDescriptor: page=" << md.startPage
                  << ", count=" << md.pageCount
                  << ", bytes=" << md.byteSize << "\n";

        // Schema descriptor
        auto sd = quartz::format::SchemaDescriptor::make(1, 50, 8);
        std::cout << "SchemaDescriptor: id=" << sd.schemaId
                  << ", rootPage=" << sd.rootPage
                  << ", fields=" << sd.fieldCount << "\n";
    }

    // Semantic Page Layer demo
    std::cout << "\n--- Semantic Page Layer ---\n";
    {
        using namespace quartz::pages;

        // Create various page types
        auto headerPage = HeaderPage::create(1, 0, 0);
        std::cout << "HeaderPage: id=" << headerPage.id()
                  << ", type=" << static_cast<int>(headerPage.rawType())
                  << ", version=0x" << std::hex << headerPage.formatVersion() << std::dec << "\n";

        auto freeList = FreeListPage::create(2);
        (void)freeList.addFreePage(100);
        (void)freeList.addFreePage(200);
        std::cout << "FreeListPage: id=" << freeList.id()
                  << ", freeCount=" << freeList.freeCount()
                  << ", capacity=" << freeList.capacity() << "\n";

        auto dataPage = DataPage::create(8);
        dataPage.setFreeSpaceOffset(64);
        std::cout << "DataPage: id=" << dataPage.id()
                  << ", freeSpaceOffset=" << dataPage.freeSpaceOffset()
                  << ", available=" << dataPage.availableSpace() << "\n";

        auto indexPage = IndexPage::create(10);
        indexPage.setNodeType(1);
        indexPage.setCapacity(50);
        std::cout << "IndexPage: id=" << indexPage.id()
                  << ", nodeType=" << indexPage.nodeType()
                  << ", capacity=" << indexPage.capacity() << "\n";

        auto overflowPage = OverflowPage::create(12);
        overflowPage.setNextPageId(13);
        overflowPage.setPayloadSize(1024);
        std::cout << "OverflowPage: id=" << overflowPage.id()
                  << ", nextPage=" << overflowPage.nextPageId()
                  << ", payloadSize=" << overflowPage.payloadSize()
                  << ", remaining=" << overflowPage.remainingCapacity() << "\n";

        auto metadataPage = MetadataPage::create(14);
        metadataPage.setVersion(1);
        metadataPage.setEntryCount(5);
        std::cout << "MetadataPage: id=" << metadataPage.id()
                  << ", version=" << metadataPage.version()
                  << ", entries=" << metadataPage.entryCount() << "\n";

        // Validate pages
        std::cout << "\nValidation:\n";
        std::cout << "  HeaderPage:   " << (PageValidator::validatePage(headerPage).ok() ? "OK" : "FAIL") << "\n";
        std::cout << "  FreeListPage: " << (PageValidator::validatePage(freeList).ok() ? "OK" : "FAIL") << "\n";
        std::cout << "  DataPage:     " << (PageValidator::validatePage(dataPage).ok() ? "OK" : "FAIL") << "\n";
        std::cout << "  IndexPage:    " << (PageValidator::validatePage(indexPage).ok() ? "OK" : "FAIL") << "\n";
        std::cout << "  OverflowPage: " << (PageValidator::validatePage(overflowPage).ok() ? "OK" : "FAIL") << "\n";
        std::cout << "  MetadataPage: " << (PageValidator::validatePage(metadataPage).ok() ? "OK" : "FAIL") << "\n";

        // Serialize and deserialize via PageFactory
        std::cout << "\nPageFactory serialization round-trip:\n";
        ser::Buffer pageBuf;
        ser::BinaryWriter pageWriter(pageBuf);

        auto st1 = headerPage.serialize(pageWriter);
        auto st2 = freeList.serialize(pageWriter);
        auto st3 = dataPage.serialize(pageWriter);
        std::cout << "  Serialize: "
                  << (st1.ok() && st2.ok() && st3.ok() ? "OK" : "FAIL") << "\n";

        ser::BinaryReader pageReader{ser::BufferView(pageBuf)};
        auto restoredHeader = PageFactory::deserialize(PageLayoutType::Header, pageReader);
        auto restoredFreeList = PageFactory::deserialize(PageLayoutType::FreeList, pageReader);
        auto restoredData = PageFactory::deserialize(PageLayoutType::Data, pageReader);

        if (restoredHeader) {
            std::cout << "  Restored HeaderPage: id=" << restoredHeader->id() << "\n";
        }
        if (restoredFreeList) {
            auto* fl = dynamic_cast<FreeListPage*>(restoredFreeList.get());
            std::cout << "  Restored FreeListPage: id=" << restoredFreeList->id()
                      << ", freeCount=" << (fl ? fl->freeCount() : 0) << "\n";
        }
        if (restoredData) {
            auto* dp = dynamic_cast<DataPage*>(restoredData.get());
            std::cout << "  Restored DataPage: id=" << restoredData->id()
                      << ", available=" << (dp ? dp->availableSpace() : 0) << "\n";
        }

        // Validate restored pages
        std::cout << "\nRestored page validation:\n";
        if (restoredHeader) {
            std::cout << "  HeaderPage: "
                      << (PageValidator::validatePage(*restoredHeader).ok() ? "OK" : "FAIL") << "\n";
        }
        if (restoredFreeList) {
            std::cout << "  FreeListPage: "
                      << (PageValidator::validatePage(*restoredFreeList).ok() ? "OK" : "FAIL") << "\n";
        }
        if (restoredData) {
            std::cout << "  DataPage: "
                      << (PageValidator::validatePage(*restoredData).ok() ? "OK" : "FAIL") << "\n";
        }

        // Page statistics
        std::cout << "\nPage Statistics:\n";
        std::cout << PageStatistics::toString(PageStatistics::compute(headerPage));
        std::cout << PageStatistics::toString(PageStatistics::compute(freeList));
        std::cout << PageStatistics::toString(PageStatistics::compute(dataPage));
        std::cout << PageStatistics::toString(PageStatistics::compute(overflowPage));
        std::cout << PageStatistics::toString(PageStatistics::compute(indexPage));
        std::cout << PageStatistics::toString(PageStatistics::compute(metadataPage));
    }

    // Space Management Layer demo
    std::cout << "\n--- Space Management Layer ---\n";
    {
        using namespace quartz::space;

        // Create SpaceManager with FirstFit policy
        SpaceManager sm;
        std::cout << "SpaceManager initialized:\n";
        std::cout << "  Free pages: " << sm.freePageCount()
                  << ", Allocated: " << sm.allocatedPageCount() << "\n";

        // Allocate single pages
        auto p1 = sm.allocatePage();
        auto p2 = sm.allocatePage();
        auto p3 = sm.allocatePage();
        std::cout << "Allocated pages: " << p1 << ", " << p2 << ", " << p3 << "\n";
        std::cout << "  Free pages: " << sm.freePageCount()
                  << ", Allocated: " << sm.allocatedPageCount() << "\n";

        // Free and reuse
        sm.freePage(p2);
        auto smRecycled = sm.allocatePage();
        std::cout << "Freed page " << p2 << ", recycled as " << smRecycled << "\n";

        // Allocate contiguous extent
        AllocationHints hints;
        hints.preferContiguous = true;
        auto ext = sm.allocateExtent(10, hints);
        std::cout << "Allocated extent: start=" << ext.start
                  << ", length=" << ext.length << "\n";
        std::cout << "  Free pages: " << sm.freePageCount()
                  << ", Allocated: " << sm.allocatedPageCount() << "\n";

        // Reserve a range
        auto st = sm.reserveRange(2000, 5);
        std::cout << "Reserved range [2000,2005): "
                  << (st.ok() ? "OK" : st.toString()) << "\n";

        // Free the extent
        st = sm.freeExtent(ext);
        std::cout << "Freed extent: " << (st.ok() ? "OK" : st.toString()) << "\n";

        // Fragmentation analysis
        auto report = FragmentationAnalyzer::analyze(
            sm.extentAllocator().freeSpaceMap(), sm.freePageCount() + sm.allocatedPageCount());
        std::cout << "\nFragmentation Report:\n";
        std::cout << "  Free pages: " << report.totalFreePages << "\n";
        std::cout << "  Extents: " << report.extentCount << "\n";
        std::cout << "  Largest extent: " << report.largestFreeExtent << "\n";
        std::cout << "  Fragmentation: " << report.fragmentationPercent << "%\n";
        std::cout << "  Recommendation: " << report.recommendation << "\n";

        // Statistics
        auto& stats = sm.statistics().stats();
        std::cout << "\nSpace Statistics:\n";
        std::cout << "  Allocations: " << stats.allocationsSucceeded << " success, "
                  << stats.allocationsFailed << " failed\n";
        std::cout << "  Frees: " << stats.freesSucceeded << " success, "
                  << stats.freesFailed << " failed\n";

        // Test with different policies
        std::cout << "\nAllocation policies:\n";
        auto seqSm = SpaceManager(std::make_unique<SequentialPolicy>());
        auto seq1 = seqSm.allocatePage();
        auto seq2 = seqSm.allocatePage();
        std::cout << "  Sequential: " << seq1 << ", " << seq2 << "\n";

        auto bfSm = SpaceManager(std::make_unique<BestFitPolicy>());
        auto bfExt = bfSm.allocateExtent(3, hints);
        std::cout << "  BestFit extent: start=" << bfExt.start
                  << ", length=" << bfExt.length << "\n";

        // Extent operations
        quartz::space::Extent e1{100, 10};
        quartz::space::Extent e2{110, 5};
        std::cout << "\nExtent operations:\n";
        std::cout << "  E1 [" << e1.start << "," << e1.endPage() << ") len=" << e1.length << "\n";
        std::cout << "  E2 [" << e2.start << "," << e2.endPage() << ") len=" << e2.length << "\n";
        std::cout << "  Adjacent: " << (e1.adjacentTo(e2) ? "yes" : "no") << "\n";
        auto merged = Extent::merge(e1, e2);
        if (merged) {
            std::cout << "  Merged: [" << merged->start << "," << merged->endPage()
                      << ") len=" << merged->length << "\n";
        }
        std::cout << "  Contains(105): " << (e1.contains(105) ? "yes" : "no") << "\n";
    }

    // B-Tree Node Engine demo
    std::cout << "\n--- B-Tree Node Engine ---\n";
    {
        using namespace quartz::btree;

        BTreeNodeConfig config;
        config.keyType = KeyType::UInt32;
        config.level = 0;

        auto leaf = LeafNode::create(200, config);
        std::cout << "Created LeafNode: page=" << leaf.pageId()
                  << ", capacity=" << leaf.capacity() << "\n";

        auto ref = quartz::format::PageReference::make(300, 1,
            static_cast<std::uint8_t>(quartz::storage::PageType::Data));
        (void)leaf.insert(Key::fromUInt32(30), ref);
        (void)leaf.insert(Key::fromUInt32(10), quartz::format::PageReference::make(100, 1, 0));
        (void)leaf.insert(Key::fromUInt32(20), quartz::format::PageReference::make(200, 1, 0));

        std::cout << "Inserted 3 keys, keyCount=" << leaf.keyCount() << "\n";
        std::cout << "  contains(20): " << (leaf.contains(Key::fromUInt32(20)) ? "yes" : "no") << "\n";
        std::cout << "  lowerBound(25): " << leaf.lowerBound(Key::fromUInt32(25)) << "\n";

        // Serialize and deserialize
        ser::Buffer nodeBuf;
        ser::BinaryWriter nodeWriter(nodeBuf);
        auto serSt = leaf.serialize(nodeWriter);
        std::cout << "Serialize: " << (serSt.ok() ? "OK" : serSt.toString()) << "\n";

        ser::BinaryReader nodeReader{ser::BufferView(nodeBuf)};
        quartz::pages::IndexPage restoredPage;
        (void)restoredPage.deserialize(nodeReader);
        auto restored = LeafNode::fromPage(std::move(restoredPage));
        std::cout << "Deserialize: keyCount=" << restored.keyCount()
                  << ", validate=" << (NodeValidator::validateLeaf(restored).ok() ? "OK" : "FAIL") << "\n";

        // Cursor traversal
        Cursor cursor;
        cursor.bindLeaf(&restored);
        std::cout << "Cursor traversal:\n";
        for (std::size_t i = 0; i < restored.keyCount(); ++i) {
            (void)cursor.seek(i);
            std::cout << "  [" << i << "] key=" << cursor.currentKey().toString()
                      << " ref.pageId=" << cursor.currentReference().pageId << "\n";
        }

        // Statistics
        auto stats = computeStatistics(restored);
        std::cout << "Node statistics:\n";
        std::cout << "  keys=" << stats.keyCount << "/" << stats.capacity
                  << ", occupancy=" << stats.occupancyPercent << "%\n";
        std::cout << "  freeSlots=" << stats.freeSlots
                  << ", utilization=" << stats.estimatedUtilization << "%\n";
    }

    // B-Tree Algorithms demo
    std::cout << "\n--- B-Tree Algorithms ---\n";
    {
        using namespace quartz::btree;
        using namespace quartz::space;

        SpaceManager space;
        BTreeNodeConfig config;
        config.keyType = KeyType::UInt32;
        auto tree = BTree::create(space, config);

        std::cout << "Inserting 300 keys...\n";
        for (std::uint32_t i = 0; i < 300; ++i) {
            auto ref = quartz::format::PageReference::make(
                1000 + i, 1, static_cast<std::uint8_t>(quartz::storage::PageType::Data));
            (void)tree.insert(Key::fromUInt32(i), ref);
        }
        std::cout << "  size=" << tree.size() << ", height=" << tree.height() << "\n";
        std::cout << "  contains(150): " << (tree.contains(Key::fromUInt32(150)) ? "yes" : "no") << "\n";

        quartz::format::PageReference found;
        if (tree.find(Key::fromUInt32(42), found).ok()) {
            std::cout << "  find(42): pageId=" << found.pageId << "\n";
        }

        std::cout << "Deleting 50 keys...\n";
        for (std::uint32_t i = 0; i < 50; ++i) {
            (void)tree.erase(Key::fromUInt32(i * 2));
        }
        std::cout << "  size after delete=" << tree.size() << "\n";

        std::cout << "Cursor traversal (first 5 keys):\n";
        auto cursor = tree.begin();
        for (int n = 0; n < 5 && cursor.valid(); ++n) {
            std::cout << "  " << cursor.currentKey().toString() << "\n";
            (void)cursor.next();
        }

        std::cout << "Validate: " << (TreeValidator::validate(tree).ok() ? "OK" : "FAIL") << "\n";

        auto tstats = tree.statistics();
        std::cout << "Tree statistics:\n";
        std::cout << "  nodes=" << tstats.nodeCount << " (leaves=" << tstats.leafCount
                  << ", internal=" << tstats.internalCount << ")\n";
        std::cout << "  avg occupancy=" << tstats.averageOccupancy << "%\n";
        std::cout << "  splits=" << tstats.operations.splitCount
                  << ", merges=" << tstats.operations.mergeCount
                  << ", rotations=" << tstats.operations.rotationCount << "\n";
    }

    // Write-Ahead Log demo
    std::cout << "\n--- Write-Ahead Log ---\n";
    {
        using namespace quartz::wal;

        const auto walPath = (fs::temp_directory_path() / "quartzdb_example.wal").string();
        fs::remove(walPath, ec);

        LogManager logManager;
        auto st = logManager.initialize(walPath);
        std::cout << "LogManager initialize: " << (st.ok() ? "OK" : st.toString()) << "\n";

        for (int i = 0; i < 5; ++i) {
            auto record = LogRecord::make(LogRecordType::PageUpdate, static_cast<quartz::storage::PageId>(i + 1));
            (void)logManager.append(std::move(record));
        }
        (void)logManager.flush();

        (void)logManager.rewindReader();
        LogRecord readRecord;
        std::cout << "Log records:\n";
        while (logManager.readNext(readRecord).ok()) {
            std::cout << "  " << readRecord.toString() << "\n";
        }

        auto logStats = logManager.statistics();
        std::cout << "WAL statistics:\n";
        std::cout << "  records=" << logStats.recordCount
                  << ", bytesWritten=" << logStats.bytesWritten
                  << ", flushes=" << logStats.flushCount
                  << ", currentLsn=" << logStats.currentLsn.toString() << "\n";

        // B-tree with WAL enabled
        std::cout << "\nB-tree with WAL:\n";
        quartz::space::SpaceManager walSpace;
        quartz::btree::BTreeNodeConfig walConfig;
        walConfig.keyType = quartz::btree::KeyType::UInt32;
        auto walTree = quartz::btree::BTree::create(walSpace, walConfig);

        BTreeWalAdapter walAdapter(logManager);
        walTree.setWalSink(&walAdapter);

        for (std::uint32_t i = 0; i < 50; ++i) {
            auto ref = quartz::format::PageReference::make(
                2000 + i, 1, static_cast<std::uint8_t>(quartz::storage::PageType::Data));
            (void)walTree.insert(quartz::btree::Key::fromUInt32(i), ref);
        }
        (void)logManager.flush();

        (void)logManager.rewindReader();
        std::size_t walRecordCount = 0;
        while (logManager.readNext(readRecord).ok()) {
            ++walRecordCount;
        }
        std::cout << "  btree inserts produced " << walRecordCount << " total WAL records\n";
        std::cout << "  btree size=" << walTree.size() << ", validate="
                  << (quartz::btree::TreeValidator::validate(walTree).ok() ? "OK" : "FAIL") << "\n";

        (void)logManager.shutdown();
        fs::remove(walPath, ec);
    }

    std::cout << "\nExample completed successfully.\n";
    return 0;
}
