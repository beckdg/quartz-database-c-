#include "quartz/serialization/BinaryReader.h"
#include "quartz/serialization/BinaryWriter.h"
#include "quartz/serialization/Buffer.h"
#include "quartz/serialization/BufferView.h"
#include "quartz/serialization/SerializationContext.h"
#include "quartz/serialization/SerializationTraits.h"
#include "quartz/serialization/Serializer.h"

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <string>

using namespace quartz::serialization;

TEST_CASE("BinaryWriter initial state", "[binary_io]") {
    Buffer buf;
    BinaryWriter writer(buf);
    CHECK(writer.tell() == 0);
    CHECK(buf.empty());
}

TEST_CASE("BinaryWriter write primitives", "[binary_io]") {
    Buffer buf;
    BinaryWriter writer(buf);

    CHECK(writer.write<std::uint32_t>(0x12345678).ok());
    CHECK(writer.write<std::uint16_t>(0xABCD).ok());
    CHECK(buf.size() == 6);
}

TEST_CASE("BinaryWriter write and BinaryReader read primitives", "[binary_io]") {
    Buffer buf;
    BinaryWriter writer(buf);

    std::uint32_t w32 = 0xDEADBEEF;
    std::uint16_t w16 = 0xCAFE;
    std::uint8_t w8 = 0x42;
    CHECK(writer.write(w32).ok());
    CHECK(writer.write(w16).ok());
    CHECK(writer.write(w8).ok());

    BufferView view(buf);
    BinaryReader reader(view);

    std::uint32_t r32 = 0;
    std::uint16_t r16 = 0;
    std::uint8_t r8 = 0;
    CHECK(reader.read(r32).ok());
    CHECK(reader.read(r16).ok());
    CHECK(reader.read(r8).ok());
    CHECK(r32 == 0xDEADBEEF);
    CHECK(r16 == 0xCAFE);
    CHECK(r8 == 0x42);
}

TEST_CASE("BinaryWriter writeLE/writeBE endianness", "[binary_io]") {
    Buffer buf;
    BinaryWriter writer(buf);

    CHECK(writer.writeLE<std::uint32_t>(0x01020304).ok());
    CHECK(writer.writeBE<std::uint32_t>(0x01020304).ok());
    CHECK(buf.size() == 8);

    // LE: 04 03 02 01, BE: 01 02 03 04
    CHECK(buf[0] == 0x04);
    CHECK(buf[3] == 0x01);
    CHECK(buf[4] == 0x01);
    CHECK(buf[7] == 0x04);
}

TEST_CASE("BinaryReader readLE/readBE", "[binary_io]") {
    std::uint8_t raw[] = {0x04, 0x03, 0x02, 0x01, 0x01, 0x02, 0x03, 0x04};
    BufferView view(raw, 8);
    BinaryReader reader(view);

    std::uint32_t le = 0, be = 0;
    CHECK(reader.readLE(le).ok());
    CHECK(reader.readBE(be).ok());
    CHECK(le == 0x01020304);
    CHECK(be == 0x01020304);
}

TEST_CASE("BinaryWriter writeBytes", "[binary_io]") {
    Buffer buf;
    BinaryWriter writer(buf);

    const char* data = "Hello, World!";
    CHECK(writer.writeBytes(data, 13).ok());
    CHECK(buf.size() == 13);
    CHECK(std::memcmp(buf.data(), data, 13) == 0);
}

TEST_CASE("BinaryReader readBytes", "[binary_io]") {
    std::uint8_t raw[] = {0x01, 0x02, 0x03, 0x04};
    BufferView view(raw, 4);
    BinaryReader reader(view);

    std::uint8_t dest[4] = {};
    CHECK(reader.readBytes(dest, 4).ok());
    CHECK(std::memcmp(dest, raw, 4) == 0);
}

