#include "quartz/format/MagicNumbers.h"
#include "quartz/format/Versioning.h"
#include "quartz/format/FeatureFlags.h"
#include "quartz/format/Compatibility.h"
#include "quartz/format/ObjectId.h"
#include "quartz/format/PageReference.h"
#include "quartz/format/DatabaseHeader.h"
#include "quartz/format/Superblock.h"
#include "quartz/format/MetadataDescriptor.h"
#include "quartz/format/SchemaDescriptor.h"
#include "quartz/format/FormatValidator.h"
#include "quartz/serialization/Buffer.h"
#include "quartz/serialization/BinaryWriter.h"
#include "quartz/serialization/BinaryReader.h"
#include "quartz/serialization/SerializationContext.h"
#include "quartz/serialization/SerializationTraits.h"
#include "quartz/serialization/Serializer.h"

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>

using namespace quartz::format;
using namespace quartz::serialization;

// ===== MagicNumbers =====

TEST_CASE("MagicNumbers constants are non-zero", "[format]") {
    CHECK(MagicNumbers::kDatabaseMagic != 0);
    CHECK(MagicNumbers::kSuperblockMagic != 0);
    CHECK(MagicNumbers::kPageMagic != 0);
}

TEST_CASE("MagicNumbers::isValid recognizes format magics", "[format]") {
    CHECK(MagicNumbers::isValid(MagicNumbers::kFormatMagicV1));
    CHECK(MagicNumbers::isValid(MagicNumbers::kFormatMagicV2));
    CHECK_FALSE(MagicNumbers::isValid(0));
    CHECK_FALSE(MagicNumbers::isValid(0xFFFFFFFF));
}

TEST_CASE("MagicNumbers::isRecognized accepts known magics", "[format]") {
    CHECK(MagicNumbers::isRecognized(MagicNumbers::kDatabaseMagic));
    CHECK(MagicNumbers::isRecognized(MagicNumbers::kSuperblockMagic));
    CHECK(MagicNumbers::isRecognized(MagicNumbers::kPageMagic));
    CHECK(MagicNumbers::isRecognized(MagicNumbers::kFreeListMagic));
    CHECK_FALSE(MagicNumbers::isRecognized(0));
}

// ===== Versioning =====

TEST_CASE("Versioning constants match expectations", "[format]") {
    CHECK(Versioning::kMajorVersion == 1);
    CHECK(Versioning::kMinorVersion == 0);
}

TEST_CASE("Versioning::current returns expected version", "[format]") {
    auto v = Versioning::current();
    CHECK(v.major == Versioning::kMajorVersion);
    CHECK(v.minor == Versioning::kMinorVersion);
}

TEST_CASE("Version equality", "[format]") {
    Versioning::Version a{1, 0, 0};
    Versioning::Version b{1, 0, 0};
    Versioning::Version c{1, 1, 0};
    CHECK(a == b);
    CHECK(a != c);
}

TEST_CASE("Version::isCompatible", "[format]") {
    Versioning::Version v1{1, 0, 0};
    Versioning::Version v2{1, 1, 0};
    Versioning::Version v3{2, 0, 0};
    CHECK(v2.isCompatible(v1));
    CHECK_FALSE(v1.isCompatible(v2));
    CHECK_FALSE(v3.isCompatible(v1));
}

TEST_CASE("Versioning::validate", "[format]") {
    CHECK(Versioning::validate({1, 0, 0}).ok());
    CHECK(Versioning::validate({1, 1, 0}).ok());
    CHECK_FALSE(Versioning::validate({0, 0, 0}).ok());
    CHECK_FALSE(Versioning::validate({2, 0, 0}).ok());
}

TEST_CASE("Versioning::isReadable", "[format]") {
    CHECK(Versioning::isReadable({1, 0, 0}, {1, 0, 0}));
    CHECK(Versioning::isReadable({1, 0, 0}, {1, 1, 0}));
    CHECK_FALSE(Versioning::isReadable({1, 1, 0}, {1, 0, 0}));
    CHECK_FALSE(Versioning::isReadable({2, 0, 0}, {1, 0, 0}));
}

TEST_CASE("Versioning::toString", "[format]") {
    CHECK(Versioning::toString({1, 0, 0}) == "1.0.0");
    CHECK(Versioning::toString({2, 3, 4}) == "2.3.4");
}

