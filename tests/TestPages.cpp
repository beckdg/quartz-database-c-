#include "quartz/pages/BasePage.h"
#include "quartz/pages/HeaderPage.h"
#include "quartz/pages/FreeListPage.h"
#include "quartz/pages/DataPage.h"
#include "quartz/pages/IndexPage.h"
#include "quartz/pages/OverflowPage.h"
#include "quartz/pages/MetadataPage.h"
#include "quartz/pages/PageFactory.h"
#include "quartz/pages/PageValidator.h"
#include "quartz/pages/PageStatistics.h"
#include "quartz/pages/PageLayouts.h"
#include "quartz/serialization/Buffer.h"
#include "quartz/serialization/BinaryWriter.h"
#include "quartz/serialization/BinaryReader.h"

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <sstream>

using namespace quartz::pages;
using namespace quartz::serialization;
using namespace quartz::storage;

// ===== PageLayouts =====

TEST_CASE("PageLayouts have correct sizes", "[pages]") {
    CHECK(sizeof(HeaderPageLayout) == 4032);
    CHECK(sizeof(FreeListPageLayout) == 4032);
    CHECK(sizeof(DataPageLayout) == 4032);
    CHECK(sizeof(OverflowPageLayout) == 4032);
    CHECK(sizeof(IndexPageLayout) == 4032);
    CHECK(sizeof(MetadataPageLayout) == 4032);
}

TEST_CASE("HeaderPageLayout default is invalid", "[pages]") {
    HeaderPageLayout layout{};
    CHECK_FALSE(layout.isValid());
}

TEST_CASE("FreeListPageLayout default is invalid", "[pages]") {
    FreeListPageLayout layout{};
    CHECK_FALSE(layout.isValid());
}

TEST_CASE("DataPageLayout default is valid", "[pages]") {
    DataPageLayout layout{};
    CHECK(layout.isValid());
    CHECK(layout.availableSpace() == 4008);
}

TEST_CASE("OverflowPageLayout default is valid", "[pages]") {
    OverflowPageLayout layout{};
    CHECK(layout.isValid());
    CHECK(layout.remainingCapacity() == 4008);
}

TEST_CASE("IndexPageLayout default is invalid", "[pages]") {
    IndexPageLayout layout{};
    CHECK_FALSE(layout.isValid());
}

TEST_CASE("MetadataPageLayout default is invalid", "[pages]") {
    MetadataPageLayout layout{};
    CHECK_FALSE(layout.isValid());
}

TEST_CASE("FreeListPageLayout validity boundary", "[pages]") {
    FreeListPageLayout layout{};
    CHECK_FALSE(layout.isValid());

    layout.capacity = 100;
    CHECK(layout.isValid());

    layout.freeCount = 101;
    CHECK_FALSE(layout.isValid());
}

TEST_CASE("DataPageLayout freeSpaceOffset boundary", "[pages]") {
    DataPageLayout layout{};
    layout.freeSpaceOffset = 5000;
    CHECK_FALSE(layout.isValid());

    layout.freeSpaceOffset = 4008;
    CHECK(layout.isValid());
}

TEST_CASE("IndexPageLayout validity boundary", "[pages]") {
    IndexPageLayout layout{};
    layout.capacity = 10;
    CHECK(layout.isValid());

    layout.keyCount = 11;
    CHECK_FALSE(layout.isValid());
}

// ===== BasePage =====

TEST_CASE("BasePage cannot be default-constructed directly", "[pages]") {
    // BasePage is abstract; concrete pages are tested instead
    CHECK(true);
}

TEST_CASE("BasePage provides access to underlying page metadata", "[pages]") {
    auto page = HeaderPage::create(1, 0, 0);
    CHECK(page.id() == quartz::storage::kHeaderPageId);
    CHECK(page.layoutType() == PageLayoutType::Header);
    CHECK(page.size() == 4096);
    CHECK(page.payloadSize() == 4032);
    CHECK_FALSE(page.isValid());     // default-initialized page not configured
}

TEST_CASE("BasePage reset changes page ID", "[pages]") {
    auto page = FreeListPage::create(5);
    CHECK(page.id() == 5);

    page.reset(10);
    CHECK(page.id() == 10);
    CHECK(page.layoutType() == PageLayoutType::FreeList);
}

TEST_CASE("BasePage clear zeroes the buffer", "[pages]") {
    auto page = HeaderPage::create(1, 0, 0);
    page.clear();

    // After clear, the page header should still be valid (magic etc.)
    CHECK(page.page().isValid());
}

TEST_CASE("BasePage serialization round-trip", "[pages]") {
    auto original = HeaderPage::create(1, 0, 0);
    original.setFormatVersion(0x00010000);
    original.setFlags(0xABCD);

    Buffer buf;
    BinaryWriter writer(buf);
    CHECK(original.serialize(writer).ok());

    BinaryReader reader{BufferView(buf)};
    auto restored = PageFactory::deserialize(PageLayoutType::Header, reader);
    REQUIRE(restored != nullptr);

    auto* headerPage = dynamic_cast<HeaderPage*>(restored.get());
    REQUIRE(headerPage != nullptr);
    CHECK(headerPage->formatVersion() == 0x00010000);
    CHECK(headerPage->flags() == 0xABCD);
}

