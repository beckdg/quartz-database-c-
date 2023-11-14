#include "quartz/recovery/CheckpointRecord.h"

namespace quartz {
namespace recovery {

Status CheckpointPayload::serialize(serialization::BinaryWriter& writer) const {
    auto st = writer.write(magic);
    if (!st.ok()) return st;
    st = writer.write(version);
    if (!st.ok()) return st;
    st = lsn.serialize(writer);
    if (!st.ok()) return st;
    st = writer.write(treeSize);
    if (!st.ok()) return st;
    st = writer.write(treeHeight);
    if (!st.ok()) return st;
    const auto snapLen = static_cast<std::uint32_t>(btreeSnapshot.size());
    st = writer.write(snapLen);
    if (!st.ok()) return st;
    if (snapLen > 0) {
        st = writer.writeBytes(btreeSnapshot);
        if (!st.ok()) return st;
    }
    return Status::success();
}

Status CheckpointPayload::deserialize(serialization::BinaryReader& reader) {
    auto st = reader.read(magic);
    if (!st.ok()) return st;
    st = reader.read(version);
    if (!st.ok()) return st;
    st = lsn.deserialize(reader);
    if (!st.ok()) return st;
    st = reader.read(treeSize);
    if (!st.ok()) return st;
    st = reader.read(treeHeight);
    if (!st.ok()) return st;
    std::uint32_t snapLen = 0;
    st = reader.read(snapLen);
    if (!st.ok()) return st;
    btreeSnapshot.clear();
    if (snapLen > 0) {
        btreeSnapshot.resize(snapLen);
        st = reader.readBytes(btreeSnapshot.data(), snapLen);
        if (!st.ok()) return st;
    }
    return validate();
}

Status CheckpointPayload::validate() const {
    if (magic != kCheckpointMagic) {
        return Status::corruption("CheckpointPayload: invalid magic");
    }
    if (version != kCheckpointFormatVersion) {
        return Status::corruption("CheckpointPayload: unsupported version");
    }
    if (!lsn.isValid()) {
        return Status::corruption("CheckpointPayload: invalid LSN");
    }
    return Status::success();
}

} // namespace recovery
} // namespace quartz