// ===== FeatureFlags =====

TEST_CASE("FeatureFlags default to none", "[format]") {
    FeatureFlags flags;
    CHECK(flags.value() == FeatureFlags::kNone);
    CHECK_FALSE(flags.hasAny(FeatureFlags::kChecksums));
}

TEST_CASE("FeatureFlags set and clear", "[format]") {
    FeatureFlags flags;
    flags.set(FeatureFlags::kChecksums);
    CHECK(flags.has(FeatureFlags::kChecksums));
    CHECK(flags.hasAny(FeatureFlags::kChecksums | FeatureFlags::kCompression));

    flags.clear(FeatureFlags::kChecksums);
    CHECK_FALSE(flags.has(FeatureFlags::kChecksums));
}

TEST_CASE("FeatureFlags multiple flags", "[format]") {
    FeatureFlags flags(FeatureFlags::kCompression | FeatureFlags::kJournaling);
    CHECK(flags.has(FeatureFlags::kCompression));
    CHECK(flags.has(FeatureFlags::kJournaling));
    CHECK_FALSE(flags.has(FeatureFlags::kEncryption));
}

TEST_CASE("FeatureFlags equality", "[format]") {
    FeatureFlags a(FeatureFlags::kChecksums);
    FeatureFlags b(FeatureFlags::kChecksums);
    FeatureFlags c(FeatureFlags::kCompression);
    CHECK(a == b);
    CHECK(a != c);
}

TEST_CASE("FeatureFlags validate", "[format]") {
    FeatureFlags valid;
    CHECK(valid.validate().ok());

    FeatureFlags invalid(0xFF00000000000000ull);
    CHECK_FALSE(invalid.validate().ok());
}

TEST_CASE("FeatureFlags hasReservedBits", "[format]") {
    FeatureFlags clean;
    CHECK_FALSE(clean.hasReservedBits());

    FeatureFlags dirty(0xFFFFFFFF00000000ull);
    CHECK(dirty.hasReservedBits());
}

TEST_CASE("FeatureFlags toString", "[format]") {
    FeatureFlags none;
    CHECK(none.toString() == "none");

    FeatureFlags withCksum(FeatureFlags::kChecksums);
    CHECK(withCksum.toString() == "checksums");

    FeatureFlags multi(FeatureFlags::kChecksums | FeatureFlags::kJournaling);
    CHECK(multi.toString() == "checksums|journaling");
}

// ===== Compatibility =====

TEST_CASE("Compatibility checkReader", "[format]") {
    CHECK(Compatibility::checkReaderCompatibility({1, 0, 0}, {1, 0, 0}).ok());
    CHECK(Compatibility::checkReaderCompatibility({1, 0, 0}, {1, 1, 0}).ok());
    CHECK_FALSE(Compatibility::checkReaderCompatibility({2, 0, 0}, {1, 0, 0}).ok());
    CHECK_FALSE(Compatibility::checkReaderCompatibility({1, 1, 0}, {1, 0, 0}).ok());
}

TEST_CASE("Compatibility checkWriter", "[format]") {
    CHECK(Compatibility::checkWriterCompatibility({1, 0, 0}, {1, 0, 0}).ok());
    CHECK(Compatibility::checkWriterCompatibility({1, 1, 0}, {1, 0, 0}).ok());
    CHECK_FALSE(Compatibility::checkWriterCompatibility({2, 0, 0}, {1, 0, 0}).ok());
}

TEST_CASE("Compatibility checkFeature", "[format]") {
    FeatureFlags fileFlags(FeatureFlags::kChecksums);
    FeatureFlags readerFlags(FeatureFlags::kChecksums);
    CHECK(Compatibility::checkFeatureCompatibility(fileFlags, readerFlags).ok());

    FeatureFlags readerNoCksum;
    CHECK_FALSE(Compatibility::checkFeatureCompatibility(fileFlags, readerNoCksum).ok());
}