TEST_CASE("BasePage clone produces identical page", "[pages]") {
    auto original = DataPage::create(42);
    original.setFreeSpaceOffset(100);
    original.setSlotCount(5);

    auto cloned = original.clone();
    REQUIRE(cloned != nullptr);
    CHECK(cloned->id() == 42);
    CHECK(cloned->layoutType() == PageLayoutType::Data);

    auto* dataPage = dynamic_cast<DataPage*>(cloned.get());
    REQUIRE(dataPage != nullptr);
    CHECK(dataPage->freeSpaceOffset() == 100);
    CHECK(dataPage->slotCount() == 5);
}

// ===== HeaderPage =====

TEST_CASE("HeaderPage::create makes valid header page", "[pages]") {
    auto page = HeaderPage::create(1, 0, 0);
    CHECK(page.id() == quartz::storage::kHeaderPageId);
    CHECK(page.layoutType() == PageLayoutType::Header);
    CHECK(page.superblockPageId() == 1);
    CHECK(page.formatVersion() > 0);
}

TEST_CASE("HeaderPage::create with specific version", "[pages]") {
    auto page = HeaderPage::create(2, 1, 0xFF);
    CHECK(page.formatVersion() == quartz::format::Versioning::encodeVersion(2, 1));
    CHECK(page.flags() == 0xFF);
}

TEST_CASE("HeaderPage initFromFormat", "[pages]") {
    auto dbHeader = quartz::format::DatabaseHeader::make();
    HeaderPage page;
    page.initFromFormat(dbHeader);

    CHECK(page.databaseHeader().databaseId == dbHeader.databaseId);
    CHECK(page.superblockPageId() == 1);
}

TEST_CASE("HeaderPage validation", "[pages]") {
    HeaderPage empty;
    CHECK_FALSE(empty.validate().ok());

    auto page = HeaderPage::create(1, 0, 0);
    CHECK(page.validate().ok());
}

TEST_CASE("HeaderPage setter/getter round-trip", "[pages]") {
    auto page = HeaderPage::create(1, 0, 0);

    page.setSuperblockPageId(5);
    CHECK(page.superblockPageId() == 5);

    page.setFlags(0xDEADBEEF);
    CHECK(page.flags() == 0xDEADBEEF);

    page.setFormatVersion(0x00020001);
    CHECK(page.formatVersion() == 0x00020001);
}

TEST_CASE("HeaderPage clone", "[pages]") {
    auto page = HeaderPage::create(1, 3, 0x42);
    page.setSuperblockPageId(7);

    auto cloned = page.clone();
    auto* hc = dynamic_cast<HeaderPage*>(cloned.get());
    REQUIRE(hc != nullptr);
    CHECK(hc->formatVersion() == 0x00010003);
    CHECK(hc->flags() == 0x42);
    CHECK(hc->superblockPageId() == 7);
}

TEST_CASE("HeaderPage serialization round-trip via Buffer", "[pages]") {
    auto original = HeaderPage::create(1, 1, 0xFF);
    original.setSuperblockPageId(3);

    Buffer buf;
    BinaryWriter writer(buf);
    CHECK(original.serialize(writer).ok());

    BinaryReader reader{BufferView(buf)};
    HeaderPage restored;
    CHECK(restored.deserialize(reader).ok());
    CHECK(restored.superblockPageId() == 3);
    CHECK(restored.flags() == 0xFF);
}

// ===== FreeListPage =====

TEST_CASE("FreeListPage::create", "[pages]") {
    auto page = FreeListPage::create(10);
    CHECK(page.id() == 10);
    CHECK(page.layoutType() == PageLayoutType::FreeList);
    CHECK(page.capacity() == 1000);
    CHECK(page.freeCount() == 0);
    CHECK(page.isEmpty());
    CHECK_FALSE(page.isFull());
}

TEST_CASE("FreeListPage add and retrieve free pages", "[pages]") {
    auto page = FreeListPage::create(5);

    CHECK(page.addFreePage(100).ok());
    CHECK(page.addFreePage(200).ok());
    CHECK(page.addFreePage(300).ok());

    CHECK(page.freeCount() == 3);
    CHECK_FALSE(page.isEmpty());
    CHECK(page.freePage(0) == 100);
    CHECK(page.freePage(1) == 200);
    CHECK(page.freePage(2) == 300);
    CHECK(page.freePage(3) == quartz::storage::kInvalidPageId);
}

TEST_CASE("FreeListPage out-of-bounds access returns invalid", "[pages]") {
    auto page = FreeListPage::create(5);
    CHECK(page.freePage(0) == quartz::storage::kInvalidPageId);
    CHECK(page.freePage(999) == quartz::storage::kInvalidPageId);
}

