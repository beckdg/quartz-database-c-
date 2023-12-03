#include "quartz/storage/PageHeader.h"

#include <catch2/catch_test_macros.hpp>

#include <cstring>

using namespace quartz::storage;

TEST_CASE("PageHeader size is exactly 64 bytes", "[page_header]") {
    CHECK(sizeof(PageHeader) == 64);
}

TEST_CASE("PageHeader::make creates valid header", "[page_header]") {
    auto h = PageHeader::make(42, PageType::Data);
    CHECK(h.pageId == 42);
    CHECK(h.pageType == PageType::Data);
    CHECK(h.magic == quartz::config::kMagicNumber);
    CHECK(h.version == quartz::config::kFileFormatVersion);
    CHECK(h.isValid());
}

TEST_CASE("PageHeader::make for invalid type is still valid header", "[page_header]") {
    auto h = PageHeader::make(0, PageType::Header);
    CHECK(h.isValid());
}

TEST_CASE("PageHeader invalid with bad magic", "[page_header]") {
    auto h = PageHeader::make(1, PageType::Data);
    h.magic = 0;
    CHECK_FALSE(h.isValid());
}

TEST_CASE("PageHeader invalid with Invalid type", "[page_header]") {
    auto h = PageHeader::make(1, PageType::Invalid);
    CHECK_FALSE(h.isValid());
}

TEST_CASE("PageHeader zero-initialized is invalid", "[page_header]") {
    PageHeader h{};
    CHECK_FALSE(h.isValid());
}

TEST_CASE("PageHeader round-trip through memory", "[page_header]") {
    auto original = PageHeader::make(99, PageType::Index);
    original.generation = 12345;
    original.payloadSize = 2048;

    unsigned char buffer[sizeof(PageHeader)];
    std::memcpy(buffer, &original, sizeof(PageHeader));

    PageHeader restored;
    std::memcpy(&restored, buffer, sizeof(PageHeader));

    CHECK(restored.pageId == 99);
    CHECK(restored.pageType == PageType::Index);
    CHECK(restored.generation == 12345);
    CHECK(restored.payloadSize == 2048);
    CHECK(restored.isValid());
}

TEST_CASE("PageHeader has reserved fields for future use", "[page_header]") {
    auto h = PageHeader::make(0, PageType::Data);
    CHECK(h.reserved1 == 0);
    for (auto r : h.reserved2) {
        CHECK(r == 0);
    }
}

TEST_CASE("PageHeader allows all page types", "[page_header]") {
    auto h = PageHeader::make(0, PageType::Metadata);
    CHECK(h.pageType == PageType::Metadata);
    CHECK(h.isValid());

    h = PageHeader::make(0, PageType::FreeList);
    CHECK(h.pageType == PageType::FreeList);
    CHECK(h.isValid());

    h = PageHeader::make(0, PageType::Journal);
    CHECK(h.pageType == PageType::Journal);
    CHECK(h.isValid());
}