TEST_CASE("Compatibility isDowngrade/isUpgrade", "[format]") {
    CHECK(Compatibility::isDowngrade({2, 0, 0}, {1, 0, 0}));
    CHECK_FALSE(Compatibility::isDowngrade({1, 0, 0}, {2, 0, 0}));
    CHECK(Compatibility::isDowngrade({1, 1, 0}, {1, 0, 0}));
    CHECK(Compatibility::isUpgrade({1, 0, 0}, {2, 0, 0}));
    CHECK(Compatibility::isUpgrade({1, 0, 0}, {1, 1, 0}));
    CHECK_FALSE(Compatibility::isUpgrade({2, 0, 0}, {1, 0, 0}));
}

TEST_CASE("Compatibility canRead", "[format]") {
    Compatibility::Requirements req;
    req.minReaderVersion = {1, 0, 0};
    req.minWriterVersion = {1, 0, 0};
    CHECK(Compatibility::canRead(req, {1, 0, 0}, FeatureFlags()));

    req.prohibitedFeatures.set(FeatureFlags::kEncryption);
    FeatureFlags encryptedFile(FeatureFlags::kEncryption);
    CHECK_FALSE(Compatibility::canRead(req, {1, 0, 0}, encryptedFile));
}

// ===== ObjectId =====

TEST_CASE("ObjectId nil is all zeros", "[format]") {
    ObjectId id;
    CHECK(id.isNil());
    CHECK(id.high() == 0);
    CHECK(id.low() == 0);
}

TEST_CASE("ObjectId::generate creates non-nil ID", "[format]") {
    auto id = ObjectId::generate();
    CHECK_FALSE(id.isNil());
    CHECK(id.size() == 16);
}

TEST_CASE("ObjectId::generate produces unique IDs", "[format]") {
    auto a = ObjectId::generate();
    auto b = ObjectId::generate();
    CHECK(a != b);
}

TEST_CASE("ObjectId string conversion", "[format]") {
    auto original = ObjectId::generate();
    auto str = original.toString();
    CHECK(str.size() == 36);
    CHECK(str[8] == '-');
    CHECK(str[13] == '-');
    CHECK(str[18] == '-');
    CHECK(str[23] == '-');

    auto restored = ObjectId::fromString(str);
    CHECK(original == restored);
}

TEST_CASE("ObjectId::fromString nil case", "[format]") {
    auto str = "00000000-0000-0000-0000-000000000000";
    auto id = ObjectId::fromString(str);
    CHECK(id.isNil());
}

TEST_CASE("ObjectId::fromString with Status", "[format]") {
    ObjectId id;
    CHECK(ObjectId::fromString("00000000-0000-0000-0000-000000000000", id).ok());
    CHECK(id.isNil());
}

TEST_CASE("ObjectId::fromString invalid inputs", "[format]") {
    ObjectId id;
    CHECK_FALSE(ObjectId::fromString("invalid", id).ok());
    CHECK_FALSE(ObjectId::fromString("00000000-0000-0000-0000-00000000000Z", id).ok());
    CHECK_FALSE(ObjectId::fromString("00000000-0000-0000-0000-00000000000", id).ok());
}

TEST_CASE("ObjectId ordering", "[format]") {
    auto a = ObjectId::generate();
    auto b = ObjectId::generate();
    // Just verify they can be compared
    const bool ordered = (a < b) || (b < a) || (a == b);
    CHECK(ordered);
}

TEST_CASE("ObjectId comparison operators", "[format]") {
    ObjectId nil1;
    ObjectId nil2;
    auto gen = ObjectId::generate();

    CHECK(nil1 == nil2);
    CHECK_FALSE(nil1 != nil2);
    CHECK_FALSE(nil1 == gen);
    CHECK(nil1 != gen);
}

TEST_CASE("ObjectId copy and data access", "[format]") {
    auto original = ObjectId::generate();
    auto copy = original;  // Trivially copyable
    CHECK(original == copy);

    for (std::size_t i = 0; i < original.size(); ++i) {
        CHECK(original.data()[i] == copy.data()[i]);
    }
}

TEST_CASE("ObjectId high/low parts", "[format]") {
    ObjectId id;
    // Nil should have both parts zero
    CHECK(id.high() == 0);
    CHECK(id.low() == 0);
}

TEST_CASE("ObjectId::generate sets variant and version bits", "[format]") {
    auto id = ObjectId::generate();
    // Version 4: bytes[6] top 4 bits should be 0100
    CHECK((id.data()[6] & 0xF0) == 0x40);
    // RFC 4122 variant: bytes[8] top 2 bits should be 10
    CHECK((id.data()[8] & 0xC0) == 0x80);
}

