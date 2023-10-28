#pragma once

#include "quartz/wal/LogRecord.h"
#include "quartz/common/Status.h"

namespace quartz {
namespace wal {

/// Validates log record structure and ordering rules.
class LogValidator {
public:
    static Status validateRecord(const LogRecord& record);
    static Status validateRecordHeader(const LogRecord& record);
    static Status validateLsnOrdering(LogSequenceNumber previous, LogSequenceNumber current);
    static Status validateReservedFields(const LogRecord& record);
};

} // namespace wal
} // namespace quartz
