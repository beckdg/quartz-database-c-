#pragma once

#include "quartz/common/Status.h"
#include "quartz/serialization/BinaryReader.h"
#include "quartz/serialization/BinaryWriter.h"
#include "quartz/serialization/SerializationContext.h"

#include <string>

namespace quartz {
namespace serialization {

// Default trait: serialize trivially copyable types directly.
// Specialize this struct for custom types.
template <typename T, typename Enable = void>
struct SerializationTraits {
    static Status serialize(BinaryWriter& writer, const T& value, const SerializationContext&) {
        return writer.write(value);
    }

    static Status deserialize(BinaryReader& reader, T& value, const SerializationContext&) {
        return reader.read(value);
    }
};

// Specialization for std::string
template <>
struct SerializationTraits<std::string> {
    static Status serialize(BinaryWriter& writer, const std::string& value,
                            const SerializationContext&) {
        auto st = writer.writeVarU32(static_cast<std::uint32_t>(value.size()));
        if (!st.ok()) return st;
        return writer.writeBytes(value.data(), value.size());
    }

    static Status deserialize(BinaryReader& reader, std::string& value,
                              const SerializationContext&) {
        std::uint32_t len = 0;
        auto st = reader.readVarU32(len);
        if (!st.ok()) return st;
        value.resize(len);
        return reader.readBytes(value.data(), len);
    }
};

// Convenience free functions that delegate to traits
template <typename T>
Status serialize(BinaryWriter& writer, const T& value, const SerializationContext& ctx) {
    return SerializationTraits<T>::serialize(writer, value, ctx);
}

template <typename T>
Status deserialize(BinaryReader& reader, T& value, const SerializationContext& ctx) {
    return SerializationTraits<T>::deserialize(reader, value, ctx);
}

} // namespace serialization
} // namespace quartz