// ===== PageReference =====

TEST_CASE("PageReference default is invalid", "[format]") {
    PageReference ref;
    CHECK_FALSE(ref.isValid());
    CHECK(ref.pageId == quartz::storage::kInvalidPageId);
}

TEST_CASE("PageReference::make creates valid reference", "[format]") {
    auto ref = PageReference::make(42, 1, 4);
    CHECK(ref.isValid());
    CHECK(ref.pageId == 42);
    CHECK(ref.generation == 1);
    CHECK(ref.pageType == 4);
}

TEST_CASE("PageReference size is 20 bytes", "[format]") {
    CHECK(sizeof(PageReference) == PageReference::kSize);
    CHECK(PageReference::kSize == 20);
}

TEST_CASE("PageReference equality", "[format]") {
    auto a = PageReference::make(1, 0, 0);
    auto b = PageReference::make(1, 0, 0);
    auto c = PageReference::make(2, 0, 0);
    CHECK(a == b);
    CHECK(a != c);
}

TEST_CASE("PageReference::invalid", "[format]") {
    auto ref = PageReference::invalid();
    CHECK_FALSE(ref.isValid());
    CHECK(ref.pageId == quartz::storage::kInvalidPageId);
}

// ===== DatabaseHeader =====

TEST_CASE("DatabaseHeader::make creates valid header", "[format]") {
    auto h = DatabaseHeader::make();
    CHECK(h.isValid());
    CHECK(h.magic == MagicNumbers::kDatabaseMagic);
    CHECK(h.majorVersion == Versioning::kMajorVersion);
    CHECK(h.pageSize == quartz::config::kPageSize);
    CHECK_FALSE(h.databaseId.isNil());
    CHECK(h.superblockPageId == 1);
}

TEST_CASE("DatabaseHeader size is 128 bytes", "[format]") {
    CHECK(sizeof(DatabaseHeader) == DatabaseHeader::kSize);
    CHECK(DatabaseHeader::kSize == 128);
}

TEST_CASE("DatabaseHeader timestamps start at zero", "[format]") {
    auto h = DatabaseHeader::make();
    CHECK(h.creationTimestamp == 0);
    CHECK(h.modificationTimestamp == 0);
}

TEST_CASE("DatabaseHeader version accessor", "[format]") {
    auto h = DatabaseHeader::make();
    auto v = h.version();
    CHECK(v.major == Versioning::kMajorVersion);
    CHECK(v.minor == Versioning::kMinorVersion);
}

TEST_CASE("DatabaseHeader invalid header detection", "[format]") {
    DatabaseHeader h{};
    CHECK_FALSE(h.isValid());

    auto valid = DatabaseHeader::make();
    CHECK(valid.isValid());

    // Corrupt magic
    auto corrupted = valid;
    corrupted.magic = 0;
    CHECK_FALSE(corrupted.isValid());
}

TEST_CASE("DatabaseHeader serialization round-trip", "[format]") {
    auto original = DatabaseHeader::make();
    original.creationTimestamp = 12345;
    original.modificationTimestamp = 67890;
    original.featureFlags = FeatureFlags::kChecksums;

    Buffer buf;
    BinaryWriter writer(buf);
    SerializationContext ctx(1);

    CHECK(serialize(writer, original, ctx).ok());
    CHECK(buf.size() == sizeof(DatabaseHeader));

    BufferView view(buf);
    BinaryReader reader(view);
    DatabaseHeader restored;
    CHECK(deserialize(reader, restored, ctx).ok());

    CHECK(restored.magic == original.magic);
    CHECK(restored.majorVersion == original.majorVersion);
    CHECK(restored.minorVersion == original.minorVersion);
    CHECK(restored.pageSize == original.pageSize);
    CHECK(restored.databaseId == original.databaseId);
    CHECK(restored.creationTimestamp == original.creationTimestamp);
    CHECK(restored.modificationTimestamp == original.modificationTimestamp);
    CHECK(restored.featureFlags == original.featureFlags);
    CHECK(restored.superblockPageId == original.superblockPageId);
}