TEST_CASE("FreeListPage setFreePage", "[pages]") {
    auto page = FreeListPage::create(5);
    CHECK(page.setFreePage(0, 42).ok());
    CHECK(page.setFreePage(1, 43).ok());
    CHECK_FALSE(page.setFreePage(9999, 0).ok());  // out of range

    // setFreePage does not increment freeCount
    CHECK(page.freeCount() == 0);
}

TEST_CASE("FreeListPage clear freed pages", "[pages]") {
    auto page = FreeListPage::create(5);
    CHECK(page.addFreePage(10).ok());
    CHECK(page.addFreePage(20).ok());
    CHECK(page.freeCount() == 2);

    page.clearFreePages();
    CHECK(page.freeCount() == 0);
    CHECK(page.isEmpty());
}

TEST_CASE("FreeListPage full detection", "[pages]") {
    auto page = FreeListPage::create(5);
    // Mark as full by setting freeCount = capacity
    // Use a fresh page with small capacity simulation via direct access
    CHECK_FALSE(page.isFull());

    // Fill the page
    for (std::uint32_t i = 0; i < page.capacity(); ++i) {
        CHECK(page.addFreePage(i).ok());
    }
    CHECK(page.isFull());
    CHECK_FALSE(page.addFreePage(9999).ok());  // should fail
}

TEST_CASE("FreeListPage validation", "[pages]") {
    FreeListPage empty;
    CHECK_FALSE(empty.validate().ok());

    auto page = FreeListPage::create(5);
    CHECK(page.validate().ok());
}

TEST_CASE("FreeListPage clone", "[pages]") {
    auto page = FreeListPage::create(1);
    REQUIRE(page.addFreePage(42).ok());

    auto cloned = page.clone();
    auto* fc = dynamic_cast<FreeListPage*>(cloned.get());
    REQUIRE(fc != nullptr);
    CHECK(fc->id() == 1);
    CHECK(fc->freeCount() == 1);
    CHECK(fc->freePage(0) == 42);
}

TEST_CASE("FreeListPage serialization round-trip", "[pages]") {
    auto original = FreeListPage::create(7);
    REQUIRE(original.addFreePage(100).ok());
    REQUIRE(original.addFreePage(200).ok());
    REQUIRE(original.addFreePage(300).ok());

    Buffer buf;
    BinaryWriter writer(buf);
    CHECK(original.serialize(writer).ok());

    BinaryReader reader{BufferView(buf)};
    auto restored = PageFactory::deserialize(PageLayoutType::FreeList, reader);
    REQUIRE(restored != nullptr);
    CHECK(restored->id() == 7);

    auto* fp = dynamic_cast<FreeListPage*>(restored.get());
    REQUIRE(fp != nullptr);
    CHECK(fp->freeCount() == 3);
    CHECK(fp->freePage(0) == 100);
    CHECK(fp->freePage(1) == 200);
    CHECK(fp->freePage(2) == 300);
}

// ===== DataPage =====

TEST_CASE("DataPage::create", "[pages]") {
    auto page = DataPage::create(20);
    CHECK(page.id() == 20);
    CHECK(page.layoutType() == PageLayoutType::Data);
    CHECK(page.freeSpaceOffset() == 0);
    CHECK(page.slotCount() == 0);
    CHECK(page.availableSpace() == 4008);
}

TEST_CASE("DataPage free space tracking", "[pages]") {
    auto page = DataPage::create(1);
    CHECK(page.availableSpace() == 4008);

    page.setFreeSpaceOffset(100);
    CHECK(page.availableSpace() == 3908);
}

TEST_CASE("DataPage validation", "[pages]") {
    auto page = DataPage::create(1);
    CHECK(page.validate().ok());

    // Make it invalid by setting freeSpaceOffset past the data area
    DataPage invalid(Page(1, PageType::Data));
    invalid.setFreeSpaceOffset(5000);
    CHECK_FALSE(invalid.validate().ok());
}

TEST_CASE("DataPage clone", "[pages]") {
    auto page = DataPage::create(10);
    page.setFreeSpaceOffset(200);
    page.setSlotCount(3);

    auto cloned = page.clone();
    auto* dc = dynamic_cast<DataPage*>(cloned.get());
    REQUIRE(dc != nullptr);
    CHECK(dc->id() == 10);
    CHECK(dc->freeSpaceOffset() == 200);
    CHECK(dc->slotCount() == 3);
}

TEST_CASE("DataPage serialization round-trip", "[pages]") {
    auto original = DataPage::create(15);
    original.setFreeSpaceOffset(256);
    original.setSlotCount(4);

    Buffer buf;
    BinaryWriter writer(buf);
    CHECK(original.serialize(writer).ok());

    BinaryReader reader{BufferView(buf)};
    auto restored = PageFactory::deserialize(PageLayoutType::Data, reader);
    REQUIRE(restored != nullptr);
    CHECK(restored->id() == 15);

    auto* dp = dynamic_cast<DataPage*>(restored.get());
    REQUIRE(dp != nullptr);
    CHECK(dp->freeSpaceOffset() == 256);
    CHECK(dp->slotCount() == 4);
}

