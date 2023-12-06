#include "quartz/common/Version.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

using namespace quartz;
using Catch::Matchers::StartsWith;

TEST_CASE("Version::string returns the expected format", "[version]") {
    auto v = Version::string();
    CHECK_THAT(v, StartsWith("0.1."));
}

TEST_CASE("Version constants are accessible", "[version]") {
    CHECK(Version::Major == 0);
    CHECK(Version::Minor == 1);
    CHECK(Version::Patch == 0);
}

TEST_CASE("Version::buildInfo returns a non-empty string", "[version]") {
    auto info = Version::buildInfo();
    CHECK_FALSE(info.empty());
    CHECK_THAT(info, StartsWith("QuartzDB"));
}
