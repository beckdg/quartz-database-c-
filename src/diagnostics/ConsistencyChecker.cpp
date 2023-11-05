#include "quartz/diagnostics/ConsistencyChecker.h"

#include "quartz/core/Database.h"
#include "quartz/recovery/RecoveryValidator.h"
#include "quartz/space/FragmentationAnalyzer.h"

namespace quartz {
namespace diagnostics {

Status ConsistencyChecker::check(core::Database& db) {
    if (!db.isOpen()) {
        return Status::invalidArgument("ConsistencyChecker: database not open");
    }

    auto st = db.tree().validate();
    if (!st.ok()) return st;

    if (db.options().enableWal) {
        st = db.wal().validate();
        if (!st.ok()) return st;
        return recovery::RecoveryValidator::verifyLogicalConsistency(db.wal(), db.tree());
    }
    return Status::success();
}

DiagnosticReport ConsistencyChecker::analyze(const core::Database& db) {
    DiagnosticReport report;
    if (!db.isOpen()) {
        report.add(FindingSeverity::Error, "database", "not open");
        return report;
    }

    auto st = db.tree().validate();
    if (!st.ok()) {
        report.add(FindingSeverity::Error, "btree", st.toString());
    } else {
        report.add(FindingSeverity::Info, "btree", "structure valid");
    }

    const auto frag = space::FragmentationAnalyzer::analyze(db.space().extentAllocator().freeSpaceMap(),
                                                            db.space().allocatedPageCount() +
                                                                db.space().freePageCount());
    if (frag.fragmentationPercent > 50.0) {
        report.add(FindingSeverity::Warning, "space",
                   "high fragmentation: " + std::to_string(frag.fragmentationPercent) + "%");
    }

    if (db.options().enableWal) {
        st = db.wal().validate();
        if (!st.ok()) {
            report.add(FindingSeverity::Error, "wal", st.toString());
        } else {
            report.add(FindingSeverity::Info, "wal", "log valid");
        }
    }
    return report;
}

} // namespace diagnostics
} // namespace quartz