TEST_CASE("BinaryReader readBytes into Buffer", "[binary_io]") {
    std::uint8_t raw[] = {10, 20, 30, 40, 50};
    BufferView view(raw, 5);
    BinaryReader reader(view);

    Buffer dest;
    CHECK(reader.readBytes(dest, 3).ok());
    CHECK(dest.size() == 3);
    CHECK(dest[0] == 10);
    CHECK(dest[2] == 30);
}

TEST_CASE("BinaryWriter dynamic growth", "[binary_io]") {
    Buffer buf(16);
    BinaryWriter writer(buf);

    std::vector<std::uint8_t> bigData(1024, 0xAB);
    CHECK(writer.writeBytes(bigData.data(), bigData.size()).ok());
    CHECK(buf.size() == 1024);
    CHECK(buf[0] == 0xAB);
    CHECK(buf[1023] == 0xAB);
}

TEST_CASE("BinaryReader seek and tell", "[binary_io]") {
    std::uint8_t raw[] = {0, 1, 2, 3, 4, 5, 6, 7};
    BufferView view(raw, 8);
    BinaryReader reader(view);

    CHECK(reader.tell() == 0);
    CHECK(reader.seek(4).ok());
    CHECK(reader.tell() == 4);

    std::uint8_t val = 0;
    CHECK(reader.read(val).ok());
    CHECK(val == 4);
    CHECK(reader.tell() == 5);
}

TEST_CASE("BinaryReader seek out of bounds", "[binary_io]") {
    std::uint8_t raw[] = {0, 1, 2};
    BufferView view(raw, 3);
    BinaryReader reader(view);
    CHECK_FALSE(reader.seek(10).ok());
}

TEST_CASE("BinaryReader skip", "[binary_io]") {
    std::uint8_t raw[] = {0, 1, 2, 3, 4, 5};
    BufferView view(raw, 6);
    BinaryReader reader(view);

    CHECK(reader.skip(3).ok());
    CHECK(reader.tell() == 3);
    std::uint8_t val = 0;
    CHECK(reader.read(val).ok());
    CHECK(val == 3);
}

TEST_CASE("BinaryReader skip past end", "[binary_io]") {
    std::uint8_t raw[] = {0, 1};
    BufferView view(raw, 2);
    BinaryReader reader(view);
    CHECK_FALSE(reader.skip(10).ok());
}

TEST_CASE("BinaryReader remaining and hasMore", "[binary_io]") {
    std::uint8_t raw[] = {0, 1, 2, 3, 4};
    BufferView view(raw, 5);
    BinaryReader reader(view);

    CHECK(reader.remaining() == 5);
    CHECK(reader.hasMore());
    CHECK(reader.skip(3).ok());
    CHECK(reader.remaining() == 2);
    CHECK(reader.hasMore());
    CHECK(reader.skip(2).ok());
    CHECK(reader.remaining() == 0);
    CHECK_FALSE(reader.hasMore());
}

TEST_CASE("BinaryReader read past end", "[binary_io]") {
    std::uint8_t raw[] = {0, 1};
    BufferView view(raw, 2);
    BinaryReader reader(view);

    std::uint32_t val = 0;
    CHECK_FALSE(reader.read(val).ok());
}

TEST_CASE("BinaryWriter seek", "[binary_io]") {
    Buffer buf;
    BinaryWriter writer(buf);

    CHECK(writer.write<std::uint32_t>(0xAAAAAAAA).ok());
    CHECK(writer.write<std::uint32_t>(0xBBBBBBBB).ok());
    CHECK(writer.seek(0).ok());
    CHECK(writer.write<std::uint32_t>(0xCCCCCCCC).ok());

    // Should have overwritten the first value
    BufferView view(buf);
    BinaryReader reader(view);
    std::uint32_t val = 0;
    CHECK(reader.read(val).ok());
    CHECK(val == 0xCCCCCCCC);
}

TEST_CASE("BinaryWriter seek out of bounds", "[binary_io]") {
    Buffer buf;
    BinaryWriter writer(buf);
    CHECK_FALSE(writer.seek(10).ok());
}