TEST_CASE("DatabaseHeader feature flags accessor", "[format]") {
    auto h = DatabaseHeader::make();
    h.featureFlags = FeatureFlags::kCompression | FeatureFlags::kJournaling;
    auto flags = h.flags();
    CHECK(flags.has(FeatureFlags::kCompression));
    CHECK(flags.has(FeatureFlags::kJournaling));
}

// ===== Superblock =====

TEST_CASE("Superblock::make creates valid superblock", "[format]") {
    auto dbId = ObjectId::generate();
    auto sb = Superblock::make(dbId);
    CHECK(sb.isValid());
    CHECK(sb.magic == MagicNumbers::kSuperblockMagic);
    CHECK(sb.databaseId == dbId);
    CHECK(sb.totalPages > 0);
    CHECK(sb.reservedPages == quartz::config::kReservedPages);
}

TEST_CASE("Superblock size is 128 bytes", "[format]") {
    CHECK(sizeof(Superblock) == Superblock::kSize);
    CHECK(Superblock::kSize == 128);
}

TEST_CASE("Superblock consistency check", "[format]") {
    auto dbId = ObjectId::generate();
    auto sb = Superblock::make(dbId);
    // Initially consistent since totalPages = initial size, no allocations
    CHECK(sb.isConsistent());
}

TEST_CASE("Superblock serialization round-trip", "[format]") {
    auto dbId = ObjectId::generate();
    auto original = Superblock::make(dbId);
    original.totalPages = 1024;
    original.allocatedPages = 16;
    original.freePages = 1008;

    Buffer buf;
    BinaryWriter writer(buf);
    SerializationContext ctx(1);

    CHECK(serialize(writer, original, ctx).ok());
    CHECK(buf.size() == sizeof(Superblock));

    BufferView view(buf);
    BinaryReader reader(view);
    Superblock restored;
    CHECK(deserialize(reader, restored, ctx).ok());

    CHECK(restored.magic == original.magic);
    CHECK(restored.databaseId == original.databaseId);
    CHECK(restored.totalPages == original.totalPages);
    CHECK(restored.allocatedPages == original.allocatedPages);
    CHECK(restored.freePages == original.freePages);
    CHECK(restored.firstFreePage == original.firstFreePage);
    CHECK(restored.freeListPage == original.freeListPage);
}

TEST_CASE("Superblock invalid magic detection", "[format]") {
    Superblock sb{};
    CHECK_FALSE(sb.isValid());
}

// ===== MetadataDescriptor =====

TEST_CASE("MetadataDescriptor::make creates valid descriptor", "[format]") {
    auto desc = MetadataDescriptor::make(MagicNumbers::kFreeListMagic, 1, 5, 3, 12288);
    CHECK(desc.isValid());
    CHECK(desc.magic == MagicNumbers::kFreeListMagic);
    CHECK(desc.typeId == 1);
    CHECK(desc.startPage == 5);
    CHECK(desc.pageCount == 3);
    CHECK(desc.byteSize == 12288);
    CHECK(desc.version == 1);
}

TEST_CASE("MetadataDescriptor size is 48 bytes", "[format]") {
    CHECK(sizeof(MetadataDescriptor) == MetadataDescriptor::kSize);
    CHECK(MetadataDescriptor::kSize == 48);
}

TEST_CASE("MetadataDescriptor invalid detection", "[format]") {
    MetadataDescriptor desc{};
    CHECK_FALSE(desc.isValid());

    auto valid = MetadataDescriptor::make(MagicNumbers::kPageMagic, 0, 10, 1, 4096);
    CHECK(valid.isValid());
}

// ===== SchemaDescriptor =====

TEST_CASE("SchemaDescriptor::make creates valid descriptor", "[format]") {
    auto desc = SchemaDescriptor::make(1, 100, 5);
    CHECK(desc.isValid());
    CHECK(desc.schemaId == 1);
    CHECK(desc.rootPage == 100);
    CHECK(desc.fieldCount == 5);
    CHECK(desc.version == 1);
}

TEST_CASE("SchemaDescriptor size is 32 bytes", "[format]") {
    CHECK(sizeof(SchemaDescriptor) == SchemaDescriptor::kSize);
    CHECK(SchemaDescriptor::kSize == 32);
}

