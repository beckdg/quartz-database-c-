#include "quartz/storage/DatabaseFile.h"

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

using namespace quartz::storage;

static std::string testPath(const char* name) {
    return fs::temp_directory_path().string() + "/quartzdb_test_" + name;
}

static void cleanup(const std::string& path) {
    std::error_code ec;
    fs::remove(fs::path(path), ec);
}

TEST_CASE("DatabaseFile fails to open nonexistent file without create", "[file]") {
    auto path = testPath("nonexistent");
    cleanup(path);
    DatabaseFile file;
    auto status = file.open(path, false);
    CHECK_FALSE(status.ok());
}

TEST_CASE("DatabaseFile creates and opens file", "[file]") {
    auto path = testPath("create_open");
    cleanup(path);

    {
        DatabaseFile file(path, true);
        CHECK(file.isOpen());
        CHECK(file.fileSize() == 0);

        auto status = file.close();
        CHECK(status.ok());
        CHECK_FALSE(file.isOpen());
    }
    cleanup(path);
}

TEST_CASE("DatabaseFile reopen detects file size", "[file]") {
    auto path = testPath("reopen_size");
    cleanup(path);

    {
        DatabaseFile file(path, true);
        // Write some data to set the size
        Page p(0, PageType::Data);
        std::memcpy(p.payload(), "test", 4);
        auto status = file.writePage(p);
        CHECK(status.ok());
        CHECK(file.fileSize() >= 4096);
    }

    {
        DatabaseFile file;
        auto status = file.open(path, false);
        CHECK(status.ok());
        CHECK(file.fileSize() >= 4096);
        REQUIRE(file.close().ok());
    }
    cleanup(path);
}

TEST_CASE("DatabaseFile readPage after writePage returns same data", "[file]") {
    auto path = testPath("read_write");
    cleanup(path);

    {
        DatabaseFile file(path, true);
        Page writePage(0, PageType::Data);
        std::memcpy(writePage.payload(), "Hello, DB!", 11);
        writePage.setDirty(true);

        auto status = file.writePage(writePage);
        CHECK(status.ok());
    }

    {
        DatabaseFile file(path, false);
        Page readPage(0, PageType::Data);
        auto status = file.readPage(readPage);
        CHECK(status.ok());
        CHECK(readPage.id() == 0);
        CHECK(std::memcmp(readPage.payload(), "Hello, DB!", 11) == 0);
        CHECK_FALSE(readPage.dirty());
    }
    cleanup(path);
}

TEST_CASE("DatabaseFile multiple pages", "[file]") {
    auto path = testPath("multi_page");
    cleanup(path);

    {
        DatabaseFile file(path, true);
        Page p0(0, PageType::Data);
        Page p1(1, PageType::Index);
        Page p2(2, PageType::Metadata);

        auto v0 = static_cast<std::uint32_t>(0xDEADBEEF);
        auto v1 = static_cast<std::uint32_t>(0xCAFEBABE);
        auto v2 = static_cast<std::uint32_t>(0x12345678);

        std::memcpy(p0.payload(), &v0, sizeof(v0));
        std::memcpy(p1.payload(), &v1, sizeof(v1));
        std::memcpy(p2.payload(), &v2, sizeof(v2));

        CHECK(file.writePage(p0).ok());
        CHECK(file.writePage(p1).ok());
        CHECK(file.writePage(p2).ok());
        CHECK(file.fileSize() >= 3 * 4096);
    }

    {
        DatabaseFile file(path, false);
        Page r0(0, PageType::Data);
        Page r1(1, PageType::Index);
        Page r2(2, PageType::Metadata);

        CHECK(file.readPage(r0).ok());
        CHECK(file.readPage(r1).ok());
        CHECK(file.readPage(r2).ok());

        const std::uint32_t expected0 = 0xDEADBEEF;
        const std::uint32_t expected1 = 0xCAFEBABE;
        const std::uint32_t expected2 = 0x12345678;
        CHECK(std::memcmp(r0.payload(), &expected0, sizeof(expected0)) == 0);
        CHECK(std::memcmp(r1.payload(), &expected1, sizeof(expected1)) == 0);
        CHECK(std::memcmp(r2.payload(), &expected2, sizeof(expected2)) == 0);
    }
    cleanup(path);
}

TEST_CASE("DatabaseFile resize", "[file]") {
    auto path = testPath("resize");
    cleanup(path);

    {
        DatabaseFile file(path, true);
        CHECK(file.fileSize() == 0);

        auto status = file.resize(65536);
        CHECK(status.ok());
        CHECK(file.fileSize() == 65536);
    }

    {
        DatabaseFile file(path, false);
        CHECK(file.fileSize() == 65536);
    }
    cleanup(path);
}

TEST_CASE("DatabaseFile seek and read bytes", "[file]") {
    auto path = testPath("seek_bytes");
    cleanup(path);

    {
        DatabaseFile file(path, true);
        char data[] = "Hello, seek!";
        auto status = file.writeBytes(data, sizeof(data));
        CHECK(status.ok());
    }

    {
        DatabaseFile file(path, false);
        auto status = file.seek(7);
        CHECK(status.ok());

        char buf[5] = {};
        status = file.readBytes(buf, 4);
        CHECK(status.ok());
        CHECK(std::memcmp(buf, "seek", 4) == 0);
    }
    cleanup(path);
}

TEST_CASE("DatabaseFile operations fail on closed file", "[file]") {
    DatabaseFile file;
    CHECK_FALSE(file.isOpen());

    Page page;
    CHECK_FALSE(file.readPage(page).ok());
    CHECK_FALSE(file.writePage(page).ok());
    CHECK_FALSE(file.flush().ok());
    CHECK_FALSE(file.sync().ok());
    CHECK_FALSE(file.seek(0).ok());

    unsigned char buf[16] = {};
    CHECK_FALSE(file.readBytes(buf, 16).ok());
    CHECK_FALSE(file.writeBytes(buf, 16).ok());
    CHECK_FALSE(file.resize(4096).ok());
}

TEST_CASE("DatabaseFile writePage failure on invalid page", "[file]") {
    auto path = testPath("invalid_page");
    cleanup(path);

    DatabaseFile file(path, true);
    Page invalidPage;  // kInvalidPageId
    auto status = file.writePage(invalidPage);
    CHECK_FALSE(status.ok());
    cleanup(path);
}

TEST_CASE("DatabaseFile fails to reopen already open file", "[file]") {
    auto path = testPath("reopen_err");
    cleanup(path);

    DatabaseFile file(path, true);
    auto status = file.open(path, false);
    CHECK_FALSE(status.ok());
    cleanup(path);
}

TEST_CASE("DatabaseFile move constructor transfers ownership", "[file]") {
    auto path = testPath("move_construct");
    cleanup(path);

    DatabaseFile file1(path, true);
    CHECK(file1.isOpen());

    DatabaseFile file2(std::move(file1));
    CHECK(file2.isOpen());
    CHECK_FALSE(file1.isOpen());

    REQUIRE(file2.close().ok());
    cleanup(path);
}

TEST_CASE("DatabaseFile move assignment transfers ownership", "[file]") {
    auto path1 = testPath("move_assign1");
    auto path2 = testPath("move_assign2");
    cleanup(path1);
    cleanup(path2);

    DatabaseFile file1(path1, true);
    DatabaseFile file2(path2, true);
    CHECK(file1.isOpen());
    CHECK(file2.isOpen());

    file1 = std::move(file2);
    CHECK(file1.isOpen());
    CHECK_FALSE(file2.isOpen());

    REQUIRE(file1.close().ok());
    cleanup(path1);
    cleanup(path2);
}
