#include "quartz/serialization/VariableLengthInteger.h"

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <limits>

using namespace quartz::serialization;

TEST_CASE("VarInt encode/decode uint32 zero", "[varint]") {
    std::uint8_t buf[VarInt::kMaxVarInt32Bytes];
    std::size_t written = 0;
    CHECK(VarInt::encodeU32(0, buf, sizeof(buf), written).ok());
    CHECK(written == 1);
    CHECK(buf[0] == 0);

    std::uint32_t value = 99;
    std::size_t consumed = 0;
    CHECK(VarInt::decodeU32(buf, written, value, consumed).ok());
    CHECK(value == 0);
    CHECK(consumed == 1);
}

TEST_CASE("VarInt encode/decode uint32 one byte values", "[varint]") {
    for (std::uint32_t v = 1; v < 128; ++v) {
        std::uint8_t buf[VarInt::kMaxVarInt32Bytes];
        std::size_t written = 0;
        CHECK(VarInt::encodeU32(v, buf, sizeof(buf), written).ok());
        CHECK(written == 1);
        CHECK(buf[0] == static_cast<std::uint8_t>(v));

        std::uint32_t decoded = 0;
        std::size_t consumed = 0;
        CHECK(VarInt::decodeU32(buf, written, decoded, consumed).ok());
        CHECK(decoded == v);
    }
}

TEST_CASE("VarInt encode/decode uint32 two byte values", "[varint]") {
    std::uint32_t v = 128;
    std::uint8_t buf[VarInt::kMaxVarInt32Bytes];
    std::size_t written = 0;
    CHECK(VarInt::encodeU32(v, buf, sizeof(buf), written).ok());
    CHECK(written == 2);
    CHECK(buf[0] == 0x80);
    CHECK(buf[1] == 0x01);

    std::uint32_t decoded = 0;
    std::size_t consumed = 0;
    CHECK(VarInt::decodeU32(buf, written, decoded, consumed).ok());
    CHECK(decoded == v);
    CHECK(consumed == 2);
}

TEST_CASE("VarInt encode/decode uint32 max value", "[varint]") {
    auto v = std::numeric_limits<std::uint32_t>::max();
    std::uint8_t buf[VarInt::kMaxVarInt32Bytes];
    std::size_t written = 0;
    CHECK(VarInt::encodeU32(v, buf, sizeof(buf), written).ok());
    CHECK(written == 5);

    std::uint32_t decoded = 0;
    std::size_t consumed = 0;
    CHECK(VarInt::decodeU32(buf, written, decoded, consumed).ok());
    CHECK(decoded == v);
    CHECK(consumed == 5);
}

TEST_CASE("VarInt encode/decode uint64", "[varint]") {
    std::uint64_t v = (std::uint64_t{1} << 63) + 12345;
    std::uint8_t buf[VarInt::kMaxVarInt64Bytes];
    std::size_t written = 0;
    CHECK(VarInt::encodeU64(v, buf, sizeof(buf), written).ok());
    CHECK(written == 10);

    std::uint64_t decoded = 0;
    std::size_t consumed = 0;
    CHECK(VarInt::decodeU64(buf, written, decoded, consumed).ok());
    CHECK(decoded == v);
}

TEST_CASE("VarInt encode/decode uint64 zero", "[varint]") {
    std::uint8_t buf[VarInt::kMaxVarInt64Bytes];
    std::size_t written = 0;
    CHECK(VarInt::encodeU64(0, buf, sizeof(buf), written).ok());
    CHECK(written == 1);

    std::uint64_t decoded = 99;
    std::size_t consumed = 0;
    CHECK(VarInt::decodeU64(buf, written, decoded, consumed).ok());
    CHECK(decoded == 0);
}

TEST_CASE("VarInt encode/decode signed int32", "[varint]") {
    struct TestCase { std::int32_t value; std::size_t expectedSize; };
    auto cases = {
        TestCase{0, 1},
        TestCase{-1, 1},
        TestCase{1, 1},
        TestCase{63, 1},
        TestCase{-64, 1},
        TestCase{64, 2},
        TestCase{-65, 2},
        TestCase{std::numeric_limits<std::int32_t>::max(), 5},
        TestCase{std::numeric_limits<std::int32_t>::min(), 5}
    };

    for (auto tc : cases) {
        std::uint8_t buf[VarInt::kMaxVarInt32Bytes];
        std::size_t written = 0;
        CHECK(VarInt::encodeS32(tc.value, buf, sizeof(buf), written).ok());
        CHECK(written == tc.expectedSize);

        std::int32_t decoded = 0;
        std::size_t consumed = 0;
        CHECK(VarInt::decodeS32(buf, written, decoded, consumed).ok());
        CHECK(decoded == tc.value);
    }
}

