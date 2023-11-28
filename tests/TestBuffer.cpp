#include "quartz/serialization/Buffer.h"
#include "quartz/serialization/BufferView.h"

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <string>

using namespace quartz::serialization;

TEST_CASE("Buffer default constructs empty", "[buffer]") {
    Buffer buf;
    CHECK(buf.size() == 0);
    CHECK(buf.capacity() == 0);
    CHECK(buf.empty());
}

TEST_CASE("Buffer initial capacity", "[buffer]") {
    Buffer buf(128);
    CHECK(buf.empty());
    CHECK(buf.capacity() >= 128);
}

TEST_CASE("Buffer append and read back", "[buffer]") {
    Buffer buf;
    const char* data = "Hello";
    CHECK(buf.append(data, 5).ok());
    CHECK(buf.size() == 5);
    CHECK(std::memcmp(buf.data(), "Hello", 5) == 0);
}

TEST_CASE("Buffer append grow", "[buffer]") {
    Buffer buf;
    const char* data = "This is a longer string that forces growth";
    CHECK(buf.append(data, std::strlen(data)).ok());
    CHECK(buf.size() == std::strlen(data));
    CHECK(std::memcmp(buf.data(), data, buf.size()) == 0);
}

TEST_CASE("Buffer appendValue", "[buffer]") {
    Buffer buf;
    int value = 42;
    CHECK(buf.appendValue(value).ok());
    CHECK(buf.size() == sizeof(int));
    int read = 0;
    std::memcpy(&read, buf.data(), sizeof(int));
    CHECK(read == 42);
}

TEST_CASE("Buffer resize", "[buffer]") {
    Buffer buf;
    buf.resize(100);
    CHECK(buf.size() == 100);
}

TEST_CASE("Buffer clear", "[buffer]") {
    Buffer buf;
    REQUIRE(buf.append("data", 4).ok());
    CHECK_FALSE(buf.empty());
    buf.clear();
    CHECK(buf.empty());
    CHECK(buf.size() == 0);
}

TEST_CASE("Buffer reserve", "[buffer]") {
    Buffer buf;
    buf.reserve(256);
    CHECK(buf.capacity() >= 256);
    CHECK(buf.empty());
}

TEST_CASE("Buffer swap", "[buffer]") {
    Buffer a;
    REQUIRE(a.append("aaa", 3).ok());
    Buffer b;
    REQUIRE(b.append("bbbbb", 5).ok());

    a.swap(b);
    CHECK(a.size() == 5);
    CHECK(b.size() == 3);
}

TEST_CASE("Buffer equality", "[buffer]") {
    Buffer a, b;
    REQUIRE(a.append("test", 4).ok());
    REQUIRE(b.append("test", 4).ok());
    CHECK(a == b);
}

TEST_CASE("Buffer inequality", "[buffer]") {
    Buffer a, b;
    REQUIRE(a.append("abc", 3).ok());
    REQUIRE(b.append("xyz", 3).ok());
    CHECK(a != b);
}

TEST_CASE("Buffer at() bounds checking", "[buffer]") {
    Buffer buf;
    REQUIRE(buf.append("xyz", 3).ok());
    CHECK(buf.at(0) == 'x');
    CHECK(buf.at(2) == 'z');
    CHECK_THROWS_AS(buf.at(3), std::out_of_range);
}

TEST_CASE("Buffer const at() bounds checking", "[buffer]") {
    Buffer buf;
    REQUIRE(buf.append("xyz", 3).ok());
    const auto& cbuf = buf;
    CHECK(cbuf.at(1) == 'y');
    CHECK_THROWS_AS(cbuf.at(5), std::out_of_range);
}

TEST_CASE("Buffer range-based iteration", "[buffer]") {
    Buffer buf;
    REQUIRE(buf.append("abcd", 4).ok());
    std::size_t count = 0;
    for (auto byte : buf) {
        CHECK(byte != 0);
        ++count;
    }
    CHECK(count == 4);
}

TEST_CASE("Buffer append string_view", "[buffer]") {
    Buffer buf;
    std::string_view sv("string_view_data");
    CHECK(buf.append(sv).ok());
    CHECK(buf.size() == sv.size());
    CHECK(std::memcmp(buf.data(), sv.data(), sv.size()) == 0);
}

TEST_CASE("Buffer move constructor", "[buffer]") {
    Buffer a;
    REQUIRE(a.append("move", 4).ok());
    auto* oldData = a.data();
    Buffer b(std::move(a));
    CHECK(b.size() == 4);
    CHECK(b.data() == oldData);
    CHECK(a.empty());
}

