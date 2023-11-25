#include "quartz/wal/LogValidator.h"

namespace quartz {
namespace wal {

Status LogValidator::validateRecord(const LogRecord& record) {
    auto st = validateRecordHeader(record);
    if (!st.ok()) return st;
    return validateReservedFields(record);
}

Status LogValidator::validateRecordHeader(const LogRecord& record) {
    if (!isValidRecordType(record.type())) {
        return Status::corruption("LogValidator: invalid record type");
    }
    if (!record.lsn().isValid()) {
        return Status::corruption("LogValidator: invalid LSN");
    }
    if (record.timestamp() == 0) {
        return Status::corruption("LogValidator: missing timestamp");
    }
    return Status::success();
}

Status LogValidator::validateLsnOrdering(LogSequenceNumber previous, LogSequenceNumber current) {
    if (!current.isValid()) {
        return Status::corruption("LogValidator: invalid LSN in sequence");
    }
    if (previous.isValid() && current <= previous) {
        return Status::corruption("LogValidator: LSN not strictly increasing");
    }
    return Status::success();
}

Status LogValidator::validateReservedFields(const LogRecord& record) {
    (void)record;
    return Status::success();
}

} // namespace wal
} // namespace quartz
