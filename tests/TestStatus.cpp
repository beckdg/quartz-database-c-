#include "quartz/common/Status.h"

#include <catch2/catch_test_macros.hpp>

using namespace quartz;

TEST_CASE("Status::ok creates an OK status", "[status]") {
    auto s = Status::success();
    CHECK(s.ok());
    CHECK(s.code() == Status::Code::Ok);
    CHECK(s.message().empty());
}

TEST_CASE("Status::invalidArgument creates an error status", "[status]") {
    auto s = Status::invalidArgument("bad arg");
    CHECK_FALSE(s.ok());
    CHECK(s.code() == Status::Code::InvalidArgument);
    CHECK(s.message() == "bad arg");
}

TEST_CASE("Status::ioError creates an IO error status", "[status]") {
    auto s = Status::ioError("file not found");
    CHECK_FALSE(s.ok());
    CHECK(s.code() == Status::Code::IOError);
    CHECK(s.message() == "file not found");
}

TEST_CASE("Status::corruption creates a corruption status", "[status]") {
    auto s = Status::corruption("checksum mismatch");
    CHECK_FALSE(s.ok());
    CHECK(s.code() == Status::Code::Corruption);
    CHECK(s.message() == "checksum mismatch");
}

TEST_CASE("Status::outOfMemory creates an OOM status", "[status]") {
    auto s = Status::outOfMemory("allocation failed");
    CHECK_FALSE(s.ok());
    CHECK(s.code() == Status::Code::OutOfMemory);
    CHECK(s.message() == "allocation failed");
}

TEST_CASE("Status::unknown creates an unknown error status", "[status]") {
    auto s = Status::unknown("something went wrong");
    CHECK_FALSE(s.ok());
    CHECK(s.code() == Status::Code::Unknown);
    CHECK(s.message() == "something went wrong");
}

TEST_CASE("Status equality", "[status]") {
    auto ok1 = Status::success();
    auto ok2 = Status::success();
    CHECK(ok1 == ok2);

    auto err1 = Status::invalidArgument("x");
    auto err2 = Status::invalidArgument("x");
    CHECK(err1 == err2);

    CHECK_FALSE(ok1 == err1);
}

TEST_CASE("Status inequality", "[status]") {
    CHECK(Status::success() != Status::invalidArgument("x"));
    CHECK(Status::invalidArgument("a") != Status::invalidArgument("b"));
}

TEST_CASE("Status toString for OK", "[status]") {
    CHECK(Status::success().toString() == "OK");
}

TEST_CASE("Status toString for errors", "[status]") {
    auto s = Status::ioError("disk full");
    CHECK(s.toString() == "IOError: disk full");
}

TEST_CASE("Status is copyable", "[status]") {
    auto original = Status::corruption("bad data");
    auto copy = original;
    CHECK(copy == original);
    CHECK(copy.message() == original.message());
}

TEST_CASE("Status is movable", "[status]") {
    auto original = Status::unknown("oops");
    auto moved = std::move(original);
    CHECK(moved.code() == Status::Code::Unknown);
    CHECK(moved.message() == "oops");
}