TEST_CASE("SchemaDescriptor invalid detection", "[format]") {
    SchemaDescriptor desc{};
    CHECK_FALSE(desc.isValid());

    auto valid = SchemaDescriptor::make(1, 10, 3);
    CHECK(valid.isValid());

    SchemaDescriptor badId{};
    badId.schemaId = 0;
    badId.rootPage = 10;
    CHECK_FALSE(badId.isValid());

    SchemaDescriptor badPage{};
    badPage.schemaId = 1;
    badPage.rootPage = quartz::storage::kInvalidPageId;
    CHECK_FALSE(badPage.isValid());
}

// ===== FormatValidator =====

TEST_CASE("FormatValidator::validateHeader valid header", "[format]") {
    auto h = DatabaseHeader::make();
    CHECK(FormatValidator::validateHeader(h).ok());
}

TEST_CASE("FormatValidator::validateHeader detects corruption", "[format]") {
    DatabaseHeader bad{};
    CHECK_FALSE(FormatValidator::validateHeader(bad).ok());
}

TEST_CASE("FormatValidator::validateHeader bad version", "[format]") {
    auto h = DatabaseHeader::make();
    h.majorVersion = 99;
    CHECK_FALSE(FormatValidator::validateHeader(h).ok());
}

TEST_CASE("FormatValidator::validateHeader bad page size", "[format]") {
    auto h = DatabaseHeader::make();
    h.pageSize = 123;
    CHECK_FALSE(FormatValidator::validateHeader(h).ok());
}

TEST_CASE("FormatValidator::validateSuperblock valid", "[format]") {
    auto sb = Superblock::make(ObjectId::generate());
    CHECK(FormatValidator::validateSuperblock(sb).ok());
}

TEST_CASE("FormatValidator::validateSuperblock invalid", "[format]") {
    Superblock sb{};
    CHECK_FALSE(FormatValidator::validateSuperblock(sb).ok());
}

TEST_CASE("FormatValidator::validateMagic", "[format]") {
    CHECK(FormatValidator::validateMagic(MagicNumbers::kFormatMagicV1).ok());
    CHECK_FALSE(FormatValidator::validateMagic(0).ok());
}

TEST_CASE("FormatValidator::validateVersion", "[format]") {
    CHECK(FormatValidator::validateVersion({1, 0, 0}).ok());
    CHECK_FALSE(FormatValidator::validateVersion({2, 0, 0}).ok());
}

TEST_CASE("FormatValidator::validatePageSize", "[format]") {
    CHECK(FormatValidator::validatePageSize(4096).ok());
    CHECK(FormatValidator::validatePageSize(512).ok());
    CHECK(FormatValidator::validatePageSize(65536).ok());
    CHECK_FALSE(FormatValidator::validatePageSize(0).ok());
    CHECK_FALSE(FormatValidator::validatePageSize(100).ok());
    CHECK_FALSE(FormatValidator::validatePageSize(100000).ok());
    CHECK_FALSE(FormatValidator::validatePageSize(3000).ok()); // not power of 2
}

TEST_CASE("FormatValidator::validateFeatureFlags", "[format]") {
    FeatureFlags valid;
    CHECK(FormatValidator::validateFeatureFlags(valid).ok());

    FeatureFlags invalid(0xFF00000000000000ull);
    CHECK_FALSE(FormatValidator::validateFeatureFlags(invalid).ok());
}

TEST_CASE("FormatValidator::validateMetadataDescriptor", "[format]") {
    auto desc = MetadataDescriptor::make(MagicNumbers::kPageMagic, 0, 5, 1, 4096);
    CHECK(FormatValidator::validateMetadataDescriptor(desc).ok());

    MetadataDescriptor empty{};
    CHECK_FALSE(FormatValidator::validateMetadataDescriptor(empty).ok());
}

TEST_CASE("FormatValidator::validateSchemaDescriptor", "[format]") {
    auto desc = SchemaDescriptor::make(1, 100, 5);
    CHECK(FormatValidator::validateSchemaDescriptor(desc).ok());

    SchemaDescriptor empty{};
    CHECK_FALSE(FormatValidator::validateSchemaDescriptor(empty).ok());
}

// ===== Serialization Integration =====