TEST_CASE("BinaryWriter overwriteAt", "[binary_io]") {
    Buffer buf;
    BinaryWriter writer(buf);
    REQUIRE(writer.writeBytes("abcdef", 6).ok());
    const char* rep = "XY";
    CHECK(writer.overwriteAt(2, rep, 2).ok());
    CHECK(std::memcmp(buf.data(), "abXYef", 6) == 0);
}

TEST_CASE("BinaryWriter overwriteAt out of bounds", "[binary_io]") {
    Buffer buf;
    BinaryWriter writer(buf);
    REQUIRE(writer.writeBytes("abc", 3).ok());
    const char* data = "too long";
    CHECK_FALSE(writer.overwriteAt(2, data, 6).ok());
}

TEST_CASE("BinaryWriter writeBytes from Buffer", "[binary_io]") {
    Buffer src, dest;
    REQUIRE(src.append("source data", 11).ok());
    BinaryWriter writer(dest);
    CHECK(writer.writeBytes(src).ok());
    CHECK(dest.size() == 11);
    CHECK(std::memcmp(dest.data(), src.data(), 11) == 0);
}

TEST_CASE("BinaryWriter writeBytes from BufferView", "[binary_io]") {
    std::uint8_t raw[] = {1, 2, 3, 4, 5};
    BufferView view(raw, 5);
    Buffer buf;
    BinaryWriter writer(buf);
    CHECK(writer.writeBytes(view).ok());
    CHECK(buf.size() == 5);
    CHECK(buf[0] == 1);
}

