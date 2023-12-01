#include "quartz/storage/Page.h"

#include <catch2/catch_test_macros.hpp>

#include <cstring>

using namespace quartz::storage;

TEST_CASE("Default Page constructor creates invalid page", "[page]") {
    Page p;
    CHECK(p.id() == quartz::config::kInvalidPageId);
    CHECK(p.type() == PageType::Invalid);
    CHECK_FALSE(p.dirty());
    CHECK(p.data() != nullptr);
    CHECK(p.size() == 4096);
}

TEST_CASE("Page constructed with ID and type", "[page]") {
    Page p(42, PageType::Data);
    CHECK(p.id() == 42);
    CHECK(p.type() == PageType::Data);
    CHECK_FALSE(p.dirty());
    CHECK(p.data() != nullptr);
}

TEST_CASE("Page header is written at start of data buffer", "[page]") {
    Page p(7, PageType::Metadata);
    auto* header = reinterpret_cast<const PageHeader*>(p.data());
    CHECK(header->pageId == 7);
    CHECK(header->pageType == PageType::Metadata);
    CHECK(header->isValid());
}

TEST_CASE("Page payload points after header", "[page]") {
    Page p(1, PageType::Data);
    CHECK(p.payload() == p.data() + 64);
    CHECK(p.payloadSize() == 4032);
}

TEST_CASE("Page dirty flag can be set and cleared", "[page]") {
    Page p(0, PageType::Data);
    CHECK_FALSE(p.dirty());
    p.setDirty(true);
    CHECK(p.dirty());
    p.setDirty(false);
    CHECK_FALSE(p.dirty());
}

TEST_CASE("Page pin count can be incremented", "[page]") {
    Page p(0, PageType::Data);
    CHECK(p.pinCount() == 0);
    p.pin();
    CHECK(p.pinCount() == 1);
    p.pin();
    CHECK(p.pinCount() == 2);
    p.unpin();
    CHECK(p.pinCount() == 1);
    p.unpin();
    CHECK(p.pinCount() == 0);
}

TEST_CASE("Page generation can be set", "[page]") {
    Page p(0, PageType::Data);
    p.setGeneration(42);
    CHECK(p.generation() == 42);
}

TEST_CASE("Page move constructor transfers ownership", "[page]") {
    Page p1(10, PageType::Data);
    auto* data1 = p1.data();
    Page p2(std::move(p1));

    CHECK(p2.id() == 10);
    CHECK(p2.type() == PageType::Data);
    CHECK(p2.data() == data1);

    CHECK(p1.data() == nullptr);
    CHECK(p1.id() == quartz::config::kInvalidPageId);
    CHECK(p1.type() == PageType::Invalid);
}

TEST_CASE("Page move assignment transfers ownership", "[page]") {
    Page p1(20, PageType::Index);
    auto* data1 = p1.data();
    Page p2(0, PageType::Invalid);
    p2 = std::move(p1);

    CHECK(p2.id() == 20);
    CHECK(p2.type() == PageType::Index);
    CHECK(p2.data() == data1);

    CHECK(p1.data() == nullptr);
    CHECK(p1.id() == quartz::config::kInvalidPageId);
    CHECK(p1.type() == PageType::Invalid);
}

TEST_CASE("Page reset changes identity but keeps buffer", "[page]") {
    Page p(1, PageType::Data);
    auto* data = p.data();
    p.reset(999, PageType::Journal);

    CHECK(p.id() == 999);
    CHECK(p.type() == PageType::Journal);
    CHECK(p.data() == data);
    CHECK_FALSE(p.dirty());

    auto* header = reinterpret_cast<const PageHeader*>(p.data());
    CHECK(header->pageId == 999);
}

TEST_CASE("Page zeroFill clears entire buffer", "[page]") {
    Page p(1, PageType::Data);
    std::memset(p.data(), 0xFF, p.size());
    p.zeroFill();
    for (std::size_t i = 0; i < p.size(); ++i) {
        CHECK(p.data()[i] == 0);
    }
}

TEST_CASE("Page toString returns correct size view", "[page]") {
    Page p(0, PageType::Data);
    auto view = p.toString();
    CHECK(view.size() == p.size());
    CHECK(view.data() == reinterpret_cast<const char*>(p.data()));
}

TEST_CASE("Page payload is writable and readable", "[page]") {
    Page p(0, PageType::Data);
    std::memcpy(p.payload(), "Hello, Page!", 13);
    CHECK(std::memcmp(p.payload(), "Hello, Page!", 13) == 0);
}