TEST_CASE("Buffer move assignment", "[buffer]") {
    Buffer a;
    REQUIRE(a.append("assign", 6).ok());
    auto* oldData = a.data();
    Buffer b;
    b = std::move(a);
    CHECK(b.size() == 6);
    CHECK(b.data() == oldData);
    CHECK(a.empty());
}

TEST_CASE("Buffer toStringView", "[buffer]") {
    Buffer buf;
    REQUIRE(buf.append("hello", 5).ok());
    auto sv = buf.toStringView();
    CHECK(sv == "hello");
}

TEST_CASE("Buffer append other Buffer", "[buffer]") {
    Buffer a, b;
    REQUIRE(a.append("hello ", 6).ok());
    REQUIRE(b.append("world", 5).ok());
    CHECK(a.append(b).ok());
    CHECK(a.size() == 11);
    CHECK(std::memcmp(a.data(), "hello world", 11) == 0);
}

TEST_CASE("BufferView default constructs empty", "[buffer_view]") {
    BufferView view;
    CHECK(view.size() == 0);
    CHECK(view.empty());
    CHECK(view.data() == nullptr);
}

TEST_CASE("BufferView from raw pointer", "[buffer_view]") {
    std::uint8_t data[] = {1, 2, 3, 4, 5};
    BufferView view(data, 5);
    CHECK(view.size() == 5);
    CHECK(view.data() == data);
    CHECK_FALSE(view.empty());
}

TEST_CASE("BufferView from Buffer", "[buffer_view]") {
    Buffer buf;
    REQUIRE(buf.append("buffer", 6).ok());
    BufferView view(buf);
    CHECK(view.size() == 6);
    CHECK(view.data() == buf.data());
}

TEST_CASE("BufferView from string_view", "[buffer_view]") {
    std::string s = "hello";
    BufferView view(s);
    CHECK(view.size() == 5);
    CHECK(view.toString() == "hello");
}

TEST_CASE("BufferView subview", "[buffer_view]") {
    std::uint8_t data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    BufferView full(data, 10);
    auto sub = full.subview(3, 4);
    CHECK(sub.size() == 4);
    CHECK(sub[0] == 3);
    CHECK(sub[3] == 6);
}

TEST_CASE("BufferView subview past end", "[buffer_view]") {
    std::uint8_t data[] = {0, 1, 2};
    BufferView view(data, 3);
    auto sub = view.subview(10, 5);
    CHECK(sub.empty());
}

TEST_CASE("BufferView slice", "[buffer_view]") {
    std::uint8_t data[] = {0, 1, 2, 3, 4};
    BufferView view(data, 5);
    auto sl = view.slice(2);
    CHECK(sl.size() == 3);
    CHECK(sl[0] == 2);
}

TEST_CASE("BufferView remove_prefix", "[buffer_view]") {
    std::uint8_t data[] = {0, 1, 2, 3};
    BufferView view(data, 4);
    view.remove_prefix(2);
    CHECK(view.size() == 2);
    CHECK(view[0] == 2);
    CHECK(view.data() == data + 2);
}

TEST_CASE("BufferView remove_suffix", "[buffer_view]") {
    std::uint8_t data[] = {0, 1, 2, 3};
    BufferView view(data, 4);
    view.remove_suffix(2);
    CHECK(view.size() == 2);
    CHECK(view[0] == 0);
    CHECK(view[1] == 1);
}

TEST_CASE("BufferView equality", "[buffer_view]") {
    std::uint8_t a[] = {1, 2, 3};
    std::uint8_t b[] = {1, 2, 3};
    BufferView va(a, 3), vb(b, 3);
    CHECK(va == vb);
}

TEST_CASE("BufferView inequality", "[buffer_view]") {
    std::uint8_t a[] = {1, 2, 3};
    std::uint8_t b[] = {4, 5, 6};
    BufferView va(a, 3), vb(b, 3);
    CHECK(va != vb);
}

TEST_CASE("BufferView at() bounds checking", "[buffer_view]") {
    std::uint8_t data[] = {10, 20};
    BufferView view(data, 2);
    CHECK(view.at(0) == 10);
    CHECK(view.at(1) == 20);
    CHECK_THROWS_AS(view.at(2), std::out_of_range);
}

TEST_CASE("BufferView fromRaw static factory", "[buffer_view]") {
    int val = 0x12345678;
    auto view = BufferView::fromRaw(&val, sizeof(val));
    CHECK(view.size() == sizeof(int));
}

TEST_CASE("BufferView range-based iteration", "[buffer_view]") {
    std::uint8_t data[] = {1, 2, 3, 4, 5};
    BufferView view(data, 5);
    int sum = 0;
    for (auto byte : view) {
        sum += byte;
    }
    CHECK(sum == 15);
}