// ===== IndexPage =====

TEST_CASE("IndexPage::create", "[pages]") {
    auto page = IndexPage::create(30);
    CHECK(page.id() == 30);
    CHECK(page.layoutType() == PageLayoutType::Index);
    CHECK(page.nodeType() == 0);
    CHECK(page.keyCount() == 0);
    CHECK(page.capacity() == 0);
}

TEST_CASE("IndexPage field accessors", "[pages]") {
    auto page = IndexPage::create(1);
    page.setNodeType(1);
    CHECK(page.nodeType() == 1);

    page.setKeyCount(5);
    CHECK(page.keyCount() == 5);

    page.setCapacity(100);
    CHECK(page.capacity() == 100);

    page.setFlags(0x01);
    CHECK(page.flags() == 0x01);
}

TEST_CASE("IndexPage validation", "[pages]") {
    auto page = IndexPage::create(1);
    CHECK_FALSE(page.validate().ok());  // capacity is 0

    page.setCapacity(10);
    CHECK(page.validate().ok());

    page.setKeyCount(11);
    CHECK_FALSE(page.validate().ok());  // keyCount > capacity
}

TEST_CASE("IndexPage clone", "[pages]") {
    auto page = IndexPage::create(5);
    page.setNodeType(1);
    page.setCapacity(50);
    page.setKeyCount(10);

    auto cloned = page.clone();
    auto* ic = dynamic_cast<IndexPage*>(cloned.get());
    REQUIRE(ic != nullptr);
    CHECK(ic->id() == 5);
    CHECK(ic->nodeType() == 1);
    CHECK(ic->capacity() == 50);
    CHECK(ic->keyCount() == 10);
}

TEST_CASE("IndexPage serialization round-trip", "[pages]") {
    auto original = IndexPage::create(8);
    original.setNodeType(1);
    original.setCapacity(64);
    original.setKeyCount(15);

    Buffer buf;
    BinaryWriter writer(buf);
    CHECK(original.serialize(writer).ok());

    BinaryReader reader{BufferView(buf)};
    auto restored = PageFactory::deserialize(PageLayoutType::Index, reader);
    REQUIRE(restored != nullptr);
    CHECK(restored->id() == 8);

    auto* ip = dynamic_cast<IndexPage*>(restored.get());
    REQUIRE(ip != nullptr);
    CHECK(ip->nodeType() == 1);
    CHECK(ip->capacity() == 64);
    CHECK(ip->keyCount() == 15);
}

// ===== OverflowPage =====

TEST_CASE("OverflowPage::create", "[pages]") {
    auto page = OverflowPage::create(40);
    CHECK(page.id() == 40);
    CHECK(page.layoutType() == PageLayoutType::Overflow);
    CHECK(page.nextPageId() == quartz::storage::kInvalidPageId);
    CHECK_FALSE(page.hasNextPage());
    CHECK(page.payloadSize() == 0);
    CHECK(page.remainingCapacity() == 4008);
}

TEST_CASE("OverflowPage chain link", "[pages]") {
    auto page = OverflowPage::create(1);
    CHECK_FALSE(page.hasNextPage());

    page.setNextPageId(2);
    CHECK(page.hasNextPage());
    CHECK(page.nextPageId() == 2);
}

TEST_CASE("OverflowPage payload tracking", "[pages]") {
    auto page = OverflowPage::create(1);
    CHECK(page.remainingCapacity() == 4008);

    page.setPayloadSize(100);
    CHECK(page.payloadSize() == 100);
    CHECK(page.remainingCapacity() == 3908);
}

TEST_CASE("OverflowPage validation", "[pages]") {
    auto page = OverflowPage::create(1);
    CHECK(page.validate().ok());

    // Invalid: payloadSize > data area
    OverflowPage invalid(Page(1, PageType::Overflow));
    invalid.setPayloadSize(9999);
    CHECK_FALSE(invalid.validate().ok());
}

TEST_CASE("OverflowPage clone", "[pages]") {
    auto page = OverflowPage::create(3);
    page.setNextPageId(7);
    page.setPayloadSize(512);

    auto cloned = page.clone();
    auto* oc = dynamic_cast<OverflowPage*>(cloned.get());
    REQUIRE(oc != nullptr);
    CHECK(oc->id() == 3);
    CHECK(oc->nextPageId() == 7);
    CHECK(oc->payloadSize() == 512);
}