TEST_CASE("DatabaseHeader in Buffer with Serializer header", "[format]") {
    auto h = DatabaseHeader::make();
    Buffer buf;
    BinaryWriter writer(buf);
    SerializationContext ctx(1);

    CHECK(Serializer::serializeWithHeader(writer, h, ctx).ok());

    BufferView view(buf);
    BinaryReader reader(view);
    SerializationContext readCtx;
    DatabaseHeader restored;
    CHECK(Serializer::deserializeWithHeader(reader, restored, readCtx).ok());
    CHECK(restored.magic == h.magic);
    CHECK(restored.databaseId == h.databaseId);
}

TEST_CASE("Superblock in Buffer with Serializer header", "[format]") {
    auto sb = Superblock::make(ObjectId::generate());
    Buffer buf;
    BinaryWriter writer(buf);
    SerializationContext ctx(1);

    CHECK(Serializer::serializeWithHeader(writer, sb, ctx).ok());

    BufferView view(buf);
    BinaryReader reader(view);
    SerializationContext readCtx;
    Superblock restored;
    CHECK(Serializer::deserializeWithHeader(reader, restored, readCtx).ok());
    CHECK(restored.magic == sb.magic);
    CHECK(restored.databaseId == sb.databaseId);
}

// ===== ObjectId stream output =====

TEST_CASE("ObjectId ostream operator", "[format]") {
    auto id = ObjectId::generate();
    std::ostringstream oss;
    oss << id;
    CHECK(oss.str() == id.toString());
}

// ===== ObjectId unordered_set (hash test) =====
// Note: ObjectId doesn't have std::hash, so we test comparison only

TEST_CASE("ObjectId can be used in std::set", "[format]") {
    std::set<ObjectId> ids;
    auto a = ObjectId::generate();
    auto b = ObjectId::generate();
    ids.insert(a);
    ids.insert(b);
    CHECK(ids.size() == 2);
    CHECK(ids.count(a) == 1);
}

// ===== PageReference serialization =====

TEST_CASE("PageReference serialization round-trip", "[format]") {
    auto ref = PageReference::make(42, 7, 4);

    Buffer buf;
    BinaryWriter writer(buf);
    CHECK(writer.write(ref).ok());
    CHECK(buf.size() == sizeof(PageReference));

    BufferView view(buf);
    BinaryReader reader(view);
    PageReference restored;
    CHECK(reader.read(restored).ok());
    CHECK(restored.pageId == 42);
    CHECK(restored.generation == 7);
    CHECK(restored.pageType == 4);
}

// ===== Combined objects =====

TEST_CASE("DatabaseHeader and Superblock with matching ObjectId", "[format]") {
    auto dbId = ObjectId::generate();

    auto h = DatabaseHeader::make();
    h.databaseId = dbId;

    auto sb = Superblock::make(dbId);

    CHECK(h.databaseId == sb.databaseId);
    CHECK_FALSE(h.databaseId.isNil());
}

// ===== Reserved fields are zeroed =====

TEST_CASE("DatabaseHeader reserved fields are zero", "[format]") {
    auto h = DatabaseHeader::make();
    for (auto r : h.reserved) {
        CHECK(r == 0);
    }
}

TEST_CASE("Superblock reserved fields are zero", "[format]") {
    auto sb = Superblock::make(ObjectId::generate());
    for (auto r : sb.reserved) {
        CHECK(r == 0);
    }
}

TEST_CASE("Versioning isReadable edge cases", "[format]") {
    CHECK_FALSE(Versioning::isReadable({0, 0, 0}, {1, 0, 0}));
    CHECK_FALSE(Versioning::isReadable({1, 0, 0}, {0, 0, 0}));
    CHECK(Versioning::isReadable({1, 0, 0}, {1, 0, 0}));
    CHECK(Versioning::isReadable({1, 5, 0}, {1, 5, 0}));
}

TEST_CASE("FeatureFlags setValue and clear", "[format]") {
    FeatureFlags f(FeatureFlags::kChecksums | FeatureFlags::kCompression);
    f.clear(FeatureFlags::kChecksums);
    CHECK_FALSE(f.has(FeatureFlags::kChecksums));
    CHECK(f.has(FeatureFlags::kCompression));
    CHECK(f.value() == FeatureFlags::kCompression);
}
