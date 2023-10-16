#pragma once

#include "quartz/common/Status.h"
#include "quartz/serialization/BinaryReader.h"
#include "quartz/serialization/BinaryWriter.h"
#include "quartz/serialization/SerializationContext.h"
#include "quartz/serialization/SerializationTraits.h"

#include <cstdint>

namespace quartz {
namespace serialization {

class Serializer {
public:
    static constexpr std::uint32_t kFormatMagic = 0x51444253u; // "QDBS"

    struct Header {
        std::uint32_t magic;
        std::uint32_t version;
        std::uint64_t dataSize;
    };

    static Status writeHeader(BinaryWriter& writer, const SerializationContext& ctx);
    static Status readHeader(BinaryReader& reader, SerializationContext& ctx);
    static bool isValidHeader(const Header& header) noexcept;

    template <typename T>
    static Status serialize(BinaryWriter& writer, const T& value,
                            const SerializationContext& ctx) {
        return SerializationTraits<T>::serialize(writer, value, ctx);
    }

    template <typename T>
    static Status deserialize(BinaryReader& reader, T& value,
                              const SerializationContext& ctx) {
        return SerializationTraits<T>::deserialize(reader, value, ctx);
    }

    template <typename T>
    static Status serializeWithHeader(BinaryWriter& writer, const T& value,
                                       const SerializationContext& ctx) {
        auto st = writeHeader(writer, ctx);
        if (!st.ok()) return st;
        return serialize(writer, value, ctx);
    }

    template <typename T>
    static Status deserializeWithHeader(BinaryReader& reader, T& value,
                                         SerializationContext& ctx) {
        auto st = readHeader(reader, ctx);
        if (!st.ok()) return st;
        return deserialize(reader, value, ctx);
    }
};

} // namespace serialization
} // namespace quartz