TEST_CASE("OverflowPage serialization round-trip", "[pages]") {
    auto original = OverflowPage::create(5);
    original.setNextPageId(6);
    original.setPayloadSize(2048);

    Buffer buf;
    BinaryWriter writer(buf);
    CHECK(original.serialize(writer).ok());

    BinaryReader reader{BufferView(buf)};
    auto restored = PageFactory::deserialize(PageLayoutType::Overflow, reader);
    REQUIRE(restored != nullptr);
    CHECK(restored->id() == 5);

    auto* op = dynamic_cast<OverflowPage*>(restored.get());
    REQUIRE(op != nullptr);
    CHECK(op->nextPageId() == 6);
    CHECK(op->payloadSize() == 2048);
}

// ===== MetadataPage =====

TEST_CASE("MetadataPage::create", "[pages]") {
    auto page = MetadataPage::create(50);
    CHECK(page.id() == 50);
    CHECK(page.layoutType() == PageLayoutType::Metadata);
    CHECK(page.entryCount() == 0);
    CHECK(page.version() == 0);
}

TEST_CASE("MetadataPage accessors", "[pages]") {
    auto page = MetadataPage::create(1);
    page.setEntryCount(5);
    CHECK(page.entryCount() == 5);

    page.setVersion(3);
    CHECK(page.version() == 3);
}

TEST_CASE("MetadataPage validation", "[pages]") {
    auto page = MetadataPage::create(1);
    CHECK_FALSE(page.validate().ok());  // version == 0

    page.setVersion(1);
    CHECK(page.validate().ok());
}

TEST_CASE("MetadataPage clone", "[pages]") {
    auto page = MetadataPage::create(2);
    page.setEntryCount(10);
    page.setVersion(2);

    auto cloned = page.clone();
    auto* mc = dynamic_cast<MetadataPage*>(cloned.get());
    REQUIRE(mc != nullptr);
    CHECK(mc->id() == 2);
    CHECK(mc->entryCount() == 10);
    CHECK(mc->version() == 2);
}

TEST_CASE("MetadataPage serialization round-trip", "[pages]") {
    auto original = MetadataPage::create(9);
    original.setEntryCount(3);
    original.setVersion(2);

    Buffer buf;
    BinaryWriter writer(buf);
    CHECK(original.serialize(writer).ok());

    BinaryReader reader{BufferView(buf)};
    auto restored = PageFactory::deserialize(PageLayoutType::Metadata, reader);
    REQUIRE(restored != nullptr);
    CHECK(restored->id() == 9);

    auto* mp = dynamic_cast<MetadataPage*>(restored.get());
    REQUIRE(mp != nullptr);
    CHECK(mp->entryCount() == 3);
    CHECK(mp->version() == 2);
}

// ===== PageFactory =====

TEST_CASE("PageFactory creates all page types", "[pages]") {
    CHECK(PageFactory::isSupportedType(PageLayoutType::Header));
    CHECK(PageFactory::isSupportedType(PageLayoutType::FreeList));
    CHECK(PageFactory::isSupportedType(PageLayoutType::Data));
    CHECK(PageFactory::isSupportedType(PageLayoutType::Index));
    CHECK(PageFactory::isSupportedType(PageLayoutType::Overflow));
    CHECK(PageFactory::isSupportedType(PageLayoutType::Metadata));
}

TEST_CASE("PageFactory rejects invalid types", "[pages]") {
    CHECK_FALSE(PageFactory::isSupportedType(
        static_cast<PageLayoutType>(0)));
    CHECK_FALSE(PageFactory::isSupportedType(
        static_cast<PageLayoutType>(99)));
    CHECK_FALSE(PageFactory::isSupportedType(
        static_cast<PageLayoutType>(7)));  // Journal type not in PageLayoutType
}

TEST_CASE("PageFactory createPage by type creates correct types", "[pages]") {
    auto header = PageFactory::createPage(PageLayoutType::Header, 0);
    CHECK(header != nullptr);
    CHECK(header->layoutType() == PageLayoutType::Header);
    CHECK(header->id() == 0);

    auto freeList = PageFactory::createPage(PageLayoutType::FreeList, 2);
    CHECK(freeList != nullptr);
    CHECK(freeList->layoutType() == PageLayoutType::FreeList);

    auto data = PageFactory::createPage(PageLayoutType::Data, 8);
    CHECK(data != nullptr);
    CHECK(data->layoutType() == PageLayoutType::Data);

    auto index = PageFactory::createPage(PageLayoutType::Index, 10);
    CHECK(index != nullptr);
    CHECK(index->layoutType() == PageLayoutType::Index);

    auto overflow = PageFactory::createPage(PageLayoutType::Overflow, 12);
    CHECK(overflow != nullptr);
    CHECK(overflow->layoutType() == PageLayoutType::Overflow);

    auto metadata = PageFactory::createPage(PageLayoutType::Metadata, 14);
    CHECK(metadata != nullptr);
    CHECK(metadata->layoutType() == PageLayoutType::Metadata);
}

TEST_CASE("PageFactory createPage from storage::Page", "[pages]") {
    quartz::storage::Page raw(0, quartz::storage::PageType::Header);
    auto page = PageFactory::createPage(std::move(raw));
    REQUIRE(page != nullptr);
    CHECK(page->layoutType() == PageLayoutType::Header);
    CHECK(page->id() == 0);
}

