#include "quartz/serialization/Serializer.h"

#include <string>

namespace quartz {
namespace serialization {

Status Serializer::writeHeader(BinaryWriter& writer, const SerializationContext& ctx) {
    Header header;
    header.magic = kFormatMagic;
    header.version = ctx.version();
    header.dataSize = 0;
    return writer.write(header);
}

Status Serializer::readHeader(BinaryReader& reader, SerializationContext& ctx) {
    Header header;
    auto st = reader.read(header);
    if (!st.ok()) {
        return Status::corruption("Serializer: failed to read header");
    }
    if (!isValidHeader(header)) {
        return Status::corruption("Serializer: invalid header magic");
    }
    if (!ctx.isVersionSupported(header.version)) {
        return Status::invalidArgument("Serializer: unsupported version " +
                                       std::to_string(header.version));
    }
    ctx.setVersion(header.version);
    return Status::success();
}

bool Serializer::isValidHeader(const Header& header) noexcept {
    return header.magic == kFormatMagic;
}

} // namespace serialization
} // namespace quartz