TEST_CASE("BinaryReader subReader", "[binary_io]") {
    std::uint8_t raw[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    BufferView view(raw, 10);
    BinaryReader reader(view);

    auto sub = reader.subReader(5);
    CHECK(sub.remaining() == 5);
    std::uint8_t vals[5];
    CHECK(sub.readBytes(vals, 5).ok());
    CHECK(vals[0] == 5);
    CHECK(vals[4] == 9);
}

TEST_CASE("BinaryWriter align", "[binary_io]") {
    Buffer buf;
    BinaryWriter writer(buf);
    CHECK(writer.write<std::uint8_t>(1).ok());
    CHECK(writer.tell() == 1);
    CHECK(writer.align(4).ok());
    CHECK(writer.tell() == 4);
    CHECK(writer.write<std::uint32_t>(0).ok());
    CHECK(buf.size() == 8);
}

TEST_CASE("BinaryReader align", "[binary_io]") {
    std::uint8_t raw[] = {0, 0, 0, 0, 1, 2, 3, 4};
    BufferView view(raw, 8);
    BinaryReader reader(view);
    CHECK(reader.skip(1).ok());
    CHECK(reader.align(4).ok());
    CHECK(reader.tell() == 4);
    std::uint32_t val = 0;
    CHECK(reader.read(val).ok());
    CHECK(val == 0x04030201);
}

TEST_CASE("BinaryWriter writeVarU32/readVarU32 round trip", "[binary_io]") {
    Buffer buf;
    BinaryWriter writer(buf);
    CHECK(writer.writeVarU32(0).ok());
    CHECK(writer.writeVarU32(127).ok());
    CHECK(writer.writeVarU32(128).ok());
    CHECK(writer.writeVarU32(16383).ok());
    CHECK(writer.writeVarU32(0xFFFFFFFF).ok());

    BufferView view(buf);
    BinaryReader reader(view);
    std::uint32_t v;
    CHECK(reader.readVarU32(v).ok()); CHECK(v == 0);
    CHECK(reader.readVarU32(v).ok()); CHECK(v == 127);
    CHECK(reader.readVarU32(v).ok()); CHECK(v == 128);
    CHECK(reader.readVarU32(v).ok()); CHECK(v == 16383);
    CHECK(reader.readVarU32(v).ok()); CHECK(v == 0xFFFFFFFF);
}

TEST_CASE("BinaryWriter writeVarU64/readVarU64 round trip", "[binary_io]") {
    Buffer buf;
    BinaryWriter writer(buf);
    std::uint64_t val = (std::uint64_t{1} << 60) + 12345;
    CHECK(writer.writeVarU64(val).ok());

    BufferView view(buf);
    BinaryReader reader(view);
    std::uint64_t decoded = 0;
    CHECK(reader.readVarU64(decoded).ok());
    CHECK(decoded == val);
}

TEST_CASE("BinaryWriter/Reader varint signed round trip", "[binary_io]") {
    Buffer buf;
    BinaryWriter writer(buf);
    std::int32_t vals[] = {0, -1, 1, -127, 128, -128, 0x7FFFFFFF, -0x7FFFFFFF - 1};
    for (auto v : vals) {
        CHECK(writer.writeVarS32(v).ok());
    }

    BufferView view(buf);
    BinaryReader reader(view);
    for (auto expected : vals) {
        std::int32_t v = 0;
        CHECK(reader.readVarS32(v).ok());
        CHECK(v == expected);
    }
}

TEST_CASE("BinaryReader readVarU32 corruption detection", "[binary_io]") {
    std::uint8_t buf[] = {0x80}; // Truncated two-byte varint
    BufferView view(buf, 1);
    BinaryReader reader(view);
    std::uint32_t v = 0;
    CHECK_FALSE(reader.readVarU32(v).ok());
}

TEST_CASE("BinaryWriter writeStrings via traits", "[binary_io]") {
    Buffer buf;
    BinaryWriter writer(buf);
    SerializationContext ctx;

    std::string hello = "Hello, Serialization!";
    CHECK(serialize(writer, hello, ctx).ok());

    BufferView view(buf);
    BinaryReader reader(view);
    std::string decoded;
    CHECK(deserialize(reader, decoded, ctx).ok());
    CHECK(decoded == hello);
}

TEST_CASE("Serializer header round trip", "[binary_io]") {
    Buffer buf;
    BinaryWriter writer(buf);
    SerializationContext ctx(1, quartz::endian::Order::Little, true);
    CHECK(Serializer::writeHeader(writer, ctx).ok());

    BufferView view(buf);
    BinaryReader reader(view);
    SerializationContext ctxOut;
    CHECK(Serializer::readHeader(reader, ctxOut).ok());
    CHECK(ctxOut.version() == 1);
}

TEST_CASE("Serializer header invalid magic", "[binary_io]") {
    std::uint8_t badHeader[sizeof(Serializer::Header)] = {};
    BufferView view(badHeader, sizeof(Serializer::Header));
    BinaryReader reader(view);
    SerializationContext ctx;
    CHECK_FALSE(Serializer::readHeader(reader, ctx).ok());
}

TEST_CASE("Serializer header unsupported version", "[binary_io]") {
    Buffer buf;
    BinaryWriter writer(buf);
    SerializationContext ctx(999, quartz::endian::Order::Little, true);
    CHECK(Serializer::writeHeader(writer, ctx).ok());

    BufferView view(buf);
    BinaryReader reader(view);
    SerializationContext ctxOut;
    // ctxOut defaults to version range [1, 1], so version 999 is unsupported
    CHECK_FALSE(Serializer::readHeader(reader, ctxOut).ok());
}

TEST_CASE("SerializationContext version support", "[binary_io]") {
    SerializationContext ctx;
    CHECK(ctx.isVersionSupported(1));
    CHECK_FALSE(ctx.isVersionSupported(0));
    CHECK_FALSE(ctx.isVersionSupported(2));
    CHECK(ctx.allowsField(1));
    CHECK(ctx.allowsField(0));
}

TEST_CASE("BinaryReader ensure bounds check", "[binary_io]") {
    std::uint8_t raw[] = {0, 1, 2};
    BufferView view(raw, 3);
    BinaryReader reader(view);
    CHECK(reader.ensure(3).ok());
    CHECK_FALSE(reader.ensure(4).ok());
}

TEST_CASE("Empty Buffer round trip", "[binary_io]") {
    Buffer buf;
    BinaryWriter writer(buf);
    CHECK(writer.tell() == 0);
    CHECK(buf.empty());

    BufferView view(buf);
    BinaryReader reader(view);
    CHECK(reader.remaining() == 0);
    CHECK_FALSE(reader.hasMore());
}

TEST_CASE("Write and read boolean as uint8", "[binary_io]") {
    Buffer buf;
    BinaryWriter writer(buf);
    CHECK(writer.write<std::uint8_t>(1).ok());
    CHECK(writer.write<std::uint8_t>(0).ok());

    BufferView view(buf);
    BinaryReader reader(view);
    std::uint8_t a = 0, b = 0;
    CHECK(reader.read(a).ok()); CHECK(a == 1);
    CHECK(reader.read(b).ok()); CHECK(b == 0);
}

TEST_CASE("BinaryWriter skip (zero fill)", "[binary_io]") {
    Buffer buf;
    BinaryWriter writer(buf);
    CHECK(writer.write<std::uint8_t>(0xAA).ok());
    CHECK(writer.skip(3).ok());
    CHECK(writer.write<std::uint8_t>(0xBB).ok());
    CHECK(buf.size() == 5);
    CHECK(buf[0] == 0xAA);
    CHECK(buf[4] == 0xBB);
}

TEST_CASE("Multiple writes and reads", "[binary_io]") {
    Buffer buf;
    BinaryWriter writer(buf);

    for (int i = 0; i < 100; ++i) {
        CHECK(writer.write(static_cast<std::uint32_t>(i)).ok());
    }
    CHECK(buf.size() == 100 * sizeof(std::uint32_t));

    BufferView view(buf);
    BinaryReader reader(view);
    for (int i = 0; i < 100; ++i) {
        std::uint32_t v = 0;
        CHECK(reader.read(v).ok());
        CHECK(v == static_cast<std::uint32_t>(i));
    }
}

TEST_CASE("Large Buffer growth", "[binary_io]") {
    Buffer buf(16);
    BinaryWriter writer(buf);

    std::vector<std::uint8_t> data(100000, 0x42);
    CHECK(writer.writeBytes(data.data(), data.size()).ok());
    CHECK(buf.size() == 100000);
    CHECK(buf[0] == 0x42);
    CHECK(buf[99999] == 0x42);
}

TEST_CASE("BinaryReader pointer access", "[binary_io]") {
    std::uint8_t raw[] = {10, 20, 30, 40, 50};
    BufferView view(raw, 5);
    BinaryReader reader(view);
    CHECK(reader.skip(2).ok());
    const auto* ptr = reader.pointer();
    CHECK(ptr[0] == 30);
    CHECK(ptr[2] == 50);
}

TEST_CASE("SerializationTraits trivially copyable struct", "[binary_io]") {
    struct Point { std::int32_t x; std::int32_t y; };
    Buffer buf;
    BinaryWriter writer(buf);
    SerializationContext ctx;

    Point p{10, 20};
    CHECK(serialize(writer, p, ctx).ok());

    BufferView view(buf);
    BinaryReader reader(view);
    Point q{};
    CHECK(deserialize(reader, q, ctx).ok());
    CHECK(q.x == 10);
    CHECK(q.y == 20);
}

TEST_CASE("Serializer serializeWithHeader/deserializeWithHeader", "[binary_io]") {
    Buffer buf;
    BinaryWriter writer(buf);
    SerializationContext ctx(1);

    std::string msg = "header test";
    CHECK(Serializer::serializeWithHeader(writer, msg, ctx).ok());

    BufferView view(buf);
    BinaryReader reader(view);
    SerializationContext readCtx;
    std::string decoded;
    CHECK(Serializer::deserializeWithHeader(reader, decoded, readCtx).ok());
    CHECK(decoded == msg);
    CHECK(readCtx.version() == 1);
}
