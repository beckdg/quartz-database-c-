#include "quartz/util/Endian.h"

#include <catch2/catch_test_macros.hpp>
#include <cstring>

using namespace quartz;
using namespace quartz::endian;

TEST_CASE("Endian::swap16 swaps bytes", "[endian]") {
    CHECK(swap16(0x0102u) == 0x0201u);
    CHECK(swap16(0x0000u) == 0x0000u);
    CHECK(swap16(0xFFFFu) == 0xFFFFu);
    CHECK(swap16(0x1234u) == 0x3412u);
}

TEST_CASE("Endian::swap32 swaps bytes", "[endian]") {
    CHECK(swap32(0x01020304u) == 0x04030201u);
    CHECK(swap32(0x00000000u) == 0x00000000u);
    CHECK(swap32(0xFFFFFFFFu) == 0xFFFFFFFFu);
    CHECK(swap32(0xDEADBEEFu) == 0xEFBEADDEu);
}

TEST_CASE("Endian::swap64 swaps bytes", "[endian]") {
    CHECK(swap64(0x0102030405060708ull) == 0x0807060504030201ull);
    CHECK(swap64(0x0000000000000000ull) == 0x0000000000000000ull);
    CHECK(swap64(0xFFFFFFFFFFFFFFFFull) == 0xFFFFFFFFFFFFFFFFull);
}

TEST_CASE("Endian::readLE and writeLE round-trip", "[endian]") {
    uint8_t buf[8] = {};

    writeLE16(buf, 0x1234u);
    CHECK(readLE16(buf) == 0x1234u);

    writeLE32(buf, 0xDEADBEEFu);
    CHECK(readLE32(buf) == 0xDEADBEEFu);

    writeLE64(buf, 0x0102030405060708ull);
    CHECK(readLE64(buf) == 0x0102030405060708ull);
}

TEST_CASE("Endian::readBE and writeBE round-trip", "[endian]") {
    uint8_t buf[8] = {};

    writeBE16(buf, 0x1234u);
    CHECK(readBE16(buf) == 0x1234u);

    writeBE32(buf, 0xDEADBEEFu);
    CHECK(readBE32(buf) == 0xDEADBEEFu);

    writeBE64(buf, 0x0102030405060708ull);
    CHECK(readBE64(buf) == 0x0102030405060708ull);
}

TEST_CASE("Endian::readLE on big-endian buffer produces correct value", "[endian]") {
    uint8_t le_buf[4] = { 0x78, 0x56, 0x34, 0x12 };

    uint32_t value = readLE32(le_buf);
    CHECK(value == 0x12345678u);
}

TEST_CASE("Endian::readBE on little-endian buffer produces correct value", "[endian]") {
    uint8_t be_buf[4] = { 0x12, 0x34, 0x56, 0x78 };

    uint32_t value = readBE32(be_buf);
    CHECK(value == 0x12345678u);
}

TEST_CASE("Endian::nativeOrder is consistent with isLittleEndian", "[endian]") {
    if (isLittleEndian()) {
        CHECK(nativeOrder() == Order::Little);
    } else {
        CHECK(nativeOrder() == Order::Big);
    }
}
