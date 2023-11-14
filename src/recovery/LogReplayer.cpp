#include "quartz/recovery/LogReplayer.h"

#include "quartz/btree/Key.h"
#include "quartz/format/PageReference.h"
#include "quartz/serialization/BinaryReader.h"
#include "quartz/wal/LogTypes.h"
#include "quartz/wal/LogValidator.h"

namespace quartz {
namespace recovery {

Status LogReplayer::applyRecord(const wal::LogRecord& record, btree::BTree& tree) {
    switch (record.type()) {
    case wal::LogRecordType::PageUpdate: {
        serialization::BinaryReader reader(serialization::BufferView(record.payload()));
        btree::Key key;
        auto st = key.deserialize(reader);
        if (!st.ok()) return st;
        format::PageReference ref;
        st = reader.read(ref);
        if (!st.ok()) return st;
        if (tree.contains(key)) {
            format::PageReference existing;
            st = tree.find(key, existing);
            if (!st.ok()) return st;
            if (existing == ref) {
                return Status::success();
            }
            st = tree.erase(key);
            if (!st.ok()) return st;
        }
        return tree.insert(key, ref);
    }
    case wal::LogRecordType::PageDelete: {
        serialization::BinaryReader reader(serialization::BufferView(record.payload()));
        btree::Key key;
        auto st = key.deserialize(reader);
        if (!st.ok()) return st;
        if (!tree.contains(key)) {
            return Status::success();
        }
        return tree.erase(key);
    }
    case wal::LogRecordType::Allocation:
    case wal::LogRecordType::Deallocation:
    case wal::LogRecordType::NodeSplit:
    case wal::LogRecordType::NodeMerge:
    case wal::LogRecordType::PageCreate:
    case wal::LogRecordType::MetadataUpdate:
    case wal::LogRecordType::CheckpointMarker:
        return Status::success();
    case wal::LogRecordType::Invalid:
    default:
        return Status::corruption("LogReplayer: invalid record type");
    }
}

Status LogReplayer::replay(wal::LogManager& wal, btree::BTree& tree, wal::LogSequenceNumber afterLsn,
                           RecoveryResult& result) {
    result = {};

    auto st = wal.rewindReader();
    if (!st.ok()) return st;

    wal::LogRecord record;
    while (wal.readNext(record).ok()) {
        st = wal::LogValidator::validateRecord(record);
        if (!st.ok()) return st;

        if (record.lsn() <= afterLsn) {
            ++result.recordsSkipped;
            continue;
        }

        if (record.type() == wal::LogRecordType::CheckpointMarker) {
            ++result.recordsSkipped;
            continue;
        }

        st = applyRecord(record, tree);
        if (!st.ok()) return st;

        result.lastReplayedLsn = record.lsn();
        ++result.recordsReplayed;
    }

    return Status::success();
}

} // namespace recovery
} // namespace quartz
