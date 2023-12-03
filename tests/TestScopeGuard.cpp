#include "quartz/common/ScopeGuard.h"

#include <catch2/catch_test_macros.hpp>

using namespace quartz;

TEST_CASE("ScopeGuard calls the function on destruction", "[scope_guard]") {
    bool called = false;
    {
        auto guard = makeScopeGuard([&called] { called = true; });
        CHECK_FALSE(called);
    }
    CHECK(called);
}

TEST_CASE("ScopeGuard::dismiss prevents the function from being called", "[scope_guard]") {
    bool called = false;
    {
        auto guard = makeScopeGuard([&called] { called = true; });
        guard.dismiss();
    }
    CHECK_FALSE(called);
}

TEST_CASE("ScopeGuard works with lambda capture", "[scope_guard]") {
    int value = 0;
    {
        auto guard = makeScopeGuard([&value] { value = 42; });
    }
    CHECK(value == 42);
}

TEST_CASE("QUARTZ_SCOPE_EXIT macro works", "[scope_guard]") {
    bool called = false;
    {
        QUARTZ_SCOPE_EXIT([&called] { called = true; });
        CHECK_FALSE(called);
    }
    CHECK(called);
}
