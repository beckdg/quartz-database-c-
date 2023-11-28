#include "quartz/common/Config.h"
#include "quartz/common/BuildOptions.h"

#include <catch2/catch_test_macros.hpp>

using namespace quartz;

TEST_CASE("Config constants are consistent", "[config]") {
    CHECK(config::kPageSize == 4096);
    CHECK(config::kPageHeaderSize == 64);
    CHECK(config::kPagePayloadSize == config::kPageSize - config::kPageHeaderSize);
    CHECK(config::kPagePayloadSize == 4032);
}

TEST_CASE("Config magic number is 4 bytes", "[config]") {
    CHECK(config::kMagicNumber != 0);
    CHECK(sizeof(config::kMagicNumber) == 4);
}

TEST_CASE("Config page sizes are sensible", "[config]") {
    CHECK(config::kPageSize > 0);
    CHECK(config::kPageHeaderSize < config::kPageSize);
    CHECK(config::kPageHeaderSize > 0);
    CHECK(config::kInvalidPageId == 0xFFFFFFFFu);
    CHECK(config::kMaxPageCount > 0);
    CHECK(config::kReservedPages < config::kMaxPageCount);
}

TEST_CASE("BuildOptions detects debug/release", "[build]") {
    // At least one should be true
    CHECK((build::kDebugBuild || build::kReleaseBuild));
}

TEST_CASE("BuildOptions has compiler info", "[build]") {
    CHECK(build::kCompilerName != nullptr);
    CHECK(build::kCompilerMajor > 0);
}

TEST_CASE("BuildOptions has platform info", "[build]") {
    CHECK(build::kPlatformName != nullptr);
    CHECK((build::kPlatformWindows || build::kPlatformLinux || build::kPlatformMacOS));
}

TEST_CASE("BuildOptions C++ standard is C++17 or later", "[build]") {
    CHECK(build::kCppStandard >= 201703L);
}