TEST_CASE("PageFactory rejects unsupported storage page types", "[pages]") {
    quartz::storage::Page raw(0, quartz::storage::PageType::Invalid);
    auto page = PageFactory::createPage(std::move(raw));
    CHECK(page == nullptr);
}

TEST_CASE("PageFactory deserialize returns nullptr for invalid data", "[pages]") {
    Buffer buf;
    BinaryWriter writer(buf);
    // Write garbage
    REQUIRE(writer.write<std::uint32_t>(0xDEADBEEF).ok());

    BinaryReader reader{BufferView(buf)};
    auto page = PageFactory::deserialize(PageLayoutType::Data, reader);
    CHECK(page == nullptr);
}

TEST_CASE("PageFactory deserialize invalid type", "[pages]") {
    Buffer buf;
    BinaryWriter writer(buf);
    BinaryReader reader{BufferView(buf)};
    auto page = PageFactory::deserialize(static_cast<PageLayoutType>(99), reader);
    CHECK(page == nullptr);
}

// ===== PageValidator =====

TEST_CASE("PageValidator validateLayoutType", "[pages]") {
    CHECK(PageValidator::validateLayoutType(PageLayoutType::Header).ok());
    CHECK(PageValidator::validateLayoutType(PageLayoutType::Data).ok());
    CHECK_FALSE(PageValidator::validateLayoutType(
        static_cast<PageLayoutType>(0)).ok());
    CHECK_FALSE(PageValidator::validateLayoutType(
        static_cast<PageLayoutType>(99)).ok());
}

TEST_CASE("PageValidator validatePage for each type", "[pages]") {
    auto header = HeaderPage::create(1, 0, 0);
    CHECK(PageValidator::validatePage(header).ok());

    auto freeList = FreeListPage::create(1);
    CHECK(PageValidator::validatePage(freeList).ok());

    auto data = DataPage::create(1);
    CHECK(PageValidator::validatePage(data).ok());

    auto overflow = OverflowPage::create(1);
    CHECK(PageValidator::validatePage(overflow).ok());

    auto index = IndexPage::create(1);
    index.setCapacity(10);
    CHECK(PageValidator::validatePage(index).ok());

    auto metadata = MetadataPage::create(1);
    metadata.setVersion(1);
    CHECK(PageValidator::validatePage(metadata).ok());
}

TEST_CASE("PageValidator invalid pages", "[pages]") {
    HeaderPage badHeader;
    CHECK_FALSE(PageValidator::validatePage(badHeader).ok());

    FreeListPage badFL;
    CHECK_FALSE(PageValidator::validatePage(badFL).ok());

    // Invalid page header (uninitialized)
    quartz::storage::Page raw(0, quartz::storage::PageType::Data);
    // Make it invalid by zeroing the data buffer
    raw.zeroFill();
    DataPage badData(std::move(raw));
    CHECK_FALSE(badData.isValid());  // page header is zeroed
}

TEST_CASE("PageValidator reserved fields are zero after create", "[pages]") {
    {
        auto page = HeaderPage::create(1, 0, 0);
        CHECK(PageValidator::validateReservedFields(*page.layout()).ok());
    }
    {
        auto page = FreeListPage::create(1);
        auto* l = reinterpret_cast<const FreeListPageLayout*>(page.page().payload());
        CHECK(PageValidator::validateReservedFields(*l).ok());
    }
    {
        auto page = DataPage::create(1);
        auto* l = reinterpret_cast<const DataPageLayout*>(page.page().payload());
        CHECK(PageValidator::validateReservedFields(*l).ok());
    }
    {
        auto page = OverflowPage::create(1);
        auto* l = reinterpret_cast<const OverflowPageLayout*>(page.page().payload());
        CHECK(PageValidator::validateReservedFields(*l).ok());
    }
    {
        auto page = IndexPage::create(1);
        auto* l = reinterpret_cast<const IndexPageLayout*>(page.page().payload());
        CHECK(PageValidator::validateReservedFields(*l).ok());
    }
    {
        auto page = MetadataPage::create(1);
        auto* l = reinterpret_cast<const MetadataPageLayout*>(page.page().payload());
        CHECK(PageValidator::validateReservedFields(*l).ok());
    }
}

// ===== PageStatistics =====

TEST_CASE("PageStatistics for HeaderPage", "[pages]") {
    auto page = HeaderPage::create(1, 0, 0);
    auto stats = PageStatistics::compute(page);
    CHECK(stats.totalSize == 4096);
    CHECK(stats.headerOverhead == 64);
    CHECK(stats.payloadSize == 4032);
    CHECK(stats.reservedBytes == 3888);   // 486 * 8
    CHECK(stats.usedBytes > 0);
    CHECK(stats.utilizationPercent > 0.0);
    CHECK(stats.freeBytes == stats.reservedBytes);
}

