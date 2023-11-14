#include "quartz/recovery/CheckpointManager.h"

#include "quartz/serialization/BinaryReader.h"
#include "quartz/serialization/BinaryWriter.h"
#include "quartz/serialization/Buffer.h"
#include "quartz/wal/LogFile.h"
#include "quartz/wal/LogRecord.h"
#include "quartz/wal/LogTypes.h"

namespace quartz {
namespace recovery {

Status CheckpointManager::createCheckpoint(btree::BTree& tree, wal::LogManager& wal,
                                           wal::LogSequenceNumber& outLsn) {
    serialization::Buffer btreeBuf;
    serialization::BinaryWriter btreeWriter(btreeBuf);
    auto st = tree.serialize(btreeWriter);
    if (!st.ok()) return st;

    CheckpointPayload payload;
    payload.lsn = wal.currentLsn().isValid() ? wal.currentLsn().next() : wal::LogSequenceNumber::initial();
    payload.treeSize = tree.size();
    payload.treeHeight = tree.height();
    payload.btreeSnapshot = std::move(btreeBuf);

    serialization::Buffer payloadBuf;
    serialization::BinaryWriter payloadWriter(payloadBuf);
    st = payload.serialize(payloadWriter);
    if (!st.ok()) return st;

    auto record = wal::LogRecord::make(wal::LogRecordType::CheckpointMarker);
    record.payload() = std::move(payloadBuf);
    st = wal.append(std::move(record));
    if (!st.ok()) return st;
    st = wal.flush();
    if (!st.ok()) return st;

    outLsn = wal.currentLsn();
    return Status::success();
}

Status CheckpointManager::findLastCheckpoint(wal::LogManager& wal, CheckpointPayload& outPayload,
                                             wal::LogSequenceNumber& outLsn) {
    outPayload = {};
    outLsn = wal::LogSequenceNumber::invalid();

    auto st = wal.rewindReader();
    if (!st.ok()) return st;

    wal::LogRecord record;
    bool found = false;
    while (wal.readNext(record).ok()) {
        if (record.type() != wal::LogRecordType::CheckpointMarker) {
            continue;
        }
        serialization::BinaryReader reader(serialization::BufferView(record.payload()));
        CheckpointPayload payload;
        st = payload.deserialize(reader);
        if (!st.ok()) return st;
        outPayload = std::move(payload);
        outLsn = record.lsn();
        found = true;
    }

    if (!found) {
        return Status::invalidArgument("CheckpointManager: no checkpoint found");
    }
    return Status::success();
}

Status CheckpointManager::restoreTree(const CheckpointPayload& payload, btree::BTree& tree) {
    auto st = payload.validate();
    if (!st.ok()) return st;

    tree.clear();
    serialization::BinaryReader reader(serialization::BufferView(payload.btreeSnapshot));
    return tree.deserialize(reader);
}

Status CheckpointManager::truncateAfterCheckpoint(wal::LogManager& wal, wal::LogSequenceNumber lsn) {
    std::uint64_t offset = wal::WalFileHeader::kSize;
    std::uint64_t endOffset = wal::WalFileHeader::kSize;

    wal::LogRecord record;
    std::uint64_t bytesRead = 0;
    while (offset < wal.file().fileSize()) {
        auto st = wal.file().readRecord(offset, record, bytesRead);
        if (!st.ok()) {
            break;
        }
        endOffset = offset + bytesRead;
        if (record.lsn() == lsn) {
            return wal.file().truncate(endOffset);
        }
        offset += bytesRead;
    }
    return Status::invalidArgument("CheckpointManager: checkpoint LSN not found for truncation");
}

} // namespace recovery
} // namespace quartz
