#include "quartz/core/Database.h"
#include "quartz/diagnostics/ConsistencyChecker.h"
#include "quartz/diagnostics/IntegrityScanner.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: quartzdb_verify <database.qdb>\n";
        return 1;
    }

    const std::string path = argv[1];
    quartz::core::DatabaseOptions options;
    options.enableWal = true;
    options.recoverOnOpen = true;

    quartz::core::Database db(options);
    auto st = db.open(path, false);
    if (!st.ok()) {
        std::cerr << "Open failed: " << st.toString() << "\n";
        return 2;
    }

    st = db.validate();
    std::cout << "Consistency: " << (st.ok() ? "PASS" : st.toString()) << "\n";

    auto report = quartz::diagnostics::ConsistencyChecker::analyze(db);
    for (const auto& finding : report.findings) {
        std::cout << "  [" << finding.component << "] " << finding.message << "\n";
    }

    quartz::diagnostics::IntegrityScanner scanner(db.file());
    st = scanner.scan();
    std::cout << "Integrity scan: " << (st.ok() ? "PASS" : st.toString()) << "\n";

    if (!db.close().ok()) {
        return 4;
    }
    return report.passed() && st.ok() ? 0 : 3;
}