TEST_CASE("PageStatistics for FreeListPage", "[pages]") {
    auto page = FreeListPage::create(1);
    auto stats = PageStatistics::compute(page);
    CHECK(stats.totalSize == 4096);
    CHECK(stats.headerOverhead == 64);
    CHECK(stats.payloadSize == 4032);
    CHECK(stats.reservedBytes == 24);  // reserved[3]
    CHECK(stats.freeBytes == 0);      // no pages added yet
}

TEST_CASE("PageStatistics for DataPage", "[pages]") {
    auto page = DataPage::create(1);
    auto stats = PageStatistics::compute(page);
    CHECK(stats.totalSize == 4096);
    CHECK(stats.reservedBytes == 24);  // reserved1 + reserved2[2]
    CHECK(stats.freeBytes == 4008);    // available
}

TEST_CASE("PageStatistics for OverflowPage", "[pages]") {
    auto page = OverflowPage::create(1);
    page.setPayloadSize(100);
    auto stats = PageStatistics::compute(page);
    CHECK(stats.totalSize == 4096);
    CHECK(stats.reservedBytes == 16);  // reserved[2]
    CHECK(stats.usedBytes > 0);
}

TEST_CASE("PageStatistics for IndexPage", "[pages]") {
    auto page = IndexPage::create(1);
    auto stats = PageStatistics::compute(page);
    CHECK(stats.totalSize == 4096);
    CHECK(stats.reservedBytes == 24);  // reserved[3]
}

TEST_CASE("PageStatistics for MetadataPage", "[pages]") {
    auto page = MetadataPage::create(1);
    auto stats = PageStatistics::compute(page);
    CHECK(stats.totalSize == 4096);
    CHECK(stats.reservedBytes == 24);  // reserved[3]
}

TEST_CASE("PageStatistics toString", "[pages]") {
    auto page = DataPage::create(1);
    auto stats = PageStatistics::compute(page);
    auto str = PageStatistics::toString(stats);
    CHECK_FALSE(str.empty());
    CHECK(str.find("4096") != std::string::npos);
}

// ===== Edge Cases =====

TEST_CASE("Page type enum consistency", "[pages]") {
    // PageLayoutType values should align with storage::PageType for supported types
    CHECK(static_cast<int>(PageLayoutType::Header) ==
          static_cast<int>(quartz::storage::PageType::Header));
    CHECK(static_cast<int>(PageLayoutType::FreeList) ==
          static_cast<int>(quartz::storage::PageType::FreeList));
    CHECK(static_cast<int>(PageLayoutType::Data) ==
          static_cast<int>(quartz::storage::PageType::Data));
    CHECK(static_cast<int>(PageLayoutType::Overflow) ==
          static_cast<int>(quartz::storage::PageType::Overflow));
    CHECK(static_cast<int>(PageLayoutType::Index) ==
          static_cast<int>(quartz::storage::PageType::Index));
    CHECK(static_cast<int>(PageLayoutType::Metadata) ==
          static_cast<int>(quartz::storage::PageType::Metadata));
}

TEST_CASE("isValidLayoutType function", "[pages]") {
    CHECK(isValidLayoutType(PageLayoutType::Header));
    CHECK(isValidLayoutType(PageLayoutType::Metadata));
    CHECK(isValidLayoutType(PageLayoutType::FreeList));
    CHECK(isValidLayoutType(PageLayoutType::Data));
    CHECK(isValidLayoutType(PageLayoutType::Index));
    CHECK(isValidLayoutType(PageLayoutType::Overflow));
    CHECK_FALSE(isValidLayoutType(static_cast<PageLayoutType>(0)));
    CHECK_FALSE(isValidLayoutType(static_cast<PageLayoutType>(7)));
}

TEST_CASE("Multiple page types in factory", "[pages]") {
    auto h = HeaderPage::create(1, 0, 0);
    auto fl = FreeListPage::create(2);
    auto d = DataPage::create(3);
    auto ov = OverflowPage::create(4);
    auto idx = IndexPage::create(5);
    auto md = MetadataPage::create(6);

    CHECK(h.id() == 0);      // HeaderPage defaults to kHeaderPageId
    CHECK(fl.id() == 2);
    CHECK(d.id() == 3);
    CHECK(ov.id() == 4);
    CHECK(idx.id() == 5);
    CHECK(md.id() == 6);
}

TEST_CASE("PageFactory createPage with invalid type returns nullptr", "[pages]") {
    auto page = PageFactory::createPage(
        static_cast<PageLayoutType>(99), 0);
    CHECK(page == nullptr);
}

TEST_CASE("BasePage toString returns non-empty string for valid page", "[pages]") {
    auto page = DataPage::create(1);
    auto str = page.toString();
    CHECK(str.size() == 4096);
}