TEST_CASE("VarInt encode/decode signed int64", "[varint]") {
    std::int64_t values[] = {
        0, 1, -1, 127, -128, 123456, -123456,
        std::numeric_limits<std::int64_t>::max(),
        std::numeric_limits<std::int64_t>::min()
    };

    for (auto v : values) {
        std::uint8_t buf[VarInt::kMaxVarInt64Bytes];
        std::size_t written = 0;
        CHECK(VarInt::encodeS64(v, buf, sizeof(buf), written).ok());
        CHECK(written > 0);
        CHECK(written <= VarInt::kMaxVarInt64Bytes);

        std::int64_t decoded = 0;
        std::size_t consumed = 0;
        CHECK(VarInt::decodeS64(buf, written, decoded, consumed).ok());
        CHECK(decoded == v);
    }
}

TEST_CASE("VarInt encode buffer too small", "[varint]") {
    std::uint8_t tiny[1];
    std::size_t written = 0;
    auto st = VarInt::encodeU32(0xFFFFFFFF, tiny, 1, written);
    CHECK_FALSE(st.ok());
}

TEST_CASE("VarInt decode overflow detection", "[varint]") {
    // 10 bytes with continuation bit set on all
    std::uint8_t buf[10];
    std::memset(buf, 0x80, 10);
    buf[9] = 0x02; // Non-zero value in last byte causes overflow

    // Actually 0x80 * 10 with last byte 0x02 is valid uint64 overflow
    // Let me test more carefully: all 10 bytes with continuation bit
    std::uint8_t overflow[10];
    std::memset(overflow, 0xFF, 10);

    std::uint64_t value = 0;
    std::size_t consumed = 0;
    auto st = VarInt::decodeU64(overflow, 10, value, consumed);
    CHECK_FALSE(st.ok());
}

TEST_CASE("VarInt decode malformed input - continuation bit never clears", "[varint]") {
    std::uint8_t buf[] = {0x81, 0x82, 0x84, 0x88, 0x90, 0xA0, 0xC0, 0x80, 0x80, 0x80, 0x00};
    std::uint64_t value = 0;
    std::size_t consumed = 0;
    // 11 bytes should exceed max
    auto st = VarInt::decodeU64(buf, 11, value, consumed);
    CHECK_FALSE(st.ok());
}

TEST_CASE("VarInt decode truncation", "[varint]") {
    // Two byte varint but only one byte provided
    std::uint8_t buf[] = {0x80};
    std::uint32_t value = 0;
    std::size_t consumed = 0;
    auto st = VarInt::decodeU32(buf, 1, value, consumed);
    CHECK_FALSE(st.ok());
}

TEST_CASE("VarInt encode/decode round-trip random values", "[varint]") {
    std::uint32_t testValues[] = {
        0, 1, 127, 128, 255, 256, 16383, 16384, 2097151, 2097152,
        268435455, 268435456, std::numeric_limits<std::uint32_t>::max()
    };

    for (auto v : testValues) {
        std::uint8_t buf[VarInt::kMaxVarInt32Bytes];
        std::size_t written = 0;
        CHECK(VarInt::encodeU32(v, buf, sizeof(buf), written).ok());

        std::uint32_t decoded = 0;
        std::size_t consumed = 0;
        CHECK(VarInt::decodeU32(buf, written, decoded, consumed).ok());
        CHECK(decoded == v);
    }
}

TEST_CASE("VarInt encodedSizeU32", "[varint]") {
    CHECK(VarInt::encodedSizeU32(0) == 1);
    CHECK(VarInt::encodedSizeU32(1) == 1);
    CHECK(VarInt::encodedSizeU32(127) == 1);
    CHECK(VarInt::encodedSizeU32(128) == 2);
    CHECK(VarInt::encodedSizeU32(16383) == 2);
    CHECK(VarInt::encodedSizeU32(16384) == 3);
    CHECK(VarInt::encodedSizeU32(std::numeric_limits<std::uint32_t>::max()) == 5);
}

TEST_CASE("VarInt encodedSizeU64", "[varint]") {
    CHECK(VarInt::encodedSizeU64(0) == 1);
    CHECK(VarInt::encodedSizeU64(std::numeric_limits<std::uint64_t>::max()) == 10);
}

TEST_CASE("VarInt encodedSizeS32", "[varint]") {
    CHECK(VarInt::encodedSizeS32(0) == 1);
    CHECK(VarInt::encodedSizeS32(-1) == 1);
    CHECK(VarInt::encodedSizeS32(64) == 2);
    CHECK(VarInt::encodedSizeS32(-65) == 2);
}
