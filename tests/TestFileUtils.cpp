#include "quartz/util/FileUtils.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace fs = std::filesystem;

using namespace quartz::file;

static fs::path testDir() {
    auto tmp = fs::temp_directory_path();
    auto dir = tmp / "quartzdb_test_files";
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directory(dir, ec);
    return dir;
}

struct TempFile {
    fs::path path;

    TempFile() {
        static std::atomic<uint64_t> counter{0};
        auto id = counter++;
        path = testDir() / ("test_" + std::to_string(id) + ".tmp");
        std::error_code ec;
        fs::remove(path, ec);
    }

    void write(const std::string& content) {
        std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
        ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
        ofs.close();
    }

    ~TempFile() {
        std::error_code ec;
        fs::remove(path, ec);
    }
};

TEST_CASE("FileUtils::exists returns true for existing files", "[file]") {
    TempFile f;
    f.write("hello");
    CHECK(exists(f.path.string()));
}

TEST_CASE("FileUtils::exists returns false for non-existing files", "[file]") {
    CHECK_FALSE(exists("nonexistent_file_xyz")); // NOLINT
}

TEST_CASE("FileUtils::size returns file size", "[file]") {
    TempFile f;
    f.write("hello");
    REQUIRE(exists(f.path.string()));
    CHECK(size(f.path.string()) == 5);
}

TEST_CASE("FileUtils::size returns file size for multi-byte content", "[file]") {
    TempFile f;
    f.write("hello world");
    REQUIRE(exists(f.path.string()));
    CHECK(size(f.path.string()) == 11);
}

TEST_CASE("FileUtils::size returns 0 for non-existing file", "[file]") {
    CHECK(size("nonexistent_file_xyz") == 0); // NOLINT
}

TEST_CASE("FileUtils::remove removes a file", "[file]") {
    TempFile f;
    f.write("data");
    REQUIRE(exists(f.path.string()));
    CHECK(remove(f.path.string()));
    CHECK_FALSE(exists(f.path.string()));
}

TEST_CASE("FileUtils::rename renames a file", "[file]") {
    TempFile src;
    src.write("data");

    auto dst = fs::temp_directory_path() / "quartzdb_test_renamed.tmp";
    // Clean up any leftover from previous runs
    fs::remove(dst);

    CHECK(rename(src.path.string(), dst.string()));

    CHECK_FALSE(exists(src.path.string()));
    CHECK(exists(dst.string()));

    // Cleanup
    fs::remove(dst);
}

TEST_CASE("FileUtils::createDirectory creates a directory", "[file]") {
    auto dir = fs::temp_directory_path() / "quartzdb_test_dir";

    // Cleanup before test
    fs::remove_all(dir);

    CHECK(createDirectory(dir.string()));
    CHECK(exists(dir.string()));

    // exists returns true for directories
    CHECK(fs::is_directory(dir));

    fs::remove_all(dir);
}

TEST_CASE("FileUtils::createDirectory handles existing directory", "[file]") {
    auto dir = fs::temp_directory_path() / "quartzdb_test_dir_exists";
    fs::create_directory(dir);
    CHECK(exists(dir.string()));
    CHECK(fs::is_directory(dir));
    fs::remove_all(dir);
}