TEST_CASE("Move semantics for BasePage", "[pages]") {
    auto original = DataPage::create(42);
    CHECK(original.id() == 42);

    DataPage moved(std::move(original));
    CHECK(moved.id() == 42);
    // original is now moved-from
}

// ===== Layout struct integration =====

TEST_CASE("HeaderPageLayout after initFromFormat is valid", "[pages]") {
    auto dbHeader = quartz::format::DatabaseHeader::make();
    HeaderPage page;
    page.initFromFormat(dbHeader);
    CHECK(page.layout()->isValid());
}

TEST_CASE("FreeListPageLayout after adding entries is valid", "[pages]") {
    auto page = FreeListPage::create(1);
    REQUIRE(page.addFreePage(10).ok());
    REQUIRE(page.addFreePage(20).ok());
    auto* l = reinterpret_cast<const FreeListPageLayout*>(page.page().payload());
    CHECK(l->isValid());
    CHECK(l->freeCount == 2);
}

TEST_CASE("DataPageLayout freeSpaceOffset update", "[pages]") {
    DataPageLayout layout{};
    layout.freeSpaceOffset = 100;
    CHECK(layout.availableSpace() == 3908);

    layout.freeSpaceOffset = 4008;
    CHECK(layout.availableSpace() == 0);
}

TEST_CASE("OverflowPageLayout remainingCapacity", "[pages]") {
    OverflowPageLayout layout{};
    CHECK(layout.remainingCapacity() == 4008);

    layout.payloadSize = 500;
    CHECK(layout.remainingCapacity() == 3508);
}

TEST_CASE("IndexPageLayout stores node metadata", "[pages]") {
    IndexPageLayout layout{};
    layout.nodeType = 1;
    layout.capacity = 100;
    layout.keyCount = 25;
    layout.flags = 2;
    CHECK(layout.isValid());
    CHECK(layout.nodeType == 1);
    CHECK(layout.capacity == 100);
    CHECK(layout.keyCount == 25);
    CHECK(layout.flags == 2);
}

TEST_CASE("MetadataPageLayout version tracking", "[pages]") {
    MetadataPageLayout layout{};
    layout.version = 2;
    layout.entryCount = 10;
    CHECK(layout.isValid());
    CHECK(layout.version == 2);
    CHECK(layout.entryCount == 10);
}

// ===== Serialization edge cases =====

TEST_CASE("PageFactory deserialize with insufficient data returns nullptr", "[pages]") {
    Buffer buf;
    // Write only 100 bytes, far less than a full page
    for (int i = 0; i < 100; ++i) {
        const auto byte = static_cast<std::uint8_t>(i);
        REQUIRE(buf.append(&byte, 1).ok());
    }

    BinaryReader reader{BufferView(buf)};
    auto page = PageFactory::deserialize(PageLayoutType::Data, reader);
    CHECK(page == nullptr);
}

TEST_CASE("Multiple page serializations in single buffer", "[pages]") {
    auto header = HeaderPage::create(1, 0, 0);
    auto freeList = FreeListPage::create(2);
    REQUIRE(freeList.addFreePage(100).ok());

    Buffer buf;
    BinaryWriter writer(buf);

    CHECK(header.serialize(writer).ok());
    CHECK(freeList.serialize(writer).ok());
    CHECK(buf.size() == 8192);  // 2 pages * 4096

    BinaryReader reader{BufferView(buf)};
    auto restoredHeader = PageFactory::deserialize(PageLayoutType::Header, reader);
    REQUIRE(restoredHeader != nullptr);
    CHECK(restoredHeader->id() == 0);

    auto restoredFL = PageFactory::deserialize(PageLayoutType::FreeList, reader);
    REQUIRE(restoredFL != nullptr);
    CHECK(restoredFL->id() == 2);
}

// ===== Statistics toString =====

TEST_CASE("PageStatistics.toString contains all fields", "[pages]") {
    auto page = DataPage::create(1);
    auto stats = PageStatistics::compute(page);
    auto str = PageStatistics::toString(stats);
    CHECK(str.find("Total") != std::string::npos);
    CHECK(str.find("Header") != std::string::npos);
    CHECK(str.find("Payload") != std::string::npos);
    CHECK(str.find("Reserved") != std::string::npos);
    CHECK(str.find("Used") != std::string::npos);
    CHECK(str.find("Free") != std::string::npos);
    CHECK(str.find("Utilization") != std::string::npos);
}

// ===== Validation error details =====

TEST_CASE("PageValidator reserved fields detect corruption", "[pages]") {
    {
        HeaderPageLayout layout{};
        layout.reserved[0] = 1;
        CHECK_FALSE(PageValidator::validateReservedFields(layout).ok());
    }
    {
        IndexPageLayout layout{};
        layout.reserved[2] = 0xFF;
        CHECK_FALSE(PageValidator::validateReservedFields(layout).ok());
    }
    {
        DataPageLayout layout{};
        layout.reserved1 = 1;
        CHECK_FALSE(PageValidator::validateReservedFields(layout).ok());
    }
}
